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

#ifndef __PANEL_LOCKDOWN_H__
#define __PANEL_LOCKDOWN_H__

#include <glib.h>
#include <glib-object.h>
#include "launcher.h"

#ifdef __cplusplus
extern "C" {
#endif

void panel_lockdown_init     (void);
void panel_lockdown_finalize (void);

gboolean panel_lockdown_get_locked_down          (void);
gboolean panel_lockdown_get_disable_command_line (void);
gboolean panel_lockdown_get_disable_lock_screen  (void);
gboolean panel_lockdown_get_disable_log_out      (void);
gboolean panel_lockdown_get_disable_force_quit   (void);

gboolean panel_lockdown_is_applet_disabled (const char *iid, const char *id);

void panel_lockdown_notify_add    (GCallback callback_func,
                                   gpointer  user_data);
void panel_lockdown_notify_remove (GCallback callback_func,
                                   gpointer  user_data);
gchar *panel_lockdown_get_stripped_exec               (const gchar *full_exec);
gboolean panel_lockdown_is_disabled_command_line      (const gchar *term_cmd);

/**
  * Returns true if the command line corresponds to an application whose use
  * has been disallowed by the administrator.
  */
gboolean panel_lockdown_is_forbidden_command           (const gchar *command);

/**
  * Returns true if the launcher application has been disallowed by the administrator.
  */
gboolean panel_lockdown_is_forbidden_launcher          (Launcher *launcher);

/**
  * Returns true if the key_file application has been disallowed by the administrator.
  */
gboolean panel_lockdown_is_forbidden_key_file (GKeyFile *key_file);

/**
 * Returns true if the user is the root user, has the root role or has the
  * Primary Administrator or System Administrator profile.
  */
gboolean panel_lockdown_is_user_authorized(void);
#define SYSTEM_ADMINISTRATOR_PROF "System Administrator"
#define ROOT_ROLE "root"

#ifdef __cplusplus
}
#endif

#endif /* __PANEL_LOCKDOWN_H__ */
