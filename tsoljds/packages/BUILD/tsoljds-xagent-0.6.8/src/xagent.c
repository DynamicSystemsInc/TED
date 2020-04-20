/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libintl.h>
#include <signal.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <glib/goption.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include "gsm-gconf.h"
#include "gsm-util.h"
#include "gsm-manager.h"
#include "gsm-xsmp-server.h"
#include "gsm-store.h"
#include "trusted.h"

#define TSOLJDS_MIGRATION_SCRIPT "/usr/dt/config/tsoljds-migration"

#define GSM_DBUS_NAME "org.gnome.SessionManager"

static int XAgentXErrorHandler (Display *, XErrorEvent *);

gboolean defaultsession = FALSE;
gboolean nosession = FALSE;

static GOptionEntry entries[] = {
  { "defaultsession", '\0', 0, G_OPTION_ARG_NONE, &defaultsession,
    N_("Do not load user-specified applications"),
    NULL },
  { "nosession", '\0', 0, G_OPTION_ARG_NONE, &nosession,
    N_("Do not startup any applications"),
    NULL },
  { NULL, 0, 0, 0, NULL, NULL, NULL }
};

static int
XAgentXErrorHandler (Display *dpy, XErrorEvent *error)
{
  char err_msg[132];

  XGetErrorText (dpy, error->error_code, err_msg, sizeof (err_msg));

  return 0;
}

static void
so_long_pipe (gpointer data)
{
  /*
   * The pipe is bust which probably means the stripe
   * has died. So there's nothing to do but die.
   */
  exit (2);
}

enum {
  PIPE_MESSAGE_PARSE_ERROR = 0,
  PIPE_MESSAGE_COMMAND,
  PIPE_MESSAGE_URI,
};

static int
parse_message_string (char *str, int *screen, char **message)
{
  gchar *p = NULL;

  if (!(p = strchr (str, ':')) || (p == str)) 
    return PIPE_MESSAGE_PARSE_ERROR;

  if (*(p+1) != '\0') 
    *message = g_strdup (p+1);
  else 
    return PIPE_MESSAGE_PARSE_ERROR;

  *p = '\0';
  *screen = atoi (str); /* defaults to 0 on error */
  *p = ':';

  if (strncmp (*message, "[URI]", 5) == 0)
	return PIPE_MESSAGE_URI;
  else
	return PIPE_MESSAGE_COMMAND;
}

static gboolean
handle_pipe_input (GIOChannel *source,
                   GIOCondition condition,
                   gpointer data)
{
#define BUFSIZE 1024
  gsize byteread, pos;
  gchar *str;
  GError *error = NULL;
  GIOStatus status=0;
  int screen_num;
  int message_type;
  gchar *message = NULL;
  GdkDisplay *gdk_dpy;

  if (condition & G_IO_ERR) return FALSE;

  if (condition & G_IO_HUP) return FALSE;

  if (condition & G_IO_IN) {
    status = g_io_channel_read_line (source, &str, &byteread, &pos, &error);

    switch (status)
    {
	case G_IO_STATUS_NORMAL:
	str[pos] = '\0';
        gdk_dpy = gdk_display_get_default ();
        message_type = parse_message_string (str, &screen_num, &message);
	switch (message_type) 
	  {
	    GAppInfo *appinfo;
	    case PIPE_MESSAGE_URI:
	      gtk_show_uri (gdk_display_get_screen (gdk_dpy, screen_num), 
			message + 5, GDK_CURRENT_TIME, &error);
	      break;
	    case PIPE_MESSAGE_COMMAND:
	      appinfo = g_app_info_create_from_commandline(message,
		      g_find_program_in_path(message), 0, &error);
	      g_app_info_launch_uris(appinfo, NULL, NULL, &error);
	      g_free(appinfo);
	      //g_spawn_command_line_async (message, &error);
	      break;
	    case PIPE_MESSAGE_PARSE_ERROR:
	    default:
	      break;
	    }
        if (message) g_free (message);
        return TRUE;
	case G_IO_STATUS_AGAIN:
	return FALSE;

	case G_IO_STATUS_EOF:
	sleep(1);
	return FALSE;

	case G_IO_STATUS_ERROR:
	return FALSE;

	default:
	g_assert_not_reached ();
	return FALSE;
    }
  }
}

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
                                     "Could not acquire name on session bus: %s",
                                     error->message);
                /* not reached */
        }

        g_object_unref (bus_proxy);

        return TRUE;
}

static void
xagent_start_dbus (void) {

	gchar *out, *err, *chr;
	gint status;
	GError *error;
	
	g_spawn_command_line_sync ("/usr/bin/dbus-launch --exit-with-session",
				   &out, &err, &status, &error);

	chr = strchr (out, '\n');
	*chr = '\0';

	putenv (out);
}

static void
xagent_start_default_clients (void) {
	
	char **app_path = NULL;

	char *default_apps[] = {"/usr/bin/caja --no-default-window", 
				NULL };

	for (app_path = default_apps; *app_path != NULL; app_path++) {
		g_spawn_command_line_async (*app_path, NULL);
	}
}

int
main (int argc, char **argv)
{
        struct sigaction sa;
        GError          *error;
        char            *display_str;
	Display *xdisp;
	GdkDisplay *gdisp;
        GsmManager      *manager;
        GsmStore        *client_store;
        GsmXsmpServer   *xsmp_server;
	int dummy_fd, pipe_fd;
	GIOChannel *channel;
	guint result;

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

	int fd = open ("/dev/null", O_RDWR);
 	dup2 (fd, 1);
	dup2 (fd, 2);

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGPIPE, &sa, 0);

	if ((pipe_fd = dup (fileno(stdin))) != -1) {
		close (fileno(stdin));
		dummy_fd = open ("/dev/null", O_RDONLY);
		fcntl (pipe_fd, F_SETFD, 1);
	} else {
		pipe_fd = fileno (stdin);
	}

        error = NULL;
        gtk_init_with_args (&argc, &argv,
                            (char *) _(" - the GNOME session manager"),
                            entries, GETTEXT_PACKAGE,
                            &error);
        if (error != NULL) {
                gsm_util_init_error (TRUE, "%s", error->message);
        }

        /* Set DISPLAY explicitly for all our children, in case --display
         * was specified on the command line.
         */
        display_str = gdk_get_display ();
        gsm_util_setenv ("DISPLAY", display_str);
        g_free (display_str);

	gdisp = gdk_display_get_default ();
	xdisp = gdk_x11_display_get_xdisplay (gdisp);
	XInternAtom (xdisp, "GNOME_SM_DESKTOP", FALSE);

	XSetErrorHandler (XAgentXErrorHandler);

	if (!nosession) {
 xagent_start_dbus ();        	/* Start up gconfd if not already running. */
        	gsm_gconf_init ();

        	gsm_gconf_check ();
	}
	
	if (g_file_test (TSOLJDS_MIGRATION_SCRIPT, G_FILE_TEST_IS_EXECUTABLE)) {
		system (TSOLJDS_MIGRATION_SCRIPT);
	}
	/*start the session*/
	xagent_start_default_clients ();

	/* we may have to spawn an exec immediately */
	g_spawn_command_line_async (g_getenv("LABEL_EXEC_COMMAND"), &error);

	channel = g_io_channel_unix_new (pipe_fd);
	result = g_io_add_watch_full (channel, G_PRIORITY_HIGH,
				      G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
				      (GIOFunc)handle_pipe_input, NULL, 
				      so_long_pipe);

        gtk_main ();

        gsm_gconf_shutdown ();

        return 0;
}
