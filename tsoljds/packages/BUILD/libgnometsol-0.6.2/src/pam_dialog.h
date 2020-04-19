/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

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

#ifndef GNOME_TSOL_PASSWORD_DIALOG_H
#define GNOME_TSOL_PASSWORD_DIALOG_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

G_BEGIN_DECLS

#define GNOME_TYPE_TSOL_PASSWORD_DIALOG            (gnome_tsol_password_dialog_get_type ())
#define GNOME_TSOL_PASSWORD_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_TSOL_PASSWORD_DIALOG, GnomeTsolPasswordDialog))
#define GNOME_TSOL_PASSWORD_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_TSOL_PASSWORD_DIALOG, GnomeTsolPasswordDialogClass))
#define GNOME_IS_TSOL_PASSWORD_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_TSOL_PASSWORD_DIALOG))
#define GNOME_IS_TSOL_PASSWORD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_TSOL_PASSWORD_DIALOG))


typedef struct _GnomeTsolPasswordDialog        GnomeTsolPasswordDialog;
typedef struct _GnomeTsolPasswordDialogClass   GnomeTsolPasswordDialogClass;
typedef struct _GnomeTsolPasswordDialogDetails GnomeTsolPasswordDialogDetails;

struct _GnomeTsolPasswordDialog
{
	GtkDialog gtk_dialog;

	GnomeTsolPasswordDialogDetails *details;
};

struct _GnomeTsolPasswordDialogClass
{
	GtkDialogClass parent_class;
};

GType    gnome_tsol_password_dialog_get_type (void);

GtkWidget* gnome_tsol_password_dialog_new      (const char *dialog_title,
					   const char *prompt,
					   const char *message,
					   gboolean sys_modal,
					   gboolean hide_input);

gchar*	gnome_tsol_password_dialog_get_auth_token		(GnomeTsolPasswordDialog *password_dialog);
void  	gnome_tsol_password_dialog_set_message			(GnomeTsolPasswordDialog *password_dialog,
						gchar *message);
void 	gnome_tsol_password_dialog_set_input_prompt		(GnomeTsolPasswordDialog *password_dialog,
						gchar *prompt);
void	gnome_tsol_password_dialog_set_input_visibility	(GnomeTsolPasswordDialog *password_dialog,
						gboolean visible);
G_END_DECLS

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GNOME_TSOL_PASSWORD_DIALOG_H */
