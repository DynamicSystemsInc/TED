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
#include <glib.h>

#include "devmgr-help.h"


void
devmgr_show_help (GtkWidget *w) 
{
	GError *err = NULL;

	gnome_help_display_desktop (NULL, "trusted", "index.xml", 
				    "device_manager" ,&err);
	if (err) {
		GtkWidget *err_dialog = gtk_message_dialog_new ((GtkWindow *)w,
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
}

