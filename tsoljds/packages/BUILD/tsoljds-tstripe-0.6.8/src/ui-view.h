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

#ifndef __UIVIEW_H__
#define __UIVIEW_H__
#include <gtk/gtk.h>
#include <libwnck/screen.h>

typedef struct _TrustedStripe TrustedStripe;

struct _TrustedStripe
{
  GtkWidget *toplevel;
  GtkWidget *trusted_path_da;
  GtkWidget *window_da;
  GtkWidget *workspace_da;
  GtkWidget *role_da;
  /* window label data */
  GdkPixbuf *shield;
  GdkPixbuf *no_shield;
  gboolean   show_shield;
  /* role label pixbuf */
  GdkPixbuf *role_hat;
  /* workspace label pixbuf */
  GdkPixbuf *workspace_icon;
  /* query window popup */
  GtkWidget *query_window_label;
  GtkWidget *query_window_label_da;
};

TrustedStripe*	      create_trusted_stripe (GdkScreen *screen);

#endif /*__UIVIEW_H__*/


