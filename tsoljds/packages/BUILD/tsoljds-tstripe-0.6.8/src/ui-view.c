/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

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

#include <gnome.h>
#include <gconf/gconf-client.h>

#include "ui-view.h"
#include "menus.h"
#include "pics.h"
#include "tsol-user.h"
#include "ui-controller.h"
#include "xutils.h"
#include <X11/extensions/Xtsol.h>

static void 
draw_state (GtkWidget * widget, GdkRectangle * area)
{
	if (GTK_WIDGET_STATE (widget) == GTK_STATE_ACTIVE) {
		gdk_draw_rectangle (widget->window, widget->style->dark_gc[GTK_STATE_ACTIVE],
				    TRUE,
				    area->x,
				    area->y,
				    area->width,
				    area->height);
	}
	if (GTK_WIDGET_STATE (widget) == GTK_STATE_PRELIGHT) {
		gdk_draw_rectangle (widget->window, widget->style->dark_gc[GTK_STATE_PRELIGHT],
				    TRUE,
				    area->x,
				    area->y,
				    area->width,
				    area->height);
	}
	if (GTK_WIDGET_STATE (widget) == GTK_STATE_SELECTED) {
		gdk_draw_rectangle (widget->window, widget->style->dark_gc[GTK_STATE_SELECTED],
				    TRUE,
				    area->x,
				    area->y,
				    area->width,
				    area->height);
	}
}

gboolean 
ws_label_stripe_expose_event (GtkWidget * widget,
			      GdkEventExpose * event,
			      gpointer data)
{
	GdkGC          *gc;
	int             text_height, pango_height, pango_width, image_y;
	GdkRectangle    area;
	ConstraintImage *cimage = NULL;
	PangoLayout    *pango_layout;
	GdkColor       *label_color = NULL;
	TrustedStripe  *stripe = (TrustedStripe *) data;

	area.x = 0;
	area.y = 0;
	area.width = widget->allocation.width;
	area.height = widget->allocation.height;

	draw_state (widget, &area);

	/* constraint image beautifier (more padding) */
	area.y = 1;
	area.height = widget->allocation.height - 2;

	label_color = current_workspace_label_get_color (widget);
	if (label_color) {
		cimage = workspace_label_stripe_get (widget,
				  current_workspace_label_get_name (widget),
						     label_color);
		pango_layout = gtk_widget_create_pango_layout (widget,
				 current_workspace_label_get_name (widget));
	}
	/* draw highlight */

	if (cimage) {
		gnome_tsol_constraint_image_render (cimage, widget->window,
						    NULL, &area,
						    FALSE,
						    area.x,
						    area.y,
						    area.width,
						    area.height);
		area.x += cimage->border_left;
	} else
		return FALSE;

	/* draw icon */
	image_y = ((widget->allocation.height / 2) - (gdk_pixbuf_get_height (stripe->workspace_icon) / 2)) - 1;

	gdk_draw_pixbuf (widget->window,
			 NULL,
			 stripe->workspace_icon,
			 0, 0,
			 area.x, image_y,
			 gdk_pixbuf_get_width (stripe->workspace_icon),
			 gdk_pixbuf_get_height (stripe->workspace_icon),
			 GDK_RGB_DITHER_NONE,
			 0, 0);

	/* draw text */

	pango_layout_get_size (pango_layout, &pango_width, &pango_height);

	area.x += gdk_pixbuf_get_width (stripe->workspace_icon) + 4;
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	if (label_layout_should_be_black (label_color)) {
		gc = widget->style->black_gc;
	} else {
		gc = widget->style->white_gc;
	}

	gdk_draw_layout (widget->window, gc,
			 area.x,
			 area.y + (text_height / 2),
			 pango_layout);

	g_object_unref (pango_layout);
}

gboolean 
window_label_stripe_expose_event (GtkWidget * widget,
				  GdkEventExpose * event,
				  gpointer data)
{
	GdkGC          *gc;
	int             text_height, pango_height, pango_width, icon_y;
	GdkRectangle    area;
	ConstraintImage *cimage = NULL;
	PangoLayout    *pango_layout;
	GdkColor       *label_color = NULL;
	GdkPixbuf      *icon;
	gboolean        popup = GPOINTER_TO_INT (data);

	area.x = 0;
	area.y = 0;
	area.width = widget->allocation.width;
	area.height = widget->allocation.height;

	/* constraint image beautifier (more padding) */
	if (!popup) {
		area.y = 1;
		area.height = widget->allocation.height - 2;
	} else {
		area.y = 1;
		/* area.height = widget->allocation.height - 2; */
	}

	label_color = current_window_label_get_color ();
	cimage = window_label_stripe_get (widget, current_window_label_get_name (),
					  label_color);
	pango_layout = gtk_widget_create_pango_layout (widget,
					  current_window_label_get_name ());

	/* draw highlight */

	if (cimage) {
		gnome_tsol_constraint_image_render (cimage, widget->window,
						    NULL, &area,
						    FALSE,
						    area.x,
						    area.y,
						    area.width,
						    area.height);
		area.x += cimage->border_left;
	} else
		return FALSE;

	/* draw icon if any */
	if (global_role_dialog_is_mapped)
		icon = NULL;
	else
		icon = current_window_icon_get ();

	if (icon) {
		icon_y = ((widget->allocation.height / 2) - (gdk_pixbuf_get_height (icon) / 2));

		if (popup)
			icon_y++;

		gdk_draw_pixbuf (widget->window,
				 NULL,
				 icon,
				 0, 0,
				 area.x, icon_y,
				 gdk_pixbuf_get_width (icon),
				 gdk_pixbuf_get_height (icon),
				 GDK_RGB_DITHER_NONE,
				 0, 0);
		area.x += gdk_pixbuf_get_width (icon) + 4;
	}
	/* draw text */

	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	if (label_layout_should_be_black (label_color)) {
		gc = widget->style->black_gc;
	} else {
		gc = widget->style->white_gc;
	}

	gdk_draw_layout (widget->window, gc,
			 area.x,
			 area.y + (text_height / 2),
			 pango_layout);

	g_object_unref (pango_layout);
}

gboolean 
role_label_stripe_expose_event (GtkWidget * widget,
				GdkEventExpose * event,
				gpointer data)
{
	int             image_y, text_height, pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	TrustedStripe  *stripe = (TrustedStripe *) data;

	area.x = 0;
	area.y = 0;
	area.width = widget->allocation.width;
	area.height = widget->allocation.height;

	draw_state (widget, &area);

	image_y = (widget->allocation.height / 2) - (gdk_pixbuf_get_height (stripe->role_hat) / 2);

	gdk_draw_pixbuf (widget->window,
			 NULL,
			 stripe->role_hat,
			 0, 0,
			 area.x, image_y,
			 gdk_pixbuf_get_width (stripe->role_hat),
			 gdk_pixbuf_get_height (stripe->role_hat),
			 GDK_RGB_DITHER_NONE,
			 0, 0);


	pango_layout = gtk_widget_create_pango_layout (widget, current_role_name (widget));
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	gdk_draw_layout (widget->window, widget->style->black_gc,
		       area.x + 2 + gdk_pixbuf_get_width (stripe->role_hat),
			 area.y + (text_height / 2),
			 pango_layout);

	g_object_unref (pango_layout);
}

gboolean 
trusted_path_stripe_expose_event (GtkWidget * widget,
				  GdkEventExpose * event,
				  gpointer data)
{
	int             text_height, pango_height, pango_width;
	GdkRectangle    area;
	PangoLayout    *pango_layout;
	TrustedStripe  *stripe = (TrustedStripe *) data;

	area.x = 0;
	area.y = 0;
	area.width = widget->allocation.width;
	area.height = widget->allocation.height;

	draw_state (widget, &area);

	if (global_role_dialog_is_mapped || stripe->show_shield) {
		gdk_draw_pixbuf (widget->window,
				 NULL,
				 stripe->shield,
				 0, 0,
				 area.x, area.y,
				 gdk_pixbuf_get_width (stripe->shield),
				 gdk_pixbuf_get_height (stripe->shield),
				 GDK_RGB_DITHER_NONE,
				 0, 0);
	} else {
		gdk_draw_pixbuf (widget->window,
				 NULL,
				 stripe->no_shield,
				 0, 0,
				 area.x, area.y,
				 gdk_pixbuf_get_width (stripe->no_shield),
				 gdk_pixbuf_get_height (stripe->no_shield),
				 GDK_RGB_DITHER_NONE,
				 0, 0);
	}

	pango_layout = gtk_widget_create_pango_layout (widget, _("Trusted Path"));
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
	text_height = PANGO_PIXELS (pango_height);

	text_height = area.height - text_height;
	if (text_height < 0)
		text_height = 0;

	gdk_draw_layout (widget->window, widget->style->black_gc,
			 area.x + gdk_pixbuf_get_width (stripe->shield),
			 area.y + (text_height / 2),
			 pango_layout);

	g_object_unref (pango_layout);
}

gboolean 
frame_expose_event (GtkWidget * widget,
		    GdkEventExpose * event,
		    gpointer data)
{
	gdk_draw_line (widget->window, widget->style->dark_gc[GTK_STATE_NORMAL],
		       0,
		       widget->allocation.height - 2,
		       widget->allocation.width,
		       widget->allocation.height - 2);

	gdk_draw_line (widget->window, widget->style->black_gc,
		       0,
		       widget->allocation.height - 1,
		       widget->allocation.width,
		       widget->allocation.height - 1);
	/* return FALSE; */
}


static gboolean 
enter_notify (GtkWidget * widget,
	      GdkEventCrossing * event,
	      gpointer data)
{
	gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);
	return FALSE;
}
static gboolean 
leave_notify (GtkWidget * widget,
	      GdkEventCrossing * event,
	      gpointer data)
{
	gtk_widget_set_state (widget, GTK_STATE_NORMAL);
	return FALSE;
}
static gboolean 
button_press (GtkWidget * widget,
	      GdkEvent * event,
	      gpointer data)
{
	gtk_widget_set_state (widget, GTK_STATE_ACTIVE);
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
	g_signal_connect (G_OBJECT (widget), "expose_event",
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

	image_height = height + 2;

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

	gtk_widget_set_size_request (da, 100, height - 4);

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

	image_height = height + 2;

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

	gtk_widget_set_size_request (da, 100, image_height - 4);

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
	pango_layout = gtk_widget_create_pango_layout (da, _("Trusted Path"));
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
				     PANGO_PIXELS (pango_width) + gdk_pixbuf_get_width (shield_scaled) + 2,
				     -1);

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

	g_signal_connect (G_OBJECT (tstripe->query_window_label_da), "expose_event",
			  G_CALLBACK (window_label_stripe_expose_event),
			  GINT_TO_POINTER(TRUE));
}

#define STRIPE_AT_BOTTOM_KEY "/desktop/gnome/trusted_extensions/stripe_at_bottom"

TrustedStripe  *
create_trusted_stripe (GdkScreen * screen)
{
	GtkWidget      *stripe, *window_stripe, *ws_stripe, *tp;
	GtkWidget      *vbox, *dumb_label, *hbox;
	GtkWidget      *placeholder;

	int             image_height;
	GtkRequisition  req;
	TrustedStripe  *tstripe;
	static int	stripeattop = -1;

	if (stripeattop == -1) {
		stripeattop = !gconf_client_get_bool(gconf_client_get_default(),
					              STRIPE_AT_BOTTOM_KEY,
						      NULL);
	}

	tstripe = g_new (TrustedStripe, 1);

	dumb_label = gtk_label_new ("Blah");
	gtk_widget_size_request (dumb_label, &req);

	if (stripeattop) {
		image_height = req.height + 14;
	} else {
		XTSOLgetSSHeight (GDK_DISPLAY_XDISPLAY (gdk_display_get_default()), gdk_screen_get_number (screen), &image_height);
	}

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
	gtk_window_resize (GTK_WINDOW (placeholder), gdk_screen_get_width (screen), image_height);

	if (stripeattop) {
		gtk_window_move (GTK_WINDOW (stripe), 0, 0);
		gtk_window_move (GTK_WINDOW (placeholder), 0, 0);
	} else {
		gtk_window_move (GTK_WINDOW (stripe), 0, 
				 gdk_screen_get_height (screen));
	}

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
		tstripe->role_da = create_role_stripe_button (req.height, tstripe);

		gtk_box_pack_start (GTK_BOX (hbox), tstripe->role_da, FALSE, FALSE, 0);
	}
	/* workspace custom button */

	ws_stripe = create_workspace_stripe_button (req.height, tstripe);

	gtk_box_pack_start (GTK_BOX (hbox), ws_stripe, FALSE, FALSE, 0);

	/* window custom button */

	window_stripe = create_window_stripe_button (req.height, tstripe);

	gtk_box_pack_start (GTK_BOX (hbox), window_stripe, TRUE, TRUE, 0);

	/* show the placeholder for the metacity-panel strut problem */
	if (stripeattop) {
		gtk_widget_show_all (placeholder);
		gtk_window_set_keep_below (GTK_WINDOW (placeholder), TRUE);
		_tstripe_window_strut_set (placeholder->window, image_height, 0,
					   gdk_screen_get_width (screen));
	}

	/* now show the real stripe */
	gtk_window_set_title (GTK_WINDOW (stripe), "Trusted Stripe");
	gtk_widget_show_all (stripe);

	/* make sure the windows are in the right order */

	gtk_window_set_keep_above (GTK_WINDOW (stripe), TRUE);
	gdk_window_set_override_redirect (stripe->window, TRUE);
	if (stripeattop) gdk_window_lower (GDK_WINDOW (placeholder->window));
	gdk_window_raise (GDK_WINDOW (stripe->window));
	gdk_display_flush (gtk_widget_get_display (stripe));

	XTSOLMakeTPWindow (GDK_WINDOW_XDISPLAY (stripe->window), GDK_WINDOW_XWINDOW (stripe->window));

	create_query_window_label_popup (image_height, tstripe);

	return tstripe;
}
