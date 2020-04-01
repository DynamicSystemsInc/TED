#ifndef __WNCK_TSOL_H__
#define __WNCK_TSOL_H__

#include <config.h>

#ifdef HAVE_LIBGNOMETSOL
#include <glib-2.0/glib.h>
#include <mate-panel-applet.h>
#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <libgnometsol/label_builder.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

/* Libtsol functions */

typedef int     (*tsol_blequal) (const m_label_t *label1, const m_label_t *label2);
typedef int     (*tsol_label_to_str) (const m_label_t *label, char **string,
                 const m_label_str_t conversion_type,
                 uint_t flags);
typedef int		(*tsol_str_to_label) (const char *string, m_label_t **label,
                 const m_label_type_t label_type, uint_t flags,
                 int *error);
typedef void (*tsol_m_label_dup) (m_label_t **dst, const m_label_t *src);
typedef void	(*tsol_m_label_free) (m_label_t *label);

/* Other misc. libtsol functions that seem to be stable */
typedef blrange_t*		(*tsol_getuserrange) (const char *username);
typedef int				(*tsol_blinrange) (const m_label_t *label,
                         const blrange_t *range);
typedef void			(*tsol_blminimum) (m_label_t *minimum_label,
                         const m_label_t *bounding_label);
typedef void			(*tsol_blmaximum) (m_label_t *maximum_label,
                         const m_label_t *bounding_label);
typedef m_label_t*		(*tsol_blabel_alloc) (void);
typedef void			(*tsol_blabel_free)  (m_label_t *label_p);
typedef void			(*tsol_bsllow)  (m_label_t *label);
typedef void			(*tsol_bslhigh) (m_label_t *label);

/* libXtsol functions */
typedef Status	(*xtsol_XTSOLgetClientLabel) (Display *dpy, XID xid,
                 bslabel_t *sl);
typedef Bool	(*xtsol_XTSOLIsWindowTrusted) (Display *dpy, Window win);

extern gboolean _trusted_extensions_initialised;

/* libgnometsol functions */
typedef GtkWidget*	(*gnometsol_gnome_label_builder_new) (char *msg,
              blevel_t *upper, blevel_t *lower, int mode);
typedef GType     	(*gnometsol_gnome_label_builder_get_type) (void);

/* libtsol functions */
tsol_blequal		libtsol_blequal;
tsol_label_to_str	libtsol_label_to_str;
tsol_str_to_label	libtsol_str_to_label;
tsol_m_label_dup libtsol_m_label_dup;
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

xtsol_XTSOLgetClientLabel	libxtsol_XTSOLgetClientLabel;
xtsol_XTSOLIsWindowTrusted	libxtsol_XTSOLIsWindowTrusted;

gnometsol_gnome_label_builder_new libgnometsol_gnome_label_builder_new;
gnometsol_gnome_label_builder_get_type libgnometsol_gnome_label_builder_get_type;

gboolean _applet_use_trusted_extensions ();
const char* _wnck_get_min_label ();
const char* _wnck_get_max_label ();

GtkWidget* window_menu_create_label_indicator (WnckWindow *window, 
					       GtkWidget  *image);
#endif /* HAVE_LIBGNOMETSOL */
#endif
