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

#ifndef SELMGR_DIALOG_H
#define SELMGR_DIALOG_H

G_BEGIN_DECLS

#include <tsol/label.h>
#include <glib.h>
#include <gtk/gtk.h>

#define SELMGR_TYPE_DIALOG		(selmgr_dialog_get_type ())
#define SELMGR_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), SELMGR_TYPE_DIALOG, SelMgrDialog))
#define SELMGR_DIALOG_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), SELMGR_TYPE_DIALOG, SelMgrDialogClass))
#define SELMGR_IS_DIALOG(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), SELMGR_TYPE_DIALOG))
#define SELMGR_IS_DIALOG_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), SELMGR_TYPE_DIALOG))

typedef struct _SelMgrDialogDetails	SelMgrDialogDetails;

typedef struct _SelMgrDialog {
        GtkDialog parent;
	
        SelMgrDialogDetails *details;
} SelMgrDialog;

typedef struct _SelMgrDialogClass {
	GtkDialogClass parent_class;

} SelMgrDialogClass;

GType selmgr_dialog_get_type (void);
GtkWidget *sel_mgr_dialog_new (char *message, char* src_label, char* dest_label,
			char *src_type, char *dest_type, char *src_owner,
			char *dest_owner, gboolean authorised, char* data);

G_END_DECLS

#endif /* SELMGR_DIALOG_H */

