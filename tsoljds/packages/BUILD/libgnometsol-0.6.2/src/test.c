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
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/ctfs.h>
#include <sys/utsname.h>
#include <stropts.h>

#include <strings.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include "label_builder.h"
#include <userattr.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>

char           *label = NULL;

static blrange_t *
get_display_range ()
{
	blrange_t      *range;

	range = g_new0 (blrange_t, 1);
	range->lower_bound = blabel_alloc ();
	range->upper_bound = blabel_alloc ();

	bsllow (range->lower_bound);
	bslhigh (range->upper_bound);

	return (range);
}

static void
lbuilder_response_cb (GtkDialog * dialog, gint id)
{
	GnomeLabelBuilder *lbuilder = GNOME_LABEL_BUILDER (dialog);
	bslabel_t      *sl;

	switch (id) {
	case GTK_RESPONSE_OK:
		g_object_get (G_OBJECT (lbuilder), "sl", &sl, NULL);
		label = g_strdup (bcleartoh (sl));
		printf ("Selected label: %s\n", label);
		gtk_widget_destroy (GTK_WIDGET (lbuilder));
		break;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (lbuilder));
		break;
	case GTK_RESPONSE_HELP:
	default:
		break;
	}

	return;
}

void
button_clicked_cb (GtkWidget * widget, gpointer data)
{
	GtkWidget      *lbuilder;
	bslabel_t       sl;
	int             err;
	blrange_t      *range = get_display_range ();

	lbuilder = gnome_label_builder_new ("TESTING...",
					    range->upper_bound,
					    range->lower_bound, 1);

	g_signal_connect (G_OBJECT (lbuilder), "response",
			  G_CALLBACK (lbuilder_response_cb), NULL);

	gtk_widget_show_all (GTK_WIDGET (lbuilder));

	if (label) {
		htobsl (label, &sl);
		g_object_set (G_OBJECT (lbuilder), "sl", &sl, NULL);
	}
}

GtkWidget      *
test_window_new ()
{
	GtkWidget      *window, *button;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Label Builder test");

	button = gtk_button_new_with_label ("Show Label Builder");

	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (button_clicked_cb), NULL);

	gtk_container_add (GTK_CONTAINER (window), button);

	return window;
}

int
main (int argc, char *argv[])
{
	GtkWidget      *window;

	gtk_init (&argc, &argv);

	window = test_window_new ();

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
