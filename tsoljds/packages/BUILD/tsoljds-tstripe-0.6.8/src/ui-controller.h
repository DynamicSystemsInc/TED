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

#ifndef __UTILS_H__
#define __UTILS_H__
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include "constraint-scaling.h"

gboolean	global_role_dialog_is_mapped; /* Set to TRUE when role dialog is mapped. */

gboolean	 label_layout_should_be_black (GdkRGBA *color);

ConstraintImage* window_label_stripe_get (GtkWidget* widget,
					  char*	  name,
					  GdkRGBA*  label_color);

ConstraintImage* workspace_label_stripe_get (GtkWidget *widget,
					     char *name,
					     GdkRGBA *label_color);


char*		window_label_get_name (Display* xdisplay, 
				       Window   xwindow);
GdkRGBA*	window_label_get_color(Display* xdisplay,
				       Window	xwindow);

GdkRGBA*	current_window_label_get_color ();
void		current_window_label_set_color (GdkRGBA *new_color);
char*		current_window_label_get_name ();
void		current_window_label_set_name (char *new_name);
GdkPixbuf *	current_window_icon_get ();


char *          current_workspace_label_get_name (GtkWidget *widget);
GdkRGBA*	current_workspace_label_get_color (GtkWidget *widget);

const char *	current_role_name (GtkWidget *widget);

void		update_trusted_stripe_roles ();

void            query_window_popups_show ();

void		trusted_stripe_help_show (GtkWidget *widget);

void		update_trusted_stripes ();
void		update_trusted_stripes_workspaces ();

void		setup_trusted_stripes ();

#endif /*__UTILS_H__*/


