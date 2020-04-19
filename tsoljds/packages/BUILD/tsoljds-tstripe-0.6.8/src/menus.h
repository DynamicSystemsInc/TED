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

#ifndef TSTRIPE_BUTTON_H
#define TSTRIPE_BUTTON_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Authorizations used by Trusted Solaris.
 * From ON sources /usr/src/head/auth_list.h
 */
#if 0
#define BYPASS_FILE_VIEW_AUTH   "solaris.label.win.noview"
#define DEVICE_CONFIG_AUTH      "solaris.device.config"
#define FILE_CHOWN_AUTH         "solaris.file.chown"
#define FILE_DOWNGRADE_SL_AUTH  "solaris.label.file.downgrade"
#define FILE_OWNER_AUTH         "solaris.file.owner"
#define FILE_UPGRADE_SL_AUTH    "solaris.label.file.upgrade"
#define PRINT_ADMIN_AUTH        "solaris.print.admin"
#define PRINT_CANCEL_AUTH       "solaris.print.cancel"
#define PRINT_LIST_AUTH         "solaris.print.list"
#define PRINT_MAC_AUTH          "solaris.label.print"
#define PRINT_NOBANNER_AUTH     "solaris.print.nobanner"
#define PRINT_POSTSCRIPT_AUTH   "solaris.print.ps"
#define PRINT_UNLABELED_AUTH    "solaris.print.unlabeled"
#define SHUTDOWN_AUTH           "solaris.system.shutdown"
#define SYS_ACCRED_SET_AUTH     "solaris.label.range"
#define WIN_DOWNGRADE_SL_AUTH   "solaris.label.win.downgrade"
#define WIN_UPGRADE_SL_AUTH     "solaris.label.win.upgrade"
#endif


GtkWidget*	_tstripe_create_role_menu		     	(GdkScreen	*screen);
GtkWidget*	_tstripe_create_trusted_path_menu		(GdkScreen  *screen);
GtkWidget*	_tstripe_create_workspace_menu			(GdkScreen	*screen);
void 	_tstripe_menu_position_func	(GtkMenu	*menu,
                                         gint		*x,
                                         gint		*y,
                                         gboolean	*push_in,
                                         gpointer	user_data);
int		_tstripe_role_authentication_top_level	(gchar    	*role,
          	                                      	 GdkScreen	*screen);
int		_tstripe_set_workspace_role           	(GdkScreen	*screen,
          	                                      	 char     	*role,
          	                                      	 int      	workspaceindex);

G_END_DECLS

#endif /* TSTRIPE_BUTTON_H */
