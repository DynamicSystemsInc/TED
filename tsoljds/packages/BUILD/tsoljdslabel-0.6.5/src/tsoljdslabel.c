/*Solaris Trusted Extensions GNOME desktop application.

  copyroght (c) 2007, 2012 Oracle and/or its affiliates. All rights reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

*/

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gspawn.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include <zone.h>
#include <priv.h>
#include <sys/contract/process.h>
#include <libcontract.h>
#include <sys/ctfs.h>
#include <sys/utsname.h>
#include <stropts.h>
#include <wait.h>
#include <signal.h>

#include <strings.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <label_builder.h>
#include <userattr.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <X11/extensions/Xtsolproto.h>


#include "tsolmotd.h"
#include "exec.h"

GtkWidget      *lbuilder = NULL;

bslabel_t       lower_sl;
bclear_t        upper_clear;
pid_t           session_pid = -1;
uid_t           session_uid;

int		savedmode;

char           *desktop_session[4] = {NULL, NULL, NULL, NULL};

#define CLOSE_FILES_ON_EXEC() \
{int ifx; for (ifx=3; ifx < _NFILE; ifx++) (void) fcntl (ifx, F_SETFD, 1);}

#define MSG_SINGLE _("Solaris Trusted Extensions: Setting Session Sensitivity")
#define MSG_MULTI  _("Solaris Trusted Extensions: Setting Session Clearance")

GPid screensaver_pid = -1;

static void
stop_screensaver (void) 
{
	if (screensaver_pid != -1) {
		kill (screensaver_pid, SIGTERM);
		screensaver_pid = -1;
	}
}

static void
start_screensaver (void) 
{
	GError *err = NULL;
	gchar *args[3];

	args[0] = "/usr/bin/xscreensaver";
	args[1] = "-nosplash";
	args[2] = NULL;
	g_spawn_async (g_get_home_dir (), args, NULL, G_SPAWN_STDOUT_TO_DEV_NULL, NULL, NULL, 
		       &screensaver_pid, &err);
	g_error_free (err);
}

blrange_t      *
get_display_range ()
{
	blrange_t *range = NULL;

	range = getdevicerange ("framebuffer");
	if (range == NULL) {
		range = g_malloc (sizeof (blrange_t));
		range->lower_bound = blabel_alloc ();
		range->upper_bound = blabel_alloc ();
		bsllow  (range->lower_bound);
		bslhigh (range->upper_bound);
	}

	return (range);
}

static void
ChildDeathHandler ()
{
	struct sigaction sa;
	int             stat_loc;
	pid_t           pid;

	while ((pid = waitpid (-1, &stat_loc, WNOHANG)) > 0) {
		/* Session exited, so we exit */
		if (pid == session_pid) {
			system ("/usr/sbin/deallocate audio0");
			exit (0);
		}
	}

	/* Reset the SIGCHLD handler */
	sa.sa_flags = 0;
	sa.sa_handler = ChildDeathHandler;
	sigemptyset (&sa.sa_mask);
	(void) sigaction (SIGCHLD, &sa, (struct sigaction *) 0);
}

static void
setup_signal_handlers ()
{
	struct sigaction sa;

	/* SIGCHLD handler */
	sa.sa_flags = 0;
	sa.sa_handler = ChildDeathHandler;
	sigemptyset (&sa.sa_mask);
	(void) sigaction (SIGCHLD, &sa, (struct sigaction *) 0);
}

static int
setplabel (bslabel_t * sl)
{
	bslabel_t       admin_low;
	zoneid_t        zid;

	bsllow (&admin_low);

	/* NO-OP for admin_low */
	if (blequal (sl, &admin_low)) {
		return (0);
	}
	if ((zid = getzoneidbylabel (sl)) == -1) {
		return (-1);
	}
	if (zone_enter (zid) == -1) {
		return (-1);
	}
	return 0;
}

static int
init_template (void)
{
	int             fd;
	int             err = 0;

	fd = open64 (CTFS_ROOT "/process/template", O_RDWR);
	if (fd == -1)
		return -1;
	/*
	 * Deliver no events, don't inherit, and allow it to be orphaned.
	 */
	err |= ct_tmpl_set_critical (fd, 0);
	err |= ct_tmpl_set_informative (fd, 0);
	err |= ct_pr_tmpl_set_fatal (fd, CT_PR_EV_HWERR);
	err |= ct_pr_tmpl_set_param (fd, CT_PR_PGRPONLY | CT_PR_REGENT);
	if (err || ct_tmpl_activate (fd)) {
		(void) close (fd);
		return -1;
	}
	return fd;
}

static int
do_rm (const char* path, const struct stat *sb, 
		  int tflag, struct FTW *ftwbuf)
{
	switch (tflag) {
		case FTW_D:
		case FTW_DNR:
		case FTW_DP:
			if (rmdir (path) == -1)
				return -1;
			break;
		default:
			if (unlink (path) == -1) 
				return -1;
			break;
	}
	
	return (0);
}

static void
session_setup (void) 
{
	int fd;
	char *txfirstfile, *panelconfpath;

	txfirstfile = g_build_filename (g_get_home_dir (), "/.txfirst", NULL);

	if (!g_file_test (txfirstfile, G_FILE_TEST_EXISTS)) {
		/* First time user has run TX Desktop so rm -rf the panel
		   configuration to ensure they get the TX default */
		panelconfpath = g_build_filename (g_get_home_dir (), 
						  "/.gconf/apps/panel", NULL);
		if (nftw (panelconfpath, do_rm, 20, FTW_DEPTH) == -1) {
			g_warning ("Could not remove users panel configuration."
				   "\n%s %s", strerror (errno), panelconfpath);
		}
		g_free (panelconfpath);

		/* touch the first time flag file so we don't do this 
		   every time */
		fd = creat (txfirstfile, S_IRUSR | S_IWUSR);
		close (fd);
	}

	g_free (txfirstfile);

}

static gboolean
is_trusted_session (char **session)
{
	if (strncmp ("/usr/bin/mate-session", session[0], 22) == 0 &&
	    strncmp ("--trusted-session", session[1], 17) == 0) {
		return TRUE;	
	} else {
		return FALSE;
	}
}


static void
start_session (int mode, char *label)
{
	char           *cmd[10];
	priv_set_t     *pset;
	m_label_t      *sl;
	char           *lower_sl_str;
	int             tmpl_fd, i, error;
	pid_t           pid;
	char		zonename[ZONENAME_MAX];
	zoneid_t	zid;
	userattr_t     *u_ent = getuseruid (session_uid);

	session_setup ();

	stop_screensaver ();
	
	if (mode == LBUILD_MODE_SL && !is_trusted_session (desktop_session)) {
		/* FIXME: THIS CODE IS BROKEN AS zone_enter() FAILS */
		/* allocate the audio device in for the labeled zone */
		str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, &error);
	
		if ((zid = getzoneidbylabel (sl)) > 0) {
			if (getzonenamebyid (zid, zonename, 
			    sizeof (zonename)) == -1) {
				strcpy (zonename, GLOBAL_ZONENAME);
			}
		} else {
			strcpy (zonename, GLOBAL_ZONENAME);
		}

		i = 0;
		cmd[i++] = "/usr/sbin/allocate";
		cmd[i++] = "-U";
		cmd[i++] = u_ent->name;
		cmd[i++] = "-z";
		cmd[i++] = zonename;
		cmd[i++] = "audio0";
		cmd[i++] = NULL;
		exec_command (cmd, TRUE);

		/* now start a session in the appropriate local zone */
		if ((tmpl_fd = init_template ()) == -1) {
			perror ("Couldn't create contract");
			exit (1);
		}
		setup_signal_handlers ();

		if ((session_pid = fork ()) == 0) {
			/* Child process */
			char            env_var[50];
			char            home_env[MAXPATHLEN];
			struct utsname  uname_ent;
			char           *disp_name;

			/* Get  the global zone hostname */
			if (uname (&uname_ent) == -1) {
				perror ("tsoljdslabel: uname failed");
				exit (1);
			}
			(void) ct_tmpl_clear (tmpl_fd);
			CLOSE_FILES_ON_EXEC ();

			(void) ioctl (STDIN_FILENO, I_ANCHOR);
			(void) ioctl (STDOUT_FILENO, I_ANCHOR);
			(void) ioctl (STDERR_FILENO, I_ANCHOR);

			/* Enter the zone */
			str_to_label (label, &sl, MAC_LABEL, L_NO_CORRECTION, 
				      &error);
			if (setplabel (sl) == -1) {
				exit (1);
			}
			/* Reset the display */
			if ((disp_name = (char *) getenv ("DISPLAY")) == NULL) {
				disp_name = ":0";
			}
			if (disp_name[0] == ':') {
				strcpy (env_var, "DISPLAY");
				strcpy (home_env, uname_ent.nodename);
				strcat (home_env, disp_name);
				setenv (env_var, home_env, 1);
			}

			if (execv (desktop_session[0], desktop_session) == -1) {
				perror (desktop_session[0]);
				exit (1);
			}
		} else if (session_pid > 0) {
			/* parent process */
			(void) ct_tmpl_clear (tmpl_fd);
			(void) close (tmpl_fd);
			gtk_main_quit ();
		}
	} else {/* Multilabel or single label trusted session */
		uid_t uid;

		if (mode == LBUILD_MODE_SL) {
			lower_sl_str = g_strdup (label);
		} else {
			lower_sl_str = g_strdup (bsltoh (&lower_sl));
		}

		setenv ("USER_MIN_SL", lower_sl_str, 1);
		setenv ("USER_MAX_SL", label, 1);

		/*
		 * Set the session label range in the Xserver
		 */
		XTSOLsetResLabel (gdk_x11_get_default_xdisplay (), 0,
			SESSIONLO, &lower_sl);
		XTSOLsetResLabel (gdk_x11_get_default_xdisplay (), 0,
			SESSIONHI, &upper_clear);
		/*
		 * Need a roundtrip request to flush the queue
		 * before dropping privileges and starting the session
		 */
		XTSOLgetWorkstationOwner (gdk_x11_get_default_xdisplay (), &uid);

		g_free (lower_sl_str);

		/*
		 * Make Inheritable same as Permitted
		 */
		if ((pset = priv_allocset ()) == NULL) {
			perror ("Can't allocate priv set\n");
			exit (1);
		}
		if (getppriv (PRIV_PERMITTED, pset) == -1) {
			perror ("Can't get process privileges\n");
			exit (1);
		}
		/*
		pset = priv_str_to_set("basic,win_config,win_dac_write", ",", NULL);
		*/

		if (setppriv (PRIV_SET, PRIV_INHERITABLE, pset) == -1) {
			perror ("Can't set process privileges\n");
			exit (1);
		}
		if (execv (desktop_session[0], desktop_session) == -1) {
			perror (desktop_session[0]);
			exit (1);
		}
	}
}

static void
lbuilder_response_cb (GtkDialog * dialog, gint id)
{
	int             mode;
	m_label_t      *sl;
	char           *label;
	GError	       *err = NULL;

	GnomeLabelBuilder *lbuilder = GNOME_LABEL_BUILDER (dialog);

	switch (id) {
	case GTK_RESPONSE_OK:
		g_object_get (G_OBJECT (lbuilder), "sl", &sl, "mode", &mode,
			      NULL);
		mode = savedmode;
		label_to_str (sl, &label, M_INTERNAL, DEF_NAMES);
		gtk_widget_destroy (GTK_WIDGET (lbuilder));
		start_session (mode, label);
		break;
	case GTK_RESPONSE_HELP:
		gnome_label_builder_show_help (GTK_WIDGET (lbuilder));
		break;
	case GTK_RESPONSE_CANCEL:
		/* We dont want to login so lets get out of here */
		stop_screensaver ();
		gtk_main_quit ();
		break;
	default:
		/* We shouldn't really have got here */
		break;
	}

	return;
}

static void 
show_label_builder (char* msg, int mode)
{
	savedmode = mode;
	lbuilder = g_object_new (GNOME_TYPE_LABEL_BUILDER,
				     "mode", LBUILD_MODE_CLR,
                                     "message", msg,
                                     "lower", &lower_sl,
                                     "upper", &upper_clear,
                                     "sl", &upper_clear,
                                     NULL);
	g_signal_connect (G_OBJECT (lbuilder), "response",
			  G_CALLBACK (lbuilder_response_cb), NULL);

	gtk_widget_show_all (lbuilder);

	g_object_set (G_OBJECT (lbuilder), "sl", &upper_clear, NULL);
}


static void
motd_response_cb (GtkDialog * dialog, gint id, gpointer data)
{
	char      *label;
	bslabel_t admin_high ,admin_low;


	MotdData *motd_data = (MotdData *) g_object_get_data (G_OBJECT (dialog),
							       "motd_data");

	gtk_widget_hide(GTK_WIDGET(dialog));
	if (motd_data->checkbutton) {
		if (gtk_toggle_button_get_active (
		      GTK_TOGGLE_BUTTON (motd_data->checkbutton))) {
			tsol_motd_dialog_destroy (GTK_WIDGET (dialog));
			show_label_builder (MSG_SINGLE, LBUILD_MODE_SL);
		} else {
			/* Admin user ML session? no need for lbuilder 
			   start session with admin_high clearance */
			bsllow (&admin_low);
			bslhigh (&admin_high);
			if (blequal (&admin_high, &upper_clear) && 
   	    		    blequal (&admin_low, &lower_sl)) {
				tsol_motd_dialog_destroy (GTK_WIDGET (dialog));
				label_to_str (&admin_high, &label, M_INTERNAL,
					      DEF_NAMES);
				start_session (LBUILD_MODE_CLR, label);
				g_free (label);
			} else { 
				tsol_motd_dialog_destroy (GTK_WIDGET (dialog));
				show_label_builder (MSG_MULTI, LBUILD_MODE_CLR);
			}
		}
	} else {
		label = g_strdup (motd_data->label);
		tsol_motd_dialog_destroy (GTK_WIDGET (dialog));

		start_session (LBUILD_MODE_CLR, label);

		g_free (label);
	}
}

int
main (int argc, char *argv[])
{
	GtkWidget      *motd_dialog;
	char           *lower_sl_str;
	char           *upper_clear_str;
	blrange_t      *display_range;
	int             err, i;
	userattr_t     *u_ent;
	MotdData       *motd_data;
	gboolean       sl_only = FALSE;
	priv_set_t	*pset;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	putenv ("GNOME_DISABLE_CRASH_DIALOG=1");

	gtk_init(&argc, &argv);

	session_uid = getuid ();
	u_ent = getuseruid (session_uid);

	/* Set the workstation owner */
	XTSOLsetWorkstationOwner (gdk_x11_get_default_xdisplay (), &session_uid);

	start_screensaver ();

	/* Get minimum label */
	if ((lower_sl_str = gnome_tsol_get_usrattr_val (u_ent,
					      USERATTR_MINLABEL)) == NULL) {
		perror ("tsoljdslabel:Can't get minimum label");
		exit (1);
	}
	/* Get upper_clear */
	if ((upper_clear_str = gnome_tsol_get_usrattr_val (u_ent,
					     USERATTR_CLEARANCE)) == NULL) {
		perror ("tsoljdslabel:Can't get upper_clear");
		exit (1);
	}
	/* Convert min label to binary */
	if (stobsl (lower_sl_str, &lower_sl, NEW_LABEL, &err) == 0) {
		perror ("tsoljdslabel:Can't convert min label");
		exit (1);
	}
	/* Convert upper_clear to binary */
	if (stobclear (upper_clear_str, &upper_clear, NEW_LABEL, &err) == 0) {
		perror ("tsoljdslabel:Can't convert upper_clear");
		exit (1);
	}
	/* Get display device range */
	if ((display_range = get_display_range ()) == NULL) {
		perror ("tsoljdslabel:Can't get display device range");
		exit (1);
	}
	/*
	 * Determine the low & high bound of the label range where the login
	 * user can operate. This is the intersection of display label range
	 * & user label range.
	 */
	blmaximum (&lower_sl, display_range->lower_bound);
	blminimum (&upper_clear, display_range->upper_bound);
		
	for (i = 0; i < argc -1 && i < 4  ; i++) {
		desktop_session[i] = g_strdup (argv[i+1]);
	}
	
	if (is_trusted_session (desktop_session)) {
		sl_only = FALSE;
	} else {
		sl_only = TRUE;
	}

	motd_dialog = tsol_motd_dialog_new (session_uid, lower_sl, upper_clear,
					    sl_only);

	g_signal_connect (G_OBJECT (motd_dialog), "response",
			  G_CALLBACK (motd_response_cb), motd_data);

	gtk_widget_show_all (motd_dialog);

	gtk_main ();

	/* if started a session in another zone lets wait for it */
	if (session_pid != -1) {
		while (wait (NULL) != session_pid);
	}

	return 0;
}
