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
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <auth_attr.h>
#include <secdb.h>
#include <tsol/label.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>

#include <strings.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>

#include <constraint-scaling.h>

#include "selmgr-dialog.h"

struct _SelMgrDialogDetails {
	guint           timerid;
	int             timercount;

	GtkWidget      *message;
	GtkWidget      *source_label;
	GtkWidget      *source_eventbox;
	GtkWidget      *source_type;
	GtkWidget      *source_owner;
	GtkWidget      *dest_label;
	GtkWidget      *dest_eventbox;
	GtkWidget      *dest_type;
	GtkWidget      *dest_owner;
	GtkTextBuffer  *buffer;
	GtkWidget      *progress;
};

static void     selmgr_dialog_class_init (SelMgrDialogClass * klass);
static void     selmgr_dialog_instance_init (SelMgrDialog * object);

static GtkDialogClass *parent_class = NULL;

static void
selmgr_dialog_class_init_trampoline (gpointer klass, gpointer data)
{
	parent_class = (GtkDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
	selmgr_dialog_class_init ((SelMgrDialogClass *) klass);
}

GType
selmgr_dialog_get_type (void)
{
	static GType    type = 0;

	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (SelMgrDialogClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			selmgr_dialog_class_init_trampoline,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SelMgrDialog),
			0,	/* n_preallocs */
			(GInstanceInitFunc) selmgr_dialog_instance_init
		};

		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "SelMgrDialogType",
					       &info, 0);
	}
	return type;
}

static void
selmgr_dialog_finalize (GObject * object)
{
	SelMgrDialog   *selmgr_dialog = SELMGR_DIALOG (object);
	SelMgrDialogDetails *details = selmgr_dialog->details;

	/* free all my vars then the details structure */

	g_source_remove (details->timerid);

	g_free (details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static          gboolean
label_expose (GtkWidget * widget, cairo_t *cr, gpointer data)
{
	gnome_tsol_render_coloured_label (cr, widget);
	return TRUE;
}

static          gboolean
timeout_func (gpointer data)
{
	char           *str;
	SelMgrDialog   *dialog = (SelMgrDialog *) data;

	if (--dialog->details->timercount == 0) {
		g_signal_emit_by_name (dialog, "response", GTK_RESPONSE_CANCEL);
		return FALSE;
	}
	str = g_strdup_printf ("%d:%.2d", dialog->details->timercount / 60,
			       dialog->details->timercount % 60);

	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->details->progress),
				   str);
	g_free (str);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (
						 dialog->details->progress),
			     (gdouble) dialog->details->timercount / 118.0);

	return TRUE;
}

static gboolean
selection_should_be_hidden ()
{
        struct passwd  *user_data = getpwuid (getuid());

        if (user_data == NULL) {
                fprintf (stderr, "Workstation owner info invalid.\n");
                return (0);
        }

        return chkauthattr ("solaris.label.win.noview", user_data->pw_name);
}

static void
selmgr_dialog_instance_init (SelMgrDialog * selmgr_dialog)
{
	GdkPixbuf      *pixbuf;
	GtkWidget      *hbox, *label, *image, *vbox1, *vbox2, *ebox;
	GtkWidget      *text_view, *scroller;
	char           *str;
	SelMgrDialogDetails *details;

	GtkDialog      *dialog = GTK_DIALOG (selmgr_dialog);

	gtk_dialog_add_buttons (dialog, GTK_STOCK_HELP, GTK_RESPONSE_HELP,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

	pixbuf = gtk_widget_render_icon (GTK_WIDGET (dialog), GTK_STOCK_PASTE,
					 GTK_ICON_SIZE_BUTTON, NULL);
	gtk_window_set_icon (GTK_WINDOW (dialog), pixbuf);
	g_object_unref (pixbuf);

	gtk_window_set_title (GTK_WINDOW (dialog), _ ("Selection Manager"));

	selmgr_dialog->details = g_new0 (SelMgrDialogDetails, 1);
	details = selmgr_dialog->details;

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);

	image = gtk_image_new_from_stock (GTK_STOCK_PASTE,
					  GTK_ICON_SIZE_DIALOG);

	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	details->message = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (details->message),
			    "<b>Information Reclassification Required</b>\n"
			      "You are transferring a selection between two "
			    "windows with different labels.  This requires "
		       "the information in the selection be reclassified.");
	gtk_label_set_line_wrap (GTK_LABEL (details->message), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), details->message, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)), hbox, FALSE, FALSE, 10);

	hbox = gtk_hbox_new (FALSE, 0);
	vbox1 = gtk_vbox_new (FALSE, 2);
	vbox2 = gtk_vbox_new (FALSE, 2);

	label = gtk_label_new (NULL);
	str = g_strdup_printf ("<b>%s</b>", _ ("Orignal Information"));
	gtk_label_set_markup (GTK_LABEL (label), str);
	g_free (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox1), label, TRUE, TRUE, 0);

	label = gtk_label_new (NULL);
	str = g_strdup_printf ("<b>%s</b>", _ ("New Information"));
	gtk_label_set_markup (GTK_LABEL (label), str);
	g_free (str);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), label, TRUE, TRUE, 0);

	details->source_label = gtk_label_new ("ORIG LABEL");
	g_signal_connect (G_OBJECT (details->source_label), "draw",
			  G_CALLBACK (label_expose), NULL);
	gtk_misc_set_alignment (GTK_MISC (details->source_label), 0.0, 0.5);
	ebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (ebox), details->source_label);
	gtk_box_pack_start (GTK_BOX (vbox1), ebox, TRUE,
			    TRUE, 0);

	details->dest_label = gtk_label_new ("NEW LABEL");
	g_signal_connect (G_OBJECT (details->dest_label), "draw",
			  G_CALLBACK (label_expose), NULL);
	gtk_misc_set_alignment (GTK_MISC (details->dest_label), 0.0, 0.5);
	ebox = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (ebox), details->dest_label);
	gtk_box_pack_start (GTK_BOX (vbox2), ebox, TRUE, TRUE, 0);
	details->source_eventbox = gtk_event_box_new ();
	gtk_box_pack_start (GTK_BOX (vbox1), details->source_eventbox, TRUE,
			    TRUE, 0);
	details->source_type = gtk_label_new ("Type: TEXT");
	gtk_misc_set_alignment (GTK_MISC (details->source_type), 0.0, 0.5);
	gtk_container_add (GTK_CONTAINER (details->source_eventbox), 
			   details->source_type);

	details->dest_eventbox = gtk_event_box_new ();
	gtk_box_pack_start (GTK_BOX (vbox2), details->dest_eventbox, TRUE, TRUE, 0);
	details->dest_type = gtk_label_new ("Type: TEXT (96 bytes)");
	gtk_misc_set_alignment (GTK_MISC (details->dest_type), 0.0, 0.5);
	gtk_container_add (GTK_CONTAINER (details->dest_eventbox), 
			   details->dest_type);

	details->source_owner = gtk_label_new ("Owner: Stephen");
	gtk_misc_set_alignment (GTK_MISC (details->source_owner), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox1), details->source_owner, TRUE,
			    TRUE, 0);

	details->dest_owner = gtk_label_new ("Owner: Stephen");
	gtk_misc_set_alignment (GTK_MISC (details->dest_owner), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox2), details->dest_owner, TRUE,
			    TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), vbox1, FALSE, FALSE, 10);
	image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
					  GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 10);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)), hbox, FALSE, FALSE, 10);

	if (!selection_should_be_hidden ()) {
		hbox = gtk_hbox_new (FALSE, 0);

		label = gtk_label_new (NULL);
		str = g_strdup_printf ("<b>%s</b>", _ ("Selection"));
		gtk_label_set_markup (GTK_LABEL (label), str);
		g_free (str);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 10);

		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)), hbox, FALSE, FALSE,
				    10);

		details->buffer = gtk_text_buffer_new (NULL);
		gtk_text_buffer_set_text (details->buffer, "Selection text, you should not see this but viewing a selection is a little bit broken right now.  Its utterly broken in the CDE selection manager too by the way :)", -1);

		text_view = gtk_text_view_new_with_buffer (details->buffer);
		gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
		g_object_set (G_OBJECT (text_view), "wrap-mode", 
		      	      GTK_WRAP_WORD, NULL);
		scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW, NULL);
		gtk_container_set_border_width (GTK_CONTAINER (scroller), 10);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
					     GTK_SHADOW_ETCHED_IN);
		gtk_container_add (GTK_CONTAINER (scroller), text_view);

		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)), scroller, TRUE,
				    TRUE, 0);
	}

	hbox = gtk_hbox_new (FALSE, 0);

	label = gtk_label_new (_ ("Time remaining to complete:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 10);

	details->progress = gtk_progress_bar_new ();
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (details->progress), "1:58");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (details->progress),
				       1.0);

	gtk_box_pack_start (GTK_BOX (hbox), details->progress, TRUE, TRUE, 10);

	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(dialog)), hbox, FALSE, FALSE, 10);

	gtk_window_set_position (GTK_WINDOW (dialog),
				 GTK_WIN_POS_CENTER);
	gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);

	details->timercount = 118;
	details->timerid = g_timeout_add (1000, timeout_func, dialog);
	gtk_window_resize(GTK_WINDOW(dialog), 400, 475);
}

static void
selmgr_dialog_class_init (SelMgrDialogClass * class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->finalize = selmgr_dialog_finalize;
}


GtkWidget      *
sel_mgr_dialog_new (char *message, char *src_label, char *dest_label,
		    char *src_type, char *dest_type, char *src_owner,
		    char *dest_owner, gboolean authorised, char *data)
{
	char *str1, *str2;
	GtkWidget      *dialog = g_object_new (SELMGR_TYPE_DIALOG, NULL);
	SelMgrDialogDetails *details = SELMGR_DIALOG (dialog)->details;

	gtk_label_set_markup (GTK_LABEL (details->message), message);
	gtk_label_set_text (GTK_LABEL (details->source_label), src_label);
	gtk_label_set_text (GTK_LABEL (details->dest_label), dest_label);
	if (strlen (src_type) > 25)  {
                str1 = g_strndup (src_type, 25);
                str2 = g_strdup_printf ("%s...", str1);
                g_free (str1);
		gtk_label_set_text (GTK_LABEL (details->source_type), str2);
		g_free (str2);
        } else {
		gtk_label_set_text (GTK_LABEL (details->source_type), src_type);
	}
	gtk_widget_set_tooltip_text (details->source_eventbox, src_type);
	if (strlen (dest_type) > 25)  {
                str1 = g_strndup (dest_type, 25);
                str2 = g_strdup_printf ("%s...", str1);
                g_free (str1);
		gtk_label_set_text (GTK_LABEL (details->dest_type), str2);
		g_free (str2);
        } else {
		gtk_label_set_text (GTK_LABEL (details->dest_type), dest_type);
	}
	gtk_widget_set_tooltip_text (details->dest_eventbox, dest_type);
	gtk_label_set_text (GTK_LABEL (details->source_owner), src_owner);
	gtk_label_set_text (GTK_LABEL (details->dest_owner), dest_owner);

	gtk_text_buffer_set_text (details->buffer, data, -1);

	if (!authorised)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GTK_RESPONSE_OK,
						   FALSE);

	return dialog;
}
