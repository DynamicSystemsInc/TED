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

#ifndef TSTRIPE_XAGENT_MANAGEMENT_H
#define TSTRIPE_XAGENT_MANAGEMENT_H

#include <signal.h>
#include <siginfo.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS
/* List of functions to export:
 *
 * start_xagent_for_workspaces ();
 * get_exec_command_prop ()
 * tsol_login_defaults ()
 *
 */ 
#define _XA_LABEL_EXEC_COMMAND "_LABEL_EXEC_COMMAND"

void 	start_xagent_for_workspaces (void);
char**	get_exec_command_prop	(Display *x_dpy, Window proxy_window);
void	tsol_login_defaults	(void);
int	TsolErrorHandler	(Display *dpy, XErrorEvent *error);
void	ChildDieHandler		(int sig, siginfo_t *sinf, void *ucon);
void	save_yourself_callback	(void);
GdkFilterReturn	process_label_command	(GdkXEvent	*gdkxevent, 
					 GdkEvent	*event, 
					 gpointer	 data);

GdkFilterReturn	local_zone_session	(GdkXEvent	*gdkxevent, 
					 GdkEvent	*event, 
					 gpointer	 data);

G_END_DECLS
#endif /* TSTRIPE_XAGENT_MANAGEMENT_H */
