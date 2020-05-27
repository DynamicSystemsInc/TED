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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>

int
main (int argc, char **argv)
{
	GtkRequisition req;
        int height, i, fd;
        GtkWidget *dummy; 
	Display *xdisp;

	gtk_init (&argc, &argv);

	dummy = gtk_label_new ("Blah");
	xdisp = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

        gtk_widget_size_request (dummy, &req);
	req.height = 16;
        height = req.height + 14;

	i = 0;

        while (i < ScreenCount (xdisp)) {
                XTSOLsetSSHeight (xdisp, i, height);
                i++;
        }

	gtk_main ();
}

