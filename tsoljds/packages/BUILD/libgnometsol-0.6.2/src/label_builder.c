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
#include <tsol/label.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>

#include <strings.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>

#include "label_builder.h"


enum {
	COL_TOGGLE = 0,
	COL_LONG,
	COL_SHORT,
	COL_SENSITIVE,
	NUM_COLS
};

struct _GnomeLabelBuilderDetails {
	int		mode;         /* label builder mode: sl/clr */
	char		*userfield;   /* user message string */

	m_label_t	*sl;          /* current label*/
	m_range_t	range;	      /* label range of the builder */
	char 		*saved_label_str; /*saved label */

	GtkListStore	*class_store; /* list of available classes */
	GtkListStore	*comps_store; /* list of available compartments */

	int		first_compartment;
	int 		size;
	int 		ready;
	gulong		hid;

	GtkWidget	*user_label;  /* show if we are SL or CLR */
	GtkWidget	*class_list;  /* list(tree) displaying classes */
	GtkWidget	*comps_list;  /* list(tree) displaying compartments */
	GtkWidget	*entry;       /* text entry for manual label entry */
	GtkWidget	*da;          /* drawing area for label colour */
};

static GtkDialogClass *parent_class = NULL;

static void     gnome_label_builder_class_init (GnomeLabelBuilderClass * klass);
static void     gnome_label_builder_instance_init (GnomeLabelBuilder * object);

static void
gnome_label_builder_class_init_trampoline (gpointer klass, gpointer data)
{
	parent_class = (GtkDialogClass *) g_type_class_ref (GTK_TYPE_DIALOG);
	gnome_label_builder_class_init ((GnomeLabelBuilderClass *) klass);
}

static void
show_color(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	GdkRGBA color;
	char *label_str, *colorname = NULL;
	GnomeLabelBuilderDetails *details;

	details = (GnomeLabelBuilderDetails *) data;
	label_to_str (details->sl, &colorname, M_COLOR, LONG_NAMES);
	if (!colorname)
		colorname = g_strdup ("white");

	gdk_rgba_parse (&color, colorname);
	cairo_set_source_rgb(cr, color.red, color.green, color.blue);
	cairo_paint(cr);
	g_free (colorname);
}

gboolean
trusted_path_in_user_range (GnomeLabelBuilderDetails * details)
{
	m_label_t adlow;
	m_label_t adhigh;

	bsllow (&adlow);
	bslhigh (&adhigh);

	if (blinrange (&adhigh, &details->range) &&
	    blinrange (&adlow, &details->range)) {
		return TRUE;
	}
	return FALSE;
}

gboolean
label_is_trusted_path (m_label_t *label, int mode)
{
	char *label_str = NULL;
	gboolean tp;

	label_to_str (label, &label_str, M_LABEL, LONG_NAMES);

	if (label_str) {
		if (strcmp ("ADMIN_LOW", label_str) == 0 ||
		    strcmp ("ADMIN_HIGH", label_str) == 0) {
			tp = TRUE;
		} else {
			tp = FALSE;
		}
		g_free (label_str);
	}
	return tp;
}

static gboolean
set_cursor_pos (gpointer data)
{
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;

	gtk_editable_set_position (GTK_EDITABLE (details->entry), -1);
	g_signal_handler_unblock (details->entry, details->hid);

	return FALSE;
}

static void
update_entry_and_colorbox (GnomeLabelBuilderDetails *details)
{
	GdkRGBA color;
	cairo_t	*cr;
	GdkWindow *window;
	char *label_str, *colorname = NULL;

	gboolean tp = label_is_trusted_path (details->sl, details->mode);

	g_signal_handler_block (details->entry, details->hid);
	label_to_str (details->sl, &label_str, M_LABEL, LONG_NAMES);	
	if (tp) {
		gtk_entry_set_text (GTK_ENTRY (details->entry), "TRUSTED PATH");
		gtk_editable_set_position (GTK_EDITABLE (details->entry), 12);
	} else {
		gtk_entry_set_text (GTK_ENTRY (details->entry), label_str);
		gtk_editable_set_position (GTK_EDITABLE (details->entry), 
					   strlen(label_str));
	}

	g_timeout_add (10, set_cursor_pos, details);
	g_free (label_str);

	window = gtk_widget_get_window(details->da);
	if (window != NULL) {
		label_to_str (details->sl, &colorname, M_COLOR, LONG_NAMES);
		if (!colorname)
			colorname = g_strdup ("white");

		gdk_rgba_parse (&color, colorname);
		cr = gdk_cairo_create(window);
		cairo_set_source_rgb(cr, color.red, color.green, color.blue);
		cairo_paint(cr);
		cairo_destroy(cr);
		g_free (colorname);
	}
}

static void
update_ui (GnomeLabelBuilderDetails *details)
{
	int ret, i;
	char *str, *display = NULL;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	gboolean tp = label_is_trusted_path (details->sl, details->mode);

	if (details->mode == LBUILD_MODE_SL) {
		ret = bslcvt (details->sl, 0, &str, &display);
	} else {
		ret = bclearcvt (details->sl, 0, &str, &display);
	}

	if (!display) { 
		/* Something has gone wrong with the label conversion
		   probbly because update_ui has been called before the 
		   label range has been setup.  Ignore the update request
		   as a new one should be imminent */
		return;
	}
	
	model = GTK_TREE_MODEL (details->class_store);
	if (tp) {
		for (i = 0; i < details->first_compartment; i++) {
			gtk_tree_model_iter_nth_child (model, &iter, NULL, i);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					    COL_TOGGLE, 0, COL_SENSITIVE, 1, -1);
		}
		gtk_tree_model_iter_nth_child (model, &iter, NULL, i++);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_TOGGLE, 1,
						  COL_SENSITIVE, 1, -1);
	} else { 
		for (i = 0; i < details->first_compartment; i++) {
			gtk_tree_model_iter_nth_child (model, &iter, NULL, i);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					COL_TOGGLE, 
				    	(display[i] & CVT_SET) ? 1 : 0, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					COL_SENSITIVE, 
				    	(display[i] & CVT_DIM) ? 0 : 1, -1);
		}
		if (trusted_path_in_user_range (details)) {
			gtk_tree_model_iter_nth_child (model, &iter, NULL, i++);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					    COL_TOGGLE, 0,
					    COL_SENSITIVE, 1, -1);
		}
	}

	model = GTK_TREE_MODEL (details->comps_store);
	if (tp) {
		for (i = 0; i < details->size-details->first_compartment; i++) {
			gtk_tree_model_iter_nth_child (model, &iter, NULL, i);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					    COL_TOGGLE, 0, COL_SENSITIVE, 0, -1);
		}
	} else {
		for (i = details->first_compartment; i < details->size; i++) {
			gtk_tree_model_iter_nth_child (model, &iter, NULL, 
						i - details->first_compartment);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					   COL_TOGGLE, 
					   (display[i] & CVT_SET) ? 1 : 0, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, 
					    COL_SENSITIVE, 
					   (display[i] & CVT_DIM) ? 0 : 1, -1);
		}
	}

	update_entry_and_colorbox (details);
}

static void
reset_lists (GnomeLabelBuilderDetails *details) 
{
	char *str, *label_str, *display;
	char **long_words, **short_words;
	int  i, ret, err;
	GtkTreeIter iter;

	if (label_to_str (details->sl, &label_str, M_LABEL, LONG_NAMES) < 0) {
		return;
	}
	m_label_free (details->sl);
	details->sl = NULL;

	/* Do a full conversion for all labels bound by the range */
	if (details->mode == LBUILD_MODE_SL) {	
		details->sl = m_label_alloc (MAC_LABEL);
		str_to_label (label_str, &details->sl, MAC_LABEL, L_DEFAULT, 
			      &err);
		ret = bslcvtfull (details->sl, &details->range,
			    0, &str, &long_words, &short_words,
			    &display, &(details->first_compartment), 
			    &(details->size));
	} else {
		details->sl = m_label_alloc (USER_CLEAR);
		str_to_label (label_str, &details->sl, USER_CLEAR, L_DEFAULT, 
			      &err);
		ret = bclearcvtfull (details->sl, &details->range,
			       0, &str, &long_words,
			       &short_words, &display,
			       &(details->first_compartment), &(details->size));
	}

	/* Make the list of classifications */
	gtk_list_store_clear (details->class_store);
	for (i = 0; i < details->first_compartment; i++) {
		gtk_list_store_append (details->class_store, &iter);
		gtk_list_store_set (details->class_store, &iter, 
				    COL_LONG, g_strdup (long_words[i]),
				    COL_SHORT, g_strdup_printf ("(%s)",
								short_words[i]),
				    COL_SENSITIVE, 1, COL_TOGGLE, 0, -1);
	}

	/* Add Trusted Path if its in the user's range */
	if (trusted_path_in_user_range (details)) {
		gtk_list_store_append (details->class_store, &iter);
		gtk_list_store_set (details->class_store, &iter, 
				    COL_LONG, "TRUSTED PATH", 
				    COL_SHORT, g_strdup (""),
				    COL_SENSITIVE, 1,  COL_TOGGLE, 0, -1);
	}

	/* Make the list of compartments */
	gtk_list_store_clear (details->comps_store);
	for (i = details->first_compartment; i < details->size; i++) {
		gtk_list_store_append (details->comps_store, &iter);
		if (long_words[i]) {
			gtk_list_store_set (details->comps_store, &iter, 
				    COL_LONG, g_strdup (long_words[i]),
				    COL_SENSITIVE, 1, COL_TOGGLE, 0, -1);
		}
		if (short_words[i]) {
			gtk_list_store_set (details->comps_store, &iter, 
				    COL_SHORT, g_strdup_printf ("(%s)",
								short_words[i]),
				    COL_SENSITIVE, 1, COL_TOGGLE, 0, -1);
		}
	}

	update_ui (details);
}

GType
gnome_label_builder_get_type (void)
{
	static GType    type = 0;

	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GnomeLabelBuilderClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			gnome_label_builder_class_init_trampoline,
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GnomeLabelBuilder),
			0,      /* n_preallocs */
			(GInstanceInitFunc) gnome_label_builder_instance_init
		};

		type = g_type_register_static (GTK_TYPE_DIALOG,
					       "GnomeLabelBuilderType",
					       &info, 0);
	}

	return type;
}

void
gnome_label_builder_finalize (GObject * object)
{
	GnomeLabelBuilder *lbuild = GNOME_LABEL_BUILDER (object);
	GnomeLabelBuilderDetails *details = lbuild->details;

	if (details->userfield)
		g_free (details->userfield);

	g_free (lbuild->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gnome_label_builder_set_property (GObject *object, guint prop_id,
				  const GValue *value, GParamSpec *pspec)
{
	GnomeLabelBuilder *lbuild = GNOME_LABEL_BUILDER (object);
	GnomeLabelBuilderDetails *details = lbuild->details;

	switch (prop_id) {
	case PROP_MODE:
		details->mode = g_value_get_int (value);
		details->ready++;
		break;
	case PROP_MESSAGE:
		if (details->userfield)
			g_free (details->userfield);
		details->userfield = g_value_dup_string (value);
		gtk_label_set_text (GTK_LABEL (details->user_label),
				    details->userfield);
		break;
	case PROP_SL:
		*details->sl = *(m_label_t *) g_value_get_pointer (value);
		if (details->saved_label_str) g_free (details->saved_label_str);
		label_to_str (details->sl, &details->saved_label_str,
			      M_LABEL, LONG_NAMES);
		update_ui (details);
		break;
	case PROP_LOWER_BOUND:
		*details->range.lower_bound =
			*(m_label_t *) g_value_get_pointer (value);
		details->ready++;
		break;
	case PROP_UPPER_BOUND:
		*details->range.upper_bound =
			*(m_label_t *) g_value_get_pointer (value);
		details->ready++;
		*details->sl = *(m_label_t *)g_value_get_pointer(value);
		if (details->saved_label_str) g_free (details->saved_label_str);
		label_to_str (details->sl, &details->saved_label_str,
			      M_LABEL, LONG_NAMES);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	if (details->ready > 2) {
		details->ready = 3;
		reset_lists (details);
	}
}

void
gnome_label_builder_get_property (GObject *object, guint prop_id,
				  GValue *value, GParamSpec *pspec)
{
	GnomeLabelBuilder *lbuild = GNOME_LABEL_BUILDER (object);
	GnomeLabelBuilderDetails *details = lbuild->details;

	switch (prop_id) {
	case PROP_MODE:
		g_value_set_int (value, details->mode);
		break;
        case PROP_MESSAGE:
		if (details->userfield) {
			g_value_set_string (value, details->userfield);
		} else {
			g_value_set_string (value, "");
		}
		break;
	case PROP_SL:
		g_value_set_pointer (value, details->sl);
		break;
	case PROP_UPPER_BOUND:
		g_value_set_pointer (value, details->range.upper_bound);
		break;
	case PROP_LOWER_BOUND:
		g_value_set_pointer (value, details->range.lower_bound);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gnome_label_builder_class_init (GnomeLabelBuilderClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->finalize = gnome_label_builder_finalize;
	gobject_class->set_property = gnome_label_builder_set_property;
	gobject_class->get_property = gnome_label_builder_get_property;

	g_object_class_install_property (gobject_class, PROP_MODE,
					 g_param_spec_int ("mode", "build mode",
							   "mode to build in",
							   LBUILD_MODE_SL,
							   LBUILD_MODE_CLR,
							   LBUILD_MODE_SL,
							   G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_MESSAGE,
					 g_param_spec_string ("message",
							   "title message",
							   "dialog message",
							   NULL,
							   G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_SL,
					 g_param_spec_pointer ("sl", "sl value",
						"sensitivity label value",
						G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_UPPER_BOUND,
			g_param_spec_pointer ("upper", "upper label in range",
					      "upper range bounds",
					      G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_LOWER_BOUND,
			g_param_spec_pointer ("lower", "lower label in range ",
					      "lower range bounds",
					      G_PARAM_READWRITE));
}

gboolean
gnome_label_builder_show_help (GtkWidget *w)
{
	GError *err = NULL;
	char *command = "atril --preview /usr/share/mate/help/LabelBuilderHelp.pdf";

	/*
	gnome_help_display_desktop (NULL, "trusted", "index.xml",
				    "label_builder" ,&err);
	*/

	g_spawn_command_line_async (command, &err);
	if (err) {
		GtkWidget *err_dialog = gtk_message_dialog_new (GTK_WINDOW (w),
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

static void
class_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeIter iter;
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;
	GtkTreeModel *model = GTK_TREE_MODEL (details->class_store);
	gboolean value;
	char *text, *comp, *tmp1, *tmp2;
	int err, i;

	gtk_tree_model_get_iter_from_string (model, &iter, path_str);

	gtk_tree_model_get (model, &iter, COL_TOGGLE, &value, 
					  COL_LONG, &text, -1);

	if (value) return;

	blabel_free (details->sl);
	details->sl = NULL;

	tmp1 = g_strdup (text);

	model = GTK_TREE_MODEL (details->comps_store);
	for (i = details->first_compartment; i < details->size; i++) {
		gtk_tree_model_iter_nth_child (model, &iter, NULL,
					       i - details->first_compartment);
		gtk_tree_model_get (model, &iter, COL_TOGGLE, &value,
						  COL_LONG, &comp, -1);
		if (value) {
			tmp2 = g_strdup_printf ("%s %s", tmp1,  comp);
			g_free (tmp1);
			tmp1 = tmp2;
		}
	}

	if (details->mode == LBUILD_MODE_SL)  {
		str_to_label (tmp1, &details->sl, MAC_LABEL, L_DEFAULT,  &err);
	} else {
		str_to_label (tmp1, &details->sl, USER_CLEAR, L_DEFAULT, &err);
	}

	update_ui (details);
}

static void
comp_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeIter iter;
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;
	GtkTreeModel *model = GTK_TREE_MODEL (details->comps_store);
	gboolean value;
	char *text, *label_str, *new_label_str;
	int err;

	gtk_tree_model_get_iter_from_string (model, &iter, path_str);

	gtk_tree_model_get (model, &iter, COL_TOGGLE, &value, 
					  COL_LONG, &text, -1);

	label_to_str (details->sl, &label_str, M_LABEL, LONG_NAMES);

	new_label_str = g_strdup_printf ("%s %c %s", label_str, 
					    value ? '-' : '+', text);

	blabel_free (details->sl);
	details->sl = NULL;

	if (details->mode == LBUILD_MODE_SL) {
		str_to_label (new_label_str, &details->sl, 
			      MAC_LABEL, L_DEFAULT,  &err);
	} else {
		str_to_label (new_label_str, &details->sl, 
			      USER_CLEAR, L_DEFAULT, &err);
	}

	g_free (new_label_str);

	update_ui (details);
}

static void
emit_cancel_cb (GtkWidget *dialog, gpointer data)
{
	g_signal_emit_by_name (G_OBJECT (dialog), "response", 
			       GTK_RESPONSE_CANCEL, NULL);
}

GdkPixbuf*
get_icon_pixbuf_from_theme (char *icon_name)
{
	static GtkIconTheme *icon_theme = NULL;

	if (!icon_theme) icon_theme = gtk_icon_theme_get_default ();
	GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon (icon_theme,
							     icon_name, 48, 0);
	GdkPixbuf *pixbuf = gtk_icon_info_load_icon (icon_info, NULL);

	g_object_unref(icon_info);
	return pixbuf;
}

static void
reset_clicked (GtkWidget *button, gpointer data)
{
	int err;
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;

	if (!details->saved_label_str) return;

	if (details->mode == LBUILD_MODE_SL) {
		str_to_label (details->saved_label_str, &details->sl,
			      MAC_LABEL, L_DEFAULT, &err);
	} else {
		str_to_label (details->saved_label_str, &details->sl,
			      USER_CLEAR, L_DEFAULT, &err);
	}

	update_ui (details);
}

static void
sensitive_func (GtkTreeViewColumn *col, GtkCellRenderer *renderer,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean sensitive;

	gtk_tree_model_get (model, iter, COL_SENSITIVE, &sensitive, -1);

	g_object_set (G_OBJECT (renderer), "sensitive", sensitive, NULL);
}

static void
update_ui_based_on_entry_text (GnomeLabelBuilderDetails *details, 
			   gboolean update_on_invalid_label)
{
	int err, ret;
	const char *text;
	char *new_label_str, *label_str;
	m_label_t *sl;

	text = gtk_entry_get_text (GTK_ENTRY (details->entry));
	if (text && *text == '\0') {
		if (update_on_invalid_label) update_ui (details);
		return;
	}

	label_to_str (details->sl, &label_str, M_LABEL, LONG_NAMES);

	if (strncmp ("+", text, 1) == 0 || 
	    strncmp ("-", text, 1) == 0) {
		new_label_str = g_strdup_printf ("%s %s", label_str, text);
	} else {
		if (strncmp ("TRUSTED PATH", text, 12) == 0) {
			new_label_str = g_strdup ("ADMIN_HIGH");
		} else {
			new_label_str = g_strdup (text);
		}
	}

	sl = NULL;

	if (details->mode == LBUILD_MODE_SL) {
               ret =  str_to_label (new_label_str, &sl,
                              MAC_LABEL, L_DEFAULT,  &err);
        } else {
               ret = str_to_label (new_label_str, &sl,
                              USER_CLEAR, L_DEFAULT, &err);
        }

	if (ret != -1 && blinrange (sl, &details->range)) {
		m_label_free (details->sl);
		details->sl = sl;
		update_ui (details);
	} else {
		m_label_free (sl);
		if (update_on_invalid_label) update_ui (details);
	}

	g_free (new_label_str);

}

static void
entry_activated (GtkWidget *entry, gpointer data)
{
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;

	update_ui_based_on_entry_text (details, TRUE);
}

static void
entry_text_changed (GtkWidget *entry, gpointer data)
{
	int len;
	GnomeLabelBuilderDetails *details = (GnomeLabelBuilderDetails *) data;
	const char *text = gtk_entry_get_text (GTK_ENTRY (details->entry));

	if (text) {
		len = strlen (text) -1;
		if (text[len] == ' ' || text[len] == '+' || text[len] == '-') {
			return;
		}
	}
	
	update_ui_based_on_entry_text (details, FALSE);
}

static void
gnome_label_builder_instance_init (GnomeLabelBuilder *lbuilder)
{
	GdkPixbuf *pixbuf;
	GnomeLabelBuilderDetails *details;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkTreeSelection *selection;
	GtkWidget *scroller;
	GtkWidget *frame;
	GdkRGBA  color;
	GtkWidget *vbox, *list_vbox, *hbox, *label;
	GtkWidget *reset_button;

	GtkDialog *dialog = GTK_DIALOG (lbuilder);

	bindtextdomain (GETTEXT_PACKAGE, LIBGNOMETSOL_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	gtk_dialog_add_buttons (dialog, GTK_STOCK_HELP, GTK_RESPONSE_HELP,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);

	pixbuf = get_icon_pixbuf_from_theme ("gnome-system");
	gtk_window_set_icon (GTK_WINDOW (dialog), pixbuf);
	g_object_unref (pixbuf);

	lbuilder->details = g_new0 (GnomeLabelBuilderDetails, 1);
	details = lbuilder->details;

	details->ready = 0;

	details->saved_label_str = NULL;

	details->range.upper_bound = m_label_alloc (MAC_LABEL);
	bslhigh (details->range.upper_bound);
	details->range.lower_bound = m_label_alloc (MAC_LABEL);
	bsllow (details->range.lower_bound);

	details->sl = m_label_alloc (MAC_LABEL);

	vbox = gtk_vbox_new (FALSE, 10);
	hbox = gtk_hbox_new (FALSE, 10);

	label = gtk_label_new ("do not translate");
	details->user_label = label;
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	details->class_store = gtk_list_store_new (NUM_COLS, G_TYPE_BOOLEAN,
						   G_TYPE_STRING,
						   G_TYPE_STRING,
						   G_TYPE_BOOLEAN);

	details->class_list = 
	   gtk_tree_view_new_with_model (GTK_TREE_MODEL (details->class_store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (details->class_list),
					   FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (class_toggled),
			  details);
	g_object_set (G_OBJECT (renderer), "radio", TRUE, NULL);
	col = gtk_tree_view_column_new_with_attributes ("dummy", renderer,
							"active", COL_TOGGLE,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->class_list), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("long", renderer,
							"text", COL_LONG, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->class_list), col);
	gtk_tree_view_column_set_cell_data_func (col, renderer, sensitive_func,
						 NULL, NULL);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("short", renderer,
							"text", COL_SHORT, 
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->class_list), col);
	gtk_tree_view_column_set_cell_data_func (col, renderer, sensitive_func,
						 NULL, NULL);

	renderer = gtk_cell_renderer_toggle_new ();
	col = gtk_tree_view_column_new_with_attributes ("dummy", renderer,
							"active", COL_SENSITIVE,
							NULL);
	gtk_tree_view_column_set_cell_data_func (col, renderer, sensitive_func,
						 NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->class_list), col);
	gtk_tree_view_column_set_visible (col, FALSE);

	selection = 
	      gtk_tree_view_get_selection (GTK_TREE_VIEW (details->class_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

	details->comps_store = gtk_list_store_new (NUM_COLS, G_TYPE_BOOLEAN,
						   G_TYPE_STRING,
						   G_TYPE_STRING,
						   G_TYPE_BOOLEAN);

        details->comps_list = 
	    gtk_tree_view_new_with_model GTK_TREE_MODEL (details->comps_store);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (details->comps_list),
					   FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (comp_toggled),
			  details);
	col = gtk_tree_view_column_new_with_attributes ("toggle", renderer,
							"active", COL_TOGGLE,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->comps_list), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("long", renderer,
							"text", COL_LONG, NULL);
	gtk_tree_view_column_set_cell_data_func (col, renderer, sensitive_func,
						 NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->comps_list), col);

	renderer = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes ("short", renderer,
							"text", COL_SHORT, 
							NULL);
	gtk_tree_view_column_set_cell_data_func (col, renderer, sensitive_func,
						 NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->comps_list), col);

	renderer = gtk_cell_renderer_toggle_new ();
	col = gtk_tree_view_column_new_with_attributes ("dummy", renderer,
							"active", COL_SENSITIVE,
							NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (details->class_list), col);
	gtk_tree_view_column_set_visible (col, FALSE);

	selection = 
	      gtk_tree_view_get_selection (GTK_TREE_VIEW (details->comps_list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

	hbox = gtk_hbox_new (TRUE, 10);

	scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
					     GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (scroller), details->class_list);

	list_vbox = gtk_vbox_new (FALSE, 0);
	label = gtk_label_new (_ ("Classification"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (list_vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (list_vbox), scroller, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), list_vbox, TRUE, TRUE, 0);

	scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
					     GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (scroller), details->comps_list);

	list_vbox = gtk_vbox_new (FALSE, 0);
	label = gtk_label_new (_ ("Sensitivity"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (list_vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (list_vbox), scroller, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), list_vbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 10);

	reset_button = gtk_button_new_from_stock (GTK_STOCK_REVERT_TO_SAVED);
	gtk_box_pack_start (GTK_BOX (hbox), reset_button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (reset_button), "clicked",
			  G_CALLBACK (reset_clicked), details);

	details->entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (details->entry), "activate", 
			  G_CALLBACK (entry_activated), details);
	details->hid = g_signal_connect (G_OBJECT (details->entry), "changed", 
			  G_CALLBACK (entry_text_changed), details);
	gtk_box_pack_start (GTK_BOX (hbox), details->entry, TRUE, TRUE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	details->da = gtk_drawing_area_new ();
	gtk_widget_set_size_request (details->da, 48, 16);
	gtk_container_add (GTK_CONTAINER (frame), details->da);

	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(dialog)), vbox, TRUE, TRUE, 0);

	gtk_window_set_default_size (GTK_WINDOW (dialog), 640, 480);

	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), NULL);

	gtk_window_set_position (GTK_WINDOW (dialog),
				 GTK_WIN_POS_CENTER);

	g_signal_connect (G_OBJECT (dialog), "close",
			  G_CALLBACK (emit_cancel_cb), NULL);
	g_signal_connect (G_OBJECT (dialog), "delete_event",
			  G_CALLBACK (emit_cancel_cb), NULL);
	g_signal_connect (G_OBJECT (details->da), "draw",
			  G_CALLBACK (show_color), details);
}

GtkWidget *
gnome_label_builder_new (char *msg, blevel_t *upper, blevel_t* lower, int mode)
{
	return g_object_new (GNOME_TYPE_LABEL_BUILDER, "message", msg,
			     "mode", mode,
			     "lower", lower,
			     "upper", upper, NULL);

}
