/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Sun Microsystems, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Authors:
 *      Matt Keenan  <matt.keenan@sun.com>
 *      Mark McLoughlin  <mark@skynet.ie>
 */

#include <config.h>

#include "panel-lockdown.h"

#include <string.h>
#include <gio/gio.h>
#include "panel-schemas.h"
#include "launcher.h"
#include <sys/types.h>
#include <unistd.h>
#include <exec_attr.h>
#include <user_attr.h>
#include <secdb.h>
#include <pwd.h>

#include "panel-solaris.h"

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-desktop-utils.h>

#include <libpanel-util/panel-error.h>
#include <libpanel-util/panel-glib.h>
#include <libpanel-util/panel-gtk.h>
#include <libpanel-util/panel-keyfile.h>
#include <libpanel-util/panel-show.h>

typedef struct {
        guint   initialized : 1;

        guint   locked_down : 1;
        guint   disable_command_line : 1;
        guint   disable_lock_screen : 1;
        guint   disable_log_out : 1;
        guint   disable_force_quit : 1;
	guint	restrict_application_launching : 1;

        gchar **disabled_applets;

        GSList *allowed_applications;
        GSList *closures;

        GSettings *panel_settings;
        GSettings *lockdown_settings;
} PanelLockdown;

static const gchar *command_line_execs[] = {
    "/usr/bin/mate-terminal",
    "/usr/bin/xterm"
};

#define	NUMBER_COMMAND_LINE_EXECS 2

static PanelLockdown panel_lockdown = { 0, };


static inline void
panel_lockdown_invoke_closures (PanelLockdown *lockdown)
{
        GSList *l;

        for (l = lockdown->closures; l; l = l->next)
                g_closure_invoke (l->data, NULL, 0, NULL, NULL);
}

static void
locked_down_notify (GSettings     *settings,
                    gchar         *key,
                    PanelLockdown *lockdown)
{
        lockdown->locked_down = g_settings_get_boolean (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

gboolean
panel_lockdown_is_allowed_application (const gchar *app)
{
        GSList *l;

        g_assert (panel_lockdown.initialized != FALSE);

        for (l = panel_lockdown.allowed_applications; l; l = l->next)
                if (!strcmp (l->data, app))
                        return TRUE;

        return FALSE;
}

static void
disable_command_line_notify (GSettings     *settings,
                             gchar         *key,
                             PanelLockdown *lockdown)
{
        lockdown->disable_command_line = g_settings_get_boolean (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

static void
disable_lock_screen_notify (GSettings     *settings,
                            gchar         *key,
                            PanelLockdown *lockdown)
{
        lockdown->disable_lock_screen = g_settings_get_boolean (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

static void
disable_log_out_notify (GSettings     *settings,
                        gchar         *key,
                        PanelLockdown *lockdown)
{
        lockdown->disable_log_out = g_settings_get_boolean (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

static void
disable_force_quit_notify (GSettings     *settings,
                           gchar         *key,
                           PanelLockdown *lockdown)
{
        lockdown->disable_force_quit = g_settings_get_boolean (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

static void
disabled_applets_notify (GSettings     *settings,
                         gchar         *key,
                         PanelLockdown *lockdown)
{
        lockdown->disabled_applets = g_settings_get_strv (settings, key);
        panel_lockdown_invoke_closures (lockdown);
}

static gboolean
panel_lockdown_load_bool (PanelLockdown         *lockdown,
                          GSettings             *settings,
                          const char            *key,
                          GCallback              notify_func)
{
        gboolean  retval;
        gchar *signal_name;

        retval = g_settings_get_boolean (settings, key);

        signal_name = g_strdup_printf ("changed::%s", key);

        g_signal_connect (settings,
                          signal_name,
                          G_CALLBACK (notify_func),
                          lockdown);

        g_free (signal_name);

        return retval;
}

static gchar **
panel_lockdown_load_disabled_applets (PanelLockdown *lockdown,
                                      GSettings     *settings)
{
        gchar **retval;

        retval = g_settings_get_strv (settings,
                                      PANEL_DISABLED_APPLETS_KEY);

        g_signal_connect (settings,
                          "changed::" PANEL_DISABLED_APPLETS_KEY,
                          G_CALLBACK (disabled_applets_notify),
                          lockdown);

        return retval;
}

void
panel_lockdown_init (void)
{
        panel_lockdown.panel_settings = g_settings_new (PANEL_SCHEMA);
        panel_lockdown.lockdown_settings = g_settings_new (LOCKDOWN_SCHEMA);

        panel_lockdown.locked_down =
                panel_lockdown_load_bool (&panel_lockdown,
                                          panel_lockdown.panel_settings,
                                          PANEL_LOCKED_DOWN_KEY,
                                          G_CALLBACK (locked_down_notify));

        panel_lockdown.disable_command_line =
                panel_lockdown_load_bool (&panel_lockdown,
                                          panel_lockdown.lockdown_settings,
                                          LOCKDOWN_DISABLE_COMMAND_LINE_KEY,
                                          G_CALLBACK (disable_command_line_notify));
        
        panel_lockdown.disable_lock_screen =
                panel_lockdown_load_bool (&panel_lockdown,
                                          panel_lockdown.lockdown_settings,
                                          LOCKDOWN_DISABLE_LOCK_SCREEN_KEY,
                                          G_CALLBACK (disable_lock_screen_notify));

        panel_lockdown.disable_log_out =
                panel_lockdown_load_bool (&panel_lockdown,
                                          panel_lockdown.lockdown_settings,
                                          LOCKDOWN_DISABLE_LOG_OUT_KEY,
                                          G_CALLBACK (disable_log_out_notify));

        panel_lockdown.disable_force_quit =
                panel_lockdown_load_bool (&panel_lockdown,
                                          panel_lockdown.panel_settings,
                                          PANEL_DISABLE_FORCE_QUIT_KEY,
                                          G_CALLBACK (disable_force_quit_notify));

        panel_lockdown.disabled_applets =
                panel_lockdown_load_disabled_applets (&panel_lockdown,
                                                      panel_lockdown.panel_settings);

        panel_lockdown.initialized = TRUE;
}

void
panel_lockdown_finalize (void)
{
        GSList *l;

        g_assert (panel_lockdown.initialized != FALSE);

        if (panel_lockdown.disabled_applets) {
                g_strfreev (panel_lockdown.disabled_applets);
                panel_lockdown.disabled_applets = NULL;
        }

        if (panel_lockdown.panel_settings) {
                g_object_unref (panel_lockdown.panel_settings);
                panel_lockdown.panel_settings = NULL;
        }
        if (panel_lockdown.lockdown_settings) {
                g_object_unref (panel_lockdown.lockdown_settings);
                panel_lockdown.lockdown_settings = NULL;
        }

        for (l = panel_lockdown.closures; l; l = l->next)
                g_closure_unref (l->data);
        g_slist_free (panel_lockdown.closures);
        panel_lockdown.closures = NULL;

        panel_lockdown.initialized = FALSE;
}

gboolean
panel_lockdown_get_locked_down (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.locked_down;
}

gboolean
panel_lockdown_get_disable_command_line (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.disable_command_line;
}

gboolean
panel_lockdown_get_disable_lock_screen (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.disable_lock_screen;
}

gboolean
panel_lockdown_get_disable_log_out (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.disable_log_out;
}

gboolean
panel_lockdown_get_disable_force_quit (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.disable_force_quit;
}

gboolean
panel_lockdown_is_applet_disabled (const char *iid, const char *location)
{
        gint i;

        g_assert (panel_lockdown.initialized != FALSE);

        if (panel_lockdown.disabled_applets)
                for (i = 0; panel_lockdown.disabled_applets[i]; i++)
                        if (!strcmp (panel_lockdown.disabled_applets[i], iid))
			      return TRUE;

        if (filter_with_rbac (location, FALSE))
                return TRUE;

        return FALSE;
}

static GClosure *
panel_lockdown_notify_find (GSList    *closures,
                            GCallback  callback_func,
                            gpointer   user_data)
{
        GSList *l;

        for (l = closures; l; l = l->next) {
                GCClosure *cclosure = l->data;
                GClosure  *closure  = l->data;

                if (closure->data == user_data &&
                    cclosure->callback == callback_func)
                        return closure;
        }

        return NULL;
}

static void
marshal_user_data (GClosure     *closure,
                   GValue       *return_value,
                   guint         n_param_values,
                   const GValue *param_values,
                   gpointer      invocation_hint,
                   gpointer      marshal_data)
{
        GCClosure *cclosure = (GCClosure*) closure;

        g_return_if_fail (cclosure->callback != NULL);
        g_return_if_fail (n_param_values == 0);

        ((void (*) (gpointer *))cclosure->callback) (closure->data);
}

void
panel_lockdown_notify_add (GCallback callback_func,
                           gpointer  user_data)
{
        GClosure *closure;

        g_assert (panel_lockdown_notify_find (panel_lockdown.closures,
                                              callback_func,
                                              user_data) == NULL);

        closure = g_cclosure_new (callback_func, user_data, NULL);
        g_closure_set_marshal (closure, marshal_user_data);

        panel_lockdown.closures = g_slist_append (panel_lockdown.closures,
                                                  closure);
}

void
panel_lockdown_notify_remove (GCallback callback_func,
                              gpointer  user_data)
{
        GClosure *closure;

        closure = panel_lockdown_notify_find (panel_lockdown.closures,
                                              callback_func,
                                              user_data);

        g_assert (closure != NULL);

        panel_lockdown.closures = g_slist_remove (panel_lockdown.closures,
                                                  closure);

        g_closure_unref (closure);
}

gchar *
panel_lockdown_get_stripped_exec (const gchar *full_exec)
{
        gchar *str1, *str2, *retval, *p;

        str1 = g_strdup (full_exec);
        p = strtok (str1, " ");

        if (p != NULL)
               str2 = g_strdup (p);
        else
                str2 = g_strdup (full_exec);

        g_free (str1);

        if (g_path_is_absolute (str2))
                retval = g_strdup (str2);
        else
                retval = g_strdup (g_find_program_in_path ((const gchar *)str2));
        g_free (str2);

        return retval;
}

gboolean
panel_lockdown_is_disabled_command_line (const gchar *term_cmd)
{
        int i = 0;
        gboolean retval = FALSE;

        for (i=0; i<NUMBER_COMMAND_LINE_EXECS; i++) {
                if (!strcmp (command_line_execs [i], term_cmd)) {
                        retval = TRUE;
                        break;
                }
        }

        return retval;
}
gboolean

panel_lockdown_get_restrict_application_launching (void)
{
        g_assert (panel_lockdown.initialized != FALSE);

        return panel_lockdown.restrict_application_launching;
}



gboolean
panel_lockdown_is_forbidden_command (const char *command)
{
        g_return_val_if_fail (command != NULL, TRUE) ;
        return panel_lockdown_get_restrict_application_launching () &&
                !panel_lockdown_is_allowed_application (command) ;
}


gboolean
panel_lockdown_is_forbidden_launcher (Launcher *launcher)
{
	return (panel_lockdown_is_forbidden_key_file(launcher->key_file));
}

gboolean
panel_lockdown_is_forbidden_key_file (GKeyFile *key_file)
{
	gchar *full_exec;		/* Executable including possible arguments */
	gchar *stripped_exec;	/* Executable with arguments stripped away */
	gboolean retval = FALSE;

	if (key_file != NULL)
	{
		full_exec = panel_key_file_get_string (key_file, "Exec");
        if (full_exec != NULL) {
        	stripped_exec = panel_lockdown_get_stripped_exec (full_exec);

		if (filter_with_rbac ((char *)stripped_exec, FALSE))
			return TRUE;

		retval = panel_lockdown_is_forbidden_command (stripped_exec);
                g_free (stripped_exec);
                if (retval == TRUE) {
                        retval = panel_lockdown_is_forbidden_command (full_exec);
                }
		}
	}

    /* If restrict_application_launching not set on return TRUE */
    if (!panel_lockdown_get_restrict_application_launching ()) {
        return FALSE;
    }

	return retval;
}

static gboolean
has_root_role (char *username)
{
    userattr_t *userattr = NULL;
    gchar *rolelist = NULL;
    gchar *rolename = NULL;
    static gboolean ret_val = FALSE;
    static gboolean cached_root = FALSE;

    if (cached_root == FALSE && (userattr = getusernam(username)) != NULL)
    {
        rolelist = kva_match(userattr->attr, USERATTR_ROLES_KW);
        rolename = strtok(rolelist, ",");
        while (rolename != NULL) {
            if (strcmp (rolename, ROOT_ROLE) == 0) {
                ret_val = TRUE;
                break;
            }
            rolename = strtok(NULL, ",");
        }
     
        free_userattr(userattr);
        cached_root = TRUE;
    }

    return ret_val;
}

static gboolean
has_admin_profile (char *username)
{
    execattr_t *execattr = NULL;
    static gboolean ret_val = FALSE;
    static gboolean cached_admin = FALSE;

    if (cached_admin == FALSE && (execattr = getexecuser (username, NULL, NULL, GET_ALL)) != NULL)
    {
        while (execattr != NULL) {
            if (strcmp (execattr->name, SYSTEM_ADMINISTRATOR_PROF) == 0)
            {
                ret_val = TRUE;
                break;
            }
            execattr = execattr->next;
        }
        free_execattr (execattr);
        cached_admin = TRUE;
    }
    return ret_val;
}

gboolean panel_lockdown_is_user_authorized(void) {
    uid_t uid = getuid();
    struct passwd *pw;

    if ((pw = getpwuid(uid)) == NULL)
        return FALSE;

    if (has_admin_profile (pw->pw_name))
        return TRUE;

    if (has_root_role (pw->pw_name))
        return TRUE;

    if (uid == 0)
        return TRUE;

    return FALSE;
}
