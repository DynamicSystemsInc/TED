/*
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgnometsol/constraint-scaling.h>

#include "ui-view.h"
#include "menus.h"
#include "pics.h"
#include "tsol-user.h"
#include "ui-controller.h"
#include "xutils.h"
#include <X11/extensions/Xtsol.h>

static GtkWidget *current_focus = NULL;

gboolean 
ws_label_stripe_expose_event (GtkWidget * widget,
			      cairo_t * cr,
			      gpointer data)
{
	int             text_height, pango_height, pango_width, image_y;
	GdkRectangle    area;
	ConstraintImage *cimage = NULL;
	PangoLayout    *pango_layout;
	GdkRGBA		*label_color;
	GdkRGBA		text_color;
	TrustedStripe  *stripe = (TrustedStripe *) data;
	char		*label_name;

	gtk_widget_get_allocation(widget, &area);

	/* constraint image beautifier (more padding) */
	area.x = 0;
	area.y = 1;

	label_name = current_workspace_label_get_name (widget);
	label_color = current_workspace_label_get_color (widget);
	if (label_color) {
		cimage = workspace_label_stripe_get (widget,
				  label_name, label_color);
	}
	/* draw highlight */

	if (cimage) {
		gnome_tsol_constraint_image_render (cr, cimage,
		    gtk_widget_get_window(widget),
		    &area,
		    FALSE,
		    area.x,
		    area.y,
		    area.width,
		    area.height);
	} else {
		return FALSE;
	}

	/* draw icon */
	image_y = (((area.height + 2) / 2) - (gdk_pixbuf_get_height (stripe->workspace_icon) / 2)) - 1;

	gdk_cairo_set_source_pixbuf(cr, stripe->workspace_icon, 4, image_y);
	cairo_paint(cr);

	/* draw text */

	pango_layout = gtk_widget_create_pango_layout (widget, label_name);
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);

	area.x += gdk_pixbuf_get_width (stripe->workspace_icon) + 4;
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;
        if (label_layout_should_be_black (label_color)) {
                gdk_rgba_parse(&text_color, "black");
        } else {
                gdk_rgba_parse(&text_color, "white");
        }
        cairo_set_source_rgb(cr, text_color.red, text_color.green, text_color.blue);
        /* draw the label */
        cairo_move_to(cr, area.x + 8, area.y + 2 + (text_height / 2));
        pango_cairo_show_layout(cr, pango_layout);

	g_object_unref (pango_layout);
	g_free(label_name);
}

gboolean 
window_label_stripe_expose_event (GtkWidget * widget,
				  cairo_t *cr,
				  gpointer data)
{
	int             text_height, pango_height, pango_width, icon_y;
	GdkRectangle    area;
	ConstraintImage *cimage = NULL;
	PangoLayout    *pango_layout;
	GdkRGBA		*label_color;
	GdkRGBA		text_color;
	GdkPixbuf      *icon;
	gboolean        popup = GPOINTER_TO_INT (data);
	char		*label_name;

	gtk_widget_get_allocation(widget, &area);

	/* constraint image beautifier (more padding) */
	area.x = 0;
	if (!popup) {
		area.y = 1;
		area.height -2;
	} else {
		area.y = 1;
		area.height -= 2;
	}

	label_color = current_window_label_get_color ();
	label_name = current_window_label_get_name();
	cimage = window_label_stripe_get (widget, label_name, label_color);

	/* draw highlight */

	if (cimage) {
		gnome_tsol_constraint_image_render (cr, cimage,
		    gtk_widget_get_window(widget),
		    &area,
		    FALSE,
		    area.x,
		    area.y,
		    area.width,
		    area.height);
		area.x -= cimage->border_left;
	} else
		return FALSE;

	/* draw icon if any */
	// if current window is trusted stripe, skip icon..
	//if XmuClientWindow(xdisplay, child...
	if (global_role_dialog_is_mapped)
		icon = NULL;
	else
		icon = current_window_icon_get ();

	if (widget == current_focus)
		icon = NULL;

	if (icon) {
		int icon_height = gdk_pixbuf_get_height(icon);
		icon_y = (((area.height + 2) / 2) - icon_height / 2);

		if (popup)
			icon_y++;

		if (icon_height > 0) {
			gdk_cairo_set_source_pixbuf(cr, icon, 4, icon_y);
			cairo_paint(cr);
			area.x += gdk_pixbuf_get_width (icon) + 6;
		}
	}

	/* draw text */
        /* draw the label */

	pango_layout = gtk_widget_create_pango_layout (widget,
					  label_name);
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

        /* draw the label */
        /* set the correct source color */
        if (label_layout_should_be_black (label_color)) {
                gdk_rgba_parse(&text_color, "black");
        } else {
                gdk_rgba_parse(&text_color, "white");
        }
        cairo_set_source_rgb(cr, text_color.red, text_color.green, text_color.blue);
        cairo_move_to(cr, area.x + 10, area.y + 2 + (text_height / 2));
        pango_cairo_show_layout(cr, pango_layout);
	g_object_unref (pango_layout);
}

gboolean 
role_label_stripe_expose_event (GtkWidget * widget,
				cairo_t *cr,
				gpointer data)
{
	int             image_x, image_y;
	int		text_height, text_width;
	int		pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	TrustedStripe  *stripe = (TrustedStripe *) data;
	GdkRGBA		label_color;

	gtk_widget_get_allocation(widget, &area);
	image_x = (area.width / 2) - (gdk_pixbuf_get_width (stripe->role_hat) / 2);
	image_y = (area.height / 2) - (gdk_pixbuf_get_height (stripe->role_hat) / 2);

	gdk_cairo_set_source_pixbuf(cr, stripe->role_hat, 4, image_y);
	cairo_paint(cr);

	pango_layout = gtk_widget_create_pango_layout (widget, current_role_name (widget));
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);
	text_width = PANGO_PIXELS (pango_width);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	gtk_widget_set_size_request (stripe->role_da,
		text_width + 35, area.height);
	
	gdk_rgba_parse(&label_color, "black");
        cairo_set_source_rgb(cr, label_color.red, label_color.green, label_color.blue);

        cairo_set_source_rgb(cr, label_color.red, label_color.green, label_color.blue);
        cairo_move_to(cr, 25, area.y + 2 + (text_height / 2));
        pango_cairo_show_layout(cr, pango_layout);
	g_object_unref (pango_layout);
	return FALSE;
}

gboolean 
trusted_path_stripe_expose_event (GtkWidget * widget,
				  cairo_t *cr,
				  gpointer data)
{
	int             text_height, pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	TrustedStripe  *stripe = (TrustedStripe *) data;
	GdkRGBA		color;

	gtk_widget_get_allocation(widget, &area);

	if (global_role_dialog_is_mapped || stripe->show_shield) {
		gdk_cairo_set_source_pixbuf(cr, stripe->shield, area.x, area.y);
	} else {
		gdk_cairo_set_source_pixbuf(cr, stripe->no_shield, area.x, area.y);
	}
	cairo_paint(cr);

	pango_layout = gtk_widget_create_pango_layout (widget, "Trusted Path");
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	gdk_rgba_parse(&color, "black");
        cairo_set_source_rgb(cr, color.red, color.green, color.blue);

        cairo_move_to(cr, area.x + 38, area.y + 2 + (text_height / 2));
        pango_cairo_show_layout(cr, pango_layout);
	g_object_unref (pango_layout);
}

gboolean 
frame_expose_event (GtkWidget * widget,
		    cairo_t *cr,
		    gpointer data)
{
	GdkRectangle    area;
	GtkStyleContext *style_gtk;
	GdkRGBA		color;
	GtkStateFlags	state;

	style_gtk = gtk_widget_get_style_context(widget);
	state = GTK_STATE_NORMAL;
	gtk_style_context_get_color(style_gtk, state, &color);

	cairo_set_source_rgb(cr, color.red, color.green, color.blue);
	cairo_set_line_width(cr, 0.5);
	
	gtk_widget_get_allocation(widget, &area);
	cairo_move_to(cr, 0, area.height - 2);
	cairo_line_to(cr, area.width, area.height - 2);
	cairo_stroke(cr);

	gdk_rgba_parse(&color, "black");
	cairo_move_to(cr, 0, area.height - 1);
	cairo_line_to(cr, area.width, area.height - 1);
	cairo_stroke(cr);

	/* return FALSE; */
}


static gboolean 
enter_notify (GtkWidget * widget,
	      GdkEventCrossing * event,
	      gpointer data)
{
	current_focus = widget;
	gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_PRELIGHT, TRUE);
	return FALSE;
}
static gboolean 
leave_notify (GtkWidget * widget,
	      GdkEventCrossing * event,
	      gpointer data)
{
	current_focus = NULL;
	gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_NORMAL, TRUE);
	return FALSE;
}
static gboolean 
button_press (GtkWidget * widget,
	      GdkEvent * event,
	      gpointer data)
{
	gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_ACTIVE, TRUE);
	return FALSE;
}

static void
setup_custom_button_events (GtkWidget * widget, gpointer callback, gpointer data)
{
	gtk_widget_add_events (widget, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK);

	g_signal_connect (G_OBJECT (widget), "enter_notify_event",
			  G_CALLBACK (enter_notify),
			  NULL);
	g_signal_connect (G_OBJECT (widget), "leave_notify_event",
			  G_CALLBACK (leave_notify),
			  NULL);
	g_signal_connect (G_OBJECT (widget), "button_press_event",
			  G_CALLBACK (button_press),
			  NULL);
	g_signal_connect (G_OBJECT (widget), "draw",
			  G_CALLBACK (callback),
			  data);
}

static GtkWidget *
create_window_stripe_button (int height, TrustedStripe * tstripe)
{
	GtkWidget      *da = gtk_drawing_area_new ();

	gtk_widget_set_size_request (da, -1, height - 4);
	setup_custom_button_events (da, (gpointer) window_label_stripe_expose_event, NULL);

	tstripe->window_da = da;
	return da;
}
static GtkWidget *
create_workspace_stripe_button (int height, TrustedStripe * tstripe)
{
	int             scaled_w, image_height;
	GdkPixbuf      *ws_scaled;
	GtkWidget      *da = gtk_drawing_area_new ();
	GdkPixbuf      *ws_pb = gdk_pixbuf_new_from_inline (-1, workspace_icon, FALSE, NULL);

	image_height = height + 6;

	scaled_w = (gdk_pixbuf_get_width (ws_pb) * image_height) / gdk_pixbuf_get_height (ws_pb);

	ws_scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				    TRUE,
				    8, scaled_w,
				    image_height);

	gdk_pixbuf_scale (ws_pb, ws_scaled, 0, 0,
			  scaled_w, image_height, 0, 0,
		      (double) image_height / gdk_pixbuf_get_height (ws_pb),
		      (double) image_height / gdk_pixbuf_get_height (ws_pb),
			  GDK_INTERP_HYPER);

	gtk_widget_set_size_request (da, 116, height - 4);

	setup_custom_button_events (da, (gpointer) ws_label_stripe_expose_event, tstripe);

	tstripe->workspace_da = da;
	tstripe->workspace_icon = ws_scaled;

	return da;
}

static GtkWidget *
create_role_stripe_button (int height, TrustedStripe * tstripe)
{
	int             scaled_w, image_height;
	GdkPixbuf      *rolehat_scaled;
	GtkWidget      *da = gtk_drawing_area_new ();
	GdkPixbuf      *rolehat_pb = gdk_pixbuf_new_from_inline (-1, rolehat, FALSE, NULL);

	image_height = height + 10;

	scaled_w = (gdk_pixbuf_get_width (rolehat_pb) * image_height) / gdk_pixbuf_get_height (rolehat_pb);

	rolehat_scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					 TRUE,
					 8, scaled_w,
					 image_height);

	gdk_pixbuf_scale (rolehat_pb, rolehat_scaled, 0, 0,
			  scaled_w, image_height, 0, 0,
		 (double) image_height / gdk_pixbuf_get_height (rolehat_pb),
		 (double) image_height / gdk_pixbuf_get_height (rolehat_pb),
			  GDK_INTERP_HYPER);

	gtk_widget_set_size_request (da, 130, height - 4);

	setup_custom_button_events (da, (gpointer) role_label_stripe_expose_event, tstripe);

	tstripe->role_da = da;
	tstripe->role_hat = rolehat_scaled;

	return da;
}

static GtkWidget *
create_trusted_path_stripe_button (int height, TrustedStripe * tstripe)
{
	PangoLayout    *pango_layout;
	int             pango_height, pango_width, scaled_w, image_height;
	GdkPixbuf      *shield_scaled, *no_shield_scaled;
	GdkPixbuf      *shield_pb = gdk_pixbuf_new_from_inline (-1, shield, FALSE, NULL);
	GdkPixbuf      *no_shield_pb = gdk_pixbuf_new_from_inline (-1, no_shield, FALSE, NULL);;
	GtkWidget      *da = gtk_drawing_area_new ();

	image_height = height - 2;
	pango_layout = gtk_widget_create_pango_layout (da, "Trusted Path");
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);

	scaled_w = (gdk_pixbuf_get_width (shield_pb) * image_height) / gdk_pixbuf_get_height (shield_pb);

	shield_scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					TRUE,
					8, scaled_w,
					image_height);

	no_shield_scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
					   TRUE,
					   8, scaled_w,
					   image_height);

	gdk_pixbuf_scale (shield_pb, shield_scaled, 0, 0,
			  scaled_w, image_height, 0, 0,
		  (double) image_height / gdk_pixbuf_get_height (shield_pb),
		  (double) image_height / gdk_pixbuf_get_height (shield_pb),
			  GDK_INTERP_HYPER);

	gdk_pixbuf_scale (no_shield_pb, no_shield_scaled, 0, 0,
			  scaled_w, image_height, 0, 0,
	       (double) image_height / gdk_pixbuf_get_height (no_shield_pb),
	       (double) image_height / gdk_pixbuf_get_height (no_shield_pb),
			  GDK_INTERP_HYPER);

	gtk_widget_set_size_request (da,
			     PANGO_PIXELS (pango_width) + gdk_pixbuf_get_width (shield_scaled) + 8, -1);

	setup_custom_button_events (da, (gpointer) trusted_path_stripe_expose_event, (gpointer) tstripe);

	tstripe->shield = shield_scaled;
	tstripe->no_shield = no_shield_scaled;
	tstripe->show_shield = TRUE;
	tstripe->trusted_path_da = da;

	return da;
}

static GtkWidget *
create_frame_stripe_button (int height, TrustedStripe * tstripe)
{
	GtkWidget      *da = gtk_drawing_area_new ();

	gtk_widget_set_size_request (da, -1, 2);

	setup_custom_button_events (da, (gpointer) frame_expose_event, NULL);

	return da;
}

static void 
create_query_window_label_popup (int height, TrustedStripe * tstripe)
{
	GtkWidget      *frame;

	tstripe->query_window_label = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_screen (GTK_WINDOW (tstripe->query_window_label),
			  gtk_widget_get_screen (tstripe->trusted_path_da));
	gtk_window_set_position (GTK_WINDOW (tstripe->query_window_label), GTK_WIN_POS_CENTER_ALWAYS);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	tstripe->query_window_label_da = gtk_label_new ("");
	gtk_container_add (GTK_CONTAINER (tstripe->query_window_label),
			   frame);
	gtk_container_add (GTK_CONTAINER (frame),
			   tstripe->query_window_label_da);

	gtk_window_resize (GTK_WINDOW (tstripe->query_window_label), 200, height);

	g_signal_connect (G_OBJECT (tstripe->query_window_label_da), "draw",
			  G_CALLBACK (window_label_stripe_expose_event),
			  tstripe->query_window_label_da);
}

#define STRIPE_AT_BOTTOM_KEY "/desktop/gnome/trusted_extensions/stripe_at_bottom"

TrustedStripe  *
create_trusted_stripe (GdkScreen * screen)
{
	GtkWidget      *stripe, *window_stripe, *ws_stripe, *tp;
	GtkWidget      *vbox, *dumb_label, *hbox;
	GtkWidget      *placeholder;
	GdkRectangle    area;
	int             image_height;
	GtkRequisition  minimum_size;
	GtkRequisition  natural_size;
	TrustedStripe  *tstripe;
	static int	stripeattop = -1;

	tstripe = g_new (TrustedStripe, 1);
	dumb_label = gtk_label_new ("Blah");
	gtk_widget_get_preferred_size (dumb_label, &minimum_size, &natural_size);

	XTSOLgetSSHeight (GDK_DISPLAY_XDISPLAY (gdk_display_get_default()), gdk_screen_get_number (screen), &image_height);
	if (image_height == 0) {
		stripeattop = 1;
	} else {
		stripeattop = 0;
	}
	image_height = 28;
	natural_size.height = 9;

	stripe = gtk_window_new (GTK_WINDOW_POPUP);
	tstripe->toplevel = stripe;
	placeholder = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_decorated (GTK_WINDOW (stripe), FALSE);
	gtk_window_set_decorated (GTK_WINDOW (placeholder), FALSE);

	gtk_window_stick (GTK_WINDOW (stripe));
	gtk_window_stick (GTK_WINDOW (placeholder));

	gtk_window_set_screen (GTK_WINDOW (stripe), screen);
	gtk_window_set_screen (GTK_WINDOW (placeholder), screen);

	gtk_window_resize (GTK_WINDOW (stripe), gdk_screen_get_width (screen), image_height);
//	gtk_window_resize (GTK_WINDOW (placeholder), gdk_screen_get_width (screen), image_height);

	if (stripeattop) {
		gtk_window_move (GTK_WINDOW (stripe), 0, 0);
		//gtk_window_move (GTK_WINDOW (placeholder), 0, 0);
	} else {
		gtk_window_move (GTK_WINDOW (stripe), 0, 
				 gdk_screen_get_height (screen) - image_height);
	}
	area.x = 0;
	area.y = gdk_screen_get_height(screen) - image_height;
	area.height = image_height;
	area.width = gdk_screen_get_width(screen);

	gtk_widget_show(stripe);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (stripe), vbox);

	/* Trusted Path custom Button */

	hbox = gtk_hbox_new (FALSE, 2);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	tp = create_trusted_path_stripe_button (image_height, tstripe);

	gtk_box_pack_start (GTK_BOX (hbox), tp, FALSE, FALSE, 0);

	/* bottom pseudo frame */

	gtk_box_pack_start (GTK_BOX (vbox), create_frame_stripe_button (0, tstripe), FALSE, FALSE, 0);

	/* Role custom button */
	if (_tstripe_user_count_get () > 1) {
		tstripe->role_da = create_role_stripe_button (natural_size.height, tstripe);

		gtk_box_pack_start (GTK_BOX (hbox), tstripe->role_da, FALSE, FALSE, 0);
	}
	/* workspace custom button */

	ws_stripe = create_workspace_stripe_button (natural_size.height, tstripe);

	gtk_box_pack_start (GTK_BOX (hbox), ws_stripe, FALSE, FALSE, 0);

	/* window custom button */

	window_stripe = create_window_stripe_button (natural_size.height, tstripe);

	gtk_box_pack_start (GTK_BOX (hbox), window_stripe, TRUE, TRUE, 0);

	/* show the placeholder for the metacity-panel strut problem */
	if (stripeattop) {
		//gtk_widget_show_all (placeholder);
		gtk_window_set_keep_below ((GtkWindow *)placeholder,
			TRUE);
		_tstripe_window_strut_set (gtk_widget_get_window(placeholder),
		     image_height, 0,
		     gdk_screen_get_width (screen));
	}

	/* now show the real stripe */
	gtk_window_set_title (GTK_WINDOW (stripe), "Trusted Stripe");
	gtk_widget_show_all (stripe);

	/* make sure the windows are in the right order */

	gtk_window_set_keep_above ((GtkWindow *)stripe, TRUE);
	gdk_window_set_override_redirect (gtk_widget_get_window(stripe), TRUE);
	if (stripeattop)
		gdk_window_lower (gtk_widget_get_window(placeholder));
	gdk_window_raise (gtk_widget_get_window(stripe));
	gdk_display_flush (gtk_widget_get_display (stripe));

	XTSOLMakeTPWindow (GDK_WINDOW_XDISPLAY (gtk_widget_get_window(stripe)),
		GDK_WINDOW_XID (gtk_widget_get_window(stripe)));

	create_query_window_label_popup (image_height, tstripe);

	return tstripe;
}
