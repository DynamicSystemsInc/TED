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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <syslog.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <glib-2.0/glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <libgnometsol/userattr.h>
#include <libgnometsol/pam_conv.h>
#include <bsm/adt_event.h>

#include "tsol-user.h"
//#include "ui-controller.h"

#define	DEF_ATTEMPTS	3	/* attempts to change password */

#define	PW_FALSE	1	/* no password change */
#define	PW_TRUE		2	/* successful password change */
#define	PW_FAILED	3	/* failed password change */

conv_info_t     c_info;		/* appdata passed to conversation */
adt_session_data_t *audithandle = NULL;	/* Audit session handle */
int             usercount = 0;

static void     free_conv_info_data (conv_info_t * conv_info);
static int
_tstripe_solaris_authenticate (char *prog_name,
			       char *display_name,
			       char *user,
			       char *line);

static void     audit_su_success (int pw_change, User * user);
static void     audit_su_failure (int pw_change, User * user, pam_handle_t * pamh, int pamerr);
static void     audit_chauthtok_success (int pw_change, User * user);
static void     audit_chauthtok_failure (int pw_change, User * user, pam_handle_t * pamh, int pamerr);

void
_tstripe_users_init ()
{
	struct passwd  *p = NULL;
	static gint		pwbuflen = 0;
	gchar			*pw_buffer; /* Storage for the password */
	userattr_t     *u = NULL;
	userattr_t     *u_ent = NULL;
	User           *wsuser = NULL;
	uid_t           wsuid;
	char           *roles;
	int             duplicated_role = 0;

	if (pwbuflen == 0) {
		/* Determine the buffer length required for passwd details */
		pwbuflen = sysconf(_SC_GETPW_R_SIZE_MAX);
		g_assert(pwbuflen >= 0);
	}

	/* Initialise the global users list */
	users = NULL;
	u_ent = getuseruid (getuid ());
	/* Create the user struct for the workstation owner */
	wsuser = g_new0 (User, 1);
	wsuser->pw_buffer = g_new0(gchar, pwbuflen);

	XTSOLgetWorkstationOwner (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &wsuid);
	if (wsuid < 0) {
		GtkWidget      *errdialog;

		errdialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_ERROR,
						    GTK_BUTTONS_CLOSE,
						    _ ("The workstation owner of this system could not be determined\n"
						       "This means that your environment is probably corrupt or you are\n"
						       "attempting to launch the Trusted JDS trusted stripe from \n"
						       "outside of a Trusted JDS session. See your system administrator"));
		gtk_dialog_run (GTK_DIALOG (errdialog));
		gtk_widget_destroy (errdialog);
		exit (-1);
	}

	wsuser->p = g_new0(struct passwd, 1);
	getpwuid_r (wsuid, wsuser->p, wsuser->pw_buffer, PW_BUF_LEN, &p);
	g_assert (p != NULL);

	wsuser->u = getuseruid (wsuid);
	users = g_slist_append (users, (gpointer) wsuser);
	roles = g_strdup (gnome_tsol_get_usrattr_val (u_ent, USERATTR_ROLES_KW));
	usercount++;
	free_userattr(u_ent);

	if (roles != NULL) {
		GtkWidget      *dialog = NULL;
		char           *role = NULL;

		while ((role = strtok (roles, ",")) != NULL) {
			int             i;
			User           *user = NULL;
			User           *tmpuser = NULL;

			roles = NULL;
			duplicated_role = 0;
			if (!strncmp (role, "none", 4))
				break;

			/* Scan for duplicates and skip over them if found */
			for (i = 0; g_slist_nth_data (users, i) != NULL; i++) {
				tmpuser = (User *) g_slist_nth_data (users, i);
				if (!strcmp (role, (char *) tmpuser->p->pw_name))
					duplicated_role = 1;
			}
			if (duplicated_role)
				continue;

			user = g_new0 (User, 1);
			user->pw_buffer = g_new0(gchar, pwbuflen);
			if (usercount >= MAX_USERS) {
				dialog = gtk_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							     GTK_BUTTONS_OK,
				 _ ("Too many roles assigned to the user.\n"
				    "Some roles may have been ignored."),
								 NULL);
				gtk_widget_show_all (dialog);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				break;
			}

			user->p = g_new0(struct passwd, 1);

			if (getpwnam_r (role, user->p, user->pw_buffer, pwbuflen, &p) == -1) {
				dialog = gtk_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							     GTK_BUTTONS_OK,
					_ ("The role \"%s\" is an invalid\n"
					            "role on this system.\n"
					  "See your system administrator."),
								 role, NULL);
				gtk_widget_show_all (dialog);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				continue;
			} else if ((u = user->u = getusernam (role)) == NULL) {
				dialog = gtk_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							GTK_MESSAGE_WARNING,
							     GTK_BUTTONS_OK,
					  _ ("The user or role \"%s\" has\n"
					"an incomplete set of attributes.\n"
					  "See your system administrator."),
								 role, NULL);

				gtk_widget_show_all (dialog);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				continue;
			} else {
				user->authenticated = FALSE;
				users = g_slist_append (users, (gpointer) user);
				usercount++;
				free_userattr(u);
			}
		}
		g_free (roles);
		g_free (role);
	}
}

User           *
_tstripe_user_find_user_by_name (const char *username)
{
	User           *user = NULL;

	if (!username)		/* Just return the workstation owner */
		return (User *) g_slist_nth_data (users, 0);
	else {
		int             i;

		for (i = 0; g_slist_nth_data (users, i) != NULL; i++) {
			user = (User *) g_slist_nth_data (users, i);
			if (!strcmp (username, (char *) user->p->pw_name)) {
				break;
			}
		}
	}
	return user;
}

int
_tstripe_user_get_user_index (User * user)
{
	if (!user)
		return -1;
	return g_slist_index (users, user);
}

int
_tstripe_user_count_get ()
{
	return usercount;
}

int
_tstripe_authenticate_role (User * user, GdkScreen * screen, int r, int w)
{
	char           *ttyline = "/dev/console";
	char           *displayname = ":0";
	char 	       *service = "tsoljds-userlogin";
	struct passwd  *p = user->p;
	User		*wsuser = (User *) g_slist_nth_data (users, 0);
	int             status;

	/*
	 * Prevent user from mucking around with workspace during auth.
	 */
	c_info.sysmodal = TRUE; 
	c_info.screen = screen;
	c_info.readfd = r;
	c_info.writefd = w;

        if (p->pw_uid != wsuser->p->pw_uid) 
		service = GETTEXT_PACKAGE;

	status = _tstripe_solaris_authenticate (service, displayname, p->pw_name, ttyline);

	free_conv_info_data (&c_info);
	return (status);
}

/****************************************************************************
 * _tstripe_solaris_authenticate
 *
 * Authenticate that role / password combination is legal for this system
 *
 ****************************************************************************/
static int
_tstripe_solaris_authenticate (char *prog_name,
			       char *display_name,
			       char *user,
			       char *line)
{
	int             pam_err;
	int             pam_flags;
	int             pw_change = PW_FALSE;
	int             tries;
	uid_t           wsownerid;
	User           *roleuser;
	pam_handle_t   *pamh;	/* PAM authentication handle */
	struct passwd  *pwd_temp;
	const struct pam_conv tstp_conv = {gnometsol_pam_conv, (void *) &c_info};

	roleuser = _tstripe_user_find_user_by_name (user);

	/* Initialise the conv_info structure */
	c_info.prog_name = g_strdup (prog_name);	/* Prepended to syslog()
							 * messages in pam
							 * conversation */
	c_info.echoonmsg = NULL;
	c_info.echooffmsg = g_strdup_printf (_ ("Authentication is required for the %s role."), user);

	/* Open Solaris PAM (Plugable Authentication module ) connection */
	if ((pam_err = pam_start (prog_name, user, &tstp_conv, &pamh)) != PAM_SUCCESS) {
		syslog (LOG_AUTH | LOG_ALERT, "PAM failed to start: %s",
			pam_strerror (NULL, pam_err));
		return pam_err;
	}
	XTSOLgetWorkstationOwner (gdk_x11_get_default_xdisplay (), &wsownerid);
	pwd_temp = getpwuid (wsownerid);

	if ((pam_err = pam_set_item (pamh, PAM_AUSER, pwd_temp->pw_name)) != PAM_SUCCESS) {
		syslog (LOG_AUTH | LOG_ALERT, "PAM failed to set PAM_AUSER: %s",
			pam_strerror (NULL, pam_err));
		goto pamerr;
	}
	pam_flags = PAM_DISALLOW_NULL_AUTHTOK;

	/*
	 * Force the stripe to show trusted path for the current window label.
	 */
	//global_role_dialog_is_mapped = TRUE;
//	update_trusted_stripes ();

	if ((pam_err = pam_authenticate (pamh, pam_flags)) != PAM_SUCCESS) {
		audit_su_failure (PW_FALSE, (User *) g_slist_nth_data (users, 0), pamh, pam_err);
		goto pamerr;
	}
	pam_err = pam_acct_mgmt (pamh, pam_flags);
	switch (pam_err) {
	case PAM_NEW_AUTHTOK_REQD:
		tries = 0;
		g_free (c_info.echooffmsg);
		c_info.echooffmsg = g_strdup_printf (
						     _ ("The existing password for %s has expired and must be updated"), user);
		while ((pam_err = pam_chauthtok (pamh,
			      PAM_CHANGE_EXPIRED_AUTHTOK)) != PAM_SUCCESS) {
			if (pam_err == GNOME_TSOL_PAM_CANCEL)
				goto pamerr;
			else if ((pam_err == PAM_AUTHTOK_ERR ||
				  pam_err == PAM_TRY_AGAIN) &&
					(tries++ < DEF_ATTEMPTS))
				continue;
			audit_su_failure (PW_FAILED,
					  roleuser,
					  pamh,
					  pam_err);
			goto pamerr;
		}
		pw_change = PW_TRUE;
		break;
	case PAM_PERM_DENIED:
		audit_su_failure (PW_FALSE,
				  roleuser,
				  pamh,
				  pam_err);
		goto pamerr;
		break;		/* Statement not reached. Yeah I know */
	default:
		break;
	}

	audit_su_success (pw_change, roleuser);
pamerr:
	/* Reset normal stripe operation for window label viewing */
	//global_role_dialog_is_mapped = FALSE;
	//update_trusted_stripes ();
	/* GNOME_TSOL_PAM_CANCEL does not map to any real pam return code */
	if (pam_err == GNOME_TSOL_PAM_CANCEL)
		pam_end (pamh, PAM_CONV_ERR);
	else
		pam_end (pamh, pam_err);
	if (pam_err == PAM_SUCCESS)
		roleuser->authenticated = TRUE;
	/* conv_info data gets freed in caller: _tstripe_authenticate_role () */
	return (pam_err);
}

/*
 * _tstripe_solaris_setcred
 * 
 * Set Users login credentials: uid, gid, and group lists **************************************************************************
 */

int
_tstripe_solaris_setcred (const char *prog_name, const char *user, uid_t uid, gid_t gid)
{
	User           *thisuser = NULL, *wsuser = NULL;
	int             cred_type, status;
	pam_handle_t   *pamh;	/* PAM authentication handle */
	const struct pam_conv tstp_conv = {gnometsol_pam_conv, (void *) &c_info};

	/* Open PAM connection */
	if (users == NULL) {
		g_warning ("No Users list found!");
		return (-1);
	}
	printf ("starting pam for %s\n",user);
	status = pam_start (prog_name, user, &tstp_conv, &pamh);
	if (status != 0) {
		pam_end (pamh, status);
		return (status);
	}
	/* Find the corresponding User structure for this user name */
	if ((thisuser = _tstripe_user_find_user_by_name (user)) == NULL) {
		g_warning ("_tstripe_solaris_setcred(): No matching user details found for %s", user);
		return (-1);
	}

	printf ("setting pam item for %s\n", thisuser->p->pw_name);

	pam_set_item (pamh, PAM_USER, thisuser->p->pw_name);

	/*
	 * compare this user/role with the workstation owner which is always
	 * the first element in the list of Users
	 */
	wsuser = (User *) g_slist_nth_data (users, 0);
	if (uid != wsuser->p->pw_uid) {
		/*
		 * If this is a role workspace.
		 */
		cred_type = PAM_REINITIALIZE_CRED;
		printf("reinit cred\n");
	} else {
		/*
		 * The normal user's cred should have been set up by gdm or
		 * dtlogin
		 */
		printf("rerefresh cred\n");
		cred_type = PAM_REFRESH_CRED;
	}
	status = pam_setcred (pamh, cred_type);
	pam_end (pamh, status);

	setuid (thisuser->p->pw_uid);
	setgid (thisuser->p->pw_gid);
	initgroups (user, thisuser->p->pw_gid);

	return (status);
}


/****************************************************************************
 *_tstripe_solaris_chauthtok
 *
 * Change user/role password
 ****************************************************************************/

int
_tstripe_solaris_chauthtok (const char *prog_name, const char *user, uid_t uid, gid_t gid, GdkScreen * screen, int readfd, int writefd)
{
	GtkWidget      *error_dialog = NULL;
	User           *thisuser = NULL, *wsuser = NULL;
	int             tries = 0;
	int             status;
	char           *error_message = NULL;
	char 		buf[PAM_MAX_MSG_SIZE];
	pam_handle_t   *pamh;	/* PAM authentication handle */
	const struct pam_conv tstp_conv = {gnometsol_pam_conv, (void *) &c_info};

	if (users == NULL) {
		g_warning ("No users list found. Password changing for %s cancelled", user);
		return (-1);
	}
	/* Find the corresponding User structure for this user name */
	if ((thisuser = _tstripe_user_find_user_by_name (user)) == NULL) {
		g_warning ("_tstripe_solaris_chauthtok(): No matching user details found for %s", user);
		return (-1);
	}
	wsuser = (User *) g_slist_nth_data (users, 0);

	/*
	 * Initialise the conv info structure
	 */
	c_info.sysmodal = FALSE;/* Don't grab the display */
	c_info.prog_name = g_strdup (prog_name);
	c_info.echoonmsg = NULL;
	c_info.echooffmsg = g_strdup_printf (_ ("Changing password for %s"), user);
	c_info.screen = screen;
	c_info.writefd = writefd;
	c_info.readfd = readfd;

	/* Open PAM connection */
	status = pam_start (prog_name, user, &tstp_conv, &pamh);
	if (status != PAM_SUCCESS) {
		/* Free up the conv info struct */
		free_conv_info_data (&c_info);
		return (status);
	}
	/* Set users credentials */
	pam_set_item (pamh, PAM_USER, thisuser->p->pw_name);

	/* Authentication */
	status = pam_authenticate (pamh, 0);
	switch (status) {
	case PAM_SUCCESS:
	case GNOME_TSOL_PAM_CANCEL:	/* Hack - fake pam return code to
					 * indicate user cancelled */
		break;
		/* case PAM_USER_UNKNOWN: - Impossible! */
	case PAM_PERM_DENIED:
		error_message = g_strdup (_ ("Sorry. Permission denied."));
		break;
	case PAM_AUTH_ERR:
		error_message = g_strdup (_ ("Wrong password. Permission denied"));
		break;
	default:
		/* System error */
		error_message = g_strdup (_ ("Unexpected failure. Password unchanged"));
		break;
	}

	if (status != PAM_SUCCESS) {
		if (status == GNOME_TSOL_PAM_CANCEL) {
			/* Don't feed pam_end () with bogus error codes */
			status = PAM_CONV_ERR;
		} else {
			if (writefd != -1) {
				write (writefd, "f", 1);
				write (writefd, error_message, strlen (error_message));
			} else {
				error_dialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
							     error_message);
				gtk_window_set_screen (GTK_WINDOW (error_dialog), screen);
				gtk_dialog_run (GTK_DIALOG (error_dialog));
				gtk_widget_destroy (error_dialog);
			}
			g_free (error_message);
			/* FIXME - audit failure here */
			audit_chauthtok_failure (PW_FALSE, thisuser, pamh, status);
		}
		pam_end (pamh, status);
		free_conv_info_data (&c_info);
		return (status);
	}
	/* Account management */
	status = pam_acct_mgmt (pamh, 0);
	switch (status) {
	case PAM_SUCCESS:
		/* Ok to change password */
		break;
	case PAM_NEW_AUTHTOK_REQD:
		/*
		 * This is also ok since we're trying to change password
		 * anyway
		 */
		break;
	case PAM_ACCT_EXPIRED:
		error_message = g_strdup_printf (_ ("The user account for %s has expired. Permission denied."),
						 thisuser->p->pw_name);
		break;
	case PAM_AUTHTOK_EXPIRED:
		error_message = g_strdup (_ ("Permission denied."));
		break;
	}

	if ((status != PAM_SUCCESS) && (status != PAM_NEW_AUTHTOK_REQD)) {
		if (writefd != -1) {
			write (writefd, "f", 1);
			write (writefd, error_message, strlen (error_message));
		} else {
			error_dialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_CLOSE,
						       error_message);
			gtk_window_set_screen (GTK_WINDOW (error_dialog), screen);
			gtk_dialog_run (GTK_DIALOG (error_dialog));
			gtk_widget_destroy (error_dialog);
		}
		g_free (error_message);
		pam_end (pamh, status);
		free_conv_info_data (&c_info);
		/* FIXME - audit failure here */
		audit_chauthtok_failure (PW_FALSE, thisuser, pamh, status);
		return (status);
	}
	/* Finally, change the authentication token */
	tries = 1;
	status = pam_chauthtok (pamh, 0);
	while ((status != PAM_SUCCESS) && (tries <= DEF_ATTEMPTS)) {
		if ((status = pam_chauthtok (pamh, 0)) == GNOME_TSOL_PAM_CANCEL)
			break;
		tries++;
	}

	if (status != PAM_SUCCESS)
		error_message = g_strdup_printf (_ ("The password for %s has not been changed."), user);

	if (status != PAM_SUCCESS) {
		if (writefd != -1) {
			write (writefd, "f", 1);
			write (writefd, error_message, strlen (error_message));
		} else {
			error_dialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
						       GTK_MESSAGE_ERROR,
						       GTK_BUTTONS_CLOSE,
						       error_message);
			gtk_window_set_screen (GTK_WINDOW (error_dialog), screen);
			gtk_dialog_run (GTK_DIALOG (error_dialog));
			gtk_widget_destroy (error_dialog);
		}
		g_free (error_message);
		pam_end (pamh, status);
		free_conv_info_data (&c_info);
		/* FIXME - audit failure here */
		audit_chauthtok_failure (PW_FALSE, thisuser, pamh, status);
		return (status);
	}
	pam_end (pamh, status);
	free_conv_info_data (&c_info);
	if (writefd != -1) write (writefd, "s", 1);
	/* FIXME - audit success here */
	audit_chauthtok_success (PW_FALSE, thisuser);
	return (status);
}

static void
free_conv_info_data (conv_info_t * conv_info)
{
	g_return_if_fail (conv_info != NULL);
	if (conv_info->prog_name) {
		g_free (conv_info->prog_name);
		conv_info->prog_name = NULL;
	}

	if (conv_info->echoonmsg) {
		g_free (conv_info->echoonmsg);
		conv_info->echoonmsg = NULL;
	}
	if (conv_info->echooffmsg) {
		g_free (conv_info->echooffmsg);
		conv_info->echooffmsg = NULL;
	}
}

/*
 * audit_su_success - audit successful role assumption.
 * 
 * Entry	process audit context established -- i.e., pam_setcred() or
 * equivalent called. pw_change = PW_TRUE, if successful password change
 * audit required. user = User structure for new user.
 */

static void
audit_su_success (int pw_change, User * user)
{
	adt_event_data_t *event;
	struct passwd  *pwd = user->p;

	if (!audithandle) {
		/*
		 * Create the audit session handle with the audit context
		 * data taken from the process. The audit context would have
		 * been correctly set at login by pam_setcred()
		 */
		if (adt_start_session (&audithandle, NULL, ADT_USE_PROC_DATA) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
				"adt_start_session(ADT_role_login, ADT_FAILURE): %m");
			audithandle = NULL;
			return;
		}
	}
	/* since proc uid/gid not yet updated */
	/*
	 * FIXME - actually proc uid/gid are never going to be updated. so
	 * use ADT_UPDATE instead of ADT_USER for now.
	 */
	if (adt_set_user (audithandle, pwd->pw_uid, pwd->pw_gid, pwd->pw_uid,
			  pwd->pw_gid, NULL, ADT_UPDATE) != 0) {
		/* pwd->pw_gid, NULL, ADT_USER) != 0) { */
		syslog (LOG_AUTH | LOG_ERR,
			"adt_set_user(ADT_role_login, ADT_FAILURE): %m");
	}
	if ((event = adt_alloc_event (audithandle, ADT_role_login)) == NULL) {
		syslog (LOG_AUTH | LOG_ALERT, "adt_alloc_event(ADT_role_login): %m");
	} else if (adt_put_event (event, ADT_SUCCESS, ADT_SUCCESS) != 0) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_put_event(ADT_role_login, ADT_SUCCESS): %m");
	}
	if (pw_change == PW_TRUE) {
		/* Also audit password change */
		adt_free_event (event);
		if ((event = adt_alloc_event (audithandle, ADT_passwd)) == NULL) {
			syslog (LOG_AUTH | LOG_ALERT,
				"adt_alloc_event(ADT_passwd): %m");
		} else if (adt_put_event (event, ADT_SUCCESS,
					  ADT_SUCCESS) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
			      "adt_put_event(ADT_passwd, ADT_SUCCESS): %m");
		}
	}
	adt_free_event (event);
}


/*
 * audit_su_failure - audit failed role assumption.
 * 
 * Entry	New audit context not set. pw_change == PW_FALSE, if no password
 * change requested. PW_FAILED, if failed password change audit required.
 * user = NULL, or User structure to use. pamh = Pam session handle for the
 * converstion. pamerr = PAM error code; reason for failure.
 */

static void
audit_su_failure (int pw_change, User * user, pam_handle_t * pamh, int pamerr)
{
	adt_event_data_t *event;/* event to generate */
	struct passwd  *pwd = user->p;

	if (!audithandle) {
		if (adt_start_session (&audithandle, NULL, ADT_USE_PROC_DATA) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
				"adt_start_session(ADT_role_login, ADT_FAILURE): %m");
			audithandle = NULL;
			return;
		}
	}
	/*
	 * We have a bit of a problem here since pam_set_cred
	 * (PAM_REINITIALIZE_CRED) wasn't called for the role in this
	 * process, but in the forked chld process, so the audit context
	 * needs to be updated. FIXME
	 */

	/* target user authenticated, merge audit state */
	if (adt_set_user (audithandle, pwd->pw_uid, pwd->pw_gid, pwd->pw_uid,
			  pwd->pw_gid, NULL, ADT_UPDATE) != 0) {
		syslog (LOG_AUTH | LOG_ERR,
			"adt_set_user(ADT_role_login, ADT_FAILURE): %m");
	}
	if ((event = adt_alloc_event (audithandle, ADT_role_login)) == NULL) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_alloc_event(ADT_role_login, ADT_FAILURE): %m");
		return;
	} else if (adt_put_event (event, ADT_FAILURE,
				  ADT_FAIL_PAM + pamerr) != 0) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_put_event(ADT_role_login (ADT_FAIL, %s): %m",
			pam_strerror (pamh, pamerr));
	}
	if (pw_change != PW_FALSE) {
		/* Also audit password change failed */
		adt_free_event (event);
		if ((event = adt_alloc_event (audithandle, ADT_passwd)) == NULL) {
			syslog (LOG_AUTH | LOG_ALERT,
				"adt_alloc_event(ADT_passwd): %m");
		} else if (adt_put_event (event, ADT_FAILURE,
					  ADT_FAIL_PAM + pamerr) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
			      "adt_put_event(ADT_passwd, ADT_FAILURE): %m");
		}
	}
	adt_free_event (event);
}


/*
 * audit_chauthtok_success - audit successful auth token update.
 * 
 * Entry	process audit context established -- i.e., pam_setcred() or
 * equivalent called. pw_change = PW_TRUE, if successful password change
 * audit required. user = User structure for new user.
 */

static void
audit_chauthtok_success (int pw_change, User * user)
{
	adt_event_data_t *event;
	struct passwd  *pwd = user->p;

	if (!audithandle) {
		if (adt_start_session (&audithandle, NULL, ADT_USE_PROC_DATA) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
			  "adt_start_session(ADT_passwd, ADT_FAILURE): %m");
			audithandle = NULL;
			return;
		}
	}
	if (adt_set_user (audithandle, pwd->pw_uid, pwd->pw_gid, pwd->pw_uid,
			  pwd->pw_gid, NULL, ADT_UPDATE) != 0) {
		syslog (LOG_AUTH | LOG_ERR,
			"adt_set_user(ADT_passwd, ADT_FAILURE): %m");
	}
	if ((event = adt_alloc_event (audithandle, ADT_passwd)) == NULL) {
		syslog (LOG_AUTH | LOG_ALERT, "adt_alloc_event(ADT_passwd): %m");
	} else if (adt_put_event (event, ADT_SUCCESS, ADT_SUCCESS) != 0) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_put_event(ADT_passwd, ADT_SUCCESS): %m");
	}
	adt_free_event (event);
}


/*
 * audit_chauthtok_failure - audit failed auth token update.
 * 
 * Entry	New audit context not set. pw_change == PW_FALSE, if no password
 * change requested. PW_FAILED, if failed password change audit required.
 * user = NULL, or User structure to use. pamh = Pam session handle for the
 * converstion. pamerr = PAM error code; reason for failure.
 */

static void
audit_chauthtok_failure (int pw_change, User * user, pam_handle_t * pamh, int pamerr)
{
	adt_event_data_t *event;/* event to generate */
	struct passwd  *pwd = user->p;

	if (!audithandle) {
		if (adt_start_session (&audithandle, NULL, ADT_USE_PROC_DATA) != 0) {
			syslog (LOG_AUTH | LOG_ALERT,
			  "adt_start_session(ADT_passwd, ADT_FAILURE): %m");
			audithandle = NULL;
			return;
		}
	}
	/* target user authenticated, merge audit state */
	if (adt_set_user (audithandle, pwd->pw_uid, pwd->pw_gid, pwd->pw_uid,
			  pwd->pw_gid, NULL, ADT_UPDATE) != 0) {
		syslog (LOG_AUTH | LOG_ERR,
			"adt_set_user(ADT_passwd, ADT_FAILURE): %m");
	}
	if ((event = adt_alloc_event (audithandle, ADT_passwd)) == NULL) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_alloc_event(ADT_passwd, ADT_FAILURE): %m");
		return;
	} else if (adt_put_event (event, ADT_FAILURE,
				  ADT_FAIL_PAM + pamerr) != 0) {
		syslog (LOG_AUTH | LOG_ALERT,
			"adt_put_event(ADT_passwd(ADT_FAIL, %s): %m",
			pam_strerror (pamh, pamerr));
	}
	adt_free_event (event);
}
