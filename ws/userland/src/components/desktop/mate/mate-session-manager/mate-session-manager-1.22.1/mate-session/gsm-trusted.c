/* trusted.c
 * Copyright (C) 2008 SUN Microsystems, Inc.
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

#include <priv.h>
#include <user_attr.h>
#include <secdb.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include "gsm-trusted.h"

void
escalate_privs (void)
{
	priv_set_t *pset;
	
	pset = priv_allocset ();
	getppriv (PRIV_PERMITTED, pset);
	setppriv (PRIV_SET, PRIV_INHERITABLE, pset);
}

void
drop_privs (void)
{
	priv_set_t *pset;
	userattr_t *uattr = NULL;
	char *value = NULL;

	pset = priv_allocset ();
	if ((uattr = getuseruid (getuid())) && 
	    (value = kva_match (uattr->attr, USERATTR_DFLTPRIV_KW))) {
		pset = priv_str_to_set (value, ",", NULL);
	} else {
		pset = priv_str_to_set ("basic", ",", NULL);
	}

	setppriv (PRIV_SET, PRIV_INHERITABLE, pset);
	priv_freeset (pset);
}

void
gsm_trusted_session_start (void)
{
	int delay = 1;
	char **app_path = NULL;
	static char *setup_apps[] = {"/usr/lib/amd64/mate-settings-daemon/mate-settings-daemon",
		/*
	        "/usr/lib/gnome-session/helpers/gnome-settings-daemon-helper", 
		"/usr/lib/gnome-session/helpers/gnome-keyring-daemon-wrapper",
		*/

		NULL};
	static char *trusted_apps[] = {
				       //"/usr/bin/tsoljds-setssheight",
                                       "/usr/bin/tsoljdsselmgr",
                                       "/usr/bin/tsoljds-tstripe",
                                       "/usr/bin/mate-panel",
                                       "/usr/lib/amd64/mate/wnck_applet",
				       "/usr/bin/marco --no-composite",
                                       NULL};

	static char *untrusted_apps[] = {"/usr/bin/xscreensaver -nosplash", NULL};

	/*
	while (delay) {
		sleep(1);
	}
	*/
  	for (app_path = setup_apps; *app_path != NULL; app_path++) {
		g_spawn_command_line_async (*app_path, NULL);
	}
	escalate_privs ();

  	for (app_path = trusted_apps; *app_path != NULL; app_path++) {
		g_spawn_command_line_async (*app_path, NULL);
	}

	drop_privs ();

  	for (app_path = untrusted_apps; *app_path != NULL; app_path++) {
		g_spawn_command_line_async (*app_path, NULL);
	}
}

gboolean
trusted_session_init (Display *display) 
{
	int major_code, first_event, first_error;
	GtkWidget *dialog;

	if (XQueryExtension (display, "SUN_TSOL", &major_code, &first_event, 
			     &first_error)) {
		g_setenv ("TRUSTED_SESSION", "TRUE", TRUE);
		drop_privs ();
		return TRUE;
	} else {
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
						 /* SUN_BRANDING */
						 GTK_BUTTONS_OK, _("Unable to login to Trusted Session. Required X server security extension not loaded."));
		gtk_widget_show (dialog);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	}
}

