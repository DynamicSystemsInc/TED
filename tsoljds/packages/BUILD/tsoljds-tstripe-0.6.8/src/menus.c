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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <user_attr.h>
#include <auth_attr.h>
#include <secdb.h>
#include <pwd.h>
#include <sys/tsol/label_macro.h>
#define  WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <libgnometsol/label_builder.h>
#include <libgnometsol/userattr.h>
#include <libgnometsol/pam_conv.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xauth.h>
#include <X11/extensions/Xtsol.h>

#include "menus.h"
#include "xutils.h"
#include "ui-controller.h"
#include "tsol-user.h"
#include "xagent-management.h"
#include "privs.h"

static void 
role_menu_item_response (GtkWidget * widget,
			 gpointer * user_data);
static void 
label_passwd_menu_item_response (GtkWidget * widget,
			   gpointer * user_data);
static void 
passwd_menu_item_response (GtkWidget * widget,
			   gpointer * user_data);
static void 
device_menu_item_response (GtkWidget * widget,
			   gpointer * user_data);
static void 
workspace_menu_item_response (GtkWidget * widget,
			      gpointer * user_data);
static void     _tstripe_label_builder_response_callback (GtkWidget * widget, gint response, gpointer user_data);

static void     _tstripe_role_menu_active_ws_callback (WnckScreen * screen,
					     WnckWorkspace * prev_active_workspace,
					     gpointer user_data);
static void     _tstripe_role_menu_ws_role_callback (WnckWorkspace * space, gpointer user_data);
static void     _tstripe_workspace_menu_active_ws_callback (WnckScreen * screen,
					     WnckWorkspace * prev_active_workspace,
					     gpointer user_data);
static void     _tstripe_workspace_menu_ws_label_callback (WnckWorkspace * workspace, gpointer user_data);
static void     _tstripe_workspace_menu_ws_role_callback (WnckWorkspace * workspace, gpointer user_data);
static void 
_tstripe_workspace_menu_ws_created_callback (WnckScreen * screen,
					     WnckWorkspace * workspace,
					     gpointer user_data);
static void     set_workspace_menu_sensitivity (WnckWorkspace * workspace, GtkWidget * menu_item);
static GList   *
get_windows_for_workspace (WnckScreen * screen,
			   WnckWorkspace * workspace);
static int 
add_role_to_acl (GdkDisplay * display,
		 gchar * rolename);
static int 
set_workspace_atom_value (Window xwindow,
			  char *atom,
			  char *atomvalue,
			  int wsindex,
			  int wscount);
static GtkWidget *create_window_list_dialog (GList * windowlist);

/* getdomainname() is a private function on Solaris */
extern char    *getdomainname (char *name, int length);

/*
 * Yuck: Needed to stuff extra data into callbacks.
 */ 
typedef struct _RoleMenuCBData RoleMenuCBData;

struct _RoleMenuCBData {
	guint       roleindex;
	GtkWidget **rolemenuitems;
};

GtkWidget      *
_tstripe_create_role_menu (GdkScreen * screen)
{
	GtkWidget      *role_menu;
	GtkWidget     **rolemenuitems;
	GSList         *radiogroup = NULL;	/* For the role radio menu */
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace;
	User           *activerole;
	const gchar    *activerolename;
	int             i, wscount;
	int             activeroleindex;
	gint            index;
	RoleMenuCBData *rolemenucbdata;

	index = gdk_screen_get_number (screen);
	wnckscreen = wnck_screen_get (index);
	wscount = wnck_screen_get_workspace_count (wnckscreen);
	activerolename = wnck_workspace_get_role (wnck_screen_get_active_workspace (wnckscreen));
	activerole = _tstripe_user_find_user_by_name (activerolename);
	activeroleindex = _tstripe_user_get_user_index (activerole);

	role_menu = gtk_menu_new ();
	gtk_menu_set_screen (GTK_MENU (role_menu), screen);

	rolemenucbdata = g_new0 (RoleMenuCBData, _tstripe_user_count_get ());
	rolemenuitems = g_new0 (GtkWidget *, _tstripe_user_count_get ());
	for (i = 0; i < _tstripe_user_count_get (); i++) {
		User           *tmpuser = (User *) g_slist_nth_data (users, i);

		rolemenuitems[i] = gtk_radio_menu_item_new_with_label (radiogroup, tmpuser->p->pw_name);
		radiogroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (rolemenuitems[i]));
		//gtk_widget_show (GTK_WIDGET (rolemenuitems[i]));
		gtk_menu_shell_append (GTK_MENU_SHELL (role_menu), rolemenuitems[i]);

		rolemenucbdata[i].roleindex = i;
		rolemenucbdata[i].rolemenuitems = rolemenuitems;
		if (i == activeroleindex)
			g_object_set (G_OBJECT (rolemenuitems[i]), "active", TRUE, NULL);
		/* Callback to authenticate and setup the role */
		g_signal_connect (G_OBJECT (rolemenuitems[i]), "toggled",
				  G_CALLBACK (role_menu_item_response),
				  (gpointer) &(rolemenucbdata[i]));
	}

	g_signal_connect (G_OBJECT (wnckscreen), "active_workspace_changed",
			  G_CALLBACK (_tstripe_role_menu_active_ws_callback),
			  (gpointer) rolemenuitems);
	for (i = 0; i < wscount; i++) {
		wnckworkspace = wnck_screen_get_workspace (wnckscreen, i);
		g_signal_connect (G_OBJECT (wnckworkspace), "role_changed",
			   G_CALLBACK (_tstripe_role_menu_ws_role_callback),
				  (gpointer) rolemenuitems);
	}

	gtk_widget_show_all (role_menu);

	return role_menu;
}

static void
nscd_per_label_changed (GFileMonitor *monitor, GFile *file, GFile *otherfile,
			GFileMonitorEvent event_type, gpointer data)
{
	GtkWidget *menu_item = (GtkWidget *) data;

	if (event_type == G_FILE_MONITOR_EVENT_CREATED)
		gtk_widget_set_sensitive (menu_item, TRUE);

	if (event_type == G_FILE_MONITOR_EVENT_DELETED)
		gtk_widget_set_sensitive (menu_item, FALSE);
}

#define NSCD_PER_LABEL_FILE "/var/tsol/doors/nscd_per_label"

GtkWidget      *
_tstripe_create_trusted_path_menu (GdkScreen * screen)
{
	GtkWidget      *trusted_path_menu;
	GtkWidget      *menu_item;
	GtkWidget      *image;
	GFile	       *nscd_per_label;
	GFileMonitor   *file_monitor;

	trusted_path_menu = gtk_menu_new ();
	gtk_menu_set_screen (GTK_MENU (trusted_path_menu), screen);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_ ("Change Login Password..."));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (trusted_path_menu), menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (passwd_menu_item_response),
			  (gpointer) NULL);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_ ("Change Workspace Password..."));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
        gtk_menu_shell_append (GTK_MENU_SHELL (trusted_path_menu), menu_item);
        g_signal_connect (G_OBJECT (menu_item), "activate",
                          G_CALLBACK (label_passwd_menu_item_response),
                          (gpointer) NULL);

	if (!g_file_test (NSCD_PER_LABEL_FILE ,G_FILE_TEST_EXISTS)) {
		gtk_widget_set_sensitive (GTK_WIDGET (menu_item), FALSE);
	}

	nscd_per_label = g_file_new_for_path (NSCD_PER_LABEL_FILE);
	file_monitor = g_file_monitor_file (nscd_per_label, 0, NULL, NULL);

	g_signal_connect (file_monitor, "changed", 
			  G_CALLBACK (nscd_per_label_changed), menu_item);

	image = gtk_image_new_from_stock (GTK_STOCK_HARDDISK, GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_ ("Allocate Device..."));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (trusted_path_menu), menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (device_menu_item_response),
			  (gpointer) NULL);

	menu_item = gtk_image_menu_item_new_with_label (_ ("Query Window Label..."));
	gtk_menu_shell_append (GTK_MENU_SHELL (trusted_path_menu), menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (query_window_popups_show),
			  (gpointer) FALSE);

	image = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_MENU);
	menu_item = gtk_image_menu_item_new_with_label (_("Help..."));
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (trusted_path_menu), menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate", 
			 G_CALLBACK (trusted_stripe_help_show), NULL);

	gtk_widget_show_all (trusted_path_menu);

	return trusted_path_menu;
}

GtkWidget      *
_tstripe_create_workspace_menu (GdkScreen * screen)
{
	GtkWidget      *workspace_menu;
	GtkWidget      *menu_item;
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace;
	int             i, wscount;
	gint            index;

	workspace_menu = gtk_menu_new ();
	gtk_menu_set_screen (GTK_MENU (workspace_menu), screen);

	menu_item = gtk_image_menu_item_new_with_label (_ ("Change Workspace Label..."));
	gtk_menu_shell_append (GTK_MENU_SHELL (workspace_menu), menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (workspace_menu_item_response),
			  NULL);

	index = gdk_screen_get_number (screen);
	wnckscreen = wnck_screen_get (index);
	wnck_screen_force_update (wnckscreen);

	wscount = wnck_screen_get_workspace_count (wnckscreen);
	for (i = 0; i < wscount; i++) {
		wnckworkspace = wnck_screen_get_workspace (wnckscreen, i);
		g_signal_connect (G_OBJECT (wnckworkspace), "label_changed",
		     G_CALLBACK (_tstripe_workspace_menu_ws_label_callback),
				  NULL);
		g_signal_connect (G_OBJECT (wnckworkspace), "role_changed",
		      G_CALLBACK (_tstripe_workspace_menu_ws_role_callback),
				  (gpointer) menu_item);
	}
	g_signal_connect (G_OBJECT (wnckscreen), "active_workspace_changed",
		    G_CALLBACK (_tstripe_workspace_menu_active_ws_callback),
			  (gpointer) menu_item);
	g_signal_connect (G_OBJECT (wnckscreen), "workspace_created",
		   G_CALLBACK (_tstripe_workspace_menu_ws_created_callback),
			  (gpointer) menu_item);
	set_workspace_menu_sensitivity (wnck_screen_get_active_workspace (wnckscreen), menu_item);

	gtk_widget_show_all (workspace_menu);

	return workspace_menu;
}

/*
 * _tstripe_role_authentication_top_level () Wraps up all the generic GUI
 * interaction for role authentication into one function.
 * 
 * Returns: 0 on success, non-zero otherwise
 */

int
_tstripe_role_authentication_top_level (gchar * role,
					GdkScreen * screen)
{
	GdkDisplay     *dpy;
	Display        *xdpy;
	User           *roleuser = NULL;
	uid_t           wsowner;
	int             result;

	if (GDK_IS_SCREEN(screen) == FALSE) {
		g_warning ("Invalid GdkScreen parameter.");
		dpy = gdk_screen_get_display (gdk_screen_get_default ());
	} else
		dpy = gdk_screen_get_display (screen);

	xdpy = GDK_DISPLAY_XDISPLAY (dpy);

	roleuser = _tstripe_user_find_user_by_name (role);
	if (!roleuser) {
		g_warning ("%s is not a valid role for this user", role);
		return -1;
	}
	XTSOLgetWorkstationOwner (xdpy, &wsowner);
	if (roleuser->p->pw_uid == wsowner) {
		/* Workstation owner so no authentication necessary */
		return 0;
	} else {
		GtkWidget      *errdialog;
		gint            response;
		gint            retry = 1;

		while (retry) {
			if ((result = _tstripe_authenticate_role (roleuser, screen, -1, -1)) != PAM_SUCCESS) {
				switch (result) {
				case PAM_PERM_DENIED:
					/* Bad hair day */
					retry = 0;
					errdialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
									    _ ("The system administrator has temporarily disabled access to the system for %s.\n"
									       "See your system administrator"), roleuser->p->pw_name);
					gtk_dialog_run (GTK_DIALOG (errdialog));
					gtk_widget_destroy (errdialog);
					break;
				case PAM_ACCT_EXPIRED:
					retry = 0;
					errdialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
									    _ ("The system administrator has disabled this account.\n"
					  "See your system administrator"));
					gtk_dialog_run (GTK_DIALOG (errdialog));
					gtk_widget_destroy (errdialog);
				case PAM_AUTH_ERR:
					errdialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_YES_NO,
									    _ ("Authentication for the role \"%s\" has failed\n"
									       "Would you like to retry?"), roleuser->p->pw_name);
					response = gtk_dialog_run (GTK_DIALOG (errdialog));
					if (response == GTK_RESPONSE_NO)
						retry = 0;
					gtk_widget_destroy (errdialog);
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
					errdialog = gtk_message_dialog_new (NULL,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
									    _ ("Couldn't set account management for %s.\n"),
						       roleuser->p->pw_name);
					gtk_dialog_run (GTK_DIALOG (errdialog));
					gtk_widget_destroy (errdialog);
					break;
				}
			} else	/* authentication succeeded - break out */
				retry = 0;
		}

		/*
		 * If Authentication succeeded add the role to the display's
		 * ACL
		 */
		if (result == PAM_SUCCESS) {
			if (!add_role_to_acl (dpy, roleuser->p->pw_name)) {
				g_warning ("Failed to add %s to the X Display's ACL\n", roleuser->p->pw_name);
				/*
				 * FIXME - consider popping up an error
				 * dialog if this happens.
				 */
				return -1;
			}
			return 0;
		} else
			/*
			 * User cancelled the authentication or some internal
			 * pam error occured
			 */
			return -1;
	}

}


/*
 * Set the role of the workspace to the login name of the role. Returns 0 on
 * success, non-zero otherwise.
 */

int
_tstripe_set_workspace_role (GdkScreen * screen, char *role, int workspaceindex)
{
	int             result, workspacecount;
	gint            index;
	char           *role_min_label;
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace;
	GdkDisplay     *dpy;
	Display        *xdpy;
	Window          root;
	userattr_t     *role_attr = NULL;

	dpy = gdk_screen_get_display (screen);
	xdpy = GDK_DISPLAY_XDISPLAY (dpy);

	index = gdk_screen_get_number (screen);
	wnckscreen = wnck_screen_get (index);
	workspacecount = wnck_screen_get_workspace_count (wnckscreen);
	wnckworkspace = wnck_screen_get_workspace (wnckscreen, workspaceindex);

	root = GDK_WINDOW_XID (gdk_screen_get_root_window (screen));
	if ((result = set_workspace_atom_value (root, "_NET_DESKTOP_ROLES",
						role, workspaceindex,
						workspacecount)) != 0) {
		g_warning ("Failed to modify X Atom: _NET_DESKTOP_ROLES");
		return result;
	}
	/* Set the label of the workspace to role's min_label */
	role_attr = getusernam (role);
	role_min_label = gnome_tsol_get_usrattr_val (role_attr, USERATTR_MINLABEL);
	wnck_workspace_change_label (wnckworkspace, role_min_label);
	free_userattr (role_attr);

	return 0;
}

/* Anchor the menu to the button instead of the mouse pointer */
void
_tstripe_menu_position_func (GtkMenu * menu,
			     gint * x,
			     gint * y,
			     gboolean * push_in,
			     gpointer user_data)
{
	GtkWidget      *widget = GTK_WIDGET (user_data);
	GtkRequisition  requisition;
	GdkScreen      *screen = NULL;
	gint            menu_xpos;
	gint            menu_ypos;
	GdkRectangle	area;

	screen = gtk_widget_get_screen (widget);
	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);
	gdk_window_get_origin (gtk_widget_get_window(widget),
			&menu_xpos, &menu_ypos);
	gtk_widget_get_allocation(widget, &area);
	menu_ypos += area.y;

	if (menu_ypos > gdk_screen_get_height (screen) / 2)
		menu_ypos -= requisition.height;
	else
		menu_ypos += area.height;

	*x = menu_xpos;
	*y = menu_ypos;
	*push_in = TRUE;
}

/*
 * Setup a new role workspace (with default minimum label).
 */
static void
role_menu_item_response (GtkWidget * widget, gpointer * user_data)
{
	char           *msg = NULL;
	int             activeworkspaceindex = 0;
	gint            index = 0;
	gboolean        itemactive;
	GtkWidget      *dialog;
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace;
	GdkScreen      *screen;
	GList          *windowlist = NULL;
	GList          *tmp;

	RoleMenuCBData *rolemenucbdata = (RoleMenuCBData *) user_data;
	User           *role = (User *) g_slist_nth_data (users, rolemenucbdata->roleindex);
	User           *wsuser = (User *) g_slist_nth_data (users, 0);
	GtkWidget     **rolemenuitems = (GtkWidget **) rolemenucbdata->rolemenuitems;

	g_object_get (G_OBJECT (widget), "active", &itemactive, NULL);
	if (!itemactive)
		return;

	if (gtk_widget_has_screen (GTK_WIDGET (widget)))
		screen = gtk_widget_get_screen (GTK_WIDGET (widget));
	else
		screen = gdk_screen_get_default ();

	index = gdk_screen_get_number (screen);
	wnckscreen = wnck_screen_get (index);
	if (wnckscreen == NULL) {
		g_warning ("Couldn't obtain the corresponding wnckscreen object for screen %d",
			   index);
		return;
	}
	wnckworkspace = wnck_screen_get_active_workspace (wnckscreen);

	windowlist = get_windows_for_workspace (wnckscreen, wnckworkspace);

	if (windowlist) {
		GtkWidget      *dialog = create_window_list_dialog (windowlist);
		gtk_window_set_screen (GTK_WINDOW (dialog), screen);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	if ((windowlist)) { //||
			//(_tstripe_role_authentication_top_level (role->p->pw_name, screen) != 0)) {
		/*
		 * Windows still on workspace or unsuccessful authentication
		 * or user hit cancel The radiomenuitem must be deactivated
		 * and the correct one reactivated ie. the one corresponding
		 * to the current workspace role.
		 */
		int             i;
		static guint    signal_id;
		const gchar    *prevname;
		User           *prevrole;

		if (windowlist)
			g_list_free (windowlist);
		prevname = wnck_workspace_get_role (wnckworkspace);

		/*
		 * Find the role's index
		 */
		if (!prevname) {/* NULL implies a workstationowner workspace */
			i = 0;
		} else {
			prevrole = _tstripe_user_find_user_by_name (prevname);
			i = _tstripe_user_get_user_index (prevrole);
		}

		signal_id = g_signal_lookup ("toggled", GTK_TYPE_RADIO_MENU_ITEM);
	
		g_signal_handlers_block_matched (G_OBJECT (rolemenuitems[i]),
			G_SIGNAL_MATCH_FUNC,
			signal_id,
			0,
			NULL,
			(void *) role_menu_item_response,
			NULL);
		g_object_set (G_OBJECT (rolemenuitems[i]), "active", TRUE, NULL);
		g_signal_handlers_unblock_matched (G_OBJECT (rolemenuitems[i]),
			G_SIGNAL_MATCH_FUNC,
			signal_id,
			0,
			NULL,
			(void *) role_menu_item_response,
			NULL);
		return;
	}
	activeworkspaceindex = wnck_workspace_get_number (wnckworkspace);

	if (_tstripe_set_workspace_role (screen, role->p->pw_name, activeworkspaceindex)) {
		/* Setting the role failed */
		return;
	}

	/* Update the menu label */
	update_trusted_stripe_roles ();
}

/*
 * Invoke device manager dialog for the current workspace user/role
 * 
 * Note that the "widget" argument is the menu being posted, NOT the button that
 * was pressed.
 */
static void
device_menu_item_response (GtkWidget * widget, gpointer * user_data)
{

	GdkScreen *screen = gtk_widget_get_screen (widget);
	GError *err = NULL;
	
	escalate_inherited_privs ();
	g_spawn_command_line_async("/usr/bin/tsoljdsdevmgr", &err);
	drop_inherited_privs ();
}

/*
 * Invoke change password dialog for the current workspace user/role
 * 
 * Note that the "widget" argument is the menu being posted, NOT the button that
 * was pressed.
 */
static void
label_passwd_menu_item_response (GtkWidget *widget, gpointer *user_data)
{
	GdkScreen *screen;
	gint index;
	Atom atom, utf8_string;
	char *command;
	Window root;
	Display *xdpy;

	if (gtk_widget_has_screen (GTK_WIDGET (widget)))
		screen = gtk_widget_get_screen (GTK_WIDGET (widget));
	else
		screen = gdk_screen_get_default ();

	index = gdk_screen_get_number (screen);
	xdpy = GDK_SCREEN_XDISPLAY (screen);
	utf8_string = XInternAtom (xdpy, "UTF8_STRING", FALSE);
	root = DefaultRootWindow (xdpy);
	atom = XInternAtom (xdpy, "_LABEL_EXEC_COMMAND", FALSE);

	command = g_strdup_printf ("%d:xagentchangepassword", index);
	
	gdk_error_trap_push ();
	XChangeProperty (xdpy, root, atom, utf8_string, 8, PropModeReplace,
			 command, strlen (command));
	XSync (xdpy, False);
	gdk_error_trap_pop ();

	g_free (command);
}

static void
passwd_menu_item_response (GtkWidget * widget, gpointer * user_data)
{
	GdkScreen      *screen;
	User           *user;

	if (gtk_widget_has_screen (GTK_WIDGET (widget)))
		screen = gtk_widget_get_screen (GTK_WIDGET (widget));
	else
		screen = gdk_screen_get_default ();

	user = g_slist_nth_data (users, 0);
	_tstripe_solaris_chauthtok ("dtpasswd", user->p->pw_name, user->p->pw_uid, user->p->pw_gid, screen, -1, -1);
}

/*
 * Invoke label builder dialog for the current workspace user/role
 */
static void
workspace_menu_item_response (GtkWidget * widget, gpointer * user_data)
{
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace;
	GdkScreen      *gdkscreen;
	GtkWidget      *labelbuilder;
	m_label_t      *ws_sl = NULL;
	m_label_t      *lower_sl = NULL;
	m_label_t      *upper_clear = NULL;

	char           *lower_bound = NULL;
	char           *upper_bound = NULL;
	const char     *workspace_label;
	char           *message = NULL;
	int             wsindex;
	int             error;
	gint            index;

	gdkscreen = gtk_widget_get_screen (widget);
	index = gdk_screen_get_number (gdkscreen);
	wnckscreen = wnck_screen_get (index);
	wnckworkspace = wnck_screen_get_active_workspace (wnckscreen);
	wsindex = wnck_workspace_get_number (wnckworkspace);

	if (wnck_workspace_get_label_range (wnckworkspace, &lower_bound, &upper_bound) != 0)
		return;
	/* Convert the lower and upper bounds to internal binary labels */
	if (str_to_label (lower_bound, &lower_sl, MAC_LABEL, L_DEFAULT, &error) < 0) {
		g_warning ("Workspace has invalid label range minimum label");
		g_free (lower_bound);
		g_free (upper_bound);
		return;
	}
	if (str_to_label (upper_bound, &upper_clear, USER_CLEAR, L_DEFAULT, &error) < 0) {
		g_warning ("Workspace has invalid label range clearance");
		g_free (lower_bound);
		g_free (upper_bound);
		m_label_free (lower_sl);
		return;
	}
	workspace_label = wnck_workspace_get_label (wnckworkspace);

	if (workspace_label != NULL) {
		/* Convert the workspace's current label to binary type */
		if (str_to_label (workspace_label, &ws_sl, MAC_LABEL, L_DEFAULT, &error) < 0) {
			g_warning ("Workspace has an invalid label");
			g_free (lower_bound);
			g_free (upper_bound);
			m_label_free (lower_sl);
			m_label_free (upper_clear);
			return;
		}
	} else {
		g_warning ("No workspace label - default to lowest in range");
		m_label_dup (&ws_sl, lower_sl);
	}

	message = g_strdup_printf (_ ("Changing workspace label on workspace: \"%s\""),
		wnck_workspace_get_name (wnckworkspace));
	labelbuilder = g_object_new (GNOME_TYPE_LABEL_BUILDER,
				     "mode", LBUILD_MODE_SL,
				     "message", message,
				     "lower", lower_sl,
				     "upper", upper_clear,
				     "sl", ws_sl,
				     NULL);
	gtk_window_set_screen (GTK_WINDOW (labelbuilder), gdkscreen);
	g_signal_connect (G_OBJECT (labelbuilder), "response",
		      G_CALLBACK (_tstripe_label_builder_response_callback),
			  (gpointer) wnckworkspace);
	gtk_widget_show_all (labelbuilder);
	/*
	 * FIXME - it shouldn't be neccessary to have to set this property
	 * after the widget_show()
	 */
	g_object_set (G_OBJECT (labelbuilder), "sl", ws_sl, NULL);

	g_free (lower_bound);
	g_free (upper_bound);
	g_free (message);
	m_label_free (lower_sl);
	m_label_free (upper_clear);
	m_label_free (ws_sl);
}

/*
 * Handle reponses from the label builder dialog
 */
static void
_tstripe_label_builder_response_callback (GtkWidget * widget, gint response, gpointer user_data)
{
	m_label_t      *mlabel = NULL;
	char           *label;
	WnckWorkspace  *wnckworkspace;

	GnomeLabelBuilder *labelbuilder = GNOME_LABEL_BUILDER (widget);

	wnckworkspace = WNCK_WORKSPACE (user_data);

	switch (response) {
	case GTK_RESPONSE_OK:
		g_object_get (G_OBJECT (labelbuilder), "sl", &mlabel, NULL);
		label_to_str (mlabel, &label, M_INTERNAL, LONG_NAMES);
		wnck_workspace_change_label (wnckworkspace, label);
		m_label_free (mlabel);
		free (label);
		gtk_widget_destroy (GTK_WIDGET (labelbuilder));
		break;
	case GTK_RESPONSE_HELP:
		gnome_label_builder_show_help (GTK_WIDGET (labelbuilder));
		break;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (labelbuilder));
		break;
	default:
		/* WTF? */
		break;
	}
	return;
}


/*
 * Callback to handle workspace switching. Needs to adjust the role menu bar
 * label and activate the appropriate radio menu item.
 */
static void
_tstripe_role_menu_active_ws_callback (WnckScreen * screen,
					     WnckWorkspace *prev_active_workspace,
					     gpointer user_data)
{
	WnckScreen     *wnckscreen = WNCK_SCREEN (screen);
	WnckWorkspace  *wnckworkspace;
	GtkWidget     **rolemenuitems;
	User           *role = NULL;
	const gchar    *rolename = NULL;
	gboolean        activeitem = FALSE;
	int             i;

	rolemenuitems = (GtkWidget **) user_data;
	wnckworkspace = wnck_screen_get_active_workspace (wnckscreen);
	rolename = wnck_workspace_get_role (wnckworkspace);
	/*
	 * Find the role
	 */
	if ((role = _tstripe_user_find_user_by_name (rolename)) == NULL)
		return;
	i = _tstripe_user_get_user_index (role);

	/*
	 * If the corresponding radiomenuitem is not already active, make it
	 * so.
	 */
	g_object_get (G_OBJECT (rolemenuitems[i]), "active", &activeitem, NULL);
	if (!activeitem) {
		static guint signal_id;
		signal_id = g_signal_lookup ("toggled", GTK_TYPE_RADIO_MENU_ITEM);
	
		g_signal_handlers_block_matched (G_OBJECT (rolemenuitems[i]),
			G_SIGNAL_MATCH_FUNC,
			signal_id,
			0,
			NULL,
			(void *)role_menu_item_response,
			NULL);
		g_object_set (G_OBJECT (rolemenuitems[i]), "active", TRUE, NULL);
		g_signal_handlers_unblock_matched (G_OBJECT (rolemenuitems[i]),
			G_SIGNAL_MATCH_FUNC,
			signal_id,
			0,
			NULL,
			(void *)role_menu_item_response,
			NULL);
	}
	update_trusted_stripe_roles ();
}

/*
 * Callback to handle workspace role switching occuring outside the role
 * menu. Eg. From a failed session restoration of a role. Needs to adjust the
 * role menu bar label and activate the appropriate radio menu item.
 */

static void
_tstripe_role_menu_ws_role_callback (WnckWorkspace * workspace, gpointer user_data)
{
	GtkWidget     **rolemenuitems;
	WnckScreen     *wnckscreen;
	WnckWorkspace  *wnckworkspace, *activeworkspace;
	int             workspaceindex, activeworkspaceindex;
	User           *role = NULL;
	const gchar    *rolename;
	gboolean        activeitem;
	int             i;
	gint            index;

	wnckworkspace = WNCK_WORKSPACE (workspace);
	workspaceindex = wnck_workspace_get_number (wnckworkspace);
	
	rolemenuitems = (GtkWidget **)user_data;
	wnckscreen = wnck_workspace_get_screen (workspace);
	activeworkspace = wnck_screen_get_active_workspace (wnckscreen);
	activeworkspaceindex = wnck_workspace_get_number (activeworkspace);

	/*
	 * Only interested if change occured on active ws. 
	 * Probably did.
	 */
	if (workspaceindex == activeworkspaceindex) {
		rolename = wnck_workspace_get_role (wnckworkspace);
		/*
		 * Find the role
		 */
		if ((role = _tstripe_user_find_user_by_name (rolename)) == NULL)
			return;
		i = _tstripe_user_get_user_index (role);
		g_object_get (G_OBJECT (rolemenuitems[i]), "active", &activeitem, NULL);
		if (!activeitem) {
			static guint signal_id;
			update_trusted_stripe_roles ();

			signal_id = g_signal_lookup ("toggled", GTK_TYPE_RADIO_MENU_ITEM);
	
			g_signal_handlers_block_matched (G_OBJECT (rolemenuitems[i]),
				G_SIGNAL_MATCH_FUNC,
				signal_id,
				0,
				NULL,
				(void *)role_menu_item_response,
				NULL);
			g_object_set (G_OBJECT (rolemenuitems[i]), "active", TRUE, NULL);
			g_signal_handlers_unblock_matched (G_OBJECT (rolemenuitems[i]),
				G_SIGNAL_MATCH_FUNC,
				signal_id,
				0,
				NULL,
				(void *)role_menu_item_response,
				NULL);
		}
	}
}

static void
_tstripe_workspace_menu_active_ws_callback (WnckScreen * screen,
					     WnckWorkspace *prev_active_workspace,
					     gpointer user_data)
{
	WnckWorkspace  *workspace;

	workspace = wnck_screen_get_active_workspace (screen);
	if (workspace)
		set_workspace_menu_sensitivity (workspace, GTK_WIDGET (user_data));
}

/*
 * Callback to handle workspace label switching.
 */
static void
_tstripe_workspace_menu_ws_label_callback (WnckWorkspace * workspace, gpointer user_data)
{
	update_trusted_stripes_workspaces ();
}

static void
_tstripe_workspace_menu_ws_role_callback (WnckWorkspace * workspace, gpointer user_data)
{
	GtkWidget      *menu_item;
	WnckScreen     *wnckscreen;
	int             workspaceindex, activeworkspaceindex;

	menu_item = GTK_WIDGET (user_data);
	wnckscreen = wnck_workspace_get_screen (workspace);
	activeworkspaceindex = wnck_workspace_get_number (wnck_screen_get_active_workspace (wnckscreen));
	workspaceindex = wnck_workspace_get_number (workspace);

	/* Ignore the role change if it occured on a non-active workspace */
	if (activeworkspaceindex != workspaceindex)
		return;

	set_workspace_menu_sensitivity (workspace, menu_item);
}

/*
 * Register workspace menu callbacks for new workspaces here
 */
static void
_tstripe_workspace_menu_ws_created_callback (WnckScreen * screen,
					     WnckWorkspace * workspace,
					     gpointer user_data)
{
	g_signal_connect (G_OBJECT (workspace), "label_changed",
		     G_CALLBACK (_tstripe_workspace_menu_ws_label_callback),
			  NULL);
	g_signal_connect (G_OBJECT (workspace), "role_changed",
		      G_CALLBACK (_tstripe_workspace_menu_ws_role_callback),
			  user_data);
}

static void
set_workspace_menu_sensitivity (WnckWorkspace * workspace, GtkWidget * menu_item)
{
	m_label_t      *lower_sl = NULL;
	m_label_t      *upper_clear = NULL;
	char           *lower_bound = NULL;
	char           *upper_bound = NULL;
	int             error;
	gboolean        menusensitivity = TRUE;

	wnck_workspace_get_label_range (workspace,
					&lower_bound, &upper_bound);

	if (str_to_label (lower_bound, &lower_sl, MAC_LABEL, L_DEFAULT, &error) < 0) {
		g_warning ("Internal label conversion of string \"%s\" failed\n", lower_bound);
		goto error;
	}
	if (str_to_label (upper_bound, &upper_clear, USER_CLEAR, L_DEFAULT, &error) < 0) {
		g_warning ("Internal label conversion of clearance string \"%s\" failed\n", upper_bound);
		m_label_free (lower_sl);
		goto error;
	}
	if (blequal (lower_sl, upper_clear))
		menusensitivity = FALSE;
	m_label_free (upper_clear);
error:
	g_free (lower_bound);
	g_free (upper_bound);
	gtk_widget_set_sensitive (menu_item, menusensitivity);
}

/*
 * Returns a list of normal application windows on then given workspace
 */
static GList   *
get_windows_for_workspace (WnckScreen * screen, WnckWorkspace * workspace)
{
	GList          *result;
	GList          *windows;
	GList          *tmp;
	gboolean        is_active;
	WnckWindow     *win;

	result = NULL;
	is_active = workspace == wnck_screen_get_active_workspace (screen);

	windows = wnck_screen_get_windows (screen);
	for (tmp = windows; tmp != NULL; tmp = tmp->next) {
		win = WNCK_WINDOW (tmp->data);

		if (!wnck_window_is_on_workspace (win, workspace))
			continue;
		if (wnck_window_get_state (win) & WNCK_WINDOW_STATE_SKIP_PAGER)
			continue;
		if (wnck_window_get_state (win) & WNCK_WINDOW_STATE_SKIP_TASKLIST)
			continue;
		/*
		 * FIXME - not sure how to treat sticky windows and role
		 * workspaces
		 */
		if (!is_active && wnck_window_is_sticky (win))
			continue;

		result = g_list_append (result, win);
	}
	return result;
}

/*
 * Copied from tsoldtwm
 */
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

static int
set_workspace_atom_value (Window xwindow, char *atom, char *atomvalue, int wsindex, int wscount)
{
	char          **curwsdata = NULL;
	char          **newwsdata = NULL;
	int             i;

	g_return_val_if_fail (atom != NULL, -1);
	g_return_val_if_fail (atomvalue != NULL, -1);
	g_return_val_if_fail (wscount >= 0, -1);
	g_return_val_if_fail (wsindex <= wscount, -1);

	curwsdata = _tstripe_get_utf8_list (xwindow,
					    _tstripe_atom_get (atom));

	if ((newwsdata = malloc ((wscount + 1) * sizeof (char *))) == NULL) {
		g_warning ("Memory allocation of workspace data failed");
		return -1;
	}
	for (i = 0; i < wscount; i++) {
		if (i == wsindex)
			newwsdata[i] = g_strdup_printf ("%s", g_strdup (atomvalue));
		else
			newwsdata[i] = g_strdup_printf ("%s", (curwsdata && curwsdata[i]) ? curwsdata[i] : "");
	}
	newwsdata[wscount] = NULL;
	_tstripe_set_utf8_list (xwindow,
				_tstripe_atom_get (atom),
				newwsdata);

	g_strfreev (curwsdata);
	g_strfreev (newwsdata);
	curwsdata = newwsdata = NULL;
	return 0;
}

GtkWidget      *
create_window_list_dialog (GList * windowlist)
{
	/* Caller has already ensured the the list is non-NULL */
	GtkWidget      *dialog;
	GtkWidget      *hbox, *vbox;
	GtkWidget      *icon;
	GtkWidget      *label;
	GtkWidget      *table;
	GdkPixbuf      *pixbuf;
	WnckWindow     *wnckwindow;
	char           *msg, *title;
	int             i;
	int             nwindows;

	nwindows = 1 + g_list_position (windowlist, g_list_last (windowlist));
	if (nwindows < 1)
		return NULL;

	dialog = gtk_dialog_new_with_buttons (NULL, NULL,
			  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);

	/* Set up the dialog */
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	/* Table to hold the window list */
	table = gtk_table_new (nwindows, 2, FALSE);

	for (i = 0; i < nwindows; i++) {
		wnckwindow = WNCK_WINDOW (g_list_nth_data (windowlist, i));
		title = (char *) wnck_window_get_icon_name (wnckwindow);
		label = gtk_label_new (title);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		pixbuf = wnck_window_get_mini_icon (wnckwindow);
		icon = gtk_image_new_from_pixbuf (pixbuf);
		gtk_table_attach (GTK_TABLE (table),
				  icon,
				  0, 1,
				  i, i + 1,
				  0, GTK_EXPAND,
				  0, 0);
		gtk_table_attach (GTK_TABLE (table),
				  label,
				  1, 2,
				  i, i + 1,
				  GTK_FILL, GTK_EXPAND,
				  4, 0);
	}

	/* Set up the dialog's icon */
	hbox = gtk_hbox_new (FALSE, 12);
	icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
					 GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	gtk_box_set_spacing (GTK_BOX(
		gtk_dialog_get_content_area(GTK_DIALOG(dialog))), 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);

	vbox = gtk_vbox_new (FALSE, 0);

	msg = g_strdup (_("Please close or move the following windows to another"
			" workspace before attempting to assume a role: \n"));
	label = gtk_label_new (msg);
	g_free (msg);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox),
			    label,
			    TRUE,	/* expand */
			    TRUE,	/* fill */
			    5);	/* padding */
	gtk_box_pack_start (GTK_BOX (vbox), table,
			    TRUE, TRUE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX (
		gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			    hbox,
			    TRUE,	/* expand */
			    TRUE,	/* fill */
			    0);	/* padding */

	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	return dialog;
}
