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

#include <config.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <libgnomeui/libgnomeui.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "message_dialog.h"

enum {
	PROP_MSGDLG_0,
	PROP_SYSTEM_MODALITY,
};

struct _GnomeTsolMessageDialogDetails {
	gboolean        system_modal;	/* Set to TRUE for system modal
					 * dialogs */
	GtkWidget      *invisible;
};

/* GnomeTsolMessageDialogClass methods */
static void     gnome_tsol_message_dialog_class_init (GnomeTsolMessageDialogClass * class);
static void     gnome_tsol_message_dialog_instance_init (GnomeTsolMessageDialog * message_dialog);

/* GObjectClass methods */
static void     gnome_tsol_message_dialog_finalize (GObject * object);

/* GtkDialog callbacks */
static void 
message_dialog_hide_callback (GtkWidget * widget,
			      gpointer callback_data);
static void 
message_dialog_show_callback (GtkWidget * widget,
			      gpointer callback_data);

static gpointer parent_class;

static void
gnome_tsol_message_dialog_class_init_trampoline (gpointer klass, gpointer data)
{
	parent_class = (GtkMessageDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
	gnome_tsol_message_dialog_class_init ((GnomeTsolMessageDialogClass *) klass);
}

GtkType
gnome_tsol_message_dialog_get_type (void)
{
	static GType    type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (GnomeTsolMessageDialogClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) gnome_tsol_message_dialog_class_init_trampoline,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (GnomeTsolMessageDialog),
			0,	/* n_preallocs */
			(GInstanceInitFunc) gnome_tsol_message_dialog_instance_init,
			NULL
		};

		type = g_type_register_static (gtk_message_dialog_get_type (),
					       "GnomeTsolMessageDialog",
					       &info, 0);
	}
	return type;
}


void
gnome_tsol_message_dialog_set_property (GObject * object, guint prop_id,
				   const GValue * value, GParamSpec * pspec)
{
	GnomeTsolMessageDialog *message_dialog = GNOME_TSOL_MESSAGE_DIALOG (object);
	GnomeTsolMessageDialogDetails *details = message_dialog->details;

	switch (prop_id) {
	case PROP_SYSTEM_MODALITY:
		details->system_modal = g_value_get_boolean (value);
		if (details->system_modal == TRUE) {
			g_signal_connect (message_dialog, "hide",
					  G_CALLBACK (message_dialog_hide_callback), message_dialog);
			g_signal_connect (message_dialog, "show",
					  G_CALLBACK (message_dialog_show_callback), message_dialog);
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

void
gnome_tsol_message_dialog_get_property (GObject * object, guint prop_id,
					GValue * value, GParamSpec * pspec)
{
	GnomeTsolMessageDialog *message_dialog = GNOME_TSOL_MESSAGE_DIALOG (object);
	GnomeTsolMessageDialogDetails *details = message_dialog->details;

	switch (prop_id) {
	case PROP_SYSTEM_MODALITY:
		g_value_set_boolean (value, details->system_modal);
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gnome_tsol_message_dialog_class_init (GnomeTsolMessageDialogClass * class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->finalize = gnome_tsol_message_dialog_finalize;
	gobject_class->set_property = gnome_tsol_message_dialog_set_property;
	gobject_class->get_property = gnome_tsol_message_dialog_get_property;

	/* Construct */
	g_object_class_install_property (gobject_class, PROP_SYSTEM_MODALITY,
				       g_param_spec_boolean ("system-modal",
						      "system modal dialog",
					     "Make the dialog system modal",
							     FALSE,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gnome_tsol_message_dialog_instance_init (GnomeTsolMessageDialog * message_dialog)
{


	/*
	 * g_object_set (G_OBJECT (message_dialog), "type", GTK_WINDOW_POPUP,
	 * NULL);
	 */
	message_dialog->details = g_new0 (GnomeTsolMessageDialogDetails, 1);
#if 0
	/*
	 * When this dialog is shown we want it to be system modal like the
	 * gnome session logout confirmation dialog. These callbacks do the
	 * Xserver grabbing crack rock.
	 */
	g_signal_connect (message_dialog, "hide",
			  G_CALLBACK (dialog_hide_callback), message_dialog);
	g_signal_connect (message_dialog, "show",
			  G_CALLBACK (dialog_show_callback), message_dialog);
#endif
}

/* GObjectClass methods */
static void
gnome_tsol_message_dialog_finalize (GObject * object)
{
	GnomeTsolMessageDialog *message_dialog;

	message_dialog = GNOME_TSOL_MESSAGE_DIALOG (object);

	g_free (message_dialog->details);

	if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/**
 * gnome_tsol_message_dialog_new:
 * @parent: transient parent, or NULL for none
 * @flags: flags
 * @type: type of message
 * @buttons: set of buttons to use
 * @message_format: printf()-style format string, or NULL
 * @Varargs: arguments for @message_format
 *
 * Creates a new message dialog, which is a simple dialog with an icon
 * indicating the dialog type (error, warning, etc.) and some text the
 * user may want to see. When the user clicks a button a "response"
 * signal is emitted with response IDs from #GtkResponseType. See
 * #GtkDialog for more details.
 *
 * Return value: a new #GnomeTsolMessageDialog
 **/
GtkWidget      *
gnome_tsol_message_dialog_new (GtkWindow * parent,
			       GtkDialogFlags flags,
			       GtkMessageType type,
			       GtkButtonsType buttons,
			       gboolean sys_modal,
			       const gchar * message_format,
			       ...)
{
	GtkWidget      *widget;
	GtkDialog      *dialog;
	gchar          *msg = 0;
	va_list         args;

	g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

	widget = g_object_new (GNOME_TYPE_TSOL_MESSAGE_DIALOG,
			       "message_type", type,
			       "buttons", buttons,
			       "system-modal", sys_modal,
			       NULL);

	dialog = GTK_DIALOG (widget);

	if (flags & GTK_DIALOG_NO_SEPARATOR) {
		g_warning ("The GTK_DIALOG_NO_SEPARATOR flag cannot be used for GnomeTsolMessageDialog");
		flags &= ~GTK_DIALOG_NO_SEPARATOR;
	}
	if (message_format) {
		va_start (args, message_format);
		msg = g_strdup_vprintf (message_format, args);
		va_end (args);


		gtk_label_set_text (GTK_LABEL (GTK_MESSAGE_DIALOG (widget)->label),
				    msg);

		g_free (msg);
	}
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (widget),
					      GTK_WINDOW (parent));

	if (flags & GTK_DIALOG_MODAL)
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

	if (flags & GTK_DIALOG_NO_SEPARATOR)
		gtk_dialog_set_has_separator (dialog, FALSE);

	return widget;
}

void
center_window_on_screen (GtkWindow * window,
			 GdkScreen * screen,
			 int monitor)
{
	GtkRequisition  requisition;
	GdkRectangle    geometry;
	int             x, y;

	gdk_screen_get_monitor_geometry (screen, monitor, &geometry);
	gtk_widget_size_request (GTK_WIDGET (window), &requisition);

	x = geometry.x + (geometry.width - requisition.width) / 2;
	y = geometry.y + (geometry.height - requisition.height) / 2;

	gtk_window_move (window, x, y);
}

static void
message_dialog_hide_callback (GtkWidget * widget, gpointer callback_data)
{
	GnomeTsolMessageDialog *message_dialog;
	GdkScreen      *screen;
	GdkDisplay     *dpy;
	Display        *xdpy;
	gint            n_monitors;
	int             i;

	message_dialog = GNOME_TSOL_MESSAGE_DIALOG (widget);

	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_keyboard_ungrab (GDK_CURRENT_TIME);

	gtk_widget_destroy (message_dialog->details->invisible);
	gdk_flush ();
}

static void
message_dialog_show_callback (GtkWidget * widget, gpointer callback_data)
{
	GnomeTsolMessageDialog *message_dialog;

	GdkScreen      *screen;
	GdkDisplay     *dpy;
	Display        *xdpy;
	gint            n_monitors;
	int             i;

	message_dialog = GNOME_TSOL_MESSAGE_DIALOG (callback_data);

	if (gtk_widget_has_screen (GTK_WIDGET (message_dialog)))
		screen = gtk_widget_get_screen (GTK_WIDGET (message_dialog));
	else
		screen = gdk_screen_get_default ();

	dpy = gdk_screen_get_display (screen);
	xdpy = GDK_DISPLAY_XDISPLAY (dpy);

	/* Lock out the rest of the screen */
	message_dialog->details->invisible = gtk_invisible_new_for_screen (screen);
	gtk_widget_show (message_dialog->details->invisible);

	/* Oooh */
	while (1) {
		if (gdk_pointer_grab (message_dialog->details->invisible->window, FALSE, 0,
				 NULL, NULL, GDK_CURRENT_TIME) == Success) {
			if (gdk_keyboard_grab (message_dialog->details->invisible->window,
					FALSE, GDK_CURRENT_TIME) == Success)
				break;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
		}
		usleep (0.2 * 1000000);
	}

	center_window_on_screen (GTK_WINDOW (message_dialog), screen, 0);

	gtk_widget_show_all (GTK_WIDGET (message_dialog));
	while (1) {
		if (gdk_pointer_grab (GTK_WIDGET (message_dialog)->window, TRUE, 0,
				 NULL, NULL, GDK_CURRENT_TIME) == Success) {
			if (gdk_keyboard_grab (GTK_WIDGET (message_dialog)->window,
					FALSE, GDK_CURRENT_TIME) == Success)
				break;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
		}
		usleep (0.2 * 1000000);
	}

	XSetInputFocus (xdpy,
		   GDK_WINDOW_XWINDOW (GTK_WIDGET (message_dialog)->window),
			RevertToParent,
			CurrentTime);
}
