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

#include <config.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "pam_dialog.h"

enum {
	PROP_PWDDLG_0,
	PROP_SYSTEM_MODALITY,
	PROP_PROMPT,
	PROP_MESSAGE,
	PROP_HIDE_INPUT,
	PROP_INPUT_TEXT,
	PROP_INPUT_SENSITIVITY
};

struct _GnomeTsolPasswordDialogDetails {
	/* Attributes */
	gboolean        hide_input;	/* Set to TRUE to hide password
					 * string */
	gboolean        input_sensitive;	/* Set to TRUE to allow
						 * password input */
	gboolean        system_modal;	/* Set to TRUE for system modal
					 * dialogs */
	gchar          *message;/* General purpose info messages from PAM */
	gchar          *prompt;	/* Pam supplied user prompt */
	gchar          *input_text;

	/* Internal widgetry */
	GtkWidget      *message_label;
	GtkWidget      *prompt_label;
	GtkWidget      *text_entry;
	GtkWidget      *invisible;
};

/* Layout constants */
static const guint DIALOG_BORDER_WIDTH = 6;
static const guint CAPTION_TABLE_BORDER_WIDTH = 4;

/* GnomeTsolPasswordDialogClass methods */
static void     gnome_tsol_password_dialog_class_init (GnomeTsolPasswordDialogClass * class);
static void     gnome_tsol_password_dialog_instance_init (GnomeTsolPasswordDialog * password_dialog);

/* GObjectClass methods */
static void     gnome_tsol_password_dialog_finalize (GObject * object);

/* GtkDialog callbacks */
static void 
password_dialog_hide_callback (GtkWidget * widget,
			       gpointer callback_data);
static void 
password_dialog_show_callback (GtkWidget * widget,
			       gpointer callback_data);

/* Private functions */
static void 
center_window_on_screen (GtkWindow * window,
			 GdkScreen * screen,
			 int monitor);

static gpointer parent_class;

static void
gnome_tsol_password_dialog_class_init_trampoline (gpointer klass, gpointer data)
{
	parent_class = (GtkDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
	gnome_tsol_password_dialog_class_init ((GnomeTsolPasswordDialogClass *) klass);
}

GType
gnome_tsol_password_dialog_get_type (void)
{
	static GType    type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (GnomeTsolPasswordDialogClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) gnome_tsol_password_dialog_class_init_trampoline,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (GnomeTsolPasswordDialog),
			0,	/* n_preallocs */
			(GInstanceInitFunc) gnome_tsol_password_dialog_instance_init,
			NULL
		};

		type = g_type_register_static (gtk_dialog_get_type (),
					       "GnomeTsolPasswordDialog",
					       &info, 0);
	}
	return type;
}

void
gnome_tsol_password_dialog_set_property (GObject * object, guint prop_id,
				   const GValue * value, GParamSpec * pspec)
{
	GnomeTsolPasswordDialog *password_dialog = GNOME_TSOL_PASSWORD_DIALOG (object);
	GnomeTsolPasswordDialogDetails *details = password_dialog->details;

	switch (prop_id) {
	case PROP_SYSTEM_MODALITY:
		details->system_modal = g_value_get_boolean (value);
		if (details->system_modal == TRUE) {
			g_signal_connect (password_dialog, "hide",
					  G_CALLBACK (password_dialog_hide_callback), password_dialog);
			g_signal_connect (password_dialog, "show",
					  G_CALLBACK (password_dialog_show_callback), password_dialog);
		}
		break;
	case PROP_PROMPT:
		if (details->prompt)
			g_free (details->prompt);
		details->prompt = g_value_dup_string (value);
		gtk_label_set_text (GTK_LABEL (details->prompt_label),
				    details->prompt);
		break;
	case PROP_MESSAGE:
		if (details->message)
			g_free (details->message);
		details->message = g_value_dup_string (value);
		gtk_label_set_text (GTK_LABEL (details->message_label),
				    details->message);
		break;
	case PROP_HIDE_INPUT:
		details->hide_input = g_value_get_boolean (value);
		gtk_entry_set_visibility (GTK_ENTRY (details->text_entry), !(details->hide_input));
		break;
	case PROP_INPUT_TEXT:
		if (details->input_text)
			g_free (details->input_text);
		details->input_text = g_value_dup_string (value);
		gtk_entry_set_text (GTK_ENTRY (details->text_entry), details->input_text);
		break;
	case PROP_INPUT_SENSITIVITY:
		details->input_sensitive = g_value_get_boolean (value);
		gtk_widget_set_sensitive (GTK_WIDGET (details->text_entry), details->input_sensitive);
		gtk_widget_set_sensitive (GTK_WIDGET (details->prompt_label), details->input_sensitive);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

void
gnome_tsol_password_dialog_get_property (GObject * object, guint prop_id,
					 GValue * value, GParamSpec * pspec)
{
	GnomeTsolPasswordDialog *password_dialog = GNOME_TSOL_PASSWORD_DIALOG (object);
	GnomeTsolPasswordDialogDetails *details = password_dialog->details;

	switch (prop_id) {
	case PROP_PROMPT:
		if (details->prompt) {
			g_value_set_string (value, details->prompt);
		} else {
			g_value_set_string (value, "");
		}
		break;
	case PROP_MESSAGE:
		if (details->message) {
			g_value_set_string (value, details->message);
		} else {
			g_value_set_string (value, "");
		}
		break;
	case PROP_HIDE_INPUT:
		g_value_set_boolean (value, details->hide_input);
		break;
	case PROP_INPUT_TEXT:
		/*
		 * NOTE - Attempting to make this simple by using this as the
		 * sync point between the contents of the text field and
		 * "input_text"
		 */
		if (details->input_text)
			g_free (details->input_text);
		details->input_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (details->text_entry)));
		g_value_set_string (value, details->input_text);
		break;
	case PROP_INPUT_SENSITIVITY:
		g_value_set_boolean (value, details->input_sensitive);
		break;
	case PROP_SYSTEM_MODALITY:
		g_value_set_boolean (value, details->system_modal);
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gnome_tsol_password_dialog_class_init (GnomeTsolPasswordDialogClass * class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->finalize = gnome_tsol_password_dialog_finalize;
	gobject_class->set_property = gnome_tsol_password_dialog_set_property;
	gobject_class->get_property = gnome_tsol_password_dialog_get_property;

	/* Construct */
	g_object_class_install_property (gobject_class, PROP_SYSTEM_MODALITY,
				       g_param_spec_boolean ("system-modal",
						      "system modal dialog",
					     "Make the dialog system modal",
							     FALSE,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/* Regular Props */
	g_object_class_install_property (gobject_class, PROP_PROMPT,
					 g_param_spec_string ("prompt",
							      "token prompt",
					      "authentication token prompt",
							      NULL,
							G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MESSAGE,
					 g_param_spec_string ("message",
							      "info message",
						      "information message",
							      NULL,
							G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_HIDE_INPUT,
					 g_param_spec_boolean ("hide-input",
							  "hide input text",
				"hide the contents of the text input field",
							       TRUE,
							G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_INPUT_TEXT,
					 g_param_spec_string ("input-text",
							"input text string",
						"input text field contents",
							      NULL,
							G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_INPUT_SENSITIVITY,
				    g_param_spec_boolean ("input-sensitive",
							"input sensitivity",
				  "The sensitivity of the text input field",
							  TRUE,
							G_PARAM_READWRITE));
}

static void
gnome_tsol_password_dialog_instance_init (GnomeTsolPasswordDialog * password_dialog)
{

	GtkWidget      *prompt_label;
	GtkWidget      *message_label;
	GtkWidget      *table;
	GtkWidget      *hbox;
	GtkWidget      *vbox;
	GtkWidget      *dialog_icon;
	gboolean        input_visible = FALSE;
	gchar          *prompt = "";
	gchar          *message = NULL;

	g_object_set (G_OBJECT (password_dialog), "type", GTK_WINDOW_POPUP, NULL);
	password_dialog->details = g_new0 (GnomeTsolPasswordDialogDetails, 1);

	gtk_dialog_add_buttons (GTK_DIALOG (password_dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);

	/* Setup the dialog */
	//gtk_dialog_set_has_separator (GTK_DIALOG (password_dialog), FALSE);

	gtk_window_set_position (GTK_WINDOW (password_dialog), GTK_WIN_POS_CENTER);

	gtk_window_set_title (GTK_WINDOW (password_dialog), " ");

	gtk_container_set_border_width (GTK_CONTAINER (password_dialog), DIALOG_BORDER_WIDTH);

	gtk_dialog_set_default_response (GTK_DIALOG (password_dialog), GTK_RESPONSE_OK);

	/* The table that holds the captions */
	table = gtk_table_new (1, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);

	password_dialog->details->text_entry = gtk_entry_new ();
	/*
	 * Visibility set to FALSE by default to prevent accidental password
	 * exposure
	 */
	gtk_entry_set_visibility (GTK_ENTRY (password_dialog->details->text_entry),
				  password_dialog->details->hide_input);

	g_signal_connect_swapped (password_dialog->details->text_entry,
				  "activate",
				  G_CALLBACK (gtk_window_activate_default),
				  password_dialog);
	password_dialog->details->prompt_label = gtk_label_new (password_dialog->details->prompt);
	prompt_label = password_dialog->details->prompt_label;
	gtk_misc_set_alignment (GTK_MISC (prompt_label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), prompt_label,
			  0, 1,
			  0, 1,
			  GTK_FILL,
			  (GTK_FILL | GTK_EXPAND),
			  0, 0);
	gtk_table_attach (GTK_TABLE (table), password_dialog->details->text_entry,
			  1, 2,
			  0, 1,
			  (GTK_FILL | GTK_EXPAND),
			  (GTK_FILL | GTK_EXPAND),
			  0, 0);

	/* Set up the dialog's icon */
	hbox = gtk_hbox_new (FALSE, 12);
	dialog_icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);
	gtk_misc_set_alignment (GTK_MISC (dialog_icon), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog_icon, FALSE, FALSE, 0);

	gtk_box_set_spacing (gtk_dialog_get_content_area(GTK_DIALOG(password_dialog)), 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_set_spacing (GTK_BOX (hbox), 12);

	/* Fills the vbox */
	vbox = gtk_vbox_new (FALSE, 0);

	message = password_dialog->details->message;
	password_dialog->details->message_label = gtk_label_new (message ? message : "");
	message_label = password_dialog->details->message_label;
	gtk_label_set_justify (GTK_LABEL (message_label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap (GTK_LABEL (message_label), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox),
			    message_label,
			    TRUE,	/* expand */
			    TRUE,	/* fill */
			    5);	/* padding */

	gtk_box_pack_start (GTK_BOX (vbox), table,
			    TRUE, TRUE, 5);

	/* Configure the table */
	gtk_container_set_border_width (GTK_CONTAINER (table), CAPTION_TABLE_BORDER_WIDTH);

	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 5);

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(password_dialog))),
			    hbox,
			    TRUE,	/* expand */
			    TRUE,	/* fill */
			    0);	/* padding */

	gtk_widget_show_all (gtk_dialog_get_content_area(GTK_DIALOG(password_dialog)));
}

/* GObjectClass methods */
static void
gnome_tsol_password_dialog_finalize (GObject * object)
{
	GnomeTsolPasswordDialog *password_dialog;

	password_dialog = GNOME_TSOL_PASSWORD_DIALOG (object);

	if (password_dialog->details->message)
		g_free (password_dialog->details->message);
	if (password_dialog->details->prompt)
		g_free (password_dialog->details->prompt);
	if (password_dialog->details->input_text)
		g_free (password_dialog->details->input_text);

	g_free (password_dialog->details);

	if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
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

/* GtkDialog callbacks */
static void
password_dialog_hide_callback (GtkWidget * widget, gpointer callback_data)
{
	GnomeTsolPasswordDialog *password_dialog;
	GdkScreen      *screen;
	GdkDisplay     *dpy;
	Display        *xdpy;
	gint            n_monitors;
	int             i;

	password_dialog = GNOME_TSOL_PASSWORD_DIALOG (widget);

	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_keyboard_ungrab (GDK_CURRENT_TIME);

	gtk_widget_destroy (password_dialog->details->invisible);
	gdk_flush ();
}

static void
password_dialog_show_callback (GtkWidget * widget, gpointer callback_data)
{
	GnomeTsolPasswordDialog *password_dialog;

	GdkScreen      *screen;
	GdkDisplay     *dpy;
	Display        *xdpy;
	gint            n_monitors;
	int             i;

	password_dialog = GNOME_TSOL_PASSWORD_DIALOG (callback_data);

	if (gtk_widget_has_screen (GTK_WIDGET (password_dialog)))
		screen = gtk_widget_get_screen (GTK_WIDGET (password_dialog));
	else
		screen = gdk_screen_get_default ();

	dpy = gdk_screen_get_display (screen);
	xdpy = GDK_DISPLAY_XDISPLAY (dpy);

	/* Lock out the rest of the screen */
	password_dialog->details->invisible = gtk_invisible_new_for_screen (screen);
	gtk_widget_show (password_dialog->details->invisible);

	/* Oooh */
	while (1) {
		GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(password_dialog->details->invisible));
		if (gdk_pointer_grab (window, FALSE, 0,
				 NULL, NULL, GDK_CURRENT_TIME) == Success) {
			if (gdk_keyboard_grab (window,
					FALSE, GDK_CURRENT_TIME) == Success)
				break;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
		}
		usleep (0.2 * 1000000);
	}

	center_window_on_screen (GTK_WINDOW (password_dialog), screen, 0);

	gtk_widget_show_all (GTK_WIDGET (password_dialog));
	while (1) {
		if (gdk_pointer_grab (gtk_widget_get_window(GTK_WIDGET(password_dialog)), TRUE, 0,
				 NULL, NULL, GDK_CURRENT_TIME) == Success) {
			if (gdk_keyboard_grab (gtk_widget_get_window(GTK_WIDGET(password_dialog)),
					FALSE, GDK_CURRENT_TIME) == Success)
				break;
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
		}
		usleep (0.2 * 1000000);
	}

	XSetInputFocus (xdpy,
		GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(password_dialog))),
		RevertToParent,
		CurrentTime);

	gtk_widget_grab_focus (password_dialog->details->text_entry);

}

/* Public GnomeTsolPasswordDialog methods */
GtkWidget      *
gnome_tsol_password_dialog_new (const char *dialog_title,
				const char *prompt,
				const char *message,
				gboolean sys_modal,
				gboolean hide_input)
{

	return g_object_new (GNOME_TYPE_TSOL_PASSWORD_DIALOG,
			     "prompt", prompt,
			     "message", message,
			     "system-modal", sys_modal,
			     "hide-input", hide_input,
			     NULL);
}

gchar          *
gnome_tsol_password_dialog_get_auth_token (GnomeTsolPasswordDialog * password_dialog)
{
	gchar          *token;

	g_return_val_if_fail (GNOME_IS_TSOL_PASSWORD_DIALOG (password_dialog), NULL);

	g_object_get (G_OBJECT (password_dialog), "input-text", &token, NULL);
	return token;
}

void
gnome_tsol_password_dialog_set_message (GnomeTsolPasswordDialog * password_dialog,
					gchar * message)
{
	g_return_if_fail (GNOME_IS_TSOL_PASSWORD_DIALOG (password_dialog));

	g_object_set (G_OBJECT (password_dialog), "message", message ? message : "", NULL);
}

void
gnome_tsol_password_dialog_set_input_prompt (GnomeTsolPasswordDialog * password_dialog,
					     gchar * prompt)
{
	g_return_if_fail (GNOME_IS_TSOL_PASSWORD_DIALOG (password_dialog));

	g_object_set (G_OBJECT (password_dialog), "prompt", prompt ? prompt : "", NULL);
}

void
gnome_tsol_password_dialog_set_input_visibility (GnomeTsolPasswordDialog * password_dialog,
						 gboolean visible)
{
	g_return_if_fail (GNOME_IS_TSOL_PASSWORD_DIALOG (password_dialog));

	g_object_set (G_OBJECT (password_dialog), "hide-input", !visible, NULL);
}
