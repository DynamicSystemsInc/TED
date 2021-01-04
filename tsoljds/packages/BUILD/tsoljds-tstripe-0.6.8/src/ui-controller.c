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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgnometsol/constraint-scaling.h>
#define  WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include "ui-view.h"
#include "pics.h"
#include "ui-controller.h"
#include "xutils.h"
#include <sys/tsol/label_macro.h>
#include <X11/extensions/Xtsol.h>
#include <tsol/label.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libgnometsol/userattr.h>
#include <libgnometsol/pam_conv.h>
#include <X11/Xmu/WinUtil.h>
#include "menus.h"
#include "tsol-user.h"
#ifdef HAVE_RANDR
#include <X11/extensions/Xrandr.h>
#endif

static Atom     net_trusted_active_window;
static Atom     XA_SCREENSAVER_STATUS;
static GSList  *stripes;
static Window   current_xwin;
static gboolean query_windows_visible = FALSE;

static GdkFilterReturn filter_func (GdkXEvent * gdkxevent,
	     GdkEvent * event,
	     gpointer data);

static void update_all_stripes ();

gboolean 
label_layout_should_be_black (GdkRGBA * color)
{
	double             ntsc;

	if (!color)
		return FALSE;

	ntsc = ((color->red) * .4450 +
		(color->blue) * .030 +
		(color->green) * .525);

	if ((1.0 - ntsc) < .61)
		return TRUE;
	return FALSE;
}

typedef struct _CacheStripe CacheStripe;

struct _CacheStripe {
	ConstraintImage *image;
	char           *name;
};

static          gint
label_string_compare (CacheStripe * tmp, char *searched_label)
{
	return strcmp (searched_label, tmp->name);
}

ConstraintImage *
window_label_stripe_get (GtkWidget * widget,
			 char *name,
			 GdkRGBA * label_color)
{
	static GSList  *hl_stripe_list = NULL;
	GSList         *stored_hl_stripe = NULL;
	CacheStripe    *hl_stripe;

	if (name == NULL || label_color == NULL)
		return NULL;

	stored_hl_stripe = g_slist_find_custom (hl_stripe_list,
						name,
				       (GCompareFunc) label_string_compare);
	if (stored_hl_stripe)
		return ((CacheStripe *) stored_hl_stripe->data)->image;

	hl_stripe = g_new0 (CacheStripe, 1);

	hl_stripe->name = g_strdup (name);

	hl_stripe->image = g_new0 (ConstraintImage, 1);

	hl_stripe->image->pixbuf = gdk_pixbuf_new_from_inline (-1,
							   workspace_highlight,
							       TRUE, NULL);

	gnome_tsol_constraint_image_set_border (hl_stripe->image, 9, 9, 8, 8);
	gnome_tsol_constraint_image_set_stretch (hl_stripe->image, TRUE);
	gnome_tsol_constraint_image_colorize (hl_stripe->image, label_color, 255, TRUE);

	hl_stripe_list = g_slist_append (hl_stripe_list, hl_stripe);
	return hl_stripe->image;
}

ConstraintImage *
workspace_label_stripe_get (GtkWidget * widget,
			    char *name,
			    GdkRGBA * label_color)
{
	static GSList  *stripe_list = NULL;
	GSList         *stored_hl_stripe = NULL;
	CacheStripe    *hl_stripe;

	stored_hl_stripe = g_slist_find_custom (stripe_list,
						name,
				       (GCompareFunc) label_string_compare);
	if (stored_hl_stripe)
		return ((CacheStripe *) stored_hl_stripe->data)->image;

	hl_stripe = g_new0 (CacheStripe, 1);

	hl_stripe->name = g_strdup (name);

	hl_stripe->image = g_new0 (ConstraintImage, 1);

	hl_stripe->image->pixbuf = gdk_pixbuf_new_from_inline (-1,
							workspace_highlight,
							       TRUE, NULL);

	gnome_tsol_constraint_image_set_border (hl_stripe->image, 8, 8, 3, 3);
	gnome_tsol_constraint_image_set_stretch (hl_stripe->image, TRUE);
	gnome_tsol_constraint_image_colorize (hl_stripe->image, label_color, 255, TRUE);

	stripe_list = g_slist_append (stripe_list, hl_stripe);
	return hl_stripe->image;
}

char           *
window_label_get_name (Display * xdisplay, Window xwindow)
{
	bslabel_t       label;

	if (XTSOLIsWindowTrusted (xdisplay, xwindow)) {
		return g_strdup ("Trusted Path");
	} else if (XTSOLgetResLabel (xdisplay, xwindow, IsWindow, &label)) {
		char           *string = NULL;

		bsltos (&label, &string, 0, LONG_CLASSIFICATION | LONG_WORDS |
			ALL_ENTRIES);
		return string;
	} else
		return g_strdup ("Couldn't get the window label\n");
}
GdkRGBA       *
window_label_get_color (Display * xdisplay, Window xwindow)
{
#define DEFAULT_COLOR	"white"
	char           *colorname;
	m_label_t       label;
	GdkRGBA       *color = g_new0 (GdkRGBA, 1);

	if (XTSOLgetResLabel (xdisplay, xwindow, IsWindow, &label)) {
		label_to_str (&label, &colorname, M_COLOR, DEF_NAMES);
		//colorname = bltocolor (&label);
		if (colorname == NULL)
			colorname = g_strdup (DEFAULT_COLOR);
	} else
		colorname = g_strdup (DEFAULT_COLOR);

	gdk_rgba_parse (color, (const char *) colorname);
	g_free (colorname);
	return color;
}

static GdkRGBA *private_label_color = NULL;

GdkRGBA       *
current_window_label_get_color ()
{
	return private_label_color;
}

void
current_window_label_set_color (GdkRGBA * new_color)
{
	if (private_label_color)
		g_free (private_label_color);
	private_label_color = new_color;
}

static char    *private_label_name = NULL;

char           *
current_window_label_get_name ()
{
	return private_label_name;
}

void 
current_window_label_set_name (char *new_name)
{
	if (private_label_name)
		g_free (private_label_name);
	private_label_name = new_name;
}

char           *
current_workspace_label_get_name (GtkWidget * widget)
{
	GdkScreen      *gdk_scr = gtk_widget_get_screen (widget);
	gint index = gdk_screen_get_number (gdk_scr);

	WnckScreen     *wnck_scr = wnck_screen_get (index);
	WnckWorkspace  *ws = wnck_screen_get_active_workspace (wnck_scr);

	return wnck_workspace_get_human_readable_label (ws);
}

static GdkRGBA *private_ws_label_color = NULL;

GdkRGBA       *
current_workspace_label_get_color (GtkWidget * widget)
{
	int             error;
	m_label_t      *mlabel = NULL;
	GdkScreen      *gdk_scr = gtk_widget_get_screen (widget);
	gint            index = gdk_screen_get_number (gdk_scr);
	WnckScreen     *wnck_scr = wnck_screen_get (index);
	WnckWorkspace  *ws = wnck_screen_get_active_workspace (wnck_scr);
	const char     *label = wnck_workspace_get_label (ws);

	if (!label)
		return NULL;

	if (str_to_label (label, &mlabel, MAC_LABEL, L_NO_CORRECTION, &error) == 0) {
		char           *colorname = NULL;

		label_to_str (mlabel, &colorname, M_COLOR, DEF_NAMES);
		m_label_free(mlabel);

#define DEFAULT_COLOR "white"

		if (colorname == NULL)
			colorname = g_strdup (DEFAULT_COLOR);

		if (private_ws_label_color)
			g_free (private_ws_label_color);

		private_ws_label_color = g_new (GdkRGBA, 1);

		gdk_rgba_parse (private_ws_label_color,
				(const char *) colorname);

		g_free (colorname);

		return private_ws_label_color;
	}
	return NULL;
}

const char     *
current_role_name (GtkWidget * widget)
{
	GdkScreen *gdk_scr;
	gint index;

	gdk_scr = gtk_widget_get_screen (widget);
	index = gdk_screen_get_number (gdk_scr);
	WnckScreen     *wnck_scr = wnck_screen_get (index);
	WnckWorkspace  *ws = wnck_screen_get_active_workspace (wnck_scr);

	if (ws) {
		User           *user;
		char           *role = (char *) wnck_workspace_get_role (ws);

		if (!role) {
			user = (User *) g_slist_nth_data (users, 0);
			return user->p->pw_name;
		}
		return role;
	}
	return "User Role";
}


GdkPixbuf      *
current_window_icon_get ()
{
	WnckWindow     *win = wnck_window_get (current_xwin);

	if (!win)
		return NULL;
	return wnck_window_get_mini_icon (win);
}

static void 
update_query_window_popup (TrustedStripe * stripe)
{
	PangoLayout    *pango_layout;
	int             pango_height, pango_width;
	int             w, h;

	/* resize workspace custom label */
	pango_layout = gtk_widget_create_pango_layout (stripe->query_window_label_da,
					  current_window_label_get_name ());
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);

#define POPUP_PADDING 4 + 45
	gtk_window_get_size (GTK_WINDOW (stripe->query_window_label), &w, &h);

	gtk_window_resize (GTK_WINDOW (stripe->query_window_label), PANGO_PIXELS (pango_width) + POPUP_PADDING, h);

	gtk_widget_queue_draw (stripe->query_window_label_da);

	g_object_unref (pango_layout);
}

static void 
update_query_window_popups ()
{
	g_slist_foreach (stripes, (GFunc) update_query_window_popup, NULL);
}

static void 
query_window_popup_show (TrustedStripe * stripe)
{
	update_query_window_popup (stripe);
	gtk_widget_show_all (stripe->query_window_label);
}

static void 
query_window_popup_hide (TrustedStripe * stripe)
{
	gtk_widget_hide(stripe->query_window_label);
}

void 
query_window_popups_show (gboolean show)
{
	GdkDevice	*pointer;
	GdkWindow      *root = gdk_screen_get_root_window (gdk_display_get_default_screen (gdk_display_get_default ()));
	pointer = gtk_get_current_event_device();
	if (show) {
		gdk_window_set_events (root,
			GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
		gdk_device_grab(pointer, root, GDK_OWNERSHIP_WINDOW, FALSE,
			GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK,
			gdk_cursor_new (GDK_CROSS), GDK_CURRENT_TIME);
		update_trusted_stripes ();
		g_slist_foreach (stripes, (GFunc) query_window_popup_show, NULL);
	} else {
		gdk_window_set_events (root, GDK_PROPERTY_CHANGE_MASK);
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		g_slist_foreach (stripes, (GFunc) query_window_popup_hide, NULL);
	}
	query_windows_visible = !query_windows_visible;
}

void
trusted_stripe_help_show (GtkWidget *widget)
{
	GError *err = NULL;

	GdkScreen *screen = gtk_widget_get_screen (widget);
	char *command = "atril --preview /usr/share/mate/help/TrustedStripeHelp.pdf";

/*
	gnome_help_display_desktop_on_screen (NULL, "trusted", "index.xml",
				    "intro_trusted_stripe", screen, &err);
*/
	g_spawn_command_line_async (command, &err);
	if (err) {
		GtkWidget *err_dialog = gtk_message_dialog_new (GTK_WINDOW (widget),
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

/* stripe update functions */

static void 
update_trusted_stripe_role (TrustedStripe * stripe)
{
	PangoLayout    *pango_layout;
	int             pango_height, pango_width;

	/* resize workspace custom label */
	if (_tstripe_user_count_get () > 1) { 
		pango_layout = gtk_widget_create_pango_layout (stripe->role_da,
				       	current_role_name (stripe->role_da));
		pango_layout_get_size (pango_layout, &pango_width, &pango_height);

#define ROLE_PADDING 4
		gtk_widget_set_size_request (stripe->role_da, PANGO_PIXELS (pango_width) + ROLE_PADDING + gdk_pixbuf_get_width (stripe->role_hat), -1);
		gtk_widget_queue_draw (stripe->role_da);

		g_object_unref (pango_layout);
	}
}

void
update_trusted_stripe_roles ()
{
	g_slist_foreach (stripes, (GFunc) update_trusted_stripe_role, NULL);
	update_trusted_stripes_workspaces ();
}

static void 
update_trusted_stripe (TrustedStripe * stripe)
{
	if (current_window_label_get_name () == NULL)
		return;

	gtk_widget_queue_draw (stripe->window_da);
	gtk_widget_queue_draw (stripe->workspace_da);
	if (_tstripe_user_count_get () > 1)
		gtk_widget_queue_draw (stripe->role_da);
	gtk_widget_queue_draw (stripe->trusted_path_da);

	if (strcmp (current_window_label_get_name (), "Trusted Path") == 0)
		stripe->show_shield = TRUE;
	else
		stripe->show_shield = FALSE;
}

static void 
update_all_stripes ()
{
	g_slist_foreach (stripes, (GFunc) update_trusted_stripe, NULL);
}

static void
resize_trusted_stripe (TrustedStripe *stripe)
{
	int height, width;
	GdkScreen *screen = gtk_widget_get_screen (stripe->toplevel);
	gtk_window_get_size (GTK_WINDOW (stripe->toplevel), &width, &height);
	gtk_window_resize (GTK_WINDOW (stripe->toplevel), gdk_screen_get_width (screen), height);
	gtk_window_move (GTK_WINDOW (stripe->toplevel), 0, 0);
}

static void
resize_all_stripes ()
{
	g_slist_foreach (stripes, (GFunc) resize_trusted_stripe, NULL);
}

static void 
hide_stripe (TrustedStripe *stripe, gboolean hide)
{
	GdkWindow *window = gtk_widget_get_window(stripe->toplevel);

	if (GPOINTER_TO_INT(hide))
	    gdk_window_hide (window);
	else
	  {
	    gdk_window_show (window);
	    XTSOLMakeTPWindow (GDK_WINDOW_XDISPLAY (window), 
			       GDK_WINDOW_XID (window));
	  }
}


static void 
hide_all_stripes (gboolean hide)
{
	g_slist_foreach (stripes, (GFunc) hide_stripe,
		GINT_TO_POINTER(hide));
}


static void 
update_trusted_stripe_workspace (TrustedStripe * stripe)
{
	PangoLayout    *pango_layout;
	int             pango_height, pango_width;

	/* resize workspace custom label */
	pango_layout = gtk_widget_create_pango_layout (stripe->workspace_da, current_workspace_label_get_name (stripe->workspace_da));
	pango_layout_get_size (pango_layout, &pango_width, &pango_height);
#define HIGHLIGHT_PADDING 16
	gtk_widget_set_size_request (stripe->workspace_da, PANGO_PIXELS (pango_width) + HIGHLIGHT_PADDING + gdk_pixbuf_get_width (stripe->workspace_icon), -1);
	gtk_widget_queue_draw (stripe->workspace_da);

	g_object_unref (pango_layout);
}

void 
update_trusted_stripes_workspaces ()
{
	g_slist_foreach (stripes, (GFunc) update_trusted_stripe_workspace, NULL);
}

static gboolean 
xscreensaver_running(void) 
{
  static Atom XA_LOCK, XA_BLANK;
  static gboolean inited = False;
  Atom rtype;
  Display *xdisplay;
  int format;
  unsigned long nitems, bytesafter;
  guchar *data = 0;
  guint *idata = 0;
  gboolean ret = False;
  
  if (! inited) 
    {
      XA_LOCK = XInternAtom(gdk_x11_get_default_xdisplay(),"LOCK", False);
      XA_BLANK = XInternAtom(gdk_x11_get_default_xdisplay(),"BLANK", False);
      inited = True;
    }
  
   xdisplay  = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
   if (XGetWindowProperty(xdisplay, DefaultRootWindow(xdisplay),
                         XA_SCREENSAVER_STATUS, 0, 999, False, 
                         XA_INTEGER, &rtype, &format, &nitems, &bytesafter,
                         &data) == Success) {
    /* sanity checking */
    if (rtype == XA_INTEGER || nitems >= 3) {
      idata = (guint *)data;
      /* when scheen is locked or blanked the screensaver is running */
      if (idata[0] == XA_LOCK || idata[0] == XA_BLANK) {
        ret = True;
      }
    }
    XFree(data);
  }
  return ret;
}

void 
update_trusted_stripes ()
{
	Window          new_xwin = None, child, root;
	Display		*xdisplay;
	int 		rootx = -1, rooty = -1;
	int		winx, winy;
	unsigned int 	xmask;

	/*
	 * If the role authentication dialog is showing,
	 * always display trusted path since the role
	 * dialog is system modal and grabs keyboard and pointer,
	 * preventing update to the display while it's mapped.
	 */
	if (global_role_dialog_is_mapped) {
		GdkRGBA *override_color = g_new0 (GdkRGBA, 1);
		char *colorname = NULL;

		m_label_t	admin_low;
		bsllow (&admin_low);
		label_to_str (&admin_low, &colorname, M_COLOR, DEF_NAMES);
		gdk_rgba_parse (override_color, (const char *) colorname); 
		g_free (colorname);

		current_window_label_set_name (g_strdup ("Trusted Path"));
		current_window_label_set_color (override_color);
		update_all_stripes ();
		return;
	}

	xdisplay  = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

	XQueryPointer (xdisplay, DefaultRootWindow (xdisplay),
                 &root, &child, &rootx, &rooty, &winx, &winy, &xmask);

	if (child != 0)
		new_xwin = XmuClientWindow (xdisplay, child);

	if (new_xwin != None) {
		if (new_xwin != current_xwin) {
			current_xwin = new_xwin;

			if (current_xwin != (Window)NULL) {
				current_window_label_set_name (window_label_get_name (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), current_xwin));
				current_window_label_set_color (window_label_get_color (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), current_xwin));
				update_all_stripes ();
			}
		}
	} else {
		GdkRGBA *error_color = g_new0 (GdkRGBA, 1);

		current_window_label_set_name (g_strdup ("Initializing Workspace"));
		gdk_rgba_parse (error_color, "white" );
		current_window_label_set_color (error_color);
	}
}

static GdkFilterReturn
filter_func (GdkXEvent * gdkxevent,
	     GdkEvent * event,
	     gpointer data)
{
	XEvent         *xevent = gdkxevent;

	switch (xevent->type) {
	case MotionNotify:
		if (query_windows_visible) {
			Window          root;
			Window          child;
			int             rootx = -1, rooty = -1;
			int             winx, winy;
			unsigned int    xmask;
			static Window   previous_win = (Window)NULL;

			XQueryPointer (xevent->xmotion.display, xevent->xmotion.root,
			&root, &child, &rootx, &rooty, &winx, &winy, &xmask);

			if (previous_win != child) {
				current_xwin = XmuClientWindow (xevent->xmotion.display, child);
				current_window_label_set_name (window_label_get_name (xevent->xmotion.display, current_xwin));
				current_window_label_set_color (window_label_get_color (xevent->xmotion.display, current_xwin));
				update_query_window_popups ();
				previous_win = current_xwin;
			}
		}
		break;
	case ButtonPress:
		if (query_windows_visible) {
			query_window_popups_show (FALSE);
			update_all_stripes ();
		}
		break;
	case PropertyNotify:
		if (xevent->xproperty.atom == net_trusted_active_window)
			update_trusted_stripes ();
		else if (xevent->xproperty.atom == XA_SCREENSAVER_STATUS)
		  hide_all_stripes (xscreensaver_running ());
		break;
	case ConfigureNotify: /*Handle screen resize */
#ifdef HAVE_RANDR
		XRRUpdateConfiguration (xevent);
#endif
		resize_all_stripes ();
		break;
	}
	return GDK_FILTER_CONTINUE;
}

/* menu part */

static gboolean 
show_menu (GtkWidget * widget,
	   GdkEvent * event,
	   gpointer data)
{
	GtkMenu        *menu = (GtkMenu *) data;

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *bevent = (GdkEventButton *) event;

		gtk_menu_popup (menu, NULL, NULL, _tstripe_menu_position_func, widget, bevent->button, bevent->time);
		return TRUE;
	}
	return FALSE;
}

static void 
add_trusted_path_menu (GtkWidget * widget)
{
	GtkWidget      *trusted_path_menu;

	trusted_path_menu = _tstripe_create_trusted_path_menu (gtk_widget_get_screen (widget));
	g_signal_connect (G_OBJECT (widget), "button_press_event",
			  G_CALLBACK (show_menu),
			  (gpointer) trusted_path_menu);
}

static void 
add_role_menu (GtkWidget * widget)
{
	GtkWidget      *role_menu;

	role_menu = _tstripe_create_role_menu (gtk_widget_get_screen (widget));
	g_signal_connect (G_OBJECT (widget), "button_press_event",
			  G_CALLBACK (show_menu),
			  (gpointer) role_menu);
}

void 
add_workspace_menu (GtkWidget * widget)
{
	GtkWidget      *workspace_menu;

	workspace_menu = _tstripe_create_workspace_menu (gtk_widget_get_screen (widget));
	g_signal_connect (G_OBJECT (widget), "button_press_event",
			  G_CALLBACK (show_menu),
			  (gpointer) workspace_menu);
}

static void 
setup_menus (TrustedStripe * stripe)
{

	add_trusted_path_menu (stripe->trusted_path_da);
	if (_tstripe_user_count_get () > 1)
		add_role_menu (stripe->role_da);
	add_workspace_menu (stripe->workspace_da);
}

/* main setup function */
void 
setup_trusted_stripes ()
{
	int             i, num_scr;
	GdkDisplay     *dpy;
	GdkWindow      *default_root;

	/* Initialise Users list needed to populate role menu */
	_tstripe_users_init ();

	global_role_dialog_is_mapped = FALSE;

	/* setup _NET_TRUSTED_ACTIVE_WINDOW prop monitoring */

	dpy = gdk_display_get_default ();

	net_trusted_active_window = XInternAtom (GDK_DISPLAY_XDISPLAY (dpy),
					       "_NET_TRUSTED_ACTIVE_WINDOW",
						 FALSE);
	XA_SCREENSAVER_STATUS =  XInternAtom (GDK_DISPLAY_XDISPLAY (dpy),
					      "_SCREENSAVER_STATUS",
					      FALSE);
	current_xwin = None;

	default_root = gdk_screen_get_root_window (gdk_display_get_default_screen (dpy));

	gdk_window_set_events (default_root, GDK_PROPERTY_CHANGE_MASK);

	gdk_window_add_filter (default_root, filter_func, NULL);

	/* create stripe UI */
	num_scr = gdk_display_get_n_screens (dpy);

	for (i = 0; i < num_scr; i++) {
		WnckScreen     *wnckscreen = NULL;
		TrustedStripe  *stripe = create_trusted_stripe (gdk_display_get_screen (dpy, i));

		wnckscreen = wnck_screen_get (i);
		wnck_screen_force_update (wnckscreen);
		g_signal_connect (G_OBJECT (wnckscreen), "active_workspace_changed",
			     G_CALLBACK (update_trusted_stripes_workspaces),
				  NULL);
		/*
		 * WARNING setup_menus () relies on the stripes gslist being
		 * initialised. Swapping these two lines around will result
		 * in a segv
		 */
		stripes = g_slist_append (stripes, stripe);
		setup_menus (stripe);
	}

	update_trusted_stripes ();
	update_trusted_stripes_workspaces ();
	if (_tstripe_user_count_get () > 1)
		update_trusted_stripe_roles ();
}

