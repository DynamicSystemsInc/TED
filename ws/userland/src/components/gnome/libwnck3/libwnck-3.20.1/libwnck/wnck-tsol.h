#ifndef __WNCK_TSOL_H__
#define __WNCK_TSOL_H__

#include <config.h>

#ifdef HAVE_XTSOL
#include <pwd.h>
#include <glib-2.0/glib.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <gtk/gtk.h>
#include "libwnck.h"

/* libbsm provides getdevicerange(3TSOL) - don't believe the man page */
typedef blrange_t*	(*bsm_getdevicerange) (const char *device);

/* Libtsol functions */
typedef int     (*tsol_label_to_str) (const m_label_t *label, char **string,
				      const m_label_str_t conversion_type, 
				      uint_t flags);
typedef int	(*tsol_str_to_label) (const char *string, m_label_t **label,
				      const m_label_type_t label_type, 
				      uint_t flags,
				      int *error);

typedef void	(*tsol_m_label_free) (m_label_t *label);

/* Other misc. libtsol functions */
typedef blrange_t*	(*tsol_getuserrange) (const char *username);
typedef int		(*tsol_blinrange)    (const m_label_t *label,
					      const blrange_t *range);
typedef void		(*tsol_blminimum)    (m_label_t *minimum_label,
					      const m_label_t *bounding_label);
typedef void		(*tsol_blmaximum)    (m_label_t *maximum_label,
					      const m_label_t *bounding_label);
typedef m_label_t*	(*tsol_blabel_alloc) (void);
typedef void		(*tsol_blabel_free)  (m_label_t *label_p);
typedef void		(*tsol_bsllow)  (m_label_t *label);
typedef void		(*tsol_bslhigh) (m_label_t *label);

/* libXtsol functions */
typedef Status	(*xtsol_XTSOLgetResLabel) (Display *dpy, XID xid,
                 ResourceType resourceFlag, bslabel_t *sl);
typedef Status	(*xtsol_XTSOLgetResUID) (Display *dpy, XID object,
                 ResourceType resourceFlag,
                 uid_t *uidp);
typedef Status	(*xtsol_XTSOLgetWorkstationOwner) (Display *dpy, uid_t *uidp);
typedef Bool    (*xtsol_XTSOLIsWindowTrusted) (Display *dpy, Window win); 

/* libgnometsol functions */
/* constraint scaling stuff */
typedef struct _ConstraintImage ConstraintImage;
struct _ConstraintImage
{
  gchar     *filename;
  GdkPixbuf *pixbuf;
  GdkPixbuf *scaled;
  gboolean   stretch;
  gint       border_left;
  gint       border_right;
  gint       border_bottom;
  gint       border_top;
  guint      hints[3][3];
  gboolean   recolorable;
  GdkRGBA   colorize_color;
  gboolean   use_as_bkg_mask;
};

typedef void  (*gnome_tsol_constraint_image_render) (cairo_t *cr, ConstraintImage *cimage,
						     GdkWindow    *window,
						     GdkRectangle *clip_rect,
						     gboolean      center,		   
						     gint          x,
						     gint          y,
						     gint          width,
						     gint          height);

typedef void (*gnome_tsol_constraint_image_set_border) (ConstraintImage *pb,
							gint         left,
							gint         right,
							gint         top,
							gint         bottom);

typedef void (*gnome_tsol_constraint_image_set_stretch) (ConstraintImage *pb,
							 gboolean     stretch);

typedef void (*gnome_tsol_constraint_image_colorize) (ConstraintImage *image,
						      GdkRGBA  *color,
						      int	alpha,
						      gboolean   use_alpha);

/* libbsm functions */
bsm_getdevicerange	libbsm_getdevicerange;
/* libtsol functions*/
tsol_label_to_str	libtsol_label_to_str;
tsol_str_to_label	libtsol_str_to_label;
tsol_m_label_free	libtsol_m_label_free;
/* Other misc. libtsol functions */
tsol_blminimum		libtsol_blminimum;
tsol_blmaximum		libtsol_blmaximum;
tsol_blinrange      libtsol_blinrange;
tsol_getuserrange	libtsol_getuserrange;
tsol_blabel_alloc	libtsol_blabel_alloc;
tsol_blabel_free	libtsol_blabel_free;
tsol_bsllow			libtsol_bsllow;
tsol_bslhigh		libtsol_bslhigh;

xtsol_XTSOLgetResLabel	libxtsol_XTSOLgetResLabel;
xtsol_XTSOLgetResUID	libxtsol_XTSOLgetResUID;
xtsol_XTSOLgetWorkstationOwner	libxtsol_XTSOLgetWorkstationOwner;
xtsol_XTSOLIsWindowTrusted	libxtsol_XTSOLIsWindowTrusted;

gnome_tsol_constraint_image_render libgnome_tsol_constraint_image_render;
gnome_tsol_constraint_image_set_border libgnome_tsol_constraint_image_set_border;
gnome_tsol_constraint_image_set_stretch libgnome_tsol_constraint_image_set_stretch;
gnome_tsol_constraint_image_colorize libgnome_tsol_constraint_image_colorize;

const char* _wnck_get_min_label ();
const char* _wnck_get_max_label ();


/* GFX part */

ConstraintImage* get_highlight_stripe (char*	  name,
				       GdkRGBA*  label_color);

GtkWidget*	 window_menu_create_label_indicator (WnckWindow *window, 
						     GtkWidget  *image);

#endif /* HAVE_XTSOL */
#endif
