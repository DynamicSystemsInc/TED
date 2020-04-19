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

#ifndef TSOL_MOTD_H
#define TSOL_MOTD_H

G_BEGIN_DECLS

#include <config.h>
#include <gtk/gtk.h>

#include <label_builder.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>

typedef struct _MotdData MotdData;

struct _MotdData {
	GtkWidget *checkbutton;
	char 	  *label;
};

GtkWidget *tsol_motd_dialog_new (uid_t uid, bslabel_t lower_sl,
				 bclear_t upper_clear, gboolean sl_only);
void tsol_motd_dialog_destroy (GtkWidget *w);

G_END_DECLS

#endif
