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

#include <gtk/gtk.h>
#include "constraint-scaling.h"


ConstraintImage *cimage;

GdkColor       *
get_label_color ()
{
	char           *tmp = g_strdup_printf ("#%x%x%x", g_random_int_range (0, 16), g_random_int_range (0, 16), g_random_int_range (0, 16));

	printf ("random color is %s\n", tmp);
	GdkColor       *color = g_new0 (GdkColor, 1);

	gdk_color_parse ((const char *) tmp, color);
	g_free (tmp);
	return color;
}

gboolean 
my_expose_event (GtkWidget * widget,
		 GdkEventExpose * event,
		 gpointer data)
{
	GtkStyle       *style;
	GdkGC          *lgc, *dgc;
	int             x, y;
	GdkRectangle    area;

	style = widget->style;

	lgc = style->light_gc[GTK_STATE_NORMAL];
	dgc = style->dark_gc[GTK_STATE_NORMAL];

	x = widget->allocation.x + widget->allocation.width -
		(style->ythickness + 10);
	y = widget->allocation.y + style->xthickness + 2;

	printf ("my_expose_event (%d,%d) (%d,%d)\n", widget->allocation.x,
		widget->allocation.y, widget->allocation.width,
		widget->allocation.height);

	area.x = widget->allocation.x;
	area.y = widget->allocation.y;
	area.width = widget->allocation.width;
	area.height = widget->allocation.height;

	constraint_render (cimage, widget->window,
			   NULL, &area,
			   COMPONENT_ALL,
			   FALSE,
			   widget->allocation.x,
			   widget->allocation.y,
			   widget->allocation.width,
			   widget->allocation.height);

}

int 
main (int argc, char **argv)
{
	GtkWidget      *widget;

	gtk_init (&argc, &argv);

	cimage = g_new0 (ConstraintImage, 1);

	widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	cimage->pixbuf = gdk_pixbuf_new_from_file ("/home/erwannc/code/tstripe/src/menuitem2-small.png", NULL);

	constraint_set_border (cimage, 3, 0, 2, 2);
	constraint_set_stretch (cimage, TRUE);
	constraint_colorize (cimage, get_label_color (),
			     255, TRUE);

	g_signal_connect (G_OBJECT (widget), "expose_event",
			  G_CALLBACK (my_expose_event),
			  NULL);

	gtk_widget_show_all (widget);

	gtk_main ();
}
