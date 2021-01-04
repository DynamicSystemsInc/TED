#include  <config.h>
#ifdef HAVE_XTSOL

#include <stdlib.h>
#include <strings.h>
#include <dlfcn.h>
#include <link.h>
#include <glib.h>
#include "wnck-tsol.h"
#include "tsol-pics.h"

static
void * dlopen_bsm (void)
{
	return dlopen ("/usr/lib/libbsm.so", RTLD_LAZY);
}

static
void * dlopen_tsol (void)
{
   void  *handle = NULL;

   /*
    * No 64-bit version of libwnck so we can get away with hardcoding
    * to a single path on this occasion
    */
   if ((handle = dlopen ("/usr/lib/libtsol.so.2", RTLD_LAZY)) != NULL)
       return handle;
   
   return handle;
}

static
void * dlopen_gnometsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/libgnometsol.so", RTLD_LAZY)) != NULL)
       return handle;
   
   return handle;
}

static
void * dlopen_xtsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/libXtsol.so.1", RTLD_LAZY)) != NULL)
       return handle;
   if ((handle = dlopen ("/usr/openwin/lib/libXtsol.so.1", RTLD_LAZY)) != NULL)
       return handle;

   return handle;
}

gboolean
_wnck_use_trusted_extensions (void)
{

  static int trusted = -1;

  /*
   * Sun Trusted Extensions (tm) for Solaris (tm) support. 
   *
   * It is necessary to use dlopen because the label aware extensions to libwnck work
   * only on systems with the trusted extensions installed and with the SUN_TSOL
   * xserver extension present
   */

    if (trusted < 0) {
      static gpointer bsm_handle = NULL;
      static gpointer tsol_handle = NULL;
      static gpointer xtsol_handle = NULL;
      static gpointer gnometsol_handle = NULL;

      if (getenv ("TRUSTED_SESSION") == NULL) {
        trusted = 0;
        return 0;
      }   	

		bsm_handle = dlopen_bsm ();
        tsol_handle = dlopen_tsol ();
		xtsol_handle = dlopen_xtsol ();

        if (bsm_handle && tsol_handle && xtsol_handle) {
           /* libbsm function (only interested in the one) */
           libbsm_getdevicerange = (bsm_getdevicerange) dlsym (bsm_handle, "getdevicerange");

           /* libtsol functions */
           libtsol_label_to_str = (tsol_label_to_str) dlsym (tsol_handle, "label_to_str");
           libtsol_str_to_label = (tsol_str_to_label) dlsym (tsol_handle, "str_to_label");
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
           libxtsol_XTSOLgetResLabel = (xtsol_XTSOLgetResLabel) dlsym (xtsol_handle,
                                     "XTSOLgetResLabel");
           libxtsol_XTSOLgetResUID = (xtsol_XTSOLgetResUID) dlsym (xtsol_handle,
                                     "XTSOLgetResUID");
           libxtsol_XTSOLgetWorkstationOwner = (xtsol_XTSOLgetWorkstationOwner) dlsym (xtsol_handle,
                                     "XTSOLgetWorkstationOwner");
           libxtsol_XTSOLIsWindowTrusted = (xtsol_XTSOLIsWindowTrusted) dlsym (xtsol_handle,
                                     "XTSOLIsWindowTrusted");

           if (libbsm_getdevicerange == NULL ||
               libtsol_label_to_str == NULL ||
               libtsol_str_to_label == NULL ||
               libtsol_m_label_free == NULL ||
               libtsol_blminimum == NULL ||
               libtsol_blmaximum == NULL ||
               libtsol_blinrange == NULL ||
               libtsol_getuserrange == NULL ||
               libtsol_blabel_alloc == NULL ||
               libtsol_blabel_free  == NULL ||
               libtsol_bsllow  == NULL ||
               libtsol_bslhigh == NULL ||
               libxtsol_XTSOLgetResLabel == NULL ||
               libxtsol_XTSOLgetResUID == NULL ||
               libxtsol_XTSOLgetWorkstationOwner == NULL ||
               libxtsol_XTSOLIsWindowTrusted == NULL) {
               dlclose (bsm_handle);
               dlclose (tsol_handle);
               dlclose (xtsol_handle);
               bsm_handle = NULL;
               tsol_handle = NULL;
               xtsol_handle = NULL;
            }
        }
	gnometsol_handle = dlopen_gnometsol ();
	if (gnometsol_handle != NULL)
	  {
	    libgnome_tsol_constraint_image_render = (gnome_tsol_constraint_image_render) dlsym (gnometsol_handle, "gnome_tsol_constraint_image_render");
	    libgnome_tsol_constraint_image_set_border = (gnome_tsol_constraint_image_set_border) dlsym (gnometsol_handle, "gnome_tsol_constraint_image_set_border");
	    libgnome_tsol_constraint_image_set_stretch = (gnome_tsol_constraint_image_set_stretch) dlsym (gnometsol_handle, "gnome_tsol_constraint_image_set_stretch");
	    libgnome_tsol_constraint_image_colorize = (gnome_tsol_constraint_image_colorize) dlsym (gnometsol_handle, "gnome_tsol_constraint_image_colorize");

	    if (libgnome_tsol_constraint_image_render == NULL ||
		libgnome_tsol_constraint_image_set_border == NULL ||
		libgnome_tsol_constraint_image_set_stretch == NULL ||
		libgnome_tsol_constraint_image_colorize == NULL)
		gnometsol_handle = NULL;

	  }
	trusted = ((bsm_handle != NULL) && (tsol_handle != NULL) && (xtsol_handle != NULL) && (gnometsol_handle != NULL)) ? 1 : 0;
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


/* GFX part */


typedef struct _HighlightStripe HighlightStripe;

struct _HighlightStripe
{
  ConstraintImage *image;
  char		  *name;
};

static gint
label_string_compare (HighlightStripe *tmp, char *searched_label)
{
  return strcmp (searched_label, tmp->name);  
}

ConstraintImage * 
get_highlight_stripe (char     *name,
		      GdkColor *label_color)
{
  static GSList *hl_stripe_list = NULL;
  GSList *stored_hl_stripe = NULL;
  HighlightStripe *hl_stripe;

  if ((name == NULL) || (label_color == NULL))
    return NULL;
    

  stored_hl_stripe = g_slist_find_custom (hl_stripe_list, 
					  name, 
					  (GCompareFunc)label_string_compare);
  if (stored_hl_stripe)
    return ((HighlightStripe* )stored_hl_stripe->data)->image;
 
  hl_stripe = g_new0 (HighlightStripe, 1);

  hl_stripe->name = g_strdup (name);

  hl_stripe->image = g_new0 (ConstraintImage, 1);

  hl_stripe->image->pixbuf = gdk_pixbuf_new_from_inline (-1, 
							  highlight_stripe_pb, 
							  TRUE, NULL);
  
  libgnome_tsol_constraint_image_set_border (hl_stripe->image, 3, 0, 1, 1);
  libgnome_tsol_constraint_image_set_stretch (hl_stripe->image, TRUE);
  libgnome_tsol_constraint_image_colorize (hl_stripe->image, label_color, 255, TRUE);
  
  hl_stripe_list = g_slist_append (hl_stripe_list, hl_stripe);
  return hl_stripe->image; 
}

static gboolean label_expose_event (GtkWidget        *widget,
				    GdkEventExpose   *event,
				    gpointer          data)
{
  WnckWindow *window = (WnckWindow *) data;

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

  return FALSE;
}

GtkWidget *
window_menu_create_label_indicator (WnckWindow *window, 
				    GtkWidget  *image)
{
  GtkWidget *da, *hbox;
  da = gtk_drawing_area_new ();
  
  g_signal_connect (G_OBJECT (da), "expose_event",  
		    G_CALLBACK (label_expose_event), 
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


#endif /* HAVE_XTSOL */
