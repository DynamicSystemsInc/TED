/* workspace object */
/* vim: set sw=2 et: */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2003 Kim Woelders
 * Copyright (C) 2003 Red Hat, Inc.
 * Copyright (C) 2006-2007 Vincent Untz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <glib/gi18n-lib.h>
#ifdef HAVE_XTSOL
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <user_attr.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <libgnometsol/userattr.h>
#include <wnck-tsol.h>
#endif
#include "workspace.h"
#include "xutils.h"
#include "private.h"
#include <string.h>

/**
 * SECTION:workspace
 * @short_description: an object representing a workspace.
 * @see_also: #WnckScreen
 * @stability: Unstable
 *
 * The #WnckWorkspace represents what is called <ulink
 * url="http://standards.freedesktop.org/wm-spec/wm-spec-latest.html&num;largedesks">virtual
 * desktops</ulink> in the <ulink
 * url="http://standards.freedesktop.org/wm-spec/wm-spec-latest.html">EWMH</ulink>.
 * A workspace is a virtualization of a #WnckScreen<!-- -->: only one workspace
 * can be shown on a #WnckScreen at a time. It makes it possible, for example,
 * to put windows on different workspaces to organize them.
 *
 * If the #WnckWorkspace size is bigger that the #WnckScreen size, the
 * workspace contains a viewport. Viewports are defined in the <ulink
 * url="http://standards.freedesktop.org/wm-spec/wm-spec-latest.html&num;id2457064">large
 * desktops</ulink> section of the <ulink
 * url="http://standards.freedesktop.org/wm-spec/wm-spec-latest.html">EWMH</ulink>.
 * The notion of workspaces and viewports are quite similar, and generally both
 * are not used at the same time: there are generally either multiple
 * workspaces with no viewport, or one workspace with a viewport. libwnck
 * supports all situations, even multiple workspaces with viewports.
 *
 * Workspaces are organized according to a layout set on the #WnckScreen. See
 * wnck_screen_try_set_workspace_layout() and
 * wnck_screen_release_workspace_layout() for more information about the
 * layout.
 *
 * The #WnckWorkspace objects are always owned by libwnck and must not be
 * referenced or unreferenced.
 */

struct _WnckWorkspacePrivate
{
  WnckScreen *screen;
  int number;
  char *name;
#ifdef HAVE_XTSOL  /* TSOL */
  char *label; /* Workspace sensitivity label */
  char *role;  /* Workspace role : login name. Set only once */
  /* The workspace range can differ for Workstation Owner and role workspaces */
  const blrange_t *ws_range;
#endif

  int width, height;            /* Workspace size */
  int viewport_x, viewport_y;   /* Viewport origin */
  gboolean is_virtual;
};

G_DEFINE_TYPE (WnckWorkspace, wnck_workspace, G_TYPE_OBJECT);
#define WNCK_WORKSPACE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), WNCK_TYPE_WORKSPACE, WnckWorkspacePrivate))

enum {
  NAME_CHANGED,
#ifdef HAVE_XTSOL
  LABEL_CHANGED,
  ROLE_CHANGED,
#endif
  LAST_SIGNAL
};

static void wnck_workspace_init        (WnckWorkspace      *workspace);
static void wnck_workspace_class_init  (WnckWorkspaceClass *klass);
static void wnck_workspace_finalize    (GObject        *object);


static void emit_name_changed (WnckWorkspace *space);

static guint signals[LAST_SIGNAL] = { 0 };

#ifdef HAVE_XTSOL
static void emit_label_changed (WnckWorkspace *space);
static void emit_role_changed (WnckWorkspace *space);
static blrange_t * get_display_range (void);
#endif

static void
wnck_workspace_init (WnckWorkspace *workspace)
{
  workspace->priv = WNCK_WORKSPACE_GET_PRIVATE (workspace);

  workspace->priv->screen = NULL;
  workspace->priv->number = -1;
  workspace->priv->name = NULL;
  workspace->priv->width = 0;
  workspace->priv->height = 0;
  workspace->priv->viewport_x = 0;
  workspace->priv->viewport_y = 0;
  workspace->priv->is_virtual = FALSE;
#ifdef HAVE_XTSOL
  workspace->priv->ws_range = NULL;
#endif
}

static void
wnck_workspace_class_init (WnckWorkspaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (WnckWorkspacePrivate));

  object_class->finalize = wnck_workspace_finalize;

  /**
   * WnckWorkspace::name-changed:
   * @space: the #WnckWorkspace which emitted the signal.
   *
   * Emitted when the name of @space changes.
   */
  signals[NAME_CHANGED] =
    g_signal_new ("name_changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (WnckWorkspaceClass, name_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
#ifdef HAVE_XTSOL
  signals[LABEL_CHANGED] =
    g_signal_new ("label_changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (WnckWorkspaceClass, label_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  signals[ROLE_CHANGED] =
    g_signal_new ("role_changed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (WnckWorkspaceClass, role_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
#endif
}

static void
wnck_workspace_finalize (GObject *object)
{
  WnckWorkspace *workspace;

  workspace = WNCK_WORKSPACE (object);

  g_free (workspace->priv->name);
  workspace->priv->name = NULL;

#ifdef HAVE_XTSOL
  g_free (workspace->priv->role);
  g_free (workspace->priv->label);
#endif
  
  G_OBJECT_CLASS (wnck_workspace_parent_class)->finalize (object);
}

/**
 * wnck_workspace_get_number:
 * @space: a #WnckWorkspace.
 * 
 * Gets the index of @space on the #WnckScreen to which it belongs. The
 * first workspace has an index of 0.
 * 
 * Return value: the index of @space on its #WnckScreen, or -1 on errors.
 **/
int
wnck_workspace_get_number (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), -1);

  return space->priv->number;
}

/**
 * wnck_workspace_get_name:
 * @space: a #WnckWorkspace.
 *
 * Gets the human-readable name that should be used to refer to @space. If
 * the user has not set a special name, a fallback like "Workspace 3" will be
 * used.
 *
 * Return value: the name of @space.
 **/
const char*
wnck_workspace_get_name (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);

  return space->priv->name;
}

#ifdef HAVE_XTSOL
/**
 * wnck_workspace_get_label_range:
 * @space: a #WnckWorkspace
 * @min_label: a string pointer to pointer to the minimum valid label value for @space
 * @max_label: a string pointer to pointer to the maximum valid label value for @space
 *
 * Gets the sensitivity label range for the specified workspace when
 * running in a label aware desktop session. @min_label represents the minimum
 * sensitivity label that the #WnckWorkspace, @space, may be assigned.
 * @max_label represents the maximum sensitivity label thatthe #WnckWorkspace,
 * @space may be assigned. Both min_label and max_label are allocated memory
 * on behalf of the caller. It is the caller's responsibility to free the memory
 * pointed to by @min_label and @max_label.
 *
 * Return value: 0 on success, non zero on failure.
 **/
int
wnck_workspace_get_label_range (WnckWorkspace *space, char **min_label, char **max_label)
{
  int error = 0;
  blrange_t *brange;
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), -1);

  if (! _wnck_check_xtsol_extension ())
      return -1;

  if (!_wnck_use_trusted_extensions())
      return -1;

  brange = _wnck_workspace_get_range (space);
  if (!brange)
      return -1;

  if (libtsol_label_to_str (brange->lower_bound, min_label, M_INTERNAL,
                        LONG_NAMES) != 0) {
      g_warning ("wnck_workspace_get_label_range: Workspace has an invalid minimum label bound");
      return -1;
  }

  if (libtsol_label_to_str (brange->upper_bound, max_label, M_INTERNAL,
                        LONG_NAMES) != 0) {
      g_warning ("wnck_workspace_get_label_range: Workspace has an invalid maximum label bound");
      return -1;
  }
  return 0;
}

/**
 * wnck_workspace_get_label:
 * @space: a #WnckWorkspace
 *
 * Gets the sensitivity label as an hex number for the specified 
 * workspace when running in a label aware desktop session.
 *
 * Return value: workspace sensitivity label, %NULL on failure.
 **/
const char*
wnck_workspace_get_label (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);
  /* A bit anal perhaps but I'd rather make sure nothing useful is returned */
  if (! (_wnck_check_xtsol_extension() && _wnck_use_trusted_extensions()) )
    return NULL;
  return space->priv->label;
}

/**
 * wnck_workspace_get_human_readable_label:
 * @space: a #WnckWorkspace
 *
 * Gets the sensitivity label as a string for the specified workspace when
 * running in a label aware desktop session.
 * 
 *
 * Return value: workspace sensitivity label, %NULL on failure.
 **/
char*
wnck_workspace_get_human_readable_label (WnckWorkspace *space)
{
  const char *hex_label;
  char *human_readable_label;
  int error;
  m_label_t *mlabel = NULL;
  
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);
  if (! (_wnck_check_xtsol_extension() && _wnck_use_trusted_extensions()) )
    return NULL;

  hex_label = space->priv->label;

  if (hex_label && (error = libtsol_str_to_label (hex_label, &mlabel, MAC_LABEL, 
			    L_NO_CORRECTION, &error) == 0))
    {
      error = libtsol_label_to_str (mlabel, 
			    &human_readable_label, 
			    M_LABEL, DEF_NAMES);
      m_label_free(mlabel);
      if (strcmp (human_readable_label, "ADMIN_HIGH") == 0 ||
	  strcmp (human_readable_label, "ADMIN_LOW") == 0) {
	/* SUN_BRANDING TJDS */
        free(human_readable_label);
	return g_strdup ("Trusted Path");
      } else {
        return human_readable_label;
      }
    }
  return NULL;
}


/**
 * wnck_workspace_get_role:
 * @space: a #WnckWorkspace
 *
 * Gets the role (login name) for the specified workspace when
 * running in a trusted desktop session.
 *
 * Return value: workspace user role, %NULL on failure.
 **/
const char*
wnck_workspace_get_role (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);
  /* Make sure to return NULL for non-tsol */
  if (! (_wnck_check_xtsol_extension() && _wnck_use_trusted_extensions()) )
    return NULL;
  return space->priv->role;
}
#endif /* HAVE_XTSOL */


/**
 * wnck_workspace_change_name:
 * @space: a #WnckWorkspace.
 * @name: new name for @space.
 *
 * Changes the name of @space.
 *
 * Since: 2.2
 **/
void
wnck_workspace_change_name (WnckWorkspace *space,
                            const char    *name)
{
  g_return_if_fail (WNCK_IS_WORKSPACE (space));
  g_return_if_fail (name != NULL);

  _wnck_screen_change_workspace_name (space->priv->screen,
                                      space->priv->number,
                                      name);
}

/**
 * wnck_workspace_get_screen:
 * @space: a #WnckWorkspace.
 *
 * Gets the #WnckScreen @space is on.
 *
 * Return value: (transfer none): the #WnckScreen @space is on. The returned
 * #WnckScreen is owned by libwnck and must not be referenced or unreferenced.
 **/
WnckScreen*
wnck_workspace_get_screen (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);

  return space->priv->screen;
}

#ifdef HAVE_XTSOL
/**
 * wnck_workspace_change_label:
 * @space: a #WnckWorkspace
 * @label: new workspace sensitivity label
 *
 * Try changing the sensitivity label of the workspace.
 *
 **/

void
wnck_workspace_change_label (WnckWorkspace *space,
                             const char    *label)
{
  g_return_if_fail (WNCK_IS_WORKSPACE (space));
  g_return_if_fail (label != NULL);

  _wnck_screen_change_workspace_label (space->priv->screen,
                                       space->priv->number,
                                       label);
}
#endif

/**
 * wnck_workspace_activate:
 * @space: a #WnckWorkspace.
 * @timestamp: the X server timestamp of the user interaction event that caused
 * this call to occur.
 *
 * Asks the window manager to make @space the active workspace. The window
 * manager may decide to refuse the request (to not steal the focus if there is
 * a more recent user activity, for example).
 *
 * This function existed before 2.10, but the @timestamp argument was missing
 * in earlier versions.
 *
 * Since: 2.10
 **/
void
wnck_workspace_activate (WnckWorkspace *space,
                         guint32        timestamp)
{
  g_return_if_fail (WNCK_IS_WORKSPACE (space));

  _wnck_activate_workspace (WNCK_SCREEN_XSCREEN (space->priv->screen),
                            space->priv->number,
                            timestamp);
}

WnckWorkspace*
_wnck_workspace_create (int number, WnckScreen *screen)
{
  WnckWorkspace *space;

  space = g_object_new (WNCK_TYPE_WORKSPACE, NULL);
  space->priv->number = number;
  space->priv->name = NULL;
  space->priv->screen = screen;

  _wnck_workspace_update_name (space, NULL);
/* FIXME - do label and role need to be updated here? */  
  
  /* Just set reasonable defaults */
  space->priv->width = wnck_screen_get_width (screen);
  space->priv->height = wnck_screen_get_height (screen);
  space->priv->is_virtual = FALSE;

  space->priv->viewport_x = 0;
  space->priv->viewport_y = 0;

  return space;
}

void
_wnck_workspace_update_name (WnckWorkspace *space,
                             const char    *name)
{
  char *old;

  g_return_if_fail (WNCK_IS_WORKSPACE (space));

  old = space->priv->name;
  space->priv->name = g_strdup (name);

  if (space->priv->name == NULL)
    space->priv->name = g_strdup_printf (_("Workspace %d"),
                                         space->priv->number + 1);

  if ((old && !name) ||
      (!old && name) ||
      (old && name && strcmp (old, name) != 0))
    emit_name_changed (space);

  g_free (old);
}

#ifdef HAVE_XTSOL

static char*
get_workstationowner (void)
{
  static char *workstationowner = NULL;
  uid_t wsuid;
  struct passwd *pwd;

  if (workstationowner == NULL) {
    if ((libxtsol_XTSOLgetWorkstationOwner (gdk_x11_get_default_xdisplay (),
					    &wsuid)) < 0) {
      g_warning ("XTSOLgetWorkstationOwner() failed. Using getuid() instead");
      pwd = getpwuid (getuid ());
    } else {
      pwd = getpwuid (wsuid);
    }
    
    workstationowner = g_strdup (pwd->pw_name);
  }
 
  return workstationowner;
}

void
_wnck_workspace_update_label (WnckWorkspace *space,
                              const char    *label)
{
  char *old;

  g_return_if_fail (WNCK_IS_WORKSPACE (space));
  /* Don't do anything unless this is a trusted system */
  if (!(_wnck_check_xtsol_extension() && _wnck_use_trusted_extensions()))
      return;

  /* Should a warning be called here? */
  if (label == NULL)
	g_warning("Workspace %d label is null\n",
                   wnck_workspace_get_number (space));

  old = space->priv->label;
  space->priv->label = g_strdup (label);

  /*
   *Initialise the label range for this workspace
   */

  if (!space->priv->ws_range) {
      if ((!space->priv->role) || (strlen (space->priv->role) == 0) ||
	   (strcmp (space->priv->role, get_workstationowner ()) == 0)) {
          blrange_t		*range;
          int error;
          char *min_label = NULL;
          char *max_label = NULL;
          range = g_malloc (sizeof (blrange_t));
          range->lower_bound = range->upper_bound = NULL;
     
          min_label = g_strdup (_wnck_get_min_label ());
          max_label = g_strdup (_wnck_get_max_label ());

          if (libtsol_str_to_label (min_label, &(range->lower_bound),
                                    MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
              g_warning ("Couldn't determine minimum workspace label");
              g_free (min_label);
              g_free (max_label);
              return;
          }
          if (libtsol_str_to_label (max_label, &(range->upper_bound),
                                    USER_CLEAR, L_NO_CORRECTION, &error) < 0) {
              g_warning ("Couldn't determine workspace clearance");
              g_free (min_label);
              g_free (max_label);
              return;
          }
          space->priv->ws_range = range;
          g_free (min_label);
          g_free (max_label);

      } else {
          int           error;
          blrange_t     *role_range;
          blrange_t		*disp_range;
          userattr_t	*u_ent; 
          /* 
           * This is a role workspace so we need to construct the correct label range
           * instead of relying on USER_MIN_SL and USER_MAX_SL
           */
          if ((role_range = libtsol_getuserrange (space->priv->role)) == NULL) {
              g_warning ("Couldn't get label range for %s\n", space->priv->role);
              return;
          }

    	  /* Get display device's range */
    	  if ((disp_range = get_display_range ()) == NULL) {
    		  g_warning ("Couldn't get the display's device range");
    		  return;
    	  }

          /*
           * Determine the low & high bound of the label range
           * where the role user can operate. This is the
           * intersection of display label range & role label
           * range.
           */
          libtsol_blmaximum (role_range->lower_bound, disp_range->lower_bound);
          libtsol_blminimum (role_range->upper_bound, disp_range->upper_bound);
          space->priv->ws_range = role_range;
          libtsol_blabel_free (disp_range->lower_bound);
          libtsol_blabel_free (disp_range->upper_bound);
          free (disp_range);
      }
  }
  /* Should we put a g_warning here? */
  /* if (space->priv->label == NULL) */

  if ((!old && label) ||
      (old && label && strcmp (old, label) != 0))
    emit_label_changed (space);

  g_free (old);
}

void
_wnck_workspace_update_role (WnckWorkspace *space,
                             const char    *role)
{
	char *workstationowner = NULL;
	char *old;

	g_return_if_fail (WNCK_IS_WORKSPACE (space));
	/* Check for a multi label trusted environment first */
	if (!(_wnck_check_xtsol_extension() && _wnck_use_trusted_extensions()))
		return;
	workstationowner = get_workstationowner ();

	if (role == NULL)
		return;
	old = space->priv->role;

	/* Check the the workspace role really is changing */
	if ((!old && role) ||
		(old && role && strcmp (old, role) != 0)) {
		g_free (space->priv->role);
		if (strlen (role) ==0)
			{ space->priv->role = g_strdup (workstationowner); return;}
		else
			space->priv->role = g_strdup (role);

		/*
		 * A role change requires that the label range of the 
		 * workspace be reset. The label also needs to be 
		 * silently set to the lowest in the range.
		 */

		if (space->priv->ws_range) {
			libtsol_blabel_free (space->priv->ws_range->lower_bound);
			libtsol_blabel_free (space->priv->ws_range->upper_bound);
			/* FIXME - man pages tell me to use free but generates a compiler warning */
			free ((void *)space->priv->ws_range);
		}

		if (strcmp (space->priv->role, workstationowner) == 0) {
			/* Workstation owner, so it's not a real role */
			blrange_t		*range;
			int error;
			char *min_label = NULL;
			char *max_label = NULL;
			range = g_malloc (sizeof (blrange_t));
			range->lower_bound = range->upper_bound = NULL;
 
			min_label = g_strdup (_wnck_get_min_label ());
			max_label = g_strdup (_wnck_get_max_label ());

			/* Workspace label must be reset by default to the min_label value */
			if (space->priv->label)
				g_free (space->priv->label);
				space->priv->label = g_strdup (min_label);  

			if (libtsol_str_to_label (min_label, &(range->lower_bound),
					MAC_LABEL, L_NO_CORRECTION, &error) < 0) {
				g_warning ("Couldn't determine minimum workspace label");
				g_free (min_label);
				g_free (max_label);
				return;
			}

			if (libtsol_str_to_label (max_label, &(range->upper_bound),
				USER_CLEAR, L_NO_CORRECTION, &error) < 0) {
				g_warning ("Couldn't determine workspace clearance");
				g_free (min_label);
				g_free (max_label);
				return;
    	  	}

			space->priv->ws_range = range;
			g_free (min_label);
			g_free (max_label);

		} else {
			int           error;
			blrange_t     *role_range;
			blrange_t		*disp_range;
			userattr_t	*u_ent;

			/* 
			 * This is a role workspace so we need to construct the correct label range
			 * instead of relying on USER_MIN_SL and USER_MAX_SL
			 */
			if ((role_range = libtsol_getuserrange (space->priv->role)) == NULL) {
				g_warning ("Couldn't get label range for %s\n", space->priv->role);
				return;
			}

			/* Get display device's range */
			if ((disp_range = get_display_range ()) == NULL) {
				g_warning ("Couldn't get the display's device range");
				return;
			}

			/*
			 * Determine the low & high bound of the label range
			 * where the role user can operate. This is the
			 * intersection of display label range & role label
			 * range.
			 */
			libtsol_blmaximum (role_range->lower_bound, disp_range->lower_bound);
			libtsol_blminimum (role_range->upper_bound, disp_range->upper_bound);
			space->priv->ws_range = role_range;

			/* Workspace label must be reset by default to the lower_bound value */
			if (libtsol_label_to_str (role_range->lower_bound, &space->priv->label, M_INTERNAL,
					LONG_NAMES) != 0) {
				/* Weird - default to admin_low */
        		space->priv->label = g_strup ("ADMIN_LOW");
			}

			libtsol_blabel_free (disp_range->lower_bound);
			libtsol_blabel_free (disp_range->upper_bound);
			free (disp_range);
		}
    	emit_role_changed (space);
	}
}
#endif /* HAVE_XTSOL */

static void
emit_name_changed (WnckWorkspace *space)
{
  g_signal_emit (G_OBJECT (space),
                 signals[NAME_CHANGED],
                 0);
}

#ifdef HAVE_XTSOL
static void
emit_label_changed (WnckWorkspace *space)
{
  g_signal_emit (G_OBJECT (space),
                 signals[LABEL_CHANGED],
                 0);
  wnck_screen_emit_labels_changed (space->priv->screen);
}

static void
emit_role_changed (WnckWorkspace *space)
{
  g_signal_emit (G_OBJECT (space),
                 signals[ROLE_CHANGED],
                 0);
  wnck_screen_emit_roles_changed (space->priv->screen);
}
#endif /* HAVE_XTSOL */

gboolean
_wnck_workspace_set_geometry (WnckWorkspace *space,
                              int            w,
                              int            h)
{
  if (space->priv->width != w || space->priv->height != h)
    {
      space->priv->width = w;
      space->priv->height = h;

      space->priv->is_virtual = w > wnck_screen_get_width (space->priv->screen) ||
				h > wnck_screen_get_height (space->priv->screen);

      return TRUE;  /* change was made */
    }
  else
    return FALSE;
}

gboolean
_wnck_workspace_set_viewport (WnckWorkspace *space,
                              int            x,
                              int            y)
{
  if (space->priv->viewport_x != x || space->priv->viewport_y != y)
    {
      space->priv->viewport_x = x;
      space->priv->viewport_y = y;

      return TRUE; /* change was made */
    }
  else
    return FALSE;
}

/**
 * wnck_workspace_get_width:
 * @space: a #WnckWorkspace.
 *
 * Gets the width of @space.
 *
 * Returns: the width of @space.
 *
 * Since: 2.4
 */
int
wnck_workspace_get_width (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), 0);

  return space->priv->width;
}

/**
 * wnck_workspace_get_height:
 * @space: a #WnckWorkspace.
 *
 * Gets the height of @space.
 *
 * Returns: the height of @space.
 *
 * Since: 2.4
 */
int
wnck_workspace_get_height (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), 0);

  return space->priv->height;
}

/**
 * wnck_workspace_get_viewport_x:
 * @space: a #WnckWorkspace.
 *
 * Gets the X coordinate of the viewport in @space.
 *
 * Returns: the X coordinate of the viewport in @space, or 0 if @space does not
 * contain a viewport.
 *
 * Since: 2.4
 */
int
wnck_workspace_get_viewport_x (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), 0);

  return space->priv->viewport_x;
}

/**
 * wnck_workspace_get_viewport_y:
 * @space: a #WnckWorkspace.
 *
 * Gets the Y coordinate of the viewport in @space.
 *
 * Returns: the Y coordinate of the viewport in @space, or 0 if @space does not
 * contain a viewport.
 *
 * Since: 2.4
 */
int
wnck_workspace_get_viewport_y (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), 0);

  return space->priv->viewport_y;
}

/**
 * wnck_workspace_is_virtual:
 * @space: a #WnckWorkspace.
 *
 * Gets whether @space contains a viewport.
 *
 * Returns: %TRUE if @space contains a viewport, %FALSE otherwise.
 *
 * Since: 2.4
 */
gboolean
wnck_workspace_is_virtual (WnckWorkspace *space)
{
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), FALSE);

  return space->priv->is_virtual;
}

/**
 * wnck_workspace_get_layout_row:
 * @space: a #WnckWorkspace.
 *
 * Gets the row of @space in the #WnckWorkspace layout. The first row has an
 * index of 0 and is always the top row, regardless of the starting corner set
 * for the layout.
 *
 * Return value: the row of @space in the #WnckWorkspace layout, or -1 on
 * errors.
 *
 * Since: 2.20
 **/
int
wnck_workspace_get_layout_row (WnckWorkspace *space)
{
  _WnckLayoutOrientation orientation;
  _WnckLayoutCorner corner;
  int n_rows;
  int n_cols;
  int row;

  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), -1);

  _wnck_screen_get_workspace_layout (space->priv->screen,
                                     &orientation, &n_rows, &n_cols, &corner);

  if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
    row = space->priv->number / n_cols;
  else
    row = space->priv->number % n_rows;

  if (corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT ||
      corner == WNCK_LAYOUT_CORNER_BOTTOMLEFT)
    row = n_rows - row;

  return row;
}

/**
 * wnck_workspace_get_layout_column:
 * @space: a #WnckWorkspace.
 *
 * Gets the column of @space in the #WnckWorkspace layout. The first column
 * has an index of 0 and is always the left column, regardless of the starting
 * corner set for the layout and regardless of the default direction of the
 * environment (i.e., in both Left-To-Right and Right-To-Left environments).
 *
 * Return value: the column of @space in the #WnckWorkspace layout, or -1 on
 * errors.
 *
 * Since: 2.20
 **/
int
wnck_workspace_get_layout_column (WnckWorkspace *space)
{
  _WnckLayoutOrientation orientation;
  _WnckLayoutCorner corner;
  int n_rows;
  int n_cols;
  int col;

  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), -1);

  _wnck_screen_get_workspace_layout (space->priv->screen,
                                     &orientation, &n_rows, &n_cols, &corner);

  if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
    col = space->priv->number % n_cols;
  else
    col = space->priv->number / n_rows;

  if (corner == WNCK_LAYOUT_CORNER_TOPRIGHT ||
      corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT)
    col = n_cols - col;

  return col;
}

/**
 * wnck_workspace_get_neighbor:
 * @space: a #WnckWorkspace.
 * @direction: direction in which to search the neighbor.
 *
 * Gets the neighbor #WnckWorkspace of @space in the @direction direction.
 *
 * Return value: (transfer none): the neighbor #WnckWorkspace of @space in the
 * @direction direction, or %NULL if no such neighbor #WnckWorkspace exists.
 * The returned #WnckWorkspace is owned by libwnck and must not be referenced
 * or unreferenced.
 *
 * Since: 2.20
 **/
WnckWorkspace*
wnck_workspace_get_neighbor (WnckWorkspace       *space,
                             WnckMotionDirection  direction)
{
  _WnckLayoutOrientation orientation;
  _WnckLayoutCorner corner;
  int n_rows;
  int n_cols;
  int row;
  int col;
  int add;
  int index;

  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), NULL);

  _wnck_screen_get_workspace_layout (space->priv->screen,
                                     &orientation, &n_rows, &n_cols, &corner);

  row = wnck_workspace_get_layout_row (space);
  col = wnck_workspace_get_layout_column (space);

  index = space->priv->number;

  switch (direction)
    {
    case WNCK_MOTION_LEFT:
      if (col == 0)
        return NULL;

      if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
        add = 1;
      else
        add = n_rows;

      if (corner == WNCK_LAYOUT_CORNER_TOPRIGHT ||
          corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT)
        index += add;
      else
        index -= add;
      break;

    case WNCK_MOTION_RIGHT:
      if (col == n_cols - 1)
        return NULL;

      if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
        add = 1;
      else
        add = n_rows;

      if (corner == WNCK_LAYOUT_CORNER_TOPRIGHT ||
          corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT)
        index -= add;
      else
        index += add;
      break;

    case WNCK_MOTION_UP:
      if (row == 0)
        return NULL;

      if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
        add = n_cols;
      else
        add = 1;

      if (corner == WNCK_LAYOUT_CORNER_BOTTOMLEFT ||
          corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT)
        index += add;
      else
        index -= add;
      break;

    case WNCK_MOTION_DOWN:
      if (row == n_rows - 1)
        return NULL;

      if (orientation == WNCK_LAYOUT_ORIENTATION_HORIZONTAL)
        add = n_cols;
      else
        add = 1;

      if (corner == WNCK_LAYOUT_CORNER_BOTTOMLEFT ||
          corner == WNCK_LAYOUT_CORNER_BOTTOMRIGHT)
        index -= add;
      else
        index += add;
      break;
    }

  if (index == space->priv->number)
    return NULL;

  return wnck_screen_get_workspace (space->priv->screen, index);
}

#ifdef HAVE_XTSOL
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

blrange_t *
_wnck_workspace_get_range (WnckWorkspace *space)
{
  return space->priv->ws_range;
}
#endif
