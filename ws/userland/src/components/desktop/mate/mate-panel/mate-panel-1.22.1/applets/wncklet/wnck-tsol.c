#include  <config.h>
#ifdef HAVE_LIBGNOMETSOL

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>
#include <glib.h>
#include <mate-panel-applet.h>
#include "wnck-tsol.h"
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

static
void * dlopen_tsol (void)
{
   void  *handle = NULL;

   /*
    * No 32-bit version of libwnck so we can get away with hardcoding
    * to a single path on this occasion
    */
   if ((handle = dlopen ("/usr/lib/amd64/libtsol.so.2", RTLD_LAZY)) != NULL)
       return handle;
   
   return handle;
}

static
void * dlopen_xtsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/amd64/libXtsol.so.1", RTLD_LAZY)) != NULL)
       return handle;

   return handle;
}

static
void * dlopen_gnometsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/amd64/libgnometsol.so", RTLD_LAZY)) != NULL)
       return handle;
   
   return handle;
}


gboolean
_applet_use_trusted_extensions (void)
{
  static int trusted = -1;

  /*
   * Sun Trusted Extensions (tm) for Solaris (tm) support. (Damn I should be a lawyer).
   *
   * It is necessary to use dlopen because the label aware extensions to libwnck work
   * only on systems with the trusted extensions installed and with the SUN_TSOL
   * xserver extension present
   */

    if (trusted < 0) {
        static gpointer tsol_handle = NULL;
        static gpointer xtsol_handle = NULL;
        static gpointer gnometsol_handle = NULL;

      if (getenv ("TRUSTED_SESSION") == NULL) {
        trusted = 0;
        return 0;
      }   	

        tsol_handle = dlopen_tsol ();
        if (tsol_handle != NULL)
            xtsol_handle = dlopen_xtsol ();
        if (tsol_handle && xtsol_handle) {

           /* libtsol functions */
           libtsol_blequal = (tsol_blequal) dlsym (tsol_handle, "blequal");
           libtsol_label_to_str = (tsol_label_to_str) dlsym (tsol_handle, "label_to_str");
           libtsol_str_to_label = (tsol_str_to_label) dlsym (tsol_handle, "str_to_label");
           libtsol_m_label_dup = (tsol_m_label_dup) dlsym (tsol_handle, "m_label_dup");
           libtsol_m_label_free = (tsol_m_label_free) dlsym (tsol_handle, "m_label_free");

           /* Other misc. libtsol functions */
           libtsol_blminimum = (tsol_blminimum) dlsym (tsol_handle, "blminimum");
           libtsol_blmaximum = (tsol_blmaximum) dlsym (tsol_handle, "blmaximum");
           libtsol_blinrange = (tsol_blinrange) dlsym (tsol_handle, "blinrange");
           libtsol_getuserrange = (tsol_getuserrange) dlsym (tsol_handle, "getuserrange");
           libtsol_blabel_alloc = (tsol_blabel_alloc) dlsym (tsol_handle, "blabel_alloc");
           libtsol_blabel_free  = (tsol_blabel_free)  dlsym (tsol_handle, "blabel_free");
           libtsol_bsllow  = (tsol_bsllow)  dlsym (tsol_handle, "bsllow");
           libtsol_bslhigh = (tsol_bslhigh) dlsym (tsol_handle, "bslhigh");

           /* libXtsol functions */
           libxtsol_XTSOLgetClientLabel = (xtsol_XTSOLgetClientLabel) dlsym (xtsol_handle,
                                     "XTSOLgetClientLabel");
           libxtsol_XTSOLIsWindowTrusted = (xtsol_XTSOLIsWindowTrusted) dlsym (xtsol_handle,
                                      "XTSOLIsWindowTrusted");

           if (libtsol_label_to_str == NULL ||
               libtsol_str_to_label == NULL ||
               libtsol_m_label_dup == NULL ||
               libtsol_m_label_free == NULL ||
               libtsol_blminimum == NULL ||
               libtsol_blmaximum == NULL ||
               libtsol_blinrange == NULL ||
               libtsol_getuserrange == NULL ||
               libtsol_blabel_alloc == NULL ||
               libtsol_blabel_free  == NULL ||
               libtsol_bsllow  == NULL ||
               libtsol_bslhigh == NULL ||
               libxtsol_XTSOLgetClientLabel == NULL ||
               libxtsol_XTSOLIsWindowTrusted == NULL) {
               dlclose (tsol_handle);
               dlclose (xtsol_handle);
               tsol_handle = NULL;
               xtsol_handle = NULL;
            }
        }
		gnometsol_handle = dlopen_gnometsol ();
		if (gnometsol_handle != NULL) {
			libgnometsol_gnome_label_builder_new = 
				(gnometsol_gnome_label_builder_new) dlsym (gnometsol_handle, 
				"gnome_label_builder_new");
			libgnometsol_gnome_label_builder_get_type =
				(gnometsol_gnome_label_builder_get_type) dlsym (gnometsol_handle,
				"gnome_label_builder_get_type");
			if (libgnometsol_gnome_label_builder_new == NULL ||
			libgnometsol_gnome_label_builder_get_type == NULL)
				gnometsol_handle = NULL;
		}
    trusted = ((tsol_handle != NULL) && (xtsol_handle != NULL) && (gnometsol_handle != NULL)) ? 1 : 0;
    }
    return trusted ? TRUE : FALSE;
}

const char *
_wnck_get_min_label ()
{
    static char *min_label = NULL;

    if (!min_label) {
        min_label = (char *) getenv ("USER_MIN_SL");
    }
    return min_label;
}

const char*
_wnck_get_max_label()
{
    static char *max_label = NULL;

    if (!max_label) {
        max_label = (char *) getenv ("USER_MAX_SL");
    }
    return max_label;
}


/* window selector part */

static gboolean tsol_win_selector_label_expose_event (GtkWidget        *widget,
						      cairo_t   	*cr,
						      gpointer          data)
{
  WnckWindow *window = (WnckWindow *) data;

  /*
  GdkGC *tmp_gc = gdk_gc_new (widget->window);
  gdk_gc_set_rgb_fg_color (tmp_gc, wnck_window_get_label_color (window));

  gdk_draw_rectangle (widget->window, 
		      widget->style->black_gc,
		      FALSE,
		      event->area.x, event->area.y,
		      event->area.width - 1, event->area.height - 1);
  
  gdk_draw_rectangle (widget->window, 
		      tmp_gc,
		      TRUE,
		      event->area.x + 1, event->area.y + 1,
		      event->area.width - 2, event->area.height - 2);

  g_object_unref (tmp_gc);
  */
  GdkRectangle	area;
  GdkRGBA	*label_color;
  GdkRGBA	black;

  gtk_widget_get_allocation(widget, &area);
  gdk_rgba_parse(&black, "black");
  label_color = wnck_window_get_label_color(window);
  cairo_set_source_rgb(cr, black.red, black.green, black.blue);
  cairo_rectangle(cr, area.x, area.y, area.width - 1, area.height - 1);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, label_color->red, label_color->green, label_color->blue);
  cairo_rectangle(cr, area.x + 1, area.y + 1, area.width - 2, area.height - 2);
  cairo_fill(cr);
  
  return FALSE;
}

GtkWidget *
window_menu_create_label_indicator (WnckWindow *window, 
				    GtkWidget  *image)
{
  GtkWidget *da, *hbox;
  da = gtk_drawing_area_new ();
  
  g_signal_connect (G_OBJECT (da), "draw",  
		    G_CALLBACK (tsol_win_selector_label_expose_event), 
		    window);
  
  gtk_widget_set_size_request (da, 5, -1);
  
  hbox = gtk_hbox_new (FALSE, 4);
  
  gtk_container_add (GTK_CONTAINER (hbox), da);
  gtk_container_add (GTK_CONTAINER (hbox), image);
  
  gtk_widget_show (da);
  gtk_widget_show (hbox);
  gtk_widget_show (image);

  return hbox;
}
#endif /* HAVE_LIBGNOMETSOL */
