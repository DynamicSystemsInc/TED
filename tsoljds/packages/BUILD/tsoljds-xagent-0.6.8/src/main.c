/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2021 Dynamic Systems, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gdm-signal-handler.h"
#include "gdm-log.h"

#include "gsm-consolekit.h"
#include "gsm-util.h"
#include "gsm-xsmp-server.h"
#include "gsm-store.h"

#define GSM_DBUS_NAME "org.gnome.SessionManager"

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

static gboolean failsafe = FALSE;
static gboolean show_version = FALSE;
static gboolean debug = FALSE;
static gboolean trusted_session = FALSE;

static void
on_bus_name_lost (DBusGProxy *bus_proxy,
                  const char *name,
                  gpointer    data)
{
        g_warning ("Lost name on bus: %s, exiting", name);
        exit (1);
}

static gboolean
acquire_name_on_proxy (DBusGProxy *bus_proxy,
                       const char *name)
{
        GError     *error;
        guint       result;
        gboolean    res;
        gboolean    ret;

        ret = FALSE;

        if (bus_proxy == NULL) {
                goto out;
        }

        error = NULL;
        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, name,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &result,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        /* register for name lost */
        dbus_g_proxy_add_signal (bus_proxy,
                                 "NameLost",
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (bus_proxy,
                                     "NameLost",
                                     G_CALLBACK (on_bus_name_lost),
                                     NULL,
                                     NULL);


        ret = TRUE;

 out:
        return ret;
}

static gboolean
acquire_name (void)
{
        DBusGProxy      *bus_proxy;
        GError          *error;
        DBusGConnection *connection;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                gsm_util_init_error (TRUE,
                                     "Could not connect to session bus: %s",
                                     error->message);
                /* not reached */
        }

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);

        if (! acquire_name_on_proxy (bus_proxy, GSM_DBUS_NAME) ) {
                gsm_util_init_error (TRUE,
                                     "%s",
                                     "Could not acquire name on session bus");
                /* not reached */
        }

        g_object_unref (bus_proxy);

        return TRUE;
}

static gboolean
check_gdm_greeter_session (GsmManager *manager,
                    char      **override_autostart_dirs)
{
        int i;
        for (i = 0; override_autostart_dirs[i]; i++) {
               if (strstr (override_autostart_dirs[i], "/gdm/autostart/LoginWindow") != NULL)
                       return TRUE;
        }
        return FALSE;
}


static gboolean
signal_cb (int      signo,
           gpointer data)
{
        int ret;
        GsmManager *manager;

        g_debug ("Got callback for signal %d", signo);

        ret = TRUE;

        switch (signo) {
        case SIGFPE:
        case SIGPIPE:
                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down abnormally.", signo);
                ret = FALSE;
                break;
        case SIGINT:
        case SIGTERM:
                manager = (GsmManager *)data;
                gsm_manager_logout (manager, GSM_MANAGER_LOGOUT_MODE_FORCE, NULL);

                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down normally.", signo);
                ret = TRUE;
                break;
        case SIGHUP:
                g_debug ("Got HUP signal");
                ret = TRUE;
                break;
        case SIGUSR1:
                g_debug ("Got USR1 signal");
                ret = TRUE;
                gdm_log_toggle_debug ();
                break;
        default:
                g_debug ("Caught unhandled signal %d", signo);
                ret = TRUE;

                break;
        }

        return ret;
}

static void
shutdown_cb (gpointer data)
{
        GsmManager *manager = (GsmManager *)data;
        g_debug ("Calling shutdown callback function");

        /*
         * When the signal handler gets a shutdown signal, it calls
         * this function to inform GsmManager to not restart
         * applications in the off chance a handler is already queued
         * to dispatch following the below call to gtk_main_quit.
         */
        gsm_manager_set_phase (manager, GSM_MANAGER_PHASE_EXIT);

        gtk_main_quit ();
}

static gboolean
require_dbus_session (int      argc,
                      char   **argv,
                      GError **error)
{
        char **new_argv;
        int    i;

        if (g_getenv ("DBUS_SESSION_BUS_ADDRESS"))
                return TRUE;

        /* Just a sanity check to prevent infinite recursion if
         * dbus-launch fails to set DBUS_SESSION_BUS_ADDRESS 
         */
        g_return_val_if_fail (!g_str_has_prefix (argv[0], "dbus-launch"),
                              TRUE);

        /* +2 for our new arguments, +1 for NULL */
        new_argv = g_malloc (argc + 3 * sizeof (*argv));

        new_argv[0] = "dbus-launch";
        new_argv[1] = "--exit-with-session";
        for (i = 0; i < argc; i++) {
                new_argv[i + 2] = argv[i];
	}
        new_argv[i + 2] = NULL;
        
        if (!execvp ("dbus-launch", new_argv)) {
                g_set_error (error, 
                             G_SPAWN_ERROR,
                             G_SPAWN_ERROR_FAILED,
                             "No session bus and could not exec dbus-launch: %s",
                             g_strerror (errno));
                return FALSE;
        }

        /* Should not be reached */
        return TRUE;
}


static gboolean
postrun_progress_update (gpointer data)
{
	int status = 0;
  	static int count = 0;
  	GError *error = NULL;
  	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));

  	if (count == 30) {
    		g_spawn_command_line_sync ("/usr/lib/postrun-query -c JDS_wait -e", NULL, NULL, &status, &error);
    		count = 0;
  	}

  	if (status == 0) {
    		count++;
    		return TRUE;
  	}

  	gtk_main_quit ();
  	return FALSE;
}

static void
gsm_wait_for_unfinished_postrun () 
{
  	int status = 0;
  	GError *error = NULL; 

  	if (!g_spawn_command_line_sync ("/usr/lib/postrun-query -c JDS_wait -e",
				        NULL, NULL, &status, &error)) {
        /* no postrun-query? WTF? bail out and hope for the best :) */
		return;
  	}

  	if (status == 0) {
    		GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    		GtkWidget *vbox = gtk_vbox_new (TRUE, 10);
    		GtkWidget *progress = gtk_progress_bar_new ();
    		GtkWidget *label;
    		gint func_ref;

    		/* SUN_BRANDING */
    		label = gtk_label_new (_("Completing post install setup..."));

    		gtk_container_add (GTK_CONTAINER (window), vbox);
    		gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    		gtk_box_pack_start_defaults (GTK_BOX (vbox), label);
    		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress));
    		func_ref = g_timeout_add (100, postrun_progress_update, progress);
    		gtk_box_pack_start_defaults (GTK_BOX (vbox), progress);

    		gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

    		gtk_widget_show_all (window);
    		gtk_main ();
  
    		gtk_widget_destroy (window);
    		g_source_remove (func_ref);
  	}
}

  
int
main (int argc, char **argv)
{
        struct sigaction  sa;
        GError           *error;
        char             *display_str;
	Display *xdisp;
	GdkDisplay *gdisp;
        GsmManager       *manager;
        GsmStore         *client_store;
        GsmXsmpServer    *xsmp_server;
        GdmSignalHandler *signal_handler;
        static char     **override_autostart_dirs = NULL;
        static char      *default_session_key = NULL;
        static GOptionEntry entries[] = {
                { "autostart", 'a', 0, G_OPTION_ARG_STRING_ARRAY, &override_autostart_dirs, N_("Override standard autostart directories"), NULL },
                { "default-session-key", 0, 0, G_OPTION_ARG_STRING, &default_session_key, N_("GConf key used to lookup default session"), NULL },
                { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },
                { "failsafe", 'f', 0, G_OPTION_ARG_NONE, &failsafe, N_("Do not load user-specified applications"), NULL },
        /* SUN_BRANDING */
                { "trusted-session", '\0', 0, G_OPTION_ARG_NONE, &trusted_session, N_("Used for Trusted Multi-Label Session"), NULL },
                { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        /* Make sure that we have a session bus */
        if (!require_dbus_session (argc, argv, &error)) {
                gsm_util_init_error (TRUE, "%s", error->message);
        }

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigemptyset (&sa.sa_mask);
        sigaction (SIGPIPE, &sa, 0);

        error = NULL;
        gtk_init_with_args (&argc, &argv,
                            (char *) _(" - the GNOME session manager"),
                            entries, GETTEXT_PACKAGE,
                            &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                exit (1);
        }

        if (show_version) {
                g_print ("%s %s\n", argv [0], VERSION);
                exit (1);
        }

        gdm_log_init ();
        gdm_log_set_debug (debug);

        /* Set DISPLAY explicitly for all our children, in case --display
         * was specified on the command line.
         */
        display_str = gdk_get_display ();
        gsm_util_setenv ("DISPLAY", display_str);
        g_free (display_str);

	gdisp = gdk_display_get_default ();
	xdisp = gdk_x11_display_get_xdisplay (gdisp);
	XInternAtom (xdisp, "GNOME_SM_DESKTOP", FALSE);

    if (trusted_session) {
        if (!trusted_session_init (xdisp)) {
            exit (1);
        }
    }

	gsm_wait_for_unfinished_postrun ();

        /* Some third-party programs rely on GNOME_DESKTOP_SESSION_ID to
         * detect if GNOME is running. We keep this for compatibility reasons.
         */
        gsm_util_setenv ("GNOME_DESKTOP_SESSION_ID", "this-is-deprecated");

        client_store = gsm_store_new ();


        xsmp_server = gsm_xsmp_server_new (client_store);

        /* Now make sure they succeeded. (They'll call
         * gsm_util_init_error() if they failed.)
         */

        manager = gsm_manager_new (client_store, failsafe);

        signal_handler = gdm_signal_handler_new ();
        gdm_signal_handler_add_fatal (signal_handler);
        gdm_signal_handler_add (signal_handler, SIGFPE, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGHUP, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGUSR1, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGTERM, signal_cb, manager);
        gdm_signal_handler_add (signal_handler, SIGINT, signal_cb, manager);
        gdm_signal_handler_set_fatal_func (signal_handler, shutdown_cb, manager);
        gsm_manager_set_inhibit_dialog_display (manager, TRUE);

        gsm_xsmp_server_start (xsmp_server);

        if (trusted_session) {
            gsm_trusted_session_start ();
            gsm_manager_set_phase (manager, GSM_MANAGER_PHASE_RUNNING);
        } else {
            gsm_manager_start (manager);
        }

        gtk_main ();

        if (xsmp_server != NULL) {
                g_object_unref (xsmp_server);
        }

        if (manager != NULL) {
                g_debug ("Unreffing manager");
                g_object_unref (manager);
        }

        if (client_store != NULL) {
                g_object_unref (client_store);
        }
        gdm_log_shutdown ();

        return 0;
}
