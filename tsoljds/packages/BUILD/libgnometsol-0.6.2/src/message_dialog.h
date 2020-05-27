/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

  The contents of this library are subject to the terms of the
  GNU Lesser General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this library except in compliance with the License.

  You should have received a copy of the License along with this
  library; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#ifndef GNOME_TSOL_MESSAGE_DIALOG_H
#define GNOME_TSOL_MESSAGE_DIALOG_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

G_BEGIN_DECLS

#define GNOME_TYPE_TSOL_MESSAGE_DIALOG            (gnome_tsol_message_dialog_get_type ())
#define GNOME_TSOL_MESSAGE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_TSOL_MESSAGE_DIALOG, GnomeTsolMessageDialog))
#define GNOME_TSOL_MESSAGE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_TSOL_MESSAGE_DIALOG, GnomeTsolMessageDialogClass))
#define GNOME_IS_TSOL_MESSAGE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_TSOL_MESSAGE_DIALOG))
#define GNOME_IS_TSOL_MESSAGE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_TSOL_MESSAGE_DIALOG))

typedef struct _GnomeTsolMessageDialog        GnomeTsolMessageDialog;
typedef struct _GnomeTsolMessageDialogClass   GnomeTsolMessageDialogClass;
typedef struct _GnomeTsolMessageDialogDetails GnomeTsolMessageDialogDetails;

struct _GnomeTsolMessageDialog
{
	GtkMessageDialog parent_instance;

	GnomeTsolMessageDialogDetails *details;
};

struct _GnomeTsolMessageDialogClass
{
	GtkMessageDialogClass parent_class;
};

GType    gnome_tsol_message_dialog_get_type (void);

GtkWidget* gnome_tsol_message_dialog_new      (GtkWindow      *parent,
                                        GtkDialogFlags  flags,
                                        GtkMessageType  type,
                                        GtkButtonsType  buttons,
                                        gboolean sys_modal,
                                        const gchar    *message_format,
                                        ...) G_GNUC_PRINTF (6, 7);

G_END_DECLS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GNOME_TSOL_MESSAGE_DIALOG_H */
