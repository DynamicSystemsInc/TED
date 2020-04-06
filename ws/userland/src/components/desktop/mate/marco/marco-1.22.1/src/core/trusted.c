/* Metacity trusted */

/* 
 * Copyright (C) 2005 Erwann Chenede
 */

#include <config.h>
#ifdef HAVE_XTSOL
#include <string.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <link.h>
#include <strings.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <unistd.h>
#include <pwd.h>
#include "trusted.h"
#include "window.h"
#include "display-private.h"
#include "screen.h"
#include "workspace.h"
#include "xprops.h"
#include "trusted-pics.h"
#include "errors.h"
#include "prefs.h"

  static gboolean _trusted_extensions_initialised = FALSE;
  static gpointer tsol_handle = NULL;
  static gpointer xtsol_handle = NULL;
  static gpointer gnometsol_handle = NULL;
  static gpointer bsm_handle = NULL;

static
void * dlopen_tsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/amd64/libtsol.so.2", RTLD_LAZY)) != NULL)
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


static
void * dlopen_xtsol (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/amd64/libXtsol.so.1", RTLD_LAZY)) != NULL)
       return handle;

   return handle;
}

static
void * dlopen_bsm (void)
{
        return dlopen ("/usr/lib/amd64/libbsm.so", RTLD_LAZY);
}


static gboolean 
tsol_is_multi_label_session (void)
{
  static int trusted = -1;
  if (trusted < 0) {
    if (getenv ("TRUSTED_SESSION")) {
      trusted = 1;
    } else {
      trusted = 0;
    }      
  }
  return trusted ? TRUE : FALSE;
}

gboolean
tsol_use_trusted_extensions (void)
{
	/*
  static gboolean _trusted_extensions_initialised = FALSE;
  static gpointer tsol_handle = NULL;
  static gpointer xtsol_handle = NULL;
  static gpointer gnometsol_handle = NULL;
  static gpointer bsm_handle = NULL;
*/
    if (!_trusted_extensions_initialised) {
        char *label = NULL;
        _trusted_extensions_initialised = TRUE;

        if (!tsol_is_multi_label_session ())
            return FALSE;

        tsol_handle = dlopen_tsol ();
        if (tsol_handle != NULL)
	  xtsol_handle = dlopen_xtsol ();

	bsm_handle = dlopen_bsm ();

        if (tsol_handle && xtsol_handle && bsm_handle) {
           /* libbsm function (only interested in the one) */
           libbsm_getdevicerange = (bsm_getdevicerange) dlsym (bsm_handle, "getdevicerange");
           /* Replacement libtsol functions */
           libtsol_label_to_str = (tsol_label_to_str) dlsym (tsol_handle, "label_to_str"); 
           libtsol_str_to_label = (tsol_str_to_label) dlsym (tsol_handle, "str_to_label");
           libtsol_m_label_free = (tsol_m_label_free) dlsym (tsol_handle, "m_label_free");


           /* Other misc. libtsol functions */
           libtsol_blminimum = (tsol_blminimum) dlsym (tsol_handle, "blminimum");
           libtsol_blmaximum = (tsol_blmaximum) dlsym (tsol_handle, "blmaximum");
           libtsol_blinrange = (tsol_blinrange) dlsym (tsol_handle, "blinrange");
           libtsol_getuserrange = (tsol_getuserrange) dlsym (tsol_handle, "getuserrange"); 
           /* libtsol_blabel_alloc = (tsol_blabel_alloc) dlsym (tsol_handle, "blabel_alloc"); */
           libtsol_blabel_free  = (tsol_blabel_free)  dlsym (tsol_handle, "blabel_free");
           /* libtsol_bsllow  = (tsol_bsllow)  dlsym (tsol_handle, "bsllow"); */
           /* libtsol_bslhigh = (tsol_bslhigh) dlsym (tsol_handle, "bslhigh"); */

           /* libXtsol functions */
           libxtsol_XTSOLgetClientLabel = (xtsol_XTSOLgetClientLabel) dlsym (xtsol_handle,
									     "XTSOLgetClientLabel");
           libxtsol_XTSOLIsWindowTrusted = (xtsol_XTSOLIsWindowTrusted) dlsym (xtsol_handle,
									       "XTSOLIsWindowTrusted");
	   libxtsol_XTSOLsetResLabel = (xtsol_XTSOLsetResLabel) dlsym (xtsol_handle,
									     "XTSOLsetResLabel");
	   libxtsol_XTSOLsetResUID = (xtsol_XTSOLsetResUID) dlsym (xtsol_handle,
									     "XTSOLsetResUID");
	   libxtsol_XTSOLgetResLabel = (xtsol_XTSOLgetResLabel) dlsym (xtsol_handle,
									     "XTSOLgetResLabel");
	   libxtsol_XTSOLgetResUID = (xtsol_XTSOLgetResUID) dlsym (xtsol_handle,
									     "XTSOLgetResUID");

           if (libbsm_getdevicerange == NULL ||
	       /*libtsol_stobsl == NULL ||
               libtsol_bsltos == NULL || */
               libtsol_label_to_str == NULL || 
               libtsol_str_to_label == NULL ||
               libtsol_m_label_free == NULL ||
               libtsol_blminimum == NULL ||
               libtsol_blmaximum == NULL ||
               libtsol_blinrange == NULL ||
               libtsol_getuserrange == NULL ||
               libtsol_blabel_free  == NULL ||
               /* libtsol_getdevicerange == NULL || 
               libtsol_blabel_alloc == NULL ||
               libtsol_bsllow  == NULL ||
               libtsol_bslhigh == NULL || */
               libxtsol_XTSOLgetClientLabel == NULL ||
               libxtsol_XTSOLIsWindowTrusted == NULL ||
	       libxtsol_XTSOLsetResLabel == NULL ||
	       libxtsol_XTSOLsetResUID == NULL ||
	       libxtsol_XTSOLgetResLabel == NULL ||
	       libxtsol_XTSOLgetResUID == NULL)
	     {
               dlclose (tsol_handle);
               dlclose (xtsol_handle);
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
    }
    return ((xtsol_handle != NULL) && (gnometsol_handle != NULL));
}

static gboolean
tsol_use_xtsol_extension ()
{
  static int foundxtsol = -1;
  int major_code, first_event, first_error;

  if (foundxtsol < 0) { 
      foundxtsol = XQueryExtension (gdk_x11_get_default_xdisplay(), "SUN_TSOL", &major_code,
                                    &first_event, &first_error);
  }
  return foundxtsol;
}

gboolean tsol_is_available ()
{
  if (tsol_use_xtsol_extension () && tsol_use_trusted_extensions ())
      return TRUE;
  return FALSE;
}

static Window
get_window_at_pointer (Display *xdisplay)
{
  Window root;
  Window child;
  int rootx = -1, rooty = -1;
  int winx, winy;
  unsigned int xmask;
                                                                                
  XQueryPointer (xdisplay, DefaultRootWindow (xdisplay),
                 &root, &child, &rootx, &rooty, &winx, &winy, &xmask);
  
  return child;
}

gboolean tsol_meta_screen_use_roles (MetaScreen *screen)
{
  char **role_list = NULL;
  int nb_roles;
  if (meta_prop_get_utf8_list (screen->display, 
			       screen->xroot, 
			       screen->display->atom__NET_DESKTOP_ROLES,
			       &role_list, &nb_roles))
    {
      int i;
      for (i=0;i<nb_roles;i++)
	{
	  if (role_list[i] != NULL && strcmp (role_list[i], "") != 0)
	    {
	      g_strfreev (role_list);
	      return TRUE;
	    }
	}
      g_strfreev (role_list);
    }
  return FALSE;
}

gboolean tsol_meta_workspace_has_role (MetaWorkspace *ws)
{
  char **role_list = NULL;
  int nb_roles;

  if (!tsol_is_available ())
    return FALSE;
  
  if (meta_prop_get_utf8_list (ws->screen->display, 
			       ws->screen->xroot, 
			       ws->screen->display->atom__NET_DESKTOP_ROLES,
			       &role_list, &nb_roles))
    {
      int ws_id = meta_workspace_index (ws);
      if (ws_id > nb_roles) /* something is wrong here we don't have the same number of roles/ws */
	{
	  g_strfreev (role_list);
	  return FALSE;
	}
      if (role_list[ws_id] != NULL && strcmp (role_list[ws_id], "") != 0)
	{	  
	  struct passwd *pwd;
	  pwd = getpwuid (getuid ());
	  if (strcmp (role_list[ws_id], pwd->pw_name) == 0) /* role is normal user */
	    {
	      g_strfreev (role_list);
	      return FALSE;
	    }
	  g_strfreev (role_list);
	  return TRUE;
	}	
      g_strfreev (role_list);
    }
    return FALSE;
}

char * tsol_meta_workspace_get_role (MetaWorkspace *ws)
{
  char **role_list = NULL;
  int nb_roles;

  if (!tsol_is_available ())
    return NULL;
  
  if (meta_prop_get_utf8_list (ws->screen->display, 
			       ws->screen->xroot, 
			       ws->screen->display->atom__NET_DESKTOP_ROLES,
			       &role_list, &nb_roles))
    {
      int ws_id = meta_workspace_index (ws);
      if (ws_id > nb_roles) /* something is wrong here we don't have the same number of roles/ws */
	{
	  g_strfreev (role_list);
	  return NULL;
	}
      if (role_list[ws_id] != NULL && strcmp (role_list[ws_id], "") != 0)
	{	  
	  char *return_role_name = NULL;
	  struct passwd *pwd;
	  pwd = getpwuid (getuid ());
	  if (strcmp (role_list[ws_id], pwd->pw_name) == 0) /* role is normal user */
	    {
	      g_strfreev (role_list);
	      return NULL;
	    }
	  return_role_name = g_strdup (role_list[ws_id]);
	  g_strfreev (role_list);
	  return return_role_name;
	}	
      g_strfreev (role_list);
    }
    return NULL;
}

gboolean tsol_xwindow_can_move_to_workspace (Display *xdisplay,
					Window xwin,
					int ws_index)
{
  MetaDisplay *display = meta_display_for_x_display (xdisplay);
  MetaWindow *window = meta_display_lookup_x_window (display, xwin);
  g_assert (window != NULL);
  MetaWorkspace *workspace = meta_screen_get_workspace_by_index (window->screen, ws_index);
  if (tsol_meta_window_can_move_to_workspace (window, workspace))
    return TRUE;
  return FALSE;
}

GList *tsol_add_all_sticky_non_tp_windows (MetaDisplay *display, GList *win_list)
{
  GList *return_list;
  GSList *all_windows;
  GSList *tmp;

  return_list = win_list;

  all_windows = meta_display_list_windows (display);

  tmp = all_windows;
  while (tmp != NULL)
    {
      MetaWindow *window = tmp->data;
      
      if (window->on_all_workspaces && 
	  window->decorated && 
	  /* SUN_BRANDING TJDS */
	  strcmp (tsol_meta_window_label_get_name(window), _("Trusted Path")) != 0)
	{
	  return_list = g_list_prepend (return_list, window);
        }
      tmp = tmp->next;
    }
  return return_list;
}

gboolean tsol_meta_window_can_move_to_workspace (MetaWindow *win,
					    MetaWorkspace *ws)
{
  if (tsol_meta_workspace_has_role (ws))
    {
      const char *name = tsol_meta_window_label_get_name (win);
      /* SUN_BRANDING TJDS */
      if (strcmp (name, _("Trusted Path")) != 0)
	{
	  g_warning ("window %s cannot be moved to workspace %s\n",
		     win->title, meta_workspace_get_name (ws));
	  return FALSE;
	}
    }
  return TRUE;
}

void tsol_trusted_stripe_atom_update (MetaDisplay* display, MetaWindow *window)
{
  unsigned long data[2];

  if (!tsol_is_available())
    return;

  if (window == NULL)
    data[0] = get_window_at_pointer (display->xdisplay);
  else
    data[0] = window->xwindow;

  data[1] = None;

  meta_error_trap_push (display);
  
  XChangeProperty (display->xdisplay, DefaultRootWindow (display->xdisplay),
                       display->atom__NET_TRUSTED_ACTIVE_WINDOW,
                       XA_WINDOW,
                       32, PropModeReplace, (guchar*) data, 2);
  meta_error_trap_pop (display, FALSE);
}


/* Preference related code
 * init labels and roles from metacity to root window */


const char * 
tsol_label_get_min ()
{
    static char *min_label = NULL;

    /*
    if (!min_label) 
    */
        min_label = (char *) getenv ("USER_MIN_SL");
    
    return min_label;
}

const char*
tsol_label_get_max ()
{
    static char *max_label = NULL;

    /*
    if (!max_label)
    */
        max_label = (char *) getenv ("USER_MAX_SL");
    
    return max_label;
}

void tsol_set_frame_label (Display* xdpy, Window xwin, Window xwin_frame)
{
  bslabel_t label;
  uid_t uid;
  
  if (!tsol_is_available ())
    return;

  if (libxtsol_XTSOLgetResLabel (xdpy, xwin, IsWindow, &label))
      libxtsol_XTSOLsetResLabel (xdpy, xwin_frame, IsWindow, &label);

  if (libxtsol_XTSOLgetResUID (xdpy, xwin, IsWindow, &uid))
      libxtsol_XTSOLsetResUID (xdpy, xwin_frame, IsWindow, &uid);
}

gboolean 
tsol_label_is_in_user_range (const char * label)
{
  static blrange_t *range = NULL;
  m_label_t *mlabel = NULL;
  int error;

  if (!tsol_is_available ())
      return FALSE;

  if (!range)
    { /* Get user label Range */
      char *min_label = NULL;
      char *max_label = NULL;
      
      range = g_malloc (sizeof (blrange_t));
      range->lower_bound = range->upper_bound = NULL;
      
      min_label = g_strdup (tsol_label_get_min ());
      max_label = g_strdup (tsol_label_get_max ());
      
      if (libtsol_str_to_label (min_label, &(range->lower_bound),
				MAC_LABEL, L_NO_CORRECTION, &error) < 0) 
	{
	  g_warning ("Couldn't determine minimum workspace label");
	  g_free (min_label);
	  g_free (max_label);
	  return FALSE;
	}
      if (libtsol_str_to_label (max_label, &(range->upper_bound),
				USER_CLEAR, L_NO_CORRECTION, &error) < 0) 
	{
	  g_warning ("Couldn't determine workspace clearance");
	  g_free (min_label);
	  g_free (max_label);
	  return FALSE;
	}
      g_free (min_label);
      g_free (max_label);
    }

  if (libtsol_str_to_label (label, &mlabel, MAC_LABEL, L_NO_CORRECTION, &error) < 0) 
    {
      g_warning("Could not validate sensitivity label \"%s\"", label);
      return FALSE;
    }
    
  if (!libtsol_blinrange (mlabel, range)) 
    {
      libtsol_m_label_free (mlabel);
      return FALSE;
    }
  libtsol_m_label_free (mlabel);
  return TRUE;
}

/*
 * These private (hint hint) functions assume that they have been called
 * from within a trusted desktop session. The caller must ensure that
 * this is the case otherwise it will trigger a load of the potentially
 * non existant tsol and xtsol libs. That would be bad!
 */
static blrange_t *
get_display_range (void)
{
  blrange_t       *range = NULL;

  range = libbsm_getdevicerange ("framebuffer");
  if (range == NULL) {
    range = g_malloc (sizeof (blrange_t));
    range->lower_bound = libtsol_blabel_alloc ();
    range->upper_bound = libtsol_blabel_alloc ();
    libtsol_bsllow  (range->lower_bound);
    libtsol_bslhigh (range->upper_bound);
  }
  return (range);
}


/* tsol_label_is_in_role_range
 *
 * return FALSE if the label is not in the username role range 
 * not if the role exist and has a range it is returned via role_range
 * Note if note NULL role_range needs to be freed
 */

gboolean
tsol_label_is_in_role_range (const char * label, const char * username, char *min_role_label)
{
  /* partial copy of _wnck_workspace_update_role in libwnck */
  int           error;
  blrange_t     *role_range;
  blrange_t	*disp_range;
  m_label_t *mlabel = NULL;
  min_role_label = NULL;

  /* validate the label passed */

  if (libtsol_str_to_label (label, &mlabel, MAC_LABEL, L_NO_CORRECTION, &error) < 0) 
    {
      g_warning("Could not validate sensitivity label \"%s\"", label);
      g_free (role_range);
      return FALSE;
    }

  /* 
   * This is a role workspace so we need to construct the correct label range
   * instead of relying on USER_MIN_SL and USER_MAX_SL
   */
  if ((role_range = libtsol_getuserrange (username)) == NULL) 
    {
      g_warning ("Couldn't get label range for %s\n", username);
      return FALSE;
    }

  /* Get display device's range */
  if ((disp_range = get_display_range ()) == NULL) 
    {
      g_warning ("Couldn't get the display's device range");
      return FALSE;
    }

  /*
   * Determine the low & high bound of the label range
   * where the role user can operate. This is the
   * intersection of display label range & role label
   * range.
   */
  libtsol_blmaximum (role_range->lower_bound, disp_range->lower_bound);
  libtsol_blminimum (role_range->upper_bound, disp_range->upper_bound);

  libtsol_blabel_free (disp_range->lower_bound);
  libtsol_blabel_free (disp_range->upper_bound);
  g_free (disp_range);


  /* check if in range */

  if (!libtsol_blinrange (mlabel, role_range)) 
    {
      libtsol_m_label_free (mlabel);
      libtsol_label_to_str (role_range->lower_bound, &min_role_label, M_INTERNAL, DEF_NAMES, &error);
      libtsol_blabel_free (role_range->lower_bound);
      libtsol_blabel_free (role_range->upper_bound);
      g_free (role_range);
      return FALSE;
    }

  libtsol_blabel_free (role_range->lower_bound);
  libtsol_blabel_free (role_range->upper_bound);
  g_free (role_range);

  libtsol_m_label_free (mlabel);

  return TRUE;
}

/* boolean is used to select between label or roles */
static void 
set_workspace_tsol_properties (MetaScreen *screen, gboolean label)
{
  GString *flattened;
  int i;
  int n_spaces;

  /* flatten to nul-separated list */
  n_spaces = meta_screen_get_n_workspaces (screen);
  flattened = g_string_new ("");
  i = 0;
  while (i < n_spaces)
    {
      const char *name;
	      

      if (label)
	{
	  name = meta_prefs_get_workspace_label (i);
	  

	  if (!tsol_meta_workspace_has_role (meta_screen_get_workspace_by_index (screen, i)))
	    {
	      /* default to min label range if the workspace label isn't defined */
	      /* printf ("set min label on a workspace (%d) that as a role !\n", i); */
	      if (name == NULL) 
		name = tsol_label_get_min ();
	      if (!tsol_label_is_in_user_range (name))
		{
		  name = tsol_label_get_min ();
		}
	    }
	}
      else
	name = meta_prefs_get_workspace_role (i);
	

      if (name)
        g_string_append_len (flattened, name,
                             strlen (name) + 1);
      else
        g_string_append_len (flattened, "", 1);
      
      ++i;
     
    }
 

  
  
  meta_error_trap_push (screen->display);
  XChangeProperty (screen->display->xdisplay,
                   screen->xroot,
                   label ? screen->display->atom__NET_DESKTOP_LABELS : screen->display->atom__NET_DESKTOP_ROLES,
		   screen->display->atom_UTF8_STRING,
                   8, PropModeReplace,
		   (const unsigned char *)flattened->str, flattened->len);
  meta_error_trap_pop (screen->display, FALSE);
  
  g_string_free (flattened, TRUE);
}

void 
tsol_workspace_labels_atom_set (MetaScreen *screen)
{
  /* This updates label names on root window when the pref changes,
   * note we only get prefs change notify if things have
   * really changed.
   */
  if (tsol_is_available ())
    set_workspace_tsol_properties (screen, TRUE);
}
  
void tsol_workspace_roles_atom_set (MetaScreen *screen)
{
  /* This updates roles names on root window when the pref changes,
   * note we only get prefs change notify if things have
   * really changed.
   */
  if (tsol_is_available ())
    set_workspace_tsol_properties (screen, FALSE);
}


void
tsol_workspace_labels_gconf_update (MetaScreen *screen)
{
  char **names;
  int n_names;
  int i;

  if (!tsol_is_available ())
    return;

  /* this updates names in prefs when the root window property changes,
   * iff the new property contents don't match what's already in prefs
   */

  
  
  names = NULL;
  n_names = 0;
  if (!meta_prop_get_utf8_list (screen->display,
                                screen->xroot,
                                screen->display->atom__NET_DESKTOP_LABELS,
                                &names, &n_names))
    {
      meta_verbose ("Failed to get workspace label from root window %d\n",
                    screen->number);
      return;
    }

  i = 0;
  while (i < n_names)
    {
     /* Check if the label is in range if not set it to USER_MIN_SL 
      * NOTE : if USER_MIN_SL is not properly set you can have an infinite loop here */
      if (names[i] && !tsol_label_is_in_user_range (names[i]))
	{
	  if (!tsol_meta_workspace_has_role (meta_screen_get_workspace_by_index (screen, i)))
	    {
	      g_free (names[i]);
	      names[i] = g_strdup (tsol_label_get_min ());
	    }
	  else
	    {
	      char *min_role_label = NULL;
	      char *role = tsol_meta_workspace_get_role (meta_screen_get_workspace_by_index (screen, i));
	      if (!tsol_label_is_in_role_range (names[i], role, min_role_label))
		{
		  if (min_role_label)
		      names[i] = min_role_label;
		  else
		      names[i] = g_strdup (tsol_label_get_min ());
		}
	    }
	}
	  
      meta_topic (META_DEBUG_PREFS,
                  "Setting workspace label %d name to \"%s\" due to _NET_DESKTOP_LABELS change ROLE workspace :%s \n",
                  i, names[i] ? names[i] : "null",
		  tsol_meta_workspace_has_role (meta_screen_get_workspace_by_index (screen, i)) ? "TRUE" : "FALSE");
      
           
           
	
      meta_prefs_change_workspace_label (i, names[i]);
      
      ++i;
    }
  
  g_strfreev (names);
}

void
tsol_workspace_roles_gconf_update (MetaScreen *screen)
{
  char **names;
  int n_names;
  int i;
  
  if (!tsol_is_available ())
    return;
 

  /* this updates names in prefs when the root window property changes,
   * iff the new property contents don't match what's already in prefs
   */
  
  names = NULL;
  n_names = 0;
  if (!meta_prop_get_utf8_list (screen->display,
                                screen->xroot,
                                screen->display->atom__NET_DESKTOP_ROLES,
                                &names, &n_names))
    {
      meta_verbose ("Failed to get workspace roles from root window %d\n",
                    screen->number);
      return;
    }

  i = 0;
  while (i < n_names)
    {
      meta_topic (META_DEBUG_PREFS,
                  "Setting workspace roles %d name to \"%s\" due to _NET_DESKTOP_ROLES change\n",
                  i, names[i] ? names[i] : "null");
      meta_prefs_change_workspace_role (i, names[i]);
      
      ++i;
    }
  
  g_strfreev (names);
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
tsol_get_highlight_stripe (char     *name,
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
							  tabpopup_highlight, 
							  TRUE, NULL);
  
  libgnome_tsol_constraint_image_set_border (hl_stripe->image, 8, 8, 3, 3);
  libgnome_tsol_constraint_image_set_stretch (hl_stripe->image, TRUE);
  libgnome_tsol_constraint_image_colorize (hl_stripe->image, label_color, 255, TRUE);
  
  hl_stripe_list = g_slist_append (hl_stripe_list, hl_stripe);
  return hl_stripe->image; 
}


#endif
