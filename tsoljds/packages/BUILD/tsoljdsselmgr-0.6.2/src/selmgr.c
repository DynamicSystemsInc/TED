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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <priv.h>

#include <gdk/gdkx.h>
#include <glib/gslist.h>

#include <sys/param.h>
#include <user_attr.h>

#include <bsm/adt.h>
#include <bsm/libbsm.h>
#include <bsm/audit_uevents.h>
#include <bsm/adt_event.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <X11/extensions/Xtsol.h>

#include <auth_attr.h>
#include <secdb.h>
#include <pwd.h>

#include <libgnome/gnome-help.h>
#include <libgnomeui/libgnomeui.h>

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

#include <sel_config.h>

#include "selmgr-dialog.h"

#define SEL_AGNT_ATOM   "_TSOL_SEL_AGNT"
#define GCONF_KEY_TIMEDIFF   "/desktop/gnome/interface/tsoljdsselmger_timediff"
#define MAX_WINDOWS 20
#define TIMEOUT_VAL     120000	/* in millisecs. 2mins */

#define BYPASS_FILE_VIEW_AUTH   "solaris.label.win.noview"
#define WIN_DOWNGRADE_SL_AUTH   "solaris.label.win.downgrade"
#define FILE_DOWNGRADE_SL_AUTH  "solaris.label.file.downgrade"
#define WIN_UPGRADE_SL_AUTH     "solaris.label.win.upgrade"
#define FILE_UPGRADE_SL_AUTH    "solaris.label.file.upgrade"
#define SYS_ACCRED_SET_AUTH     "solaris.label.range"

#define WDOWNGRADE_AUTHORISED(uid) (uid_has_auth (uid, WIN_DOWNGRADE_SL_AUTH))
#define WUPGRADE_AUTHORISED(uid) (uid_has_auth (uid, WIN_UPGRADE_SL_AUTH))
#define FDOWNGRADE_AUTHORISED(uid) (uid_has_auth (uid, FILE_DOWNGRADE_SL_AUTH))
#define FUPGRADE_AUTHORISED(uid) (uid_has_auth (uid, FILE_UPGRADE_SL_AUTH))
#define OUTSIDE_AUTHORISED(uid) (uid_has_auth (uid, SYS_ACCRED_SET_AUTH))
#define VIEWNONE_AUTHORISED(uid) (uid_has_auth (uid, BYPASS_FILE_VIEW_AUTH))

typedef struct selwindow {
	Window          window;
	gboolean        in_use;
}               SelWindow;

typedef struct transfer_info {
	Display        *dpy;
	Window          window;
	XSelectionRequestEvent orig_event;
	XSelectionEvent reply_event;
	bslabel_t       holder_label;
	uid_t           holder_uid;
	Window          holder_window;
	bslabel_t       requestor_label;
	uid_t           requestor_uid;
	gid_t           requestor_gid;
	au_id_t         requestor_auditid;
	pid_t           requestor_pid;
	Window          requestor_window;
	struct timeval  starttime;
	TransferType    transfer_type;
	gboolean        outside_accred_range;
	gboolean        data_viewed;

	/* these are used to cache the transfer */
	Atom            actual_type;
	int             actual_format;
	unsigned long   nitems;
	unsigned long   bytes_after;
	unsigned char  *prop;

	/* Fields used in handling duplicate requests */
	struct timeval  endtime;
	gboolean        transfer_accepted;
	unsigned long   data_checksum;

	/* used for file transfers */
	gboolean file_transfer;
	unsigned char  *dest_dir;
}               SelInfo;

static XContext sel_info_context;
static Atom     ATOM_MULTIPLE;
static Atom     ATOM_PROXY_COPY;
static Atom     ATOM_COPY_FILES;
static gboolean audit_upgraded_data;
static gboolean audit_downgraded_data;
static SelWindow transfer_windows[MAX_WINDOWS];
static SelInfo *saved_sel_info = NULL;
static int saved_timediffval = 1000;
static adt_session_data_t *audit_handle = NULL;

AutoReply       auto_replies;
AutoConfirm     auto_confirm_settings = {
	{0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static void
save_new_selinfo_and_destroy_old (SelInfo * sel_info)
{
	XDeleteContext (sel_info->dpy, sel_info->window, sel_info_context);

	if (sel_info->prop) {
		g_free (sel_info->prop);
	}
	XSync (sel_info->dpy, FALSE);

	if (sel_info->window) {
		register int    i;

		for (i = 0; i < MAX_WINDOWS; i++) {
			if (transfer_windows[i].window == sel_info->window) {
				transfer_windows[i].in_use = FALSE;
			}
		}
	}
	g_free (saved_sel_info);
	saved_sel_info = sel_info;
}

static void
notify_refresh_locations (char *notifyuris)
{
	GdkDisplay *dpy;
	Display *xdpy;
	Window root;
	Atom atom, utf8_string;

	if (!notifyuris) return;
	
	dpy = gdk_display_get_default ();
	xdpy = GDK_DISPLAY_XDISPLAY (dpy);
	
	utf8_string = XInternAtom (xdpy, "UTF8_STRING", FALSE);
	
	root = DefaultRootWindow (xdpy);
	
	atom = XInternAtom (xdpy, "NAUTILUS_REFRESH_LOCATIONS", FALSE);
	
	gdk_error_trap_push ();
	
	XChangeProperty (xdpy, root, atom, utf8_string, 8, PropModeReplace,
			 notifyuris, strlen (notifyuris));

	XSync (xdpy, False);

	gdk_error_trap_pop ();
}

#define MAX_PATH_LEN 1024

void
copy_move_files (SelInfo * sel_info)
{
	char          **lines;
	int             i;
	char            src_path[MAX_PATH_LEN];
	char            dest_path[MAX_PATH_LEN];
	char           *filename, *src, *dest;
	GnomeVFSResult  result;
	GnomeVFSXferOptions xfer_options;
	gboolean cut = FALSE;
	GList *src_list = NULL, *dest_list= NULL;
	char *notifyuris;

	/* FIXME clean this function up and add some file:// uri checking */

	lines = g_strsplit (sel_info->prop, "\n", 0);

	if (strcmp (lines[0], "cut") == 0) {
		cut = TRUE;
	} else {
		cut = FALSE;
	}

	for (i = 2; lines[i] != NULL; i++) {
		filename = g_path_get_basename (lines[i] + 7);
		src = g_strdup_printf ("file://%s",
			      getpathbylabel ((const char *) (lines[i] + 7),
					      src_path,
					      MAX_PATH_LEN,
					      &sel_info->holder_label));
		src_list = g_list_prepend (src_list, gnome_vfs_uri_new (src));
		dest = g_strdup_printf ("file://%s/%s",
		    getpathbylabel ((const char *) (sel_info->dest_dir + 7),
				    dest_path,
				    MAX_PATH_LEN,
				    &sel_info->requestor_label),
					filename);
		g_free (filename);
		dest_list = g_list_prepend (dest_list, gnome_vfs_uri_new(dest));
		if (i = 2) { /* only need to do this once */
			notifyuris = g_strdup_printf ("%s\n%s", src, dest);
		}
		g_free (src);
		g_free (dest);
	}

	g_strfreev (lines);

	xfer_options = GNOME_VFS_XFER_RECURSIVE |
		GNOME_VFS_XFER_USE_UNIQUE_NAMES;

	if (cut) {
		xfer_options |= GNOME_VFS_XFER_REMOVESOURCE;
	}

	/* FIXME need to make this an asynchronous operation so that the
	   user knows something is actually happening, especialy for large
	   file transfers */ 

	result = gnome_vfs_xfer_uri_list (src_list, dest_list, xfer_options, 
				          GNOME_VFS_XFER_ERROR_MODE_ABORT,
				          GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
				          NULL, NULL);
	/* FIXME: need to add result testing and display the appropriate 
	   dialog */

	/* FIXME: need to add nautilus location refresh notification */
	notify_refresh_locations (notifyuris);
	g_free (notifyuris);
}

static void
audit_transfer (SelInfo * sel_info, gboolean auto_confirm)
{
	int rd;
	char *holder_label, *requestor_label, *str;
	adt_event_data_t *event;
	struct passwd *pwd = getpwuid (sel_info->requestor_uid);

	if (!audit_handle) {
		if (adt_start_session (&audit_handle, NULL, 
				       ADT_USE_PROC_DATA) != 0) {
			audit_handle = NULL;
			return;
		}
	}

	adt_set_user (audit_handle, pwd->pw_uid, pwd->pw_gid, pwd->pw_uid,
		      pwd->pw_gid, NULL, ADT_UPDATE);
		
	if (sel_info->outside_accred_range) {
		event = adt_alloc_event (audit_handle, ADT_uauth);
		event->adt_uauth.auth_used = SYS_ACCRED_SET_AUTH;
		event->adt_uauth.objectname = "tsoljdsselmgr";
		adt_put_event (event, 0, 0);
		adt_free_event (event);
	}

	switch (sel_info->transfer_type) {
	case DGSL_DGIL:
	case DGSL_EQIL:
	case DGSL_UGIL:
	case DGSL_DJIL:
	case DJSL_DGIL:
	case DJSL_EQIL:
	case DJSL_UGIL:
	case DJSL_DJIL:
		if (sel_info->file_transfer) {
			event = adt_alloc_event (audit_handle, ADT_uauth);
			event->adt_uauth.auth_used = FILE_DOWNGRADE_SL_AUTH;
			event->adt_uauth.objectname = "tsoljdsselmgr";
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		} else {
			event = adt_alloc_event (audit_handle, ADT_uauth);
			event->adt_uauth.auth_used = WIN_DOWNGRADE_SL_AUTH;
			event->adt_uauth.objectname = "tsoljdsselmgr";
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		}
		break;
	case UGSL_DGIL:
	case UGSL_EQIL:
	case UGSL_UGIL:
	case UGSL_DJIL:
		if (sel_info->file_transfer) {
			event = adt_alloc_event (audit_handle, ADT_uauth);
			event->adt_uauth.auth_used = FILE_UPGRADE_SL_AUTH;
			event->adt_uauth.objectname = "tsoljdsselmgr";
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		} else {
			event = adt_alloc_event (audit_handle, ADT_uauth);
			event->adt_uauth.auth_used = WIN_UPGRADE_SL_AUTH;
			event->adt_uauth.objectname = "tsoljdsselmgr";
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		}
		break;
	}

	if (sel_info->prop == NULL) {
		return;
	}
	if (sel_info->file_transfer) {
		char **lines = g_strsplit (sel_info->prop, "\n", 0);
        	if (strcmp (lines[0], "cut") == 0) {
			event = adt_alloc_event (audit_handle, AUE_file_move);
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		} else {
			event = adt_alloc_event (audit_handle, AUE_file_copy);
			adt_put_event (event, 0, 0);
			adt_free_event (event);
		}
	} else {
		event = adt_alloc_event (audit_handle, AUE_sel_mgr_xfer);
		adt_put_event (event, 0, 0);
		adt_free_event (event);


		label_to_str (&sel_info->holder_label, &holder_label, 
			      M_LABEL, DEF_NAMES);
		label_to_str (&sel_info->requestor_label, &requestor_label, 
			      M_LABEL, DEF_NAMES);
		str = g_strdup_printf ("From xwindow %d, uid %d, label %s "
					"To xwindow %d, uid %d, label %s", 
					sel_info->holder_window, 
					sel_info->holder_uid, 
					holder_label, 
					sel_info->orig_event.requestor, 
					sel_info->requestor_uid, 
					requestor_label);
		
		rd = au_open ();
		au_write (rd, au_to_text (str));
		g_free (str);

		str = g_strdup_printf ("Selection %s, Atom %s, "
				       "Property %s",
					XGetAtomName (sel_info->dpy, 
						sel_info->orig_event.selection),
					XGetAtomName (sel_info->dpy,
						sel_info->actual_type), 
					sel_info->prop);

		au_write (rd, au_to_text (str));
		g_free (str);
		
		if (auto_confirm) {
			au_write (rd, au_to_text ("Autoconfirm"));
		} else if (sel_info->data_viewed) {
			au_write (rd, au_to_text ("Viewed"));
		} else {
			au_write (rd, au_to_text ("Not Viewed"));
		}

		au_close (rd, AU_TO_WRITE, AUE_sel_mgr_xfer, 2);
	}
}

gboolean
uid_has_auth (uid_t uid, char *authname)
{
	struct passwd  *user_data = getpwuid (uid);

	if (user_data == NULL) {
		fprintf (stderr, "Workstation owner info invalid.\n");
		return (0);
	}
	return chkauthattr (authname, user_data->pw_name);
}

static          gboolean
has_required_privileges (void)
{
	char           *priv_str;
	priv_set_t     *expected_privs = priv_allocset ();;
	priv_set_t     *permitted_privs = priv_allocset ();

	priv_isemptyset (expected_privs);
	priv_addset (expected_privs, "win_mac_read");
	priv_addset (expected_privs, "win_mac_write");
	priv_addset (expected_privs, "win_selection");
	priv_addset (expected_privs, "win_dac_read");
	priv_addset (expected_privs, "win_dac_write");
	priv_addset (expected_privs, "win_downgrade_sl");
	priv_addset (expected_privs, "win_upgrade_sl");
	priv_addset (expected_privs, "proc_audit");
	priv_addset (expected_privs, "sys_trans_label");
	priv_addset (expected_privs, "file_dac_read");
	priv_addset (expected_privs, "file_dac_write");
	priv_addset (expected_privs, "file_mac_read");
	priv_addset (expected_privs, "file_mac_write");

	/* Check that we have our expected privileges */
	if (getppriv (PRIV_PERMITTED, permitted_privs) == -1) {
		fprintf (stderr,
			 "tsoljdsselmgr: error getting privilege set: %s\n",
			 strerror (errno));
		priv_freeset (expected_privs);
		priv_freeset (permitted_privs);
		return FALSE;
	}
	if (!priv_issubset (expected_privs, permitted_privs)) {
		priv_inverse (permitted_privs);
		priv_intersect (expected_privs, permitted_privs);
		priv_str = priv_set_to_str (permitted_privs, ',', PRIV_STR_PORT);
		fprintf (stderr,
			 "tsoljdsselmgr: required privileges missing: %s\n",
			 priv_str);
		g_free (priv_str);
		priv_freeset (expected_privs);
		priv_freeset (permitted_privs);
		return FALSE;
	}
	/* Clear unneeded process privileges */
	//setppriv (PRIV_SET, PRIV_EFFECTIVE, expected_privs);
	priv_freeset (expected_privs);
	priv_freeset (permitted_privs);

	return TRUE;
}

static          gboolean
is_already_running ()
{
	Atom            sel_mgr;
	Window          sel_win;
	GdkDisplay     *gdk_dpy = gdk_display_get_default ();
	Display        *x_dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);

	sel_mgr = XInternAtom (x_dpy, SEL_AGNT_ATOM, FALSE);
	sel_win = XGetSelectionOwner (x_dpy, sel_mgr);
	if (sel_win != None) {
		fprintf (stderr, "tsoljdsselmgr is already active.\n");
		return TRUE;
	}
	return FALSE;
}

gboolean
transfer_authorized (SelInfo * sel_info)
{
	switch (sel_info->transfer_type) {
		/* downgrades? */
	case DGSL_DGIL:
	case DGSL_EQIL:
	case DGSL_UGIL:
	case DGSL_DJIL:
	case DJSL_DGIL:
	case DJSL_EQIL:
	case DJSL_UGIL:
	case DJSL_DJIL:
		if (!sel_info->file_transfer) {
			if (!WDOWNGRADE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			if (sel_info->outside_accred_range &&
			    	!OUTSIDE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			return TRUE;
		} else {
			if (!FDOWNGRADE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			if (sel_info->outside_accred_range &&
			    	!OUTSIDE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			return TRUE;
		}
		/* upgrades? */
	case UGSL_DGIL:
	case UGSL_EQIL:
	case UGSL_UGIL:
	case UGSL_DJIL:
		if (!sel_info->file_transfer) {
			if (!WUPGRADE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			if (sel_info->outside_accred_range &&
			    	!OUTSIDE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			return TRUE;
		} else {
			if (!FUPGRADE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			if (sel_info->outside_accred_range &&
			    	!OUTSIDE_AUTHORISED (sel_info->requestor_uid)) {
				return FALSE;
			}
			return TRUE;
		}
	}

	return TRUE;
}

static void
send_real_reply (SelInfo * sel_info)
{
	XSelectionEvent xselection;
	bslabel_t       label;

	/* put the property on the requestor window */
	XChangeProperty (sel_info->dpy, sel_info->orig_event.requestor,
		       sel_info->orig_event.property, sel_info->actual_type,
			 sel_info->actual_format, PropModeReplace,
			 sel_info->prop, sel_info->nitems);

	/*
	 * change the label of the prop to SL = requestor SL or if auto
	 * confirm, use the selected label
	 */

	label = sel_info->requestor_label;
	XTSOLsetPropLabel (sel_info->dpy, sel_info->orig_event.requestor,
			   sel_info->orig_event.property, &label);
	XTSOLsetPropUID (sel_info->dpy, sel_info->orig_event.requestor,
			 sel_info->orig_event.property,
			 &sel_info->requestor_uid);

	/* reply as if transfer passed.  */
	xselection.type = SelectionNotify;
	xselection.display = sel_info->dpy;
	xselection.requestor = sel_info->orig_event.requestor;
	xselection.selection = sel_info->orig_event.selection;
	xselection.target = sel_info->orig_event.target;
	xselection.property = sel_info->orig_event.property;
	xselection.time = sel_info->orig_event.time;

	XSendEvent (sel_info->dpy, xselection.requestor, True, 0,
		    (XEvent *) & xselection);

	gettimeofday (&sel_info->endtime, (struct timezone *) NULL);
	sel_info->transfer_accepted = TRUE;
	save_new_selinfo_and_destroy_old (sel_info);
}

static void
send_null_reply (SelInfo * sel_info)
{
	XSelectionEvent xselection;

	/* reply as if failed transfer. */
	xselection.type = SelectionNotify;
	xselection.display = sel_info->dpy;
	xselection.requestor = sel_info->orig_event.requestor;
	xselection.selection = sel_info->orig_event.selection;
	xselection.target = sel_info->orig_event.target;
	xselection.property = None;
	xselection.time = sel_info->orig_event.time;

	XSendEvent (sel_info->dpy, xselection.requestor, True, 0,
		    (XEvent *) & xselection);

	gettimeofday (&sel_info->endtime, (struct timezone *) NULL);
	sel_info->transfer_accepted = FALSE;
	save_new_selinfo_and_destroy_old (sel_info);
}

static void
accept_transfer (SelMgrDialog * dialog)
{
	SelInfo        *sel_info = g_object_get_data (G_OBJECT (dialog), "sel_info");

	audit_transfer (sel_info, FALSE);

	if (sel_info->dest_dir != NULL) {
		copy_move_files (sel_info);
	}
	send_real_reply (sel_info);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
cancel_transfer (SelMgrDialog * dialog)
{
	Display *xdpy = GDK_DISPLAY_XDISPLAY (gtk_widget_get_display ((GtkWidget *)dialog));
	SelInfo *sel_info = g_object_get_data (G_OBJECT (dialog), "sel_info");

	XSetSelectionOwner (xdpy, sel_info->orig_event.selection, None,
			    CurrentTime);

	send_null_reply (sel_info);

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
dialog_response_cb (GtkDialog * dialog, gint id)
{
	GError *err = NULL;
	SelMgrDialog   *selmgr_dialog = SELMGR_DIALOG (dialog);

	switch (id) {
	case GTK_RESPONSE_OK:
		accept_transfer (selmgr_dialog);
		break;
	case GTK_RESPONSE_HELP:
		gnome_help_display_desktop (NULL, "trusted",
					   "index.xml", "selection_manager" ,&err);
		if (err) {
			GtkWidget *err_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						err->message);

			g_signal_connect (G_OBJECT (err_dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
			gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);
			gtk_widget_show (err_dialog);

			g_error_free (err);
		}
		break;
	case GTK_RESPONSE_CANCEL:
		cancel_transfer (selmgr_dialog);
		break;
	default:
		break;
	}
}

/*
 * This code was borrowed from the cksum command.
 * 
 * crcposix -- compute posix.2 compatable 32 bit CRC
 * 
 * The POSIX.2 (draft 10) CRC algorithm. This is a 32 bit CRC with polynomial
 * x**32 + x**26 + x**23 + x**22 + x**16 + x**12 + x**11 + x**10 + x**8 +
 * x**7 + x**5 + x**4 + x**2 + x**1 + x**0
 */

/* layout is from the POSIX.2 Rationale */
static ulong    crctab_posix[256] = {
	0x00000000L,
	0x04C11DB7L, 0x09823B6EL, 0x0D4326D9L, 0x130476DCL, 0x17C56B6BL,
	0x1A864DB2L, 0x1E475005L, 0x2608EDB8L, 0x22C9F00FL, 0x2F8AD6D6L,
	0x2B4BCB61L, 0x350C9B64L, 0x31CD86D3L, 0x3C8EA00AL, 0x384FBDBDL,
	0x4C11DB70L, 0x48D0C6C7L, 0x4593E01EL, 0x4152FDA9L, 0x5F15ADACL,
	0x5BD4B01BL, 0x569796C2L, 0x52568B75L, 0x6A1936C8L, 0x6ED82B7FL,
	0x639B0DA6L, 0x675A1011L, 0x791D4014L, 0x7DDC5DA3L, 0x709F7B7AL,
	0x745E66CDL, 0x9823B6E0L, 0x9CE2AB57L, 0x91A18D8EL, 0x95609039L,
	0x8B27C03CL, 0x8FE6DD8BL, 0x82A5FB52L, 0x8664E6E5L, 0xBE2B5B58L,
	0xBAEA46EFL, 0xB7A96036L, 0xB3687D81L, 0xAD2F2D84L, 0xA9EE3033L,
	0xA4AD16EAL, 0xA06C0B5DL, 0xD4326D90L, 0xD0F37027L, 0xDDB056FEL,
	0xD9714B49L, 0xC7361B4CL, 0xC3F706FBL, 0xCEB42022L, 0xCA753D95L,
	0xF23A8028L, 0xF6FB9D9FL, 0xFBB8BB46L, 0xFF79A6F1L, 0xE13EF6F4L,
	0xE5FFEB43L, 0xE8BCCD9AL, 0xEC7DD02DL, 0x34867077L, 0x30476DC0L,
	0x3D044B19L, 0x39C556AEL, 0x278206ABL, 0x23431B1CL, 0x2E003DC5L,
	0x2AC12072L, 0x128E9DCFL, 0x164F8078L, 0x1B0CA6A1L, 0x1FCDBB16L,
	0x018AEB13L, 0x054BF6A4L, 0x0808D07DL, 0x0CC9CDCAL, 0x7897AB07L,
	0x7C56B6B0L, 0x71159069L, 0x75D48DDEL, 0x6B93DDDBL, 0x6F52C06CL,
	0x6211E6B5L, 0x66D0FB02L, 0x5E9F46BFL, 0x5A5E5B08L, 0x571D7DD1L,
	0x53DC6066L, 0x4D9B3063L, 0x495A2DD4L, 0x44190B0DL, 0x40D816BAL,
	0xACA5C697L, 0xA864DB20L, 0xA527FDF9L, 0xA1E6E04EL, 0xBFA1B04BL,
	0xBB60ADFCL, 0xB6238B25L, 0xB2E29692L, 0x8AAD2B2FL, 0x8E6C3698L,
	0x832F1041L, 0x87EE0DF6L, 0x99A95DF3L, 0x9D684044L, 0x902B669DL,
	0x94EA7B2AL, 0xE0B41DE7L, 0xE4750050L, 0xE9362689L, 0xEDF73B3EL,
	0xF3B06B3BL, 0xF771768CL, 0xFA325055L, 0xFEF34DE2L, 0xC6BCF05FL,
	0xC27DEDE8L, 0xCF3ECB31L, 0xCBFFD686L, 0xD5B88683L, 0xD1799B34L,
	0xDC3ABDEDL, 0xD8FBA05AL, 0x690CE0EEL, 0x6DCDFD59L, 0x608EDB80L,
	0x644FC637L, 0x7A089632L, 0x7EC98B85L, 0x738AAD5CL, 0x774BB0EBL,
	0x4F040D56L, 0x4BC510E1L, 0x46863638L, 0x42472B8FL, 0x5C007B8AL,
	0x58C1663DL, 0x558240E4L, 0x51435D53L, 0x251D3B9EL, 0x21DC2629L,
	0x2C9F00F0L, 0x285E1D47L, 0x36194D42L, 0x32D850F5L, 0x3F9B762CL,
	0x3B5A6B9BL, 0x0315D626L, 0x07D4CB91L, 0x0A97ED48L, 0x0E56F0FFL,
	0x1011A0FAL, 0x14D0BD4DL, 0x19939B94L, 0x1D528623L, 0xF12F560EL,
	0xF5EE4BB9L, 0xF8AD6D60L, 0xFC6C70D7L, 0xE22B20D2L, 0xE6EA3D65L,
	0xEBA91BBCL, 0xEF68060BL, 0xD727BBB6L, 0xD3E6A601L, 0xDEA580D8L,
	0xDA649D6FL, 0xC423CD6AL, 0xC0E2D0DDL, 0xCDA1F604L, 0xC960EBB3L,
	0xBD3E8D7EL, 0xB9FF90C9L, 0xB4BCB610L, 0xB07DABA7L, 0xAE3AFBA2L,
	0xAAFBE615L, 0xA7B8C0CCL, 0xA379DD7BL, 0x9B3660C6L, 0x9FF77D71L,
	0x92B45BA8L, 0x9675461FL, 0x8832161AL, 0x8CF30BADL, 0x81B02D74L,
	0x857130C3L, 0x5D8A9099L, 0x594B8D2EL, 0x5408ABF7L, 0x50C9B640L,
	0x4E8EE645L, 0x4A4FFBF2L, 0x470CDD2BL, 0x43CDC09CL, 0x7B827D21L,
	0x7F436096L, 0x7200464FL, 0x76C15BF8L, 0x68860BFDL, 0x6C47164AL,
	0x61043093L, 0x65C52D24L, 0x119B4BE9L, 0x155A565EL, 0x18197087L,
	0x1CD86D30L, 0x029F3D35L, 0x065E2082L, 0x0B1D065BL, 0x0FDC1BECL,
	0x3793A651L, 0x3352BBE6L, 0x3E119D3FL, 0x3AD08088L, 0x2497D08DL,
	0x2056CD3AL, 0x2D15EBE3L, 0x29D4F654L, 0xC5A92679L, 0xC1683BCEL,
	0xCC2B1D17L, 0xC8EA00A0L, 0xD6AD50A5L, 0xD26C4D12L, 0xDF2F6BCBL,
	0xDBEE767CL, 0xE3A1CBC1L, 0xE760D676L, 0xEA23F0AFL, 0xEEE2ED18L,
	0xF0A5BD1DL, 0xF464A0AAL, 0xF9278673L, 0xFDE69BC4L, 0x89B8FD09L,
	0x8D79E0BEL, 0x803AC667L, 0x84FBDBD0L, 0x9ABC8BD5L, 0x9E7D9662L,
	0x933EB0BBL, 0x97FFAD0CL, 0xAFB010B1L, 0xAB710D06L, 0xA6322BDFL,
	0xA2F33668L, 0xBCB4666DL, 0xB8757BDAL, 0xB5365D03L, 0xB1F740B4L
};

/* * crcposix -- compute posix.2 compatible 32 bit CRC */
static void
m_crcposix (crcp, bp, n)
	register ulong *crcp;
	const uchar_t  *bp;
	register size_t n;
{
	while (n-- > 0)
		*crcp = (*crcp << 8) ^ crctab_posix[(uchar_t) ((*crcp >> 24) ^ *bp++)];
}

static          gboolean
conversion_timeout (gpointer data)
{
	SelInfo        *info = (SelInfo *) data;

	save_new_selinfo_and_destroy_old (info);

	return FALSE;
}

static int
reset_transfer_windows (Display * dpy)
{
	int             i;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (transfer_windows[i].window != None) {
			XDestroyWindow (dpy, transfer_windows[i].window);
			transfer_windows[i].window = None;
		}
		transfer_windows[i].in_use = FALSE;
	}

	return 0;		/* first available */
}

/* debug code of 6466531
void 
print_human_readable_label (bslabel_t *label)
{
  char *text, *text_bsltos = NULL;
  label_to_str (label, &text, M_LABEL, DEF_NAMES);
  bsltos (label, &text_bsltos, 0, LONG_CLASSIFICATION);
  printf ("from label_to_str label %x is %s\n", label, text);
  printf ("from bsltos %x is %s\n\n", label, text_bsltos);

}
*/
static void
selection_request (Display * dpy, XSelectionRequestEvent * event)
{
	register int    i;
	SelInfo        *sel_info;
	XTsolClientAttributes clientattr;
	static set_id   u_acc_range = {USER_ACCREDITATION_RANGE, (char *) 0};

	sel_info = g_new0 (SelInfo, 1);
	sel_info->data_viewed = FALSE;
	sel_info->dpy = dpy;
	sel_info->orig_event = *event;
	gettimeofday (&sel_info->starttime, NULL);
	
	sel_info->requestor_window = event->requestor;

	/* Get Paste clients Label/UID */
	XTSOLgetClientLabel (dpy, event->requestor, &sel_info->requestor_label);
	XTSOLgetClientAttributes (dpy, event->requestor, &clientattr);
	sel_info->requestor_uid = clientattr.uid;
	sel_info->requestor_gid = clientattr.gid;
	sel_info->requestor_auditid = clientattr.auditid;
	sel_info->requestor_pid = clientattr.pid;

	for (i = 0; i < MAX_WINDOWS; i++) {
		if (!transfer_windows[i].in_use) {
			break;	/* found */
		}
	}

	/*
	 * Reset the transfer windows Normally this should not happen.
	 * However, if this happens due to some error condition, reset
	 */
	if (i == MAX_WINDOWS) {
		fprintf (stderr, "Too many transfers in progress, "
			 "Resetting transfer windows\n");
		i = reset_transfer_windows (dpy);
	}
	if (transfer_windows[i].window == None) {
		transfer_windows[i].window = XCreateSimpleWindow (dpy,
							RootWindow (dpy, 0),
							      0, 0, 1, 1, 1,
							BlackPixel (dpy, 0),
						       WhitePixel (dpy, 0));
		XTSOLMakeUntrustedWindow (dpy, sel_info->window);
	}
	sel_info->window = transfer_windows[i].window;
	transfer_windows[i].in_use = TRUE;

	XSaveContext (sel_info->dpy, sel_info->window, sel_info_context,
		      (caddr_t) sel_info);

	sel_info->holder_window = XGetSelectionOwner (dpy, event->selection);
	if (sel_info->holder_window == None) {
		send_null_reply (sel_info);
		return;
	}
	XTSOLgetResLabel (dpy, sel_info->holder_window, IsWindow,
			  &sel_info->holder_label);

	XTSOLgetResUID (dpy, sel_info->holder_window, IsWindow,
			&sel_info->holder_uid);

	if (event->target == ATOM_COPY_FILES) {
		Atom            type, property;
		int             format;
		unsigned long   nitems, bytes_after;

		sel_info->file_transfer = TRUE;

		property = XInternAtom (dpy, "_NAUTILUS_TARGET_DIR", TRUE);

		if (XGetWindowProperty (dpy, event->requestor, property,
				   0L, 0L - 1, TRUE, AnyPropertyType, &type,
					&format, &nitems, &bytes_after,
					&sel_info->dest_dir) != Success) {
			send_null_reply (sel_info);
			return;
		}
	}
	/*
	 * The MULTIPLE target requires more extensive two way communication
	 * between the holder and the requestor. Each client must be able to
	 * read the properties created by the other. If the SLs are equal, we
	 * step aside and let the clients handle it.  We do not worry about
	 * floating, because it will happen automatically on both sides.
	 */
	if (event->target == ATOM_MULTIPLE) {
		if ((sel_info->holder_uid == sel_info->requestor_uid) &&
		   (!XTSOLIsWindowTrusted (dpy, sel_info->holder_window)) &&
			  (!XTSOLIsWindowTrusted (dpy, event->requestor)) &&
				(blequal (&sel_info->holder_label,
					  &sel_info->requestor_label))) {
			XConvertSelection (dpy, event->selection, event->target,
					   event->property, event->requestor,
					   event->time);
			XSync (dpy, False);
		} else {
			send_null_reply (sel_info);
		}
		return;
	}
	XTSOLsetResLabel (dpy, sel_info->window, IsWindow,
			  &sel_info->holder_label);
	XTSOLsetResUID (dpy, sel_info->window, IsWindow,
			&sel_info->holder_uid);

	/* Target window in accred range ? */
	sel_info->outside_accred_range =
		(blinset (&sel_info->requestor_label, &u_acc_range) != 1);

	/* have the holder convert the selection to our window */
	XConvertSelection (dpy, event->selection, event->target,
			   event->property, sel_info->window, event->time);

	XSync (dpy, False);

	//g_timeout_add (TIMEOUT_VAL, conversion_timeout, sel_info);
}

TransferType
determine_transfer_type (SelInfo * sel_info)
{
	return (tsol_check_transfer_type (&sel_info->holder_label,
					  &sel_info->requestor_label));
}

static          gboolean
is_auto_reply_target (SelInfo * sel_info)
{
	char           *target_name;
	int             i;
	gboolean        result = FALSE;

	if (auto_replies.enabled) {
		target_name = XGetAtomName (sel_info->dpy,
					    sel_info->reply_event.target);
		for (i = 0; i < MAX_AUTO_REPLY; i++) {
			if ((auto_replies.selection[i]) &&
			(!strcmp (target_name, auto_replies.selection[i]))) {
				result = TRUE;
				break;
			}
		}
		XFree (target_name);
	}
	return result;
}

static char *
unescaped_path_from_uri (SelInfo * sel_info)
{
	char           *data;
	char          **lines;
	int             i;

	lines = g_strsplit ((char *) sel_info->prop, "\n", 0);

	i = 0;
	while (lines[i]) i++;

	if (i >= 3 && !strncmp(lines[2], "file:/", 6)) {
		char *locale_path = gnome_vfs_get_local_path_from_uri (lines[2]);
		char *path = g_locale_to_utf8 (locale_path, -1, NULL, NULL, NULL);
		data = g_strdup_printf ("%s\n%s\n%s", lines[0], lines[1], path);
		g_free (path);
		g_free (locale_path);
	} else {
		data = g_strdup ((char *) sel_info->prop);
	}

	g_strfreev (lines);
	return data;
}

static char *
compound_text_to_utf8 (SelInfo * sel_info)
{
	XTextProperty   property;
	int             count = 0;
	int             res;
	char           *data;
	char           *locale_data;
	char          **local_list;

	property.value = (unsigned char *) sel_info->prop;
	property.encoding = sel_info->reply_event.target;
	property.format = sel_info->actual_format;
	property.nitems = sel_info->nitems;

	res = XmbTextPropertyToTextList (sel_info->dpy, &property,
	                                 &local_list, &count);
	if (res == XNoMemory || res == XLocaleNotSupported || res == XConverterNotFound) {
		data = g_strdup ((char *) sel_info->prop);
		return data;
	}

	locale_data = local_list[0];
	data = g_locale_to_utf8 (locale_data, -1, NULL, NULL, NULL);
	return data;
}

static char *
strdup_prop_data (SelInfo * sel_info)
{
	char           *data;
	char           *locale_data;
	char           *target_name;

	target_name = XGetAtomName (sel_info->dpy,
	                            sel_info->reply_event.target);
	if (!target_name) {
		data = g_strdup ((char *) sel_info->prop);
	} else if (!strcmp (target_name, "x-special/gnome-copied-files")) {
		data = unescaped_path_from_uri (sel_info);
	} else if (!strcmp (target_name, "COMPOUND_TEXT")) {
		data = compound_text_to_utf8 (sel_info);
	} else if (!strcmp (target_name, "TEXT")) {
		locale_data = (char *) sel_info->prop;
		data = g_locale_to_utf8 (locale_data, -1, NULL, NULL, NULL);
	} else {
		data = g_strdup ((char *) sel_info->prop);
	}

	XFree (target_name);
	return data;
}

static void
present_confirmer_dialog (SelInfo * sel_info)
{
	GtkWidget      *dialog;
	struct passwd  *pw;
	char           *message;
	char           *str;
	char           *src_label = NULL;
	char           *dest_label = NULL;
	char           *src_type, *dest_type;
	char           *src_owner, *dest_owner, *data;
	gboolean        authorised;

	/* fill in these strings from the sel info data */

	/* FIXME need to special case file transfers and use a more appropriate 
	   message in the confirmer dialog */

	switch (sel_info->transfer_type) {
	case DGSL_DGIL:
	case DGSL_EQIL:
	case DGSL_UGIL:
	case DGSL_DJIL:
	case DJSL_DGIL:
	case DJSL_EQIL:
	case DJSL_UGIL:
	case DJSL_DJIL:
		if (!sel_info->file_transfer) {
			if (WDOWNGRADE_AUTHORISED (sel_info->requestor_uid)) {
				message = g_strdup_printf ("<b>%s</b>\n%s %s",
				          _ ("Information Downgrade Required"),
				          _ ("You are transferring a selection "
					  "between windows with different "
					  "labels.  This requires the "
				          "information in the selection to be"),
					   _ ("downgraded"));
				authorised = TRUE;
			} else {
				message = g_strdup_printf("<b>%s</b>\n%s %s %s",
				   _ ("Transfer not authorised."),
				   _ ("You are not authorised to"),
				   _ ("downgrade"), _ ("this information"));
				authorised = FALSE;
			}
		} else {
			if (FDOWNGRADE_AUTHORISED (sel_info->requestor_uid)) {
				message = g_strdup_printf ("<b>%s</b>\n%s %s",
				          _ ("File Downgrade Required"),
				          _ ("You are transferring a file "
					  "between windows with different "
					  "labels.  This requires the "
				          "file to be"),
					   _ ("downgraded"));
				authorised = TRUE;
			} else {
				message = g_strdup_printf("<b>%s</b>\n%s %s %s",
				   _ ("Transfer not authorised."),
				   _ ("You are not authorised to"),
				   _ ("downgrade"), _ ("this file"));
				authorised = FALSE;
			}
		}
		break;
	case UGSL_DGIL:
	case UGSL_EQIL:
	case UGSL_UGIL:
	case UGSL_DJIL:
		if (!sel_info->file_transfer) {
			if (WUPGRADE_AUTHORISED (sel_info->requestor_uid)) {
				message = g_strdup_printf ("<b>%s</b>\n%s %s",
					 _ ("Information Upgrade Required"),
				      _ ("You are transferring a selection "
					 "between windows with different "
					 "labels.  This requires the "
				      "information in the selection to be"),
						   _ ("upgraded"));
				authorised = TRUE;
			} else {
				message = g_strdup_printf("<b>%s</b>\n%s %s %s",
					     _ ("Transfer not authorised."),
					    _ ("You are not authorised to"),
				     _ ("upgrade"), _ ("this information"));
				authorised = FALSE;
			}
		} else {
			if (FUPGRADE_AUTHORISED (sel_info->requestor_uid)) {
				message = g_strdup_printf ("<b>%s</b>\n%s %s",
					 _ ("File Upgrade Required"),
				      _ ("You are transferring a file "
					 "between windows with different "
					 "labels.  This requires the "
				      "file to be"),
						   _ ("upgraded"));
				authorised = TRUE;
			} else {
				message = g_strdup_printf("<b>%s</b>\n%s %s %s",
					     _ ("Transfer not authorised."),
					    _ ("You are not authorised to"),
				     _ ("upgrade"), _ ("this file"));
				authorised = FALSE;
			}
		}
		break;
	}

	if (XTSOLIsWindowTrusted (sel_info->dpy, sel_info->holder_window))
	  {
	    src_label = g_strdup ( _ ("Trusted Path"));
	  }
	else
	  {
	    label_to_str (&sel_info->holder_label, &src_label, M_LABEL, LONG_NAMES);
	    if (strlen (src_label) > 20) {
	      g_free (src_label);
	      src_label = NULL;
	      label_to_str (&sel_info->holder_label, &src_label, M_LABEL, SHORT_NAMES);
	    }
	  }
	
	if (XTSOLIsWindowTrusted (sel_info->dpy, sel_info->requestor_window))
	  {
	    dest_label = g_strdup ( _ ("Trusted Path"));
	  }
	else
	  {
	    label_to_str (&sel_info->requestor_label, &dest_label, M_LABEL, LONG_NAMES);
	    if (strlen (dest_label) > 20) {
	      g_free (dest_label);
	      dest_label = NULL;
	      label_to_str (&sel_info->requestor_label, &dest_label, M_LABEL, SHORT_NAMES);
	    }
	  }
	str = XGetAtomName (sel_info->dpy, sel_info->actual_type);
	src_type = g_strdup_printf ("%s: %s", _ ("Type"), str);
	g_free (str);

	str = XGetAtomName (sel_info->dpy, sel_info->reply_event.target);
	dest_type = g_strdup_printf ("%s: %s (%d %s)", _ ("Type"), str,
				     sel_info->nitems *
				(sel_info->actual_format / 8), _ ("bytes"));
	g_free (str);

	if (pw = getpwuid (sel_info->holder_uid)) {
		src_owner = g_strdup_printf ("%s: %s", _ ("Owner"), pw->pw_name);
	} else {
		src_owner = g_strdup_printf ("%s: (%s)", _ ("Owner"),
					     sel_info->holder_uid);
	}

	if (pw = getpwuid (sel_info->requestor_uid)) {
		dest_owner = g_strdup_printf ("%s: %s", _ ("Owner"), pw->pw_name);
	} else {
		dest_owner = g_strdup_printf ("%s: (%s)", _ ("Owner"),
					      sel_info->requestor_uid);
	}

	/* FIXME: for file transfers this should be file contents */
	data = strdup_prop_data (sel_info);

	dialog = sel_mgr_dialog_new (message, src_label, dest_label,
				     src_type, dest_type,
				     src_owner, dest_owner,
				     authorised, data);

	g_free (message);
	g_free (src_label);
	g_free (dest_label);
	g_free (src_type);
	g_free (dest_type);
	g_free (src_owner);
	g_free (dest_owner);
	g_free (data);

	g_object_set_data (G_OBJECT (dialog), "sel_info", sel_info);

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (dialog_response_cb), NULL);
	g_signal_connect (G_OBJECT (dialog), "close",
			  G_CALLBACK (cancel_transfer), dialog);
	g_signal_connect (G_OBJECT (dialog), "delete_event",
			  G_CALLBACK (cancel_transfer), dialog);

	gtk_widget_show_all (dialog);
}

/* compute t2 - t1 and return the time value in diff */
static void
tvdiff (struct timeval * t1, struct timeval * t2, struct timeval * diff)
{
	diff->tv_sec = t2->tv_sec - t1->tv_sec;
	diff->tv_usec = t2->tv_usec - t1->tv_usec;
	if (diff->tv_usec < 0) {
		diff->tv_sec -= 1;
		diff->tv_usec += 1000000;
	}
}

/*
 * Duplicate requests may occur for several reasons.  If a request doesn't go
 * through for one data type (e.g., TEXT), the transfer may be retried with
 * another data type (e.g., STRING).  Some applications do an initial request
 * to get the size of the data, then do a second request to actually get the
 * data.
 * 
 * In Solaris, such duplicate requests are harmless and invisible to the user.
 * But in TSOL we don't want to make the confirmer come up twice, so we try
 * to recognize these duplicates and automatically give the same response.
 * This requires using some heuristics to decide if the request is a
 * duplicate.
 */
static          gboolean
is_duplicate_request (SelInfo * sel_info)
{
	struct timeval  timediff;
	char           *type;
	char           *prevtype;
	gboolean        typematch;
	unsigned long   timediffval;	/* millisecs */

	if (!saved_sel_info)
		return FALSE;

	/* When copy UTF-8 strings between labeled zones, it's more heavy
	 * than copying ASCII strings and the if statement returns FALSE. */

	/* Request must occur within  a time to be considered duplicate */
	tvdiff (&saved_sel_info->endtime, &sel_info->starttime, &timediff);

	/* convert timediff to millisecs */
	timediffval = timediff.tv_sec * 1000L + timediff.tv_usec / 1000L;
	if ((int) timediffval > saved_timediffval) {
		return FALSE;
	}
	/*
	 * Data size and contents, display, holder window and requestor
	 * window must match
	 */
	if (//sel_info->nitems != saved_sel_info->nitems ||
	       /* When copy strings from java to gtk, below is TRUE. */
	       //sel_info->actual_format != saved_sel_info->actual_format ||
	       //sel_info->data_checksum != saved_sel_info->data_checksum ||
			sel_info->window != saved_sel_info->window ||
		 sel_info->holder_window != saved_sel_info->holder_window ||
			sel_info->dpy != saved_sel_info->dpy) {
		return (False);
	}
	/* Selection type must match. Consider TEXT, STRING, etc. equivalent. */
	type = XGetAtomName (sel_info->dpy, sel_info->actual_type);
	prevtype = XGetAtomName (saved_sel_info->dpy,
				 saved_sel_info->actual_type ?
				 saved_sel_info->actual_type :
				 sel_info->actual_type);

	typematch = FALSE;
	/* When copy strings from Java apps to GTK apps, UTF8_STRING and
	 * COMPOUND_TEXT strings are sent to the target app.
	 * When copy strings from CDE apps to GTK apps, COMPOUND_TEXT
	 * strings only is sent to the target app. */
	if (saved_sel_info->actual_type && strcmp (type, prevtype) == 0) {
		typematch = TRUE;
	}
	if (!strcmp ("COMPOUND_TEXT", type) &&
	    !strcmp ("UTF8_STRING", prevtype)) {
		typematch = TRUE;
	}
	if (strstr ("::TEXT::STRING::EUC::", type) != NULL &&
			strstr ("::UTF8_STRING::TEXT::STRING::EUC::COMPOUND_TEXT::", prevtype) != NULL) {
		typematch = TRUE;
	}
	/* When copy strings from Java apps to Java apps, the following
	   atoms are sent. */
	if ((!strncmp ("text/plain", type, 10) &&
	     !strcmp ("STRING", prevtype)) ||
	    (!strncmp ("text/plain", type, 10) && 
	     !strncmp ("text/plain", prevtype, 10)) ||
	    (!strncmp ("JAVA_DATAFLAVOR", type, 15) && 
	     (!strncmp ("text/plain", prevtype, 10) ||
	      !strcmp ("UTF8_STRING", prevtype)))) {
		typematch = TRUE;
	}
	XFree (type);
	XFree (prevtype);
	if (typematch == FALSE) {
		return (False);
	}
	return TRUE;
}

static int
selection_notify (Display * dpy, XSelectionEvent * event)
{
	SelInfo        *sel_info;
	Atom NONEATOM;

	if (XFindContext (dpy, event->requestor, sel_info_context,
			  (caddr_t *) & sel_info) == XCNOENT) {
		return FALSE;
	}
	/* cache the reply event from the holder */
	sel_info->reply_event = *event;

	NONEATOM = XInternAtom (dpy, "NONE", FALSE);

	/* did the transfer succeed? */
	if (event->property == None || event->property == NONEATOM) {
		send_null_reply (sel_info);
		return TRUE;
	}
	/*
	 * Get the label of the actual transfer This might have a different
	 * IL from the holders window because ChangeProperty doesn't float
	 * the window, just the Property. However, it is not necessary to get
	 * the uid again, because it is guaranteed to be the same as the
	 * window.
	 */

	XTSOLgetPropLabel (dpy, event->requestor, event->property,
			   &sel_info->holder_label);

	/* cache the actual selection */
	if (XGetWindowProperty (dpy, event->requestor, event->property,
				0L, 0L - 1, TRUE, AnyPropertyType,
				&sel_info->actual_type,
				&sel_info->actual_format, &sel_info->nitems,
				&sel_info->bytes_after,
				&sel_info->prop) != Success) {
		send_null_reply (sel_info);
		return TRUE;
	}
	m_crcposix (&sel_info->data_checksum, sel_info->prop,
		    sel_info->nitems * sel_info->actual_format / 8);

	sel_info->transfer_type = determine_transfer_type (sel_info);

	if (sel_info->prop == NULL) {
		send_null_reply (sel_info);
		return TRUE;
	}
	if (sel_info->nitems == 0) {
		/* This is a special case for the ACK target in D&D */
		sel_info->actual_type = sel_info->orig_event.target;
		send_real_reply (sel_info);
		return TRUE;
	}
	/* is this one of the auto reply targets? */
	if (is_auto_reply_target (sel_info)) {
		send_real_reply (sel_info);
		return TRUE;
	}
	/* do we need a confirmer or can we let the transfer go through? */
	if (auto_confirm_settings.do_auto_confirm[sel_info->transfer_type] &&
			transfer_authorized (sel_info)) {
		audit_transfer (sel_info, True);
		send_real_reply (sel_info);
		return TRUE;
	}
	if (auto_confirm_settings.do_auto_cancel[sel_info->transfer_type] &&
			transfer_authorized (sel_info)) {
		send_null_reply (sel_info);
		return TRUE;
	}
	if (is_duplicate_request (sel_info)) {
		if (saved_sel_info->transfer_accepted) {
			send_real_reply (sel_info);
		} else {
			send_null_reply (sel_info);
		}
	} else {
		sel_info->data_viewed = !VIEWNONE_AUTHORISED (getuid());
		present_confirmer_dialog (sel_info);
	}

	return TRUE;
}

static          GdkFilterReturn
selection_filter_func (GdkXEvent * gdkxevent, GdkEvent * event, gpointer data)
{
	XEvent         *xevent = gdkxevent;
	GtkWidget      *widget = (GtkWidget *) data;
	GdkDisplay     *gdk_dpy = gdk_display_get_default ();
	Display        *x_dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);

	switch (xevent->type) {
	case SelectionRequest:
		selection_request (x_dpy, &(xevent->xselectionrequest));
		break;
	case SelectionNotify:
	case SelectionClear:
		selection_notify (x_dpy, &(xevent->xselection));
		break;
	default:
		break;
	}

	return GDK_FILTER_CONTINUE;
}

static void
sel_mgr_init (void)
{
	GtkWidget      *dialog;
	Atom            sel_mgr;
	Window          sel_win, win;
	XSetWindowAttributes attr;
	int             policy;
	uid_t           uid;
	m_label_t      *low_label = blabel_alloc ();
	GdkDisplay     *gdk_dpy = gdk_display_get_default ();
	Display        *x_dpy = GDK_DISPLAY_XDISPLAY (gdk_dpy);

	attr.event_mask = PropertyChangeMask;


	sel_win = XCreateWindow (x_dpy, DefaultRootWindow (x_dpy),
				 0, 0, 1, 1, 0, 0, InputOnly, CopyFromParent,
				 CWEventMask, &attr);

	gdk_window_add_filter (NULL, selection_filter_func, NULL);

	/* Register as the sel_mgr window */
	sel_mgr = XInternAtom (x_dpy, SEL_AGNT_ATOM, FALSE);
	XSetSelectionOwner (x_dpy, sel_mgr, sel_win, CurrentTime);
	win = XGetSelectionOwner (x_dpy, sel_mgr);
	if (sel_win != win) {
		fprintf (stderr, "Can't register selection manager. Exiting\n");
		exit (1);
	}
	/* Set this window to admin_low & 0 for others to read */
	bsllow (low_label);
	uid = 0;
	XTSOLsetResLabel (x_dpy, sel_win, IsWindow, low_label);
	XTSOLsetResUID (x_dpy, sel_win, IsWindow, &uid);
	blabel_free (low_label);

	sel_info_context = XUniqueContext ();
	ATOM_MULTIPLE = XInternAtom (x_dpy, "MULTIPLE", FALSE);
	ATOM_PROXY_COPY = XInternAtom (x_dpy, "_NAUTILUS_PROXY_COPY", FALSE);
	ATOM_COPY_FILES = XInternAtom (x_dpy, "x-special/gnome-copied-files", FALSE);

	if (tsol_load_sel_config (&auto_confirm_settings, &auto_replies) != 0) {
		fprintf (stderr, "Can't load sel_config file. Exiting\n");
		exit (1);
	}
	/* Get auditing policies */
	auditon (A_GETPOLICY, (caddr_t) & policy, 0);
	if (policy & AUDIT_WINDATA_DOWN)
		audit_downgraded_data = TRUE;
	if (policy & AUDIT_WINDATA_UP)
		audit_upgraded_data = TRUE;
}

static void
timediff_notify (GConfClient *client,
                 guint        cnxn_id,
                 GConfEntry  *entry,
                 gpointer     user_data)
{
	GConfValue *val;
	int timediffval;

	val = gconf_entry_get_value (entry);
	timediffval = gconf_value_get_int (val);

	if (timediffval > 0)
		saved_timediffval = timediffval;
}


int
main (int argc, char **argv)
{
	GnomeClient    *client;
	GConfClient    *conf;
	GError         *err;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			    NULL);

	/* session management */
	client = gnome_master_client ();
	gnome_client_set_restart_command (client, 1, argv);
	gnome_client_set_priority (client, 10);
	gnome_client_set_restart_style (client, GNOME_RESTART_IMMEDIATELY);

	if (!has_required_privileges ()) {
		exit (1);
	}
	if (is_already_running ()) {
		exit (2);
	}

	conf = gconf_client_get_default ();
	err = NULL;
	gconf_client_notify_add (conf,
	                         GCONF_KEY_TIMEDIFF,
	                         timediff_notify,
	                         client,
	                         NULL, &err);

	if (err) {
		g_printerr (_("There was an error adding to the notification of gconf. (%s)\n"),
		            err->message);
		g_error_free (err);
	}

	sel_mgr_init ();

	gtk_main ();

}
