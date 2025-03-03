/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2009 Sun Microsystems, Inc. All Rights Reserved.
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
#include <gtk/gtk.h>
#include <stdlib.h>
#include <strings.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include "label_builder.h"

int
main (int argc, char *argv[])
{
	GtkWidget *lbuilder;
	GOptionContext *context;
	m_label_t *lower = NULL, *upper = NULL, *def = NULL, *label = NULL;
	int label_type;
	int err;
	uint_t flags;
	uint_t format_flags;
	GError *error = NULL;
	char *min = NULL, *max = NULL, *defstr = NULL, *accredcheck = NULL;
	char *modestr = NULL, *format = NULL, *title = NULL, *text=NULL;
	char *labelstr;
	int mode, checkar;

	const GOptionEntry options [] = {
		{ "min", 'n', 0, G_OPTION_ARG_STRING, &min, 
		  "Minimim Label (Required)", "label string" },
		{ "max", 'x', 0, G_OPTION_ARG_STRING, &max, 
		  "Maximum Label (Required)", "label string" },
		{ "default", 'd', 0, G_OPTION_ARG_STRING, &defstr, 
		  "Default Selected Label", "label string" },
		{ "accredcheck", 'a', 0, G_OPTION_ARG_STRING, &accredcheck, 
		  "Enable Accreditation checking", "[yes|no]" },
		{ "mode", 'm', 0, G_OPTION_ARG_STRING, &modestr,
		  "Clearance or Sensitivity mode", "[clearance|sensitivity]" },
		{ "format", 'f', 0, G_OPTION_ARG_STRING, &format,
		  "Internal or Human Readable format", "[internal|human]" },
		{ "title", 'i', 0, G_OPTION_ARG_STRING, &title, 
		  "Window title", "title" },
		{ "text", 't', 0, G_OPTION_ARG_STRING, &text, 
		  "Operation description", "text" },
		{ NULL }
	};

	if (!is_system_labeled ()) {
		g_printerr ("This command is only available if the system is "
			    "configured with Trusted Extensions\n");
		return 3;
	}

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("Could not parse arguments: %s\n", error->message);
		g_error_free (error);
		return 1;
	}

	if (!min || !max) {
		g_printerr ("You must specify both min and max labels\n\n%s", 
			 g_option_context_get_help (context, TRUE, NULL));
		return 1;
	}

	g_option_context_free (context);

	if (!defstr) {
		defstr = g_strdup (max);
	}

	if (modestr && strncmp (modestr, "clearance", 9) == 0) {
		mode = LBUILD_MODE_CLR;
		label_type = USER_CLEAR;
	} else {
		mode = LBUILD_MODE_SL;
		label_type = MAC_LABEL;
	}

	if (format && strncmp (format, "human", 8) == 0) {
		format_flags = M_LABEL;
	} else {
		format_flags = M_INTERNAL;
	}

	if (accredcheck && strncmp (accredcheck, "yes", 3) == 0) {
		flags = L_DEFAULT | L_CHECK_AR;
	} else {
		flags = L_DEFAULT;
	}

	if (str_to_label (min, &lower, label_type, flags, &err) < 0) {
		g_printerr ("%s: Can not convert minimum label\n", argv[0]);
		if (err == M_OUTSIDE_AR) {
			g_printerr ("Label is outside the accreditation range");
			return (4);
		}
		return (2);
	}

	if (str_to_label (max, &upper, label_type, flags, &err) < 0) {
		g_printerr ("%s: Can not convert maximum label\n", argv[0]);
		if (err == M_OUTSIDE_AR) {
			g_printerr ("Label is outside the accreditation range");
			return (4);
		}
		return (2);
	}

	if (str_to_label (defstr, &def, label_type, flags, &err)<0){
		g_printerr ("%s: Can not convert default label\n", argv[0]);
		if (err == M_OUTSIDE_AR) {
			g_printerr ("Label is outside the accreditation range");
			return (4);
		}
		return (2);
	}

	gtk_init (&argc, &argv);

	lbuilder = g_object_new (GNOME_TYPE_LABEL_BUILDER, "mode", mode,
				 "message", text ? text : "Select a Label",
				 "lower", lower, "upper", upper, NULL);

	gtk_window_set_title (GTK_WINDOW (lbuilder), 
			      title ? title : "Label Selector");

	gtk_widget_show_all (lbuilder);

	g_object_set (G_OBJECT (lbuilder), "sl", def, NULL);

	switch (gtk_dialog_run (GTK_DIALOG (lbuilder))) {
	case GTK_RESPONSE_OK:
		g_object_get (G_OBJECT (lbuilder), "sl", &label, NULL);
		if (label_to_str (label, &labelstr, format_flags, DEF_NAMES<0)){
			g_printerr ("%s: Can not convert selected label",
				    argv[0]);
			exit (2);
		}
		g_print ("%s\n", labelstr);
		gtk_widget_destroy (GTK_WIDGET (lbuilder));
		break;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (lbuilder));
		exit (5);
		break;
	case GTK_RESPONSE_HELP:
	default:
		break;
	}

	return 0;
}
