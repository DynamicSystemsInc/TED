/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wait.h>
#include <unistd.h>
#include <user_attr.h>
#include <secdb.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <deflt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libgen.h>
#include <signal.h>
#include <siginfo.h>

#include <rpc/rpc.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xauth.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <X11/extensions/Xtsol.h>

#include <libcontract.h>
#include <sys/contract/process.h>
#include <sys/ctfs.h>
#include <zone.h>
#include <libgnometsol/userattr.h>
#include <libgnometsol/pam_conv.h>

#include "xagent-management.h"
#include "menus.h"
#include "tsol-user.h"
#include "ui-controller.h"
#include "privs.h"

#define MAX_AGENTS      20	/* Number of xagents in session */
#define DEF_TIMEOUT     (5 * 60)
#define DEFUMASK        022
#define	MAXPATHLEN	1024
#define PW_BUF_LEN      256	/* buffer area for password context */
#define CLOSE_FILES() \
{int ifx; for (ifx=1; ifx < _NFILE; ifx++) (void) close (ifx);}

typedef struct _XAgent {
	int             fd;	/* file descriptor where the command proxy
				 * writes to */
	int             pid;	/* pid of the local xagent */
	struct passwd   p;	/* user password information */
	char            pw_buffer[PW_BUF_LEN];	/* Storage for the password
						 * structure */
	m_label_t      *sl;	/* sensitivty label used for the zone entered
				 * into */
}               XAgent;


struct DefaultLogin {
	char           *console;
	char           *altshell;
	char           *passreq;
	char           *timezone;
	char           *hz;
	char           *path;
	char           *supath;
	long            ulimit;
	unsigned        timeout;
	mode_t          umask;
	int             idleweeks;
	int             sleeptime;
	int             dosyslog;
};

int             TsolErrorHandler (Display * dpy, XErrorEvent * error);
void            spawn_xagent (XAgent * agent, char *fullcmd);
static gchar   *get_real_command (char *exec_cmd);
static int		get_screen_number (char *exec_cmd);

struct DefaultLogin deflogin = {
	NULL, NULL, NULL, NULL, NULL, "/usr/bin", NULL,
	-1, DEF_TIMEOUT, DEFUMASK, 0, 0, 0
};

XAgent         *localzones[MAX_AGENTS];
int             AgentCount = 0;
int             TotalScreen = 1;
gboolean        LocalZoneSessionSave = FALSE;
static char    *Pndefault = "/etc/default/login";

/*
 * Ignore X protocol errors
 */
int
TsolErrorHandler (Display * dpy, XErrorEvent * error)
{
	char            err_msg[132];

	/* ignore all errors */

	XGetErrorText (dpy, error->error_code, err_msg, sizeof (err_msg));

	return 0;
}


/*
 * Fills up deflogin structure as appropriate
 */
void
tsol_login_defaults (void)
{
	register char  *ptr;

	if (defopen (Pndefault) == 0) {
		if ((ptr = defread ("CONSOLE=")) != NULL)
			deflogin.console = strdup (ptr);

		if ((ptr = defread ("ALTSHELL=")) != NULL)
			deflogin.altshell = strdup (ptr);

		if ((ptr = defread ("PASSREQ=")) != NULL)
			deflogin.passreq = strdup (ptr);

		if ((ptr = defread ("TIMEZONE=")) != NULL)
			deflogin.timezone = strdup (ptr);

		if ((ptr = defread ("HZ=")) != NULL)
			deflogin.hz = strdup (ptr);

		if ((ptr = defread ("PATH=")) != NULL)
			deflogin.path = strdup (ptr);

		if ((ptr = defread ("SUPATH=")) != NULL)
			deflogin.supath = strdup (ptr);

		if ((ptr = defread ("ULIMIT=")) != NULL)
			deflogin.ulimit = atol (ptr);

		if ((ptr = defread ("TIMEOUT=")) != NULL)
			deflogin.timeout = (unsigned) atoi (ptr);

		if ((ptr = defread ("UMASK=")) != NULL)
			if (sscanf (ptr, "%lo", &deflogin.umask) != 1)
				deflogin.umask = DEFUMASK;

		if ((ptr = defread ("IDLEWEEKS=")) != NULL)
			deflogin.idleweeks = atoi (ptr);

		if ((ptr = defread ("SLEEPTIME =")) != NULL)
			deflogin.sleeptime = atoi (ptr);

		if ((ptr = defread ("SYSLOG=")) != NULL)
			deflogin.dosyslog = strcmp (ptr, "YES") == 0;

		(void) defopen ((char *) NULL);
	}
}


static          gboolean
is_local_zone (m_label_t * sl)
{
	m_label_t       admin_low, admin_high;

	bsllow (&admin_low);
	bslhigh (&admin_high);

	if (blequal (sl, &admin_low) || blequal (sl, &admin_high)) {
		return FALSE;
	} else
		return TRUE;
}

static void
exec_cmd (char *cmd)
{
	const char  *real_cmd;
	int screen_num = 0;
	GError *error;

	GdkDisplay *gdk_dpy;
        gdk_dpy = gdk_display_get_default ();
	screen_num = get_screen_number (cmd);
	real_cmd = get_real_command(cmd);
        g_spawn_command_line_async (real_cmd, &error);
}


static void
run_in_global_zone (XAgent * agent, char *cmd)
{
	int             i;
	m_label_t       admin_low, admin_high;
	gboolean	found_global_xagent = FALSE;

	bsllow (&admin_low);
	bslhigh (&admin_high);

	for (i = 0; i < AgentCount; i++) {
		if ((blequal (localzones[i]->sl, &admin_low) ||
		     blequal (localzones[i]->sl, &admin_high)) &&
			     (localzones[i]->p.pw_uid == agent->p.pw_uid)) {
			write (localzones[i]->fd, cmd, strlen (cmd));
			found_global_xagent = TRUE;
			break;
		}
	}
	if (!found_global_xagent)
		exec_cmd (cmd);
}

static void
run_in_own_zone (XAgent * agent, char *cmd)
{
	int             i;

	for (i = 0; i < AgentCount; i++) {
		if ((blequal (localzones[i]->sl, agent->sl) &&
		     localzones[i]->p.pw_uid == agent->p.pw_uid)) {
			write (localzones[i]->fd, cmd, strlen (cmd));
			break;
		}
	}
}

GSList *trusted_execs = NULL;
#define USR_TPEXEC_FILE "/usr/share/mate/TrustedPathExecutables"
#define ETC_TPEXEC_FILE "/etc/share/mate/TrustedPathExecutables"
#define LINESIZE 1024

static void 
load_trusted_execs_from_file (char *filename)
{
	char line[LINESIZE];
	FILE *file = NULL;

	if ((file = fopen (filename, "r")) != NULL) {
		while (fgets (line, LINESIZE, file) != NULL) {
			if (line[0] == '#' ||
		    	    line[0] == ' ') continue;
			trusted_execs = g_slist_prepend (trusted_execs,
							 strdup(line));
		}

		fclose (file);
	}
}

static void
load_trusted_executables_list (void)
{
	load_trusted_execs_from_file (USR_TPEXEC_FILE);
	load_trusted_execs_from_file (ETC_TPEXEC_FILE);
}

static gboolean
is_trusted_executable (char *cmd)
{
	GSList *exec;
	char *tmp, *end = NULL;

	/*strip the screen number*/
	tmp = strdup (strchr (cmd, ':')+1);

	/*strip any arguments*/
	end = strchr (tmp, ' ');
	if (end) *end = '\n';

	for (exec = trusted_execs; exec != NULL; exec=exec->next) {
		if (strncmp (tmp, exec->data, strlen (exec->data)) == 0) {
			g_free (tmp);
			return TRUE;
		}
	}
	
	g_free (tmp);
	return FALSE;
}

/* ad a newline to the end of the command so that it can be read by the child */
static char    *
command_pad_newline (char *cmd)
{
	int             i = 0, len;
	char           *str, *dest, *src;

	src = cmd;
	len = strlen (cmd);
	str = dest = malloc (len + 2);
	for (i = 0; i < len; i++)
		*dest++ = *src++;
	*dest++ = '\n';
	*dest = '\0';

	return (str);
}

static int
get_screen_number (char *exec_cmd)
{
	gchar         **token;
	int             scrnum;

	token = g_strsplit (exec_cmd, ":", 2);
	if (token[0]) {
		scrnum = atoi (token[0]);
		return scrnum;
	} else
		return 0;
}

static gchar   *
get_real_command (char *exec_cmd)
{
	gchar         **token;

	token = g_strsplit (exec_cmd, ":", 2);
	if (token[1])
		return (token[1]);
	else
		return exec_cmd;
}

/* Expand the command into absolute path */
static char    *
expand_full_path (char *cmd)
{
	int             screen_num;
	char           *real_cmd;
	gchar         **token = NULL;
	gchar          *fpp;
	gchar           res[256];
	gchar          *prog;
	gchar          *para = NULL;

	/* command format: <screen no>':'<command> [<option>] */
	screen_num = get_screen_number (cmd);
	real_cmd = get_real_command (cmd);

	/* split <command> [<option>] */
	token = g_strsplit (real_cmd, " ", 2);

	prog = strdup (token[0]);

	if (token[1])
		para = strdup (token[1]);

	fpp = g_find_program_in_path (prog);

	if (fpp != NULL) {
		if (para)
			snprintf (res, 256, "%d:%s %s", screen_num, fpp, para);
		else
			snprintf (res, 256, "%d:%s", screen_num, fpp);
		return (command_pad_newline (res));
	} else
		return (command_pad_newline (cmd));
}

char          **
get_exec_command_prop (Display * x_dpy, Window proxy_window)
{
	Atom            labelCommandAtom;
	Atom            utf8_string;
	Atom            type = None;
	int             format;
	unsigned long   nitems;
	unsigned long   bytesafter;
	unsigned char  *data = NULL;
	char          **retval;
	int             i, n_strings;
	gchar          *p;

	utf8_string = XInternAtom (x_dpy, "UTF8_STRING", FALSE);

	labelCommandAtom = XInternAtom (x_dpy, _XA_LABEL_EXEC_COMMAND, False);

	if (XGetWindowProperty (x_dpy, proxy_window, labelCommandAtom, 0L,
			  (long) BUFSIZ, False, utf8_string, &type, &format,
			     &nitems, &bytesafter, (unsigned char **) &data)
			!= Success) {
		return NULL;
	}
	if (type != utf8_string || format != 8 || nitems == 0) {
		if (data)
			XFree (data);
		return NULL;
	}
	i = 0;
	n_strings = 0;
	while (i < nitems) {
		if (data[i] == '\0')
			++n_strings;
		++i;
	}

	if (data[nitems - 1] != '\0')
		++n_strings;

	retval = g_new0 (char *, n_strings + 1);

	p = (char *) data;
	i = 0;
	while (i < n_strings) {
		if (!g_utf8_validate ((const gchar *) p, -1, NULL)) {
			g_warning ("Property contained invalid UTF-8\n");
			XFree (data);
			g_strfreev (retval);
			return NULL;
		}
		retval[i] = g_strdup (p);

		p = p + strlen (p) + 1;
		++i;
	}

	if (data)
		XFree ((char **) data);

	return retval;
}


static void
handle_command (XAgent * agent, char *cmd)
{
	userattr_t     *u_ent = NULL;
	int             i, found = 0;
	char           *fullcmd = NULL;

	if (cmd) {
		fullcmd = strdup (expand_full_path (cmd));
		if (is_trusted_executable (fullcmd)) {
			run_in_global_zone (agent, fullcmd);
			return;
		}
	}

	/* Search through the array of previously launched xagents */
	for (i = 0; i < AgentCount; i++) {
		if (blequal (localzones[i]->sl, agent->sl) &&
			     (localzones[i]->p.pw_uid == agent->p.pw_uid)) {
			/* handle xagent died abnormally during a session */
			if (localzones[i]->pid == -1) {
				localzones[i] = agent;
				spawn_xagent (agent, fullcmd);
				return;
			} else {
				found = 1;
				if (fullcmd) {
					write (localzones[i]->fd, fullcmd, strlen (fullcmd));
				}
			}
		}
	}

	if (!found) {
		localzones[AgentCount] = agent;
		AgentCount++;
		spawn_xagent (agent, fullcmd);
	}
}

static          gboolean
get_workspace_label (WnckWorkspace * wksp, m_label_t ** sl)
{
	const char     *workspace_label;
	int             error;

	workspace_label = wnck_workspace_get_label (wksp);
	if (workspace_label == NULL) {
		return FALSE;
	}
	if (str_to_label (workspace_label, sl, MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
		perror ("str_to_label () \n");
		if (error == -1)
			g_warning ("Could not validate sensitivity label \"%s\" because the \
		   label encodings file could not be accessed",
				   workspace_label);
		else if (error > 0)
			g_warning ("\"%s\" is not a valid sensitivity label", workspace_label);
		return FALSE;
	}
	return TRUE;
}

GdkFilterReturn
process_label_command (GdkXEvent * gdkxevent,
		       GdkEvent * event,
		       gpointer data)
{
	Window          win = (Window) data;
	GdkDisplay     *gdk_dpy;
	Display        *x_dpy;
	XEvent         *xevent = gdkxevent;
	char          **cmd;
	XAgent         *agent;

	gdk_dpy = gdk_display_get_default ();
	x_dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);

	/* we are only interested in property change event */
	if (xevent->type != PropertyNotify)
		return GDK_FILTER_CONTINUE;

	/* Only property change event on this property is on interest to us */
	if (xevent->xproperty.atom != XInternAtom (x_dpy, _XA_LABEL_EXEC_COMMAND,
						   False))
		return GDK_FILTER_CONTINUE;

	cmd = get_exec_command_prop (x_dpy, win);
	if (cmd && cmd[0]) {
		int             screen_num;
		m_label_t      *sl = NULL;
		WnckScreen     *wnckscreen;
		WnckWorkspace  *workspace;
		const char     *role = NULL;

		screen_num = get_screen_number (cmd[0]);

		wnckscreen = wnck_screen_get (screen_num);
		if (!wnckscreen)
			return GDK_FILTER_CONTINUE;

		workspace = wnck_screen_get_active_workspace (wnckscreen);

		role = wnck_workspace_get_role (workspace);

		if (get_workspace_label (workspace, &sl)) {
			struct passwd  *p = NULL;

			agent = g_new (XAgent, 1);
			p = &agent->p;
			agent->fd = 0;
			if (role)
				getpwnam_r (role, p, agent->pw_buffer, PW_BUF_LEN, &p);
			else
				getpwuid_r (geteuid (), p, agent->pw_buffer, PW_BUF_LEN, &p);

			agent->sl = sl;
			handle_command (agent, cmd[0]);
		}
	} else
		return GDK_FILTER_TRANSLATE;
}

void 
spawn_xagent (XAgent * agent, char *fullcmd)
{
	int             ToAgentFD[2];
	int             pid;
	struct passwd  *p;
	char 	       *zoneintstr = NULL;

	label_to_str (agent->sl, &zoneintstr, M_INTERNAL, DEF_NAMES);

	p = &agent->p;
	pipe (ToAgentFD);

	if (pid = fork (), pid == 0) {	/* in the child */
		static char     exec_env[MAXPATHLEN];

		umask (deflogin.umask);

		/* Set from /etc/default/login file */

		putenv (deflogin.path);

		/*
		 * Child end of the communication channel
		 */

		close (0);
		dup (ToAgentFD[0]);

		signal (SIGPIPE, SIG_DFL);

		/*
		 * Setting the exec_env so that it can be picked up by
		 * tsoljds-xagent during first invocation of the command.
		 */

		strcpy (exec_env, "LABEL_EXEC_COMMAND=");
		if (fullcmd)
			strcat (exec_env, fullcmd);
		else 
			strcat (exec_env, "");
		putenv (exec_env);

		escalate_inherited_privs (); 
		execl ("/usr/bin/tsoljds-xagent-proxy", "tsoljds-xagent-proxy", zoneintstr, p->pw_name, 0);
		drop_inherited_privs ();

		/*
		 * Error - command could not be exec'ed.
		 */
		_exit (127);
	} else if (pid > 0) {	/* in the parent */
		agent->fd = ToAgentFD[1];
		agent->pid = pid;
		(void) close (ToAgentFD[0]);
		free (zoneintstr);
	} else if (pid == -1) {	/* error occurred in fork () */
		fprintf (stderr, "fork() failed \n");
	}
}

static void
set_root_window_background_colour_for_label (Display *dpy, int screen_num,
					     Window root_win, m_label_t *label)
{
	char *colorname;
	XColor color;

	label_to_str (label, &colorname, M_COLOR, DEF_NAMES);

	XParseColor (dpy, DefaultColormap (dpy, screen_num), colorname, &color);
	XAllocColor (dpy, DefaultColormap (dpy, screen_num), &color);

	XSetWindowBackground (dpy, root_win, color.pixel); 
	XClearWindow (dpy, root_win);
	free(colorname);
}

static void
update_desktop_window (WnckWorkspace *active_ws)
{
	char           *value;
	const char     *label, *role;
	m_label_t      *sl = NULL;
	int             error;
	gint		    screennumber;
	uid_t           uid;
	User           *workstationowner, *roleuser;
	zoneid_t        zid;
	struct passwd  *pw;
	GdkScreen      *gdk_screen;
	GdkDisplay     *gdk_dpy;
	Display        *x_dpy;
	Window          root_win = None;
	Atom            utf8_string, ndw_atom;
	WnckScreen     *wnckscreen;

	wnckscreen = wnck_workspace_get_screen (active_ws);
	screennumber = wnck_screen_get_number (wnckscreen);
	g_assert (screennumber >= 0);

	gdk_dpy = gdk_display_get_default ();
	x_dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);
	gdk_screen = gdk_display_get_screen (gdk_dpy, screennumber);

	role = wnck_workspace_get_role (active_ws);
	label = wnck_workspace_get_label (active_ws);

	if (!role) {
		uid = getuid ();
	} else {
		/*
		 * Workspace could be either a role or a normal user
		 * workspace.
		 */
		workstationowner = (User *) g_slist_nth_data (users, 0);
		if (strcmp (role, workstationowner->p->pw_name)) {
			int             roleok = 0;

			/*
			 * Find the User struct corresponding to this role.
			 */
			if ((roleuser = _tstripe_user_find_user_by_name (role)) != NULL)
				roleok = 1;

			/* No User struct found so the role is invalid */
			if (!roleok) {
				GtkWidget      *dialog;

				/*
				 * Drop the role from the ws and break out.
				 * The role change will be handled in the
				 * next invocation of the callback.
				 */
				_tstripe_set_workspace_role (gdk_screen,
						workstationowner->p->pw_name,
				     wnck_workspace_get_number (active_ws));
				dialog = gtk_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							     GTK_BUTTONS_OK,
								 _ ("The saved role: \"%s\" is no longer granted to this user.\n"
				     "See your system administrator"), role,
								 NULL);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				return;
			}
			/*
			 * If the role hasn't been authenticated it needs to be, before going any further.
			 */
			if ((roleok) && (roleuser->authenticated == FALSE)) {
				if (0) { //_tstripe_role_authentication_top_level ((char *) role, gdk_screen)) {
					GtkWidget      *dialog;
					char *saved = g_strdup (role);

					/*
					 * Authentication failed or was
					 * cancelled. Drop the role.
					 */
					_tstripe_set_workspace_role (gdk_screen,
						workstationowner->p->pw_name,
								     wnck_workspace_get_number (active_ws));
					dialog = gtk_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							   GTK_MESSAGE_INFO,
							     GTK_BUTTONS_OK,
									 _ ("The role: \"%s\" has been removed from this workspace"),
								       saved,
								      NULL);
					gtk_dialog_run (GTK_DIALOG (dialog));
					gtk_widget_destroy (dialog);
					g_free (saved);
					return;
				} else {
					/* start an xagent */
					struct passwd *p = NULL;
					XAgent *agent = g_new0 (XAgent, 1);
					p = &agent->p;
					agent->fd = 0;
					getpwnam_r (role, p, agent->pw_buffer, PW_BUF_LEN, &p);
					str_to_label (label, &agent->sl, MAC_LABEL, L_NO_CORRECTION, &error); 
					handle_command (agent, NULL);
					
				}
			}
		}
		pw = getpwnam (role);
		uid = pw->pw_uid;
	}

	if (!label) {
		g_warning ("Workspace created without a sensitivity label\n");
		return;
	}
	if (str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
		g_warning ("Error %d:could not convert label \"%s\"", error, label);
		return;
	}
	zid = getzoneidbylabel (sl);

	value = g_strdup_printf ("%d_%d", uid, zid);

	root_win = GDK_WINDOW_XID (gdk_screen_get_root_window (gdk_screen));

	set_root_window_background_colour_for_label (x_dpy, screennumber, 
						     root_win, sl);

	utf8_string = XInternAtom (x_dpy, "UTF8_STRING", False);

	ndw_atom = XInternAtom (x_dpy, "NAUTILUS_ACTIVE_DESKTOP_ID", False);

	gdk_error_trap_push ();
	XChangeProperty (x_dpy, root_win, ndw_atom, utf8_string, 8, PropModeReplace,
			 (guchar *) value, strlen (value));

	XSync (x_dpy, False);
	gdk_error_trap_pop ();

	g_free (value);

}

void
ChildDieHandler (int sig, siginfo_t * sinf, void *ucon)
{
	int status, i;

	waitpid (sinf->si_pid, &status, WNOHANG);

	printf ("child process die handler: pid = %d\n", sinf->si_pid);
	psiginfo (sinf, "Received signal");
	for (i = 0; i < AgentCount; i++) {
		if (localzones[i]->pid == sinf->si_pid) {
			localzones[i]->pid = -1;
			break;
		}
	}
}

static void
role_or_label_changed_callback (WnckWorkspace *space,
			gpointer user_data)
{
	WnckScreen     *wnckscreen;
	WnckWorkspace  *activeworkspace;
	const char     *label;
	const char     *role;
	m_label_t      *sl = NULL;

	label = wnck_workspace_get_label (space);
	role = wnck_workspace_get_role (space);

	if (!label) {
		g_warning ("Workspace created without a sensitivity label\n");
		return;
	} else {
		int             error;

		if (str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
			perror ("converting initial workspacelabel str_to_label()");
			g_warning ("error %d: could not convert label \"%s\"", error, label);
		} else {
			XAgent         *agent;
			struct passwd  *p;

			agent = g_new (XAgent, 1);
			agent->fd = 0;
			p = &agent->p;
			if (role)
				getpwnam_r (role, p, agent->pw_buffer, PW_BUF_LEN, &p);
			else
				getpwuid_r (geteuid (), p, agent->pw_buffer, PW_BUF_LEN, &p);
			agent->sl = sl;

			handle_command (agent, NULL);
		}

		/*
		 * If the role/label change happened on a non active workspace
		 * the desktop doesn't need to be updated.
		 */
		wnckscreen = wnck_workspace_get_screen (space);
		activeworkspace = wnck_screen_get_active_workspace (wnckscreen);
		if (wnck_workspace_get_number (space) == wnck_workspace_get_number (activeworkspace))
			update_desktop_window (space);
	}
}

static void
active_workspace_changed_callback (WnckScreen *screen,
			   gpointer user_data)
{	
	update_desktop_window (wnck_screen_get_active_workspace (screen));
	role_or_label_changed_callback (wnck_screen_get_active_workspace (screen), NULL);
}

static void
workspace_created_callback (WnckScreen * screen,
			    WnckWorkspace * space,
			    gpointer user_data)
{
	XAgent         *agent;
	struct passwd  *p = NULL;
	m_label_t      *sl = NULL;
	int             error;
	const char     *label, *role;

	g_signal_connect (G_OBJECT (space), "label_changed",
			  G_CALLBACK (role_or_label_changed_callback),
			  NULL);
	g_signal_connect (G_OBJECT (space), "role_changed",
			  G_CALLBACK (role_or_label_changed_callback),
			  NULL);

	label = wnck_workspace_get_label (space);

	if (!label) {
		g_warning ("Workspace created without a sensitivity label\n");
		return;
	}
	if (str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
		/*
		 * Something's wrong with the workspace - bail out for now.
		 * FIXME - consider deleting the workspace entirely or assign
		 * it the min label?
		 */
		g_warning ("Workspace created with an invalid label: \"%s\"", label);
		return;
	}
	role = wnck_workspace_get_role (space);

	agent = g_new (XAgent, 1);
	agent->fd = 0;
	p = &agent->p;
	if (role) {
		/*
		 * Assume the role is valid since it was checked and created
		 * in button.c
		 */
		getpwnam_r (role, p, agent->pw_buffer, PW_BUF_LEN, &p);
	} else {
		/*
		 * FIXME - why is the effective UID used instead of the real
		 * UID here ?? Investigate using getuid_r() to see what
		 * happens.
		 */
		getpwuid_r (geteuid (), p, agent->pw_buffer, PW_BUF_LEN, &p);
	}
	agent->sl = sl;
	handle_command (agent, NULL);

	update_desktop_window (space);
	return;
}

void
start_xagent_for_workspaces ()
{
	WnckScreen     *screen;
	int             total_scr, scrno, no_of_workspace, i, error;

	drop_inherited_privs ();

	total_scr = gdk_display_get_n_screens (gdk_display_get_default ());

	for (scrno = 0; scrno < total_scr; scrno++) {
		screen = wnck_screen_get (scrno);
		wnck_screen_force_update (screen);
		update_desktop_window (wnck_screen_get_active_workspace (screen));
		no_of_workspace = wnck_screen_get_workspace_count (screen);

		for (i = 0; i < no_of_workspace; i++) {
			WnckWorkspace  *wksp;
			const char     *label, *role;
			m_label_t      *sl = NULL;

			wksp = wnck_screen_get_workspace (screen, i);

			label = wnck_workspace_get_label (wksp);
			role = wnck_workspace_get_role (wksp);

			if (!label && !role) {
				g_warning ("Workspace created without a sensitivity label or role\n");
				return;
			}
			if (label) {
				if (str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
					g_warning ("Error %d:could not convert label \"%s\"", error, label);
					continue;
				} else {
					XAgent         *agent;
					struct passwd  *p;
					User *roleuser = NULL;
					User *workstationowner = NULL;
					gboolean userisrole = FALSE;

					agent = g_new (XAgent, 1);
					agent->fd = 0;
					p = &agent->p;
					if (role) {
						getpwnam_r (role, p, agent->pw_buffer, PW_BUF_LEN, &p);
						roleuser = _tstripe_user_find_user_by_name (role);
						workstationowner = (User *) g_slist_nth_data (users, 0);
						if (strcmp (role, workstationowner->p->pw_name))  {
							userisrole = TRUE;
						}
					} else {
						getpwuid_r (geteuid (), p, agent->pw_buffer, PW_BUF_LEN, &p);
					}

					agent->sl = sl;

					g_signal_connect (G_OBJECT (wksp), "label_changed",
							  G_CALLBACK (role_or_label_changed_callback),
							  NULL);
					g_signal_connect (G_OBJECT (wksp), "role_changed",
							  G_CALLBACK (role_or_label_changed_callback),
							  NULL);
					if (i == 0) {
					if (userisrole) {
						if (roleuser->authenticated) {
							handle_command (agent, 
									NULL);
						}
					} else {
						handle_command (agent, NULL);
					}
					}

				}
			}
		}

		update_desktop_window (wnck_screen_get_active_workspace (screen));

		g_signal_connect (G_OBJECT (screen), "active_workspace_changed",
			     G_CALLBACK (active_workspace_changed_callback),
				  NULL);

		g_signal_connect (G_OBJECT (screen), "workspace_created",
				  G_CALLBACK (workspace_created_callback),
				  NULL);
	}

	load_trusted_executables_list ();
}
