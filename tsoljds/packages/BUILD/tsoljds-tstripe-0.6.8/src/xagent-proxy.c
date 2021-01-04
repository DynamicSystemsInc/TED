/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

  The contents of this file are subject to the terms of the
  GNU General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this file except in compliance with the License.

  You should have received a copy of the License along with this
  file; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include <strings.h>

#include <libcontract.h>
#include <sys/contract/process.h>
#include <sys/ctfs.h>
#include <sys/utsname.h>
#include <zone.h>
#include <errno.h>
#include <limits.h>
#include <secdb.h>
#include <security/pam_appl.h>
#include <user_attr.h>
#include <libtsnet.h>
#include <wait.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xauth.h>

#include <gtk/gtk.h>
#include <libgnometsol/pam_dialog.h>
#include <libgnometsol/pam_conv.h>
#include <libgnometsol/message_dialog.h>

#define  WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "privs.h"
#include "tsol-user.h"

int pipe_in_fd;  /* read from stripe from this */
int pipe_out_fd; /* write to xagent to this */

struct passwd *p = NULL;
struct passwd pwd;
zoneid_t zoneid;
m_label_t *sl = NULL;
gboolean nscd_per_label = FALSE ;

static int init_template (void);
static gint validate_label (char *label);

#define CLOSE_FILES() \
{int ifx; for (ifx=1; ifx < _NFILE; ifx++) (void) close (ifx);}

void
xagent_death_handler (int sig, siginfo_t * sinf, void *ucon)
{
	int status;

	waitpid (sinf->si_pid, &status, WNOHANG);
	exit (0);
}

static gboolean
process_chauthtok_request (int readfd, int writefd, char *name)
{
	GtkWidget *dialog;
	char c[1];
	char buf[PAM_MAX_MSG_SIZE];
	ssize_t nbytes;
	char *userid, *password, *message;
	int response;
	char *label_string = NULL;
	int breakout = 0;
	
	label_to_str (sl, &label_string, M_LABEL, DEF_NAMES);
	if (strncmp (label_string, "ADMIN_LOW", 9) == 0) {
		g_free (label_string);
		label_string = g_strdup ("Trusted Path");
	}

	message = g_strdup_printf ("Changing %s password for %s", label_string, name);

	while (breakout < 3 && (nbytes = read (readfd, c, 1) != -1)) {
		switch (c[0]) {
			case 'u':
				/*pop up user dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_password_dialog_new ("", buf, message, TRUE, FALSE);
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				g_object_get (G_OBJECT (dialog), "input-text", &userid, NULL);
				if (response == GTK_RESPONSE_OK && strlen (userid) != 0) {
					write (writefd, userid, strlen(userid));
				} else  write (writefd, "RESPONSE_CANCELLED", 18);
				gtk_widget_destroy (dialog);
				break;
			case 'p':
				/*pop up password dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_password_dialog_new ("", buf, message, TRUE, TRUE);
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				g_object_get (G_OBJECT (dialog), "input-text", &password, NULL);
				if (response == GTK_RESPONSE_OK && strlen (password) != 0) {
					write (writefd, password, strlen(password));
				} else  write (writefd, "RESPONSE_CANCELLED", 18);
				gtk_widget_destroy (dialog);
				break;
			case 'e':
				/*pop up error dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				write (writefd, "DONE", 4);
				break;
			case 'i':
				/*pop up info dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				write (writefd, "DONE", 4);
				break;
			case 'f':
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				breakout=3;
				break;
			case 's':
				breakout=3;
				break;
			default:
				break;
		}
	}

	g_free (message);
	g_free (label_string);

	if (c[0] == 'f') {
		return FALSE;
	} else {
		return TRUE;
	}
}

static void
change_password (int argc, char **argv)
{
	int global[2];
	int zone[2];
	int pid, status;
	int tmpl_fd;

	if (zoneid == 0) {
		gtk_init (&argc, &argv);
		_tstripe_users_init ();
		_tstripe_solaris_chauthtok ("dtpasswd", pwd.pw_name, pwd.pw_uid,
					    pwd.pw_gid, NULL, -1, -1);
		return;
	}

	pipe (global);
	pipe (zone);

	/*Initializing with contract file system is mandatory for zone_enter()*/
	if ((tmpl_fd = init_template ()) == -1) {
		perror ("Couldn't create contract");
		return;
	}

	if (pid = fork (), pid == 0) {  /* child */
		(void) ct_tmpl_clear (tmpl_fd);
		if (zone_enter (zoneid) == -1) {
			_exit (127);
		}
		gtk_init (&argc, &argv);
		_tstripe_users_init ();
		_tstripe_solaris_chauthtok ("dtpasswd", pwd.pw_name, pwd.pw_uid,
					  pwd.pw_gid, NULL, zone[0], global[1]);
		exit (0);
	} else if (pid > 0) { /* parent */
		gtk_init (&argc, &argv);
		_tstripe_users_init ();
		process_chauthtok_request (global[0], zone[1], pwd.pw_name);
                (void) ct_tmpl_clear (tmpl_fd);
                (void) close (tmpl_fd);
		waitpid (pid, &status, 0);
		close (global[0]);
		close (global[1]);
		close (zone[0]);
		close (zone[1]);
	} 
}

static gboolean 
handle_pipe_input (GIOChannel *source, GIOCondition condition, gpointer data)
{
#define BUFSIZE 1024
	gsize byteread, pos;
	gchar *str, *tmp, *label_string = NULL;
	GError *error = NULL;
	GIOStatus status=0;
	int fd;

	if (condition & G_IO_ERR) return FALSE;
	if (condition & G_IO_HUP) return FALSE;

	if (condition & G_IO_IN) {
		status = g_io_channel_read_line (source, &str, &byteread, &pos,
						 &error);
		switch (status)
		{
			case G_IO_STATUS_NORMAL: 
				if ((tmp = strchr (str, ':')) &&
				 !strncmp (tmp+1, "xagentchangepassword",20)) {

					if (fork() == 0) {
						label_to_str (sl, &label_string, M_INTERNAL, DEF_NAMES);
						execl ("/usr/bin/tsoljds-xagent-proxy", "tsoljds-xagent-proxy", label_string, pwd.pw_name, "cp", 0);
					}
				} else  {
					str[pos] = '\n';
					/*forward the messgae to zoned xagent*/
					write (pipe_out_fd, str, strlen (str));
				}
				return TRUE;
			case G_IO_STATUS_AGAIN: 
				return FALSE;
			case G_IO_STATUS_EOF:
				sleep(1);
				return FALSE;
			case G_IO_STATUS_ERROR:
				return FALSE;
			default: 
				return FALSE;
		}
	}
}

static void
so_long_pipe (gpointer data)
{
	/* The pipe is bust which probably means the stripe
	 * has died. So there's nothing to do but die. */
       exit (2);
}

static int
init_template (void)
{
	int fd;
	int err = 0;

	fd = open64 (CTFS_ROOT "/process/template", O_RDWR);
	if (fd == -1) return (-1);

	err |= ct_tmpl_set_critical (fd, 0);
	err |= ct_tmpl_set_informative (fd, 0);
	err |= ct_pr_tmpl_set_fatal (fd, CT_PR_EV_HWERR);
	err |= ct_pr_tmpl_set_param (fd, CT_PR_PGRPONLY | CT_PR_REGENT);
	if (err || ct_tmpl_activate (fd)) {
		(void) close (fd);
		return (-1);
	}
	return (fd);
}

static int
add_role_to_acl (GdkDisplay * display, gchar * rolename)
{
	int             ret = 0;
	struct passwd  *pwd;
	XHostAddress    ha;
	XServerInterpretedAddress siaddr;

	if ((pwd = getpwnam (rolename)) == NULL)
		return -1;

	siaddr.type = "localuser";
	siaddr.typelength = strlen("localuser");
	siaddr.value = rolename;
	siaddr.valuelength = strlen(rolename);
	ha.family = FamilyServerInterpreted;
	ha.address = (char *) &siaddr;
	ret = XAddHost (GDK_DISPLAY_XDISPLAY (display), &ha);
	return ret;
}

static gboolean
process_auth_request (int readfd, int writefd, m_label_t *sl, char *name)
{
	GtkWidget *dialog;
	char c[1];
	char buf[PAM_MAX_MSG_SIZE];
	ssize_t nbytes;
	char *userid, *password, *message;
	int response;
	char *label_string = NULL;
	gboolean breakout = FALSE;
	
	label_to_str (sl, &label_string, M_LABEL, DEF_NAMES);
	if (strncmp (label_string, "ADMIN_LOW", 9) == 0) {
		g_free (label_string);
		label_string = g_strdup ("Trusted Path");
	}

	message = g_strdup_printf ("%s labeled workspace requires authentication for %s", label_string, name);

	while (breakout == FALSE && (nbytes = read (readfd, c, 1) != -1)) {
		switch (c[0]) {
			case 'u':
				/*pop up user dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_password_dialog_new ("", buf, message, TRUE, FALSE);
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				g_object_get (G_OBJECT (dialog), "input-text", &userid, NULL);
				if (response == GTK_RESPONSE_OK && strlen (userid) != 0) {
					write (writefd, userid, strlen(userid));
				} else  write (writefd, "RESPONSE_CANCELLED", 18);
				gtk_widget_destroy (dialog);
				break;
			case 'p':
				/*pop up password dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_password_dialog_new ("", buf, message, TRUE, TRUE);
				response = gtk_dialog_run (GTK_DIALOG (dialog));
				g_object_get (G_OBJECT (dialog), "input-text", &password, NULL);
				if (response == GTK_RESPONSE_OK && strlen (password) != 0) {
					write (writefd, password, strlen(password));
				} else  write (writefd, "RESPONSE_CANCELLED", 18);
				gtk_widget_destroy (dialog);
				break;
			case 'e':
				/*pop up error dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				write (writefd, "DONE", 4);
				break;
			case 'i':
				/*pop up info dialog*/
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				write (writefd, "DONE", 4);
				break;
			case 'f':
				nbytes = read (readfd, buf, PAM_MAX_MSG_SIZE);
				buf[nbytes] = '\0';
				dialog = gnome_tsol_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, TRUE, buf, NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				breakout = TRUE;
				break;
			case 's':
				add_role_to_acl(gdk_display_get_default(),name);
				breakout = TRUE;
				break;
			default:
				break;
		}
	}

	g_free (message);
	g_free (label_string);

	if (c[0] == 'f') {
		return FALSE;
	} else {
		return TRUE;
	}
}

static int
authenticate_user (int readfd, int writefd, char *name)
{
	char *message;
	int nbytes;
	int retry = 1;
	int ret, result, response;
	char buf[PAM_MAX_MSG_SIZE];
	User *user;

	user = _tstripe_user_find_user_by_name (name);

	while (retry) {
		if ((result = _tstripe_authenticate_role (user, NULL, readfd, writefd)) != PAM_SUCCESS) {
			switch (result) {
				case PAM_PERM_DENIED:
					retry = 0;
					message = _("The system administrator has temporarily disabled access to the system.\nSee your system administrator");
					write (writefd, "e", 1);
					write (writefd, message, strlen (message));
					read (readfd, buf, PAM_MAX_MSG_SIZE);
                                        break;
                                case PAM_ACCT_EXPIRED:
                                        retry = 0;
					message = _("The system administrator has disabled this account.\nSee your system administrator");
					write (writefd, "e", 1);
					write (writefd, message, strlen (message));
					read (readfd, buf, PAM_MAX_MSG_SIZE);
					break;
                                case PAM_AUTH_ERR:
					message = _("Authentication failed");
					write (writefd, "e", 1);
					write (writefd, message, strlen (message));
					read (readfd, buf, PAM_MAX_MSG_SIZE);
                                        break;
                                case GNOME_TSOL_PAM_CANCEL:
                                        /*
                                         * User hit cancel during
                                         * authentication - don't prompt to
                                         * retry
                                         */
                                        retry = 0;
                                        break;
                                default:
                                        /*
                                         * Something very wierd went wrong in
                                         * the PAM conversation
                                         */
                                        retry = 0;
					message = _("Couldn't set account management.\n");
					write (writefd, "e", 1);
					write (writefd, message, strlen (message));
					read (readfd, buf, PAM_MAX_MSG_SIZE);
                                        break;
                                }
                        } else  /* authentication succeeded - break out */
                                retry = 0;
	}

	if (result == PAM_SUCCESS) {
		write (writefd, "s", 1);
		return 0;
	} else {
		write (writefd, "f", 1);
		write (writefd, "Authentication cancelled.  This workspace will remain inactive until you authenticate.", 86);
		return -1;
	}
}


gboolean
spawn_xagent (int argc, char **argv)
{
	int ToAgentFD[2];
	int global[2];
	int zone[2];
	int pid;
	char *display_name;
	int tmpl_fd;
	gboolean role = FALSE;


	if (geteuid () != pwd.pw_uid) {
		role = TRUE;
	} else {
		role = FALSE;
	}
	
	/*Initializing with contract file system is mandatory for zone_enter()*/
	if ((tmpl_fd = init_template ()) == -1) {
		perror ("Couldn't create contract");
		return FALSE;
	}
	pipe (ToAgentFD);
	pipe (global);
	pipe (zone);

	if (pid = fork (), pid == 0) {  /* in the child */
		int status;
		char *tmp;
		int retries = 10;   /* Wait for zone to come up */
		static char display_env[MAXPATHLEN];
		struct utsname uname_ent;

		/* get the global zone hostname */
		if (uname (&uname_ent) == -1) {
			perror ("tsoljds-xagent-proxy: uname failed");
		}

		/* Child end of the communication channel */
		close (0);
		dup (ToAgentFD[0]);
		(void) ct_tmpl_clear (tmpl_fd);

		if (zoneid != 0 && zone_enter (zoneid) == -1) {
			_exit (127);
		}

		gtk_init (&argc, &argv);

		_tstripe_users_init ();

		if ((role || (zoneid != 0  && nscd_per_label)) &&
		    	(authenticate_user (zone[0], global[1], pwd.pw_name))) {
				_exit (127);
		}

		signal (SIGPIPE, SIG_DFL);

		setsid ();

                status = _tstripe_solaris_setcred (GETTEXT_PACKAGE, pwd.pw_name,
						   pwd.pw_uid, pwd.pw_gid);
		CLOSE_FILES ();

		drop_inherited_privs ();

		g_setenv ("HOME", pwd.pw_dir, TRUE);
		g_setenv ("USER", pwd.pw_name, TRUE);
		g_setenv ("SHELL", pwd.pw_shell, TRUE);
		tmp = g_strdup_printf ("/var/mail/%s", pwd.pw_name);
		g_setenv ("MAIL", tmp, TRUE);
		g_free (tmp);

		putenv ("GTK3_RC_FILES=/usr/share/mate/gtkrc.tjds");

		if (role || zoneid != 0) {
			g_unsetenv ("SESSION_MANAGER");
			g_unsetenv ("DBUS_SESSION_BUS_ADDRESS");
			g_unsetenv ("DBUS_SESSION_BUS_PID");
			g_unsetenv ("DBUS_SESSION_BUS_WINDOWID");
			g_unsetenv ("GNOME_KEYRING_SOCKET");
		}

		chdir (pwd.pw_dir);

		if (role) {
			execl ("/usr/bin/pfexec", "/usr/bin/pfexec", "/usr/bin/tsoljds-xagent",
			       "--defaultsession", 0);
		} else {
			if (zoneid == 0) {
				execl ("/usr/bin/pfexec", "/usr/bin/pfexec",
				       "/usr/bin/tsoljds-xagent", 
				       "--nosession", 0);
			} else {
				execl ("/usr/bin/pfexec", "/usr/bin/pfexec",
				       "/usr/bin/tsoljds-xagent", 0);
			}
		}
	} else if (pid > 0) { /* in the parent */
		pipe_out_fd = ToAgentFD[1];
                (void) close (ToAgentFD[0]);		
		if (role || (zoneid != 0 && nscd_per_label)) {
			gtk_init (&argc, &argv);
			_tstripe_users_init ();
			if (!process_auth_request (global[0], zone[1], sl, 
						   argv[2]))
				_exit (127);
		}
                (void) ct_tmpl_clear (tmpl_fd);
                (void) close (tmpl_fd);
		return TRUE;
        } else if (pid == -1) { /* error occurred in fork () */
                fprintf (stderr, "fork() failed \n");
		return FALSE;
        }
}

static char    *
get_label_string (m_label_t * label, int view_flag)
{
	char *str = NULL;

	if (bsltos (label, &str, 0, view_flag | LONG_CLASSIFICATION |
		    LONG_WORDS | ALL_ENTRIES) <= 0) {
		return NULL;
	}
	return ((char *) strdup (str));
}

static void
wait_for_zone (m_label_t *sl, char *zonename)
{
	GError *error;
	GPid pid;
	char *command;
	char *label = NULL;
	char *standard_out = NULL;
	int exit_status = -1;
	int tries = 0;

	char *argv[] = {"/usr/bin/zenity", "--title", "Booting Zone", 
			"--progress", "--pulsate", "--no-cancel",
			"--text", "arg7",
			"--timeout", "55", NULL};

	label_to_str (sl, &label, M_LABEL, DEF_NAMES);

	argv[7] = g_strdup_printf("Initializing %s Labeled Workspace...",label);
	g_free (label);

	g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, &pid, NULL);

	g_free (argv[7]);

	command = g_strdup_printf ("zlogin %s /usr/bin/svcs -H -o STA milestone/multi-user", zonename);

	while (tries < 50) {
		g_spawn_command_line_sync (command, &standard_out, NULL,
					   &exit_status, &error);
		if (standard_out && strncmp (standard_out, "ON", 2) == 0) {
			g_free (standard_out);
			break;
		}
		g_free (standard_out);
		sleep (1);
		tries++;
	}
		
	kill (pid, SIGTERM);

	g_free (command);
}

static char *
get_zonename_from_label (m_label_t *sl)
{
	FILE *fp;
	tsol_zcent_t *zcent;
	char line[1024];
	char *zonename = NULL;

	if ((fp = fopen (TNZONECFG_PATH, "r")) == NULL) {
		return NULL;
	}

	while (fgets (line, sizeof (line), fp) != NULL) {
		if ((zcent = tsol_sgetzcent (line, NULL, NULL)) == NULL)
			continue;
		if (zcent->zc_match & TSOL_MATCH_SHARED_LABEL)
			continue;
		if (blequal (sl, &zcent->zc_label)) {
			zonename = strdup (zcent->zc_name);
			tsol_freezcent (zcent);
			break;
		}
		tsol_freezcent (zcent);
	}
	fclose (fp);

	return zonename;
}

static gint
validate_label (char *label)
{
	zoneid_t zid;
        GError *error;
        int  exit_status;
	int err;
	char *error_msg;	
	char *zonename = NULL;
	char *command = NULL;

	if ((zid = getzoneidbylabel (sl)) == -1) {
		zonename = get_zonename_from_label (sl);
		if (zonename) { /*try to boot the zone*/
			command = g_strdup_printf ("/usr/bin/pfexec "
						   "/usr/sbin/zoneadm -z "
						   "%s boot",zonename);
			g_spawn_command_line_sync (command, NULL, NULL,
						   &exit_status, &error);
			g_free (command);
			if (WEXITSTATUS (exit_status) == 0) {
				wait_for_zone (sl, zonename);
				g_free (zonename);
				return (1);
			}
			g_free (zonename);
			return (0);
		}
		return (-1);
	} else {
		return (1);
	}
}

gboolean 
current_workspace_is_labeled_zone (int zoneid)
{
	char *currzoneidstr;
	Display *xdpy;
	Window root;
	Atom atom, utf8_string;
	int format;
	unsigned long nitems;
	unsigned long bytesafter;
	unsigned char *prop_data = NULL;
	Atom type = None;
	char *zoneidstr = g_strdup_printf ("%d", zoneid);

	xdpy = XOpenDisplay (NULL);

	utf8_string = XInternAtom (xdpy, "UTF8_STRING", FALSE);

	root = DefaultRootWindow (xdpy);

	atom = XInternAtom (xdpy, "NAUTILUS_ACTIVE_DESKTOP_ID", FALSE);

	gdk_error_trap_push ();

	XGetWindowProperty (xdpy, root, atom, 0L, (long)1024, FALSE,
				utf8_string, &type, &format, &nitems,
				&bytesafter, (unsigned char **)&prop_data);

	gdk_error_trap_pop ();

	currzoneidstr = strchr (prop_data, '_') + 1;
	if (strcmp (currzoneidstr, zoneidstr) == 0 ){
		g_free (zoneidstr);
		return FALSE;
	} else {
		g_free (zoneidstr);
		return TRUE;
	}
}

int 
main (int argc, char **argv)
{
	GMainContext *gcontext;
	GMainLoop *gmain;
	GIOChannel *channel;
	gboolean xagent_started = FALSE;
	int dummy_fd;
	int err;
	guint result;
	struct sigaction sa;
	char pw_buffer[PW_BUF_LEN];
	gint status;

	if (argc == 1) exit (1);

	if (getzoneid () != 0) exit (1);

	str_to_label (argv[1], &sl, MAC_LABEL, L_DEFAULT, &err);

	getpwnam_r (argv[2], &pwd, pw_buffer, PW_BUF_LEN, &p);

	nscd_per_label = g_file_test ("/var/tsol/doors/nscd_per_label", 
				      G_FILE_TEST_EXISTS);

	if (argv[3] != NULL) { /* change passwrd */
		zoneid = getzoneidbylabel (sl);
		change_password (argc, argv);
		exit (0);
	}

	status = validate_label (argv[1]);
	if (status < 1) {
		GtkWidget *dialog;	
		char *error_msg;
		char *label_str = get_label_string (sl, VIEW_INTERNAL);
		char *zonename = get_zonename_from_label (sl);
		char *errormsg1 = g_strdup_printf ("The label %s \n"
						"has no matching labeled zone.",
						label_str);
		char *errormsg2 = g_strdup_printf ("The zone, %s, is offline "
				"and %s \nlacks the authorization, "
				"solaris.zone.manage, to boot it",
				zonename, pwd.pw_name);


		gtk_init (&argc, &argv);

		dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,							 GTK_MESSAGE_WARNING, 
					 	 GTK_BUTTONS_OK, 
						 (status == -1 ) ?
						 	errormsg1:
							errormsg2,
							NULL);
		gtk_widget_show_all (dialog);
		gtk_dialog_run (GTK_DIALOG (dialog));
		g_free (errormsg1);
		g_free (errormsg2);
		gtk_widget_destroy (dialog);
		
		g_free (label_str);
		g_free (zonename);
		exit (-1);
	}

	zoneid = getzoneidbylabel (sl);

	/*if (!current_workspace_is_labeled_zone (zoneid)) {
		exit (0);
	}*/

	if (!spawn_xagent (argc, argv))
		exit (-1);

	/* redirect stderr to /dev/null */
	int fd = open ("/dev/null", O_RDWR);
	dup2 (fd, 2);

	if ((pipe_in_fd = dup (fileno (stdin))) != -1) {
		close (fileno (stdin));
		dummy_fd = open ("/dev/null", O_RDONLY);
		fcntl (pipe_in_fd, F_SETFD, 1);
	} else {
		pipe_in_fd = fileno (stdin);
	}

	channel = g_io_channel_unix_new (pipe_in_fd);
	result = g_io_add_watch_full (channel, G_PRIORITY_HIGH,
				  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
				  (GIOFunc)handle_pipe_input, NULL, so_long_pipe);
	 /* Child signal handler */
	sa.sa_handler = xagent_death_handler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset (&sa.sa_mask);
	(void) sigaction (SIGCLD, &sa, (struct sigaction *) 0);
	
	gmain = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (gmain);

	return 0;
}
