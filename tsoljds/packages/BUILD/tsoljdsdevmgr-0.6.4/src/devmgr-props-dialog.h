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

#ifndef DEVMGR_PROPS_DIALOG_H
#define DEVMGR_PROPS_DIALOG_H

G_BEGIN_DECLS

#if 0
#include <tsol/label.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#define DEVMGR_PROPS_TYPE_DIALOG		(devmgr_props_dialog_get_type ())
#define DEVMGR_PROPS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), DEVMGR_PROPS_TYPE_DIALOG, DevMgrPropsDialog))
#define DEVMGR_PROPS_DIALOG_CLASS(klass)    	(G_TYPE_CHECK_CLASS_CAST ((klass), DEVMGR_PROPS_TYPE_DIALOG, DevMgrPropsDialogClass))
#define DEVMGR_PROPS_IS_DIALOG(obj)         	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEVMGR_PROPS_TYPE_DIALOG))
#define DEVMGR_PROPS_IS_DIALOG_CLASS(klass) 	(G_TYPE_CHECK_CLASS_TYPE ((klass), DEVMGR_PROPS_TYPE_DIALOG))

typedef struct _DevMgrPropsDialogDetails	DevMgrPropsDialogDetails;

typedef struct _DevMgrPropsDialog {
        GtkDialog parent;
	
        DevMgrPropsDialogDetails *details;
} DevMgrPropsDialog;

typedef struct _DevMgrPropsDialogClass {
	GtkDialogClass parent_class;

} DevMgrPropsDialogClass;

GType devmgr_props_dialog_get_type (void);
GtkWidget *dev_mgr_props_dialog_new (char *, char *, char *, char *, char *, char *, char *, char *, char *, char *);

#define DEVMGR_PROPS_RESET	-15

G_END_DECLS

#endif /* DEVMGR_PROPS_DIALOG_H */

