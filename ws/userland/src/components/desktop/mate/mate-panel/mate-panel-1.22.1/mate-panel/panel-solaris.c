/*
 * Note that the following code is in three patches:
 * - gnome-panel-XX-rbac.diff (filter_with_rbac)
 * - gnome-menus-XX-rbac.diff (filter_with_rbac)
 * - glib-XX-gio-rbac.diff    (get_gksu_role)
 * - gnome-session-XX-rbac.diff (get_gksu_role)
 *
 * So if there is a need to fix this code, it is probably necessary to fix the
 * code in these other two places as well.  Though the functions are a bit
 * different.
 */
#include <config.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>
#include <user_attr.h>
#include <exec_attr.h>
#include <secdb.h>

#include "panel-solaris.h"

#define ATOM "_LABEL_EXEC_COMMAND"

typedef Status (*xtsol_XTSOLgetWorkstationOwner) (Display *xpdy, uid_t *uidp);

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

static
void * dlopen_libwnck (void)
{
   void  *handle = NULL;

   if ((handle = dlopen ("/usr/lib/amd64/libwnck-3.so", RTLD_LAZY)) != NULL)
       return handle;

   return handle;
}

gboolean
use_trusted_extensions (void)
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
        static gpointer libwnck_handle = NULL;

        if (getenv ("TRUSTED_SESSION") == NULL) {
            trusted = 0;
            return 0;
        }

	libwnck_handle = dlopen_libwnck ();
        if (libwnck_handle != NULL)
            tsol_handle = dlopen_tsol ();
        if (tsol_handle != NULL)
            xtsol_handle = dlopen_xtsol ();
        if (libwnck_handle && tsol_handle && xtsol_handle) {

           libmenu_wnck_screen_get_default = (menu_wnck_screen_get_default) dlsym (libwnck_handle, "wnck_screen_get_default");
           libmenu_wnck_screen_get_active_workspace = (menu_wnck_screen_get_active_workspace) dlsym (libwnck_handle, "wnck_screen_get_active_workspace");
           libmenu_wnck_workspace_get_role = (menu_wnck_workspace_get_role) dlsym (libwnck_handle, "wnck_workspace_get_role");
           libmenu_wnck_workspace_get_label = (menu_wnck_workspace_get_label) dlsym (libwnck_handle, "wnck_workspace_get_label");

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
	   libtsol_getzonerootbylabel = (char *) dlsym (tsol_handle, "getzonerootbylabel");

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
               libxtsol_XTSOLIsWindowTrusted == NULL ||
               libmenu_wnck_screen_get_default == NULL ||
               libmenu_wnck_screen_get_active_workspace == NULL ||
               libmenu_wnck_workspace_get_role == NULL ||
               libmenu_wnck_workspace_get_label == NULL) {
               dlclose (tsol_handle);
               dlclose (xtsol_handle);
               dlclose (libwnck_handle);
               tsol_handle = NULL;
               xtsol_handle = NULL;
               libwnck_handle = NULL;
            }
        }

        gnometsol_handle = dlopen_gnometsol ();
        if (gnometsol_handle != NULL)
            {
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
        trusted = ((tsol_handle != NULL) && (xtsol_handle != NULL) && (gnometsol_handle != NULL) && (libwnck_handle != NULL)) ? 1 : 0;
    }
    return trusted ? TRUE : FALSE;
}

static gchar *
get_stripped_exec (const gchar *full_exec, gboolean use_global)
{
	gchar *str1, *str2, *retval, *p;
	char *zoneroot = NULL;
	gboolean trusted;

	str1 = g_strdup (full_exec);
	p = strtok (str1, " ");

	if (p != NULL)
		str2 = g_strdup (p);
	else
		str2 = g_strdup (full_exec);

	g_free (str1);

	trusted = use_trusted_extensions ();
	if (trusted && use_global == FALSE) {
		zoneroot = get_zoneroot ();
	}

	if (g_path_is_absolute (str2)) {
		if (zoneroot != NULL) {
			retval = g_strdup_printf ("%s/%s", zoneroot, str2);
		} else {
			retval = g_strdup (str2);
		}
	} else {
		if (zoneroot != NULL) {
			/*
			 * If the desktop file doesn't specify the full path
			 * and in Trusted mode, then check the zone's /usr/bin
			 * directory.
			 */
			retval = g_strdup_printf ("%s/usr/bin/%s", zoneroot, str2);
		} else {
			retval = g_strdup (g_find_program_in_path ((const gchar *)str2));

			/*
			 * If a program is not installed in the global zone,
			 * then assume it is installed in /usr/bin.
			 */
			if (use_global == TRUE && retval == NULL) {
				retval = g_strdup_printf ("/usr/bin/%s", str2);
			}
		}
	}
	g_free (str2);

	return retval;
}

/*
 * Checks RBAC to see if the user can run the command.
 */
gboolean
filter_with_rbac (gchar *command, gboolean use_global)
{
	execattr_t *exec;
	gchar *stripped_cmd;
	gchar *real_cmd;
	char *path;
	const char *username = NULL;
	userattr_t *user;
	int        i;
	gboolean   program_has_profile;
	gboolean   rc;
	gboolean   trusted;

	rc = TRUE;

	stripped_cmd = get_stripped_exec (command, TRUE);
	real_cmd     = get_stripped_exec (command, use_global);

	trusted = use_trusted_extensions ();
	if (trusted) {
		/*
		 * In trusted mode, use the single role associated with
		 * the workspace.
		 */
		gpointer wnckscreen = NULL;
		gpointer wnckworkspace = NULL;

		wnckscreen = libmenu_wnck_screen_get_default ();
		if (wnckscreen != NULL)
			wnckworkspace = libmenu_wnck_screen_get_active_workspace (wnckscreen);

		if (wnckworkspace != NULL)
			username = libmenu_wnck_workspace_get_role (wnckworkspace);
	}

	if (username == NULL) {
		username = g_get_user_name ();
	}

	/* If the command does not exist, do not show it. */
	if (real_cmd == NULL || stripped_cmd == NULL) {
		goto out;
	}

	path = g_find_program_in_path (g_strstrip (real_cmd));
	if (path == NULL)
		goto out;

	/*
	 * All programs should be available to root.  This check is done after
	 * verifying the binary is in path.
	 */
	if (strcmp (username, "root") == 0) {
		rc = FALSE;
		goto out;
	}

	/* Check if the program is in any profile. */
	program_has_profile = FALSE;
	exec = getexecprof (NULL, KV_COMMAND, stripped_cmd, GET_ONE);
	if (exec == NULL) {
		goto out;
	}

	while (exec != NULL) {
		if (exec->attr != NULL) {
			program_has_profile = TRUE;
			break;
		}
		exec = exec->next;
	}

	free_execattr (exec);

	/* Check if the user can run the command.  If not filter it. */
	exec = getexecuser (username, KV_COMMAND, stripped_cmd, GET_ONE);

	/*
	 * If the program is not associated with any profile, then do not
	 * show it.
	 */
	if (exec == NULL)
		goto out;

	/*
	 * If getexecuser does not return NULL and the program is not
	 * associated with any profile, then show it.  Otherwise, more
	 * tests are needed.
	 */
	if (use_global == TRUE || program_has_profile == FALSE) {
		rc = FALSE;
		free_execattr (exec);
		goto out;
	}

	/*
	 * If the user has a profile that can run the command, then it can
	 * be shown.
	 */
	while (exec != NULL) {
		if (exec->attr != NULL) {
			rc = FALSE;
			break;
		}
		exec = exec->next;
	}

	free_execattr (exec);

	if (rc == FALSE)
		goto out;

	if (!trusted) {
		/* If no gksu is available, then do not try to use it */
	        path = g_find_program_in_path ("/usr/bin/gksu");
		if (path == NULL)
			goto out;
	}

	/* Check if the user is in a role that can run the command. */
	/* If so, use gksu with that role */
	if ((user = getusernam (username)) != NULL) {
		const char *rolelist = NULL;
		char **v = NULL;
		char *role = NULL;

		if (trusted && username != NULL) {
			/* In trusted mode, use role associated with workspace */
			rolelist = username;
		} else {
			/* Otherwise use roles associated with the user. */
			rolelist = kva_match (user->attr, USERATTR_ROLES_KW);
		}

		if (rolelist != NULL)
			v = g_strsplit (rolelist, ",", -1);

		for (i=0; v != NULL && v[i] != NULL; i++) {
			role = g_strdup (v[i]);
			g_strstrip (role);

			exec = getexecuser (role, KV_COMMAND, stripped_cmd, GET_ONE);
			while (exec != NULL) {
				if ((strcmp (role, "root") == 0) ||
				    (exec->attr != NULL)) {
					rc = FALSE;
					break;
				}
				exec = exec->next;
			}

			g_free (role);
			free_execattr (exec);

			if (rc == FALSE) {
				break;
			}
		}
		if (v != NULL)
			g_strfreev (v);
	}

out:
	if (stripped_cmd)
		g_free (stripped_cmd);
	if (real_cmd)
		g_free (real_cmd);

	return (rc);
}

/* Function to return the zone root directory for the current workspace. */
char *
get_zoneroot (void)
{
  gpointer    wnckscreen    = NULL;
  gpointer    wnckworkspace = NULL;
  const char *zonelabelstr  = NULL;
  m_label_t  *zonelabel     = NULL;
  char       *zoneroot      = NULL;
  int         err;

  wnckscreen = libmenu_wnck_screen_get_default ();
  if (wnckscreen != NULL)
    wnckworkspace = libmenu_wnck_screen_get_active_workspace (wnckscreen);

  if (wnckworkspace != NULL)
    zonelabelstr = libmenu_wnck_workspace_get_label (wnckworkspace);

  if (zonelabelstr != NULL)
    libtsol_str_to_label (zonelabelstr, &zonelabel, MAC_LABEL, L_NO_CORRECTION, &err);

  if (zonelabel != NULL)
    zoneroot = libtsol_getzonerootbylabel (zonelabel);

  return zoneroot;
}
gboolean
gnome_desktop_tsol_is_clearance_admin_high (void)
{
        userattr_t      *uattr;
        char            *value = NULL;

        uattr = getuseruid (getuid ());

        if (uattr) {
                value = kva_match (uattr->attr, USERATTR_CLEARANCE);
                if (value)
                        if (strncasecmp ("admin_high", value, 10) == 0 ||
                            strncasecmp ("ADMIN_HIGH", value, 10) == 0)
                                return TRUE;
        }
        return FALSE;
}

gboolean
gnome_desktop_tsol_is_multi_label_session (void)
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
gnome_desktop_tsol_user_is_workstation_owner (void)
{
        uid_t uid;
        gpointer handle;
        Display *xdpy;
        static int ret = -1;
        xtsol_XTSOLgetWorkstationOwner libxtsol_XTSOLgetWorkstationOwner= NULL;

        if (ret == -1) {
                if (!(handle = dlopen ("/usr/lib/libXtsol.so.1", RTLD_LAZY)) ||
                    !(libxtsol_XTSOLgetWorkstationOwner =
                        (xtsol_XTSOLgetWorkstationOwner) dlsym (handle,
                                                "XTSOLgetWorkstationOwner"))) {
                        ret = 0;
                        return FALSE;
                }

                xdpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

                libxtsol_XTSOLgetWorkstationOwner (xdpy, &uid);

                if (uid == getuid ()) {
                        ret = 1;
                } else {
                        ret = 0;
                }
        }

        return ret ? TRUE : FALSE;
}

void
gnome_desktop_tsol_proxy_app_launch (char *command)
{
        GdkDisplay *dpy;
        Display *xdpy;
        Window root;
        Atom atom, utf8_string;

        if (!command) return;

        dpy = gdk_display_get_default ();
        xdpy = GDK_DISPLAY_XDISPLAY (dpy);

        utf8_string = XInternAtom (xdpy, "UTF8_STRING", FALSE);

        root = DefaultRootWindow (xdpy);

        atom = XInternAtom (xdpy, ATOM, FALSE);

        gdk_error_trap_push ();

        XChangeProperty (xdpy, root, atom, utf8_string, 8, PropModeReplace,
                         command, strlen (command));

        XSync (xdpy, False);

        gdk_error_trap_pop ();
}

