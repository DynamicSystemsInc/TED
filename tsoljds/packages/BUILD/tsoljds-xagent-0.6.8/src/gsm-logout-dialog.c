/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Vincent Untz
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Authors:
 *	Vincent Untz <vuntz@gnome.org>
 */

#include <config.h>

#include <pwd.h>
#include <auth_attr.h>
#include <secdb.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "gsm-power-manager.h"
#include "gsm-logout-dialog.h"
#include "gsm-consolekit.h"
#include "gdm.h"

#define GSM_LOGOUT_DIALOG_GET_PRIVATE(o)                                \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_LOGOUT_DIALOG, GsmLogoutDialogPrivate))

#define AUTOMATIC_ACTION_TIMEOUT 60

#define GSM_ICON_LOGOUT   "system-log-out"
#define GSM_ICON_SHUTDOWN "system-shutdown"

typedef enum {
        GSM_DIALOG_LOGOUT_TYPE_LOGOUT,
        GSM_DIALOG_LOGOUT_TYPE_SHUTDOWN
} GsmDialogLogoutType;

struct _GsmLogoutDialogPrivate
{
        GsmDialogLogoutType  type;

        GsmConsolekit       *consolekit;

        int                  timeout;
        unsigned int         timeout_id;

        unsigned int         default_response;

	unsigned int fast;
	int id;
};

static GsmLogoutDialog *current_dialog = NULL;

static void gsm_logout_dialog_set_timeout  (GsmLogoutDialog *logout_dialog);

static void gsm_logout_dialog_destroy  (GsmLogoutDialog *logout_dialog,
                                        gpointer         data);

static void gsm_logout_dialog_show     (GsmLogoutDialog *logout_dialog,
                                        gpointer         data);

enum {
        PROP_0,
        PROP_MESSAGE_TYPE
};

G_DEFINE_TYPE (GsmLogoutDialog, gsm_logout_dialog, GTK_TYPE_MESSAGE_DIALOG);

static void
gsm_logout_dialog_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        switch (prop_id) {
        case PROP_MESSAGE_TYPE:
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_logout_dialog_get_property (GObject     *object,
                                guint        prop_id,
                                GValue      *value,
                                GParamSpec  *pspec)
{
        switch (prop_id) {
        case PROP_MESSAGE_TYPE:
                g_value_set_enum (value, GTK_MESSAGE_WARNING);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_logout_dialog_class_init (GsmLogoutDialogClass *klass)
{
        GObjectClass *gobject_class;

        gobject_class = G_OBJECT_CLASS (klass);

        /* This is a workaround to avoid a stupid crash: libgnomeui
         * listens for the "show" signal on all GtkMessageDialog and
         * gets the "message-type" of the dialogs. We will crash when
         * it accesses this property if we don't override it since we
         * didn't define it. */
        gobject_class->set_property = gsm_logout_dialog_set_property;
        gobject_class->get_property = gsm_logout_dialog_get_property;

        g_object_class_override_property (gobject_class,
                                          PROP_MESSAGE_TYPE,
                                          "message-type");

        g_type_class_add_private (klass, sizeof (GsmLogoutDialogPrivate));
}

static void
gsm_logout_dialog_init (GsmLogoutDialog *logout_dialog)
{
        logout_dialog->priv = GSM_LOGOUT_DIALOG_GET_PRIVATE (logout_dialog);

        logout_dialog->priv->timeout_id = 0;
        logout_dialog->priv->timeout = 0;
        logout_dialog->priv->default_response = GTK_RESPONSE_CANCEL;

        logout_dialog->priv->fast = 1;
        logout_dialog->priv->id = -1;

        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (logout_dialog), TRUE);
        gtk_window_set_keep_above (GTK_WINDOW (logout_dialog), TRUE);
        gtk_window_stick (GTK_WINDOW (logout_dialog));

        logout_dialog->priv->consolekit = gsm_get_consolekit ();

        g_signal_connect (logout_dialog,
                          "destroy",
                          G_CALLBACK (gsm_logout_dialog_destroy),
                          NULL);

        g_signal_connect (logout_dialog,
                          "show",
                          G_CALLBACK (gsm_logout_dialog_show),
                          NULL);
}

static void
gsm_logout_dialog_destroy (GsmLogoutDialog *logout_dialog,
                           gpointer         data)
{
        if (logout_dialog->priv->timeout_id != 0) {
                g_source_remove (logout_dialog->priv->timeout_id);
                logout_dialog->priv->timeout_id = 0;
        }

        if (logout_dialog->priv->consolekit) {
                g_object_unref (logout_dialog->priv->consolekit);
                logout_dialog->priv->consolekit = NULL;
        }

        current_dialog = NULL;
}

static gboolean
gsm_logout_supports_system_suspend (GsmLogoutDialog *logout_dialog)
{
	GsmPowerManager *power_manager;
	gboolean ret;

        power_manager = gsm_get_power_manager ();

	ret= gsm_power_manager_can_suspend (power_manager);
        g_object_unref (power_manager);

	return ret;
}

static gboolean
gsm_logout_supports_system_hibernate (GsmLogoutDialog *logout_dialog)
{
	GsmPowerManager *power_manager;
	gboolean ret;

        power_manager = gsm_get_power_manager ();

	ret= gsm_power_manager_can_hibernate (power_manager);
        g_object_unref (power_manager);

        return ret;
}

static gboolean
gsm_logout_supports_switch_user (GsmLogoutDialog *logout_dialog)
{
        gboolean ret;

        ret = gsm_consolekit_can_switch_user (logout_dialog->priv->consolekit);

        return ret;
}

static gboolean
gsm_logout_supports_reboot (GsmLogoutDialog *logout_dialog)
{
        gboolean ret;

        ret = gsm_consolekit_can_restart (logout_dialog->priv->consolekit);
        if (!ret) {
                ret = gdm_supports_logout_action (GDM_LOGOUT_ACTION_REBOOT);
        }

        return ret;
}

static gboolean
gsm_logout_supports_shutdown (GsmLogoutDialog *logout_dialog)
{
        gboolean ret;

        ret = gsm_consolekit_can_stop (logout_dialog->priv->consolekit);

        if (!ret) {
                ret = gdm_supports_logout_action (GDM_LOGOUT_ACTION_SHUTDOWN);
        }

        return ret;
}

static void
gsm_logout_dialog_show (GsmLogoutDialog *logout_dialog, gpointer user_data)
{
        gsm_logout_dialog_set_timeout (logout_dialog);
}

static gboolean
gsm_logout_dialog_timeout (gpointer data)
{
        GsmLogoutDialog *logout_dialog;
        char            *seconds_warning;
        char            *secondary_text;
        int              seconds_to_show;
        static char     *session_type = NULL;

        logout_dialog = (GsmLogoutDialog *) data;

        if (!logout_dialog->priv->timeout) {
                gtk_dialog_response (GTK_DIALOG (logout_dialog),
                                     logout_dialog->priv->default_response);

                return FALSE;
        }

        if (logout_dialog->priv->timeout <= 30) {
                seconds_to_show = logout_dialog->priv->timeout;
        } else {
                seconds_to_show = (logout_dialog->priv->timeout/10) * 10;

                if (logout_dialog->priv->timeout % 10)
                        seconds_to_show += 10;
        }

        switch (logout_dialog->priv->type) {
        case GSM_DIALOG_LOGOUT_TYPE_LOGOUT:
                seconds_warning = ngettext ("You will be automatically logged "
                                            "out in %d second.",
                                            "You will be automatically logged "
                                            "out in %d seconds.",
                                            seconds_to_show);
                break;

        case GSM_DIALOG_LOGOUT_TYPE_SHUTDOWN:
                seconds_warning = ngettext ("This system will be automatically "
                                            "shut down in %d second.",
                                            "This system will be automatically "
                                            "shut down in %d seconds.",
                                            seconds_to_show);
                break;

        default:
                g_assert_not_reached ();
        }

        if (session_type == NULL) {
		GsmConsolekit *consolekit;

                consolekit = gsm_get_consolekit ();
                session_type = gsm_consolekit_get_current_session_type (consolekit);
                g_object_unref (consolekit);
        }

        if (g_strcmp0 (session_type, GSM_CONSOLEKIT_SESSION_TYPE_LOGIN_WINDOW) != 0) {
                char *name, *tmp;

                name = g_locale_to_utf8 (g_get_real_name (), -1, NULL, NULL, NULL);

                if (!name || name[0] == '\0' || strcmp (name, "Unknown") == 0) {
                        name = g_locale_to_utf8 (g_get_user_name (), -1 , NULL, NULL, NULL);
                }

                if (!name) {
                        name = g_strdup (g_get_user_name ());
                }

                tmp = g_strdup_printf (_("You are currently logged in as \"%s\"."), name);
                secondary_text = g_strconcat (tmp, "\n", seconds_warning, NULL);
                g_free (tmp);

                g_free (name);
	} else {
		secondary_text = g_strdup (seconds_warning);
	}

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (logout_dialog),
                                                  secondary_text,
                                                  seconds_to_show,
                                                  NULL);

        logout_dialog->priv->timeout--;

        g_free (secondary_text);

        return TRUE;
}

static void
gsm_logout_dialog_set_timeout (GsmLogoutDialog *logout_dialog)
{
        logout_dialog->priv->timeout = AUTOMATIC_ACTION_TIMEOUT;

        /* Sets the secondary text */
        gsm_logout_dialog_timeout (logout_dialog);

        if (logout_dialog->priv->timeout_id != 0) {
                g_source_remove (logout_dialog->priv->timeout_id);
        }

        logout_dialog->priv->timeout_id = g_timeout_add (1000,
                                                         gsm_logout_dialog_timeout,
                                                         logout_dialog);
}

#if defined(__x86) || defined(__x86__)
static void
fast_reboot_cb (GtkWidget *button, gpointer data)
{
	GsmLogoutDialog *logout_dialog = (GsmLogoutDialog *)data;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
		logout_dialog->priv->fast = 1;
	} else {
		logout_dialog->priv->fast = 0;
	}
}

/* Option to skip boot menu on restart. */
static GtkWidget *
get_fast_reboot_option (GsmLogoutDialog *logout_dialog)
{
	GtkWidget *check;

	/* SUN_BRANDING */
	check = gtk_check_button_new_with_mnemonic (_("S_kip boot menu on restart"));
	gtk_widget_show (check);
	g_signal_connect (GTK_WIDGET (check),
			"toggled",
			G_CALLBACK (fast_reboot_cb),
			logout_dialog);
	gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (check), TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);

	return check;
}

static void
boot_environment_cb (GtkWidget *button, gpointer data)
{
	GsmLogoutDialog *logout_dialog = (GsmLogoutDialog *)data;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button))) {
		gint id;

		id = (gint)g_object_get_data (G_OBJECT (button), "id");
		logout_dialog->priv->id = id;
	}
}

static GtkWidget *
get_warning ()
{
	GtkBox *hbox;
	GtkImage *image;
	GtkWidget *label;

	hbox = gtk_hbox_new (FALSE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
			GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	label = gtk_label_new (NULL);
	gtk_widget_show (label);
	gtk_label_set_markup (GTK_LABEL (label),
	/* SUN_BRANDING */
		_("<span style=\"italic\" size=\"smaller\">Options apply only after Restart, not after Shut Down</span>"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	return hbox;
}

/* Options to choose BE. */
static GtkWidget *
get_be_option (GsmLogoutDialog *logout_dialog)
{
	GtkWidget *warning;
	GtkWidget *label;
	GtkWidget *radio;
	GtkWidget *scroll;
	GtkWidget *vbox;
	GSList *group;
	GPtrArray *array = NULL;
	int i;

	warning = get_warning ();
	gtk_widget_show (warning);

	/* SUN_BRANDING */
	label = gtk_label_new_with_mnemonic (_("_Default boot environment at next restart:"));
	gtk_widget_show (label);

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_widget_show (vbox);

	gsm_consolekit_get_available_systems (logout_dialog->priv->consolekit,
			&array);
	group = NULL;
	for (i = 0; i < array->len; i++) {
		GValue elem = {0};
		gint id;
		gchar *name;
		gboolean is_default = FALSE;

		g_value_init (&elem, OS_STRUCT_TYPE);
		g_value_set_static_boxed (&elem, g_ptr_array_index (array, i));
		dbus_g_type_struct_get (&elem,
				0, &id,
				3, &name,
				4, &is_default,
				G_MAXUINT);

		radio = gtk_radio_button_new_with_label (group, name);
		gtk_widget_show (radio);
		g_object_set_data (G_OBJECT (radio), "id", (gpointer)id);
		if (is_default) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
					TRUE);
			logout_dialog->priv->id = id;
		}
		gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 0);
		group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio));
		g_signal_connect (GTK_WIDGET (radio),
				"toggled",
				G_CALLBACK (boot_environment_cb),
				logout_dialog);
	}
	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scroll);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll),
			vbox);

	/* a new vbox to put label, scrolled window and warning in */
	vbox = gtk_vbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), scroll, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), warning, FALSE, FALSE, 0);

	if (array->len > 1)
		gtk_widget_show (vbox);
	g_ptr_array_free (array, TRUE);

	return vbox;
}

static void
create_fastreboot_options (GsmLogoutDialog *logout_dialog)
{
	GtkWidget *expander;
	GtkWidget *align;
	GtkWidget *check;
	GtkWidget *be;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GList *list;
	GList *l;
	gint spacing;
	gint size;

	/* SUN_BRANDING */
	expander = gtk_expander_new_with_mnemonic (_("_More Options"));
	gtk_widget_show (expander);
	gtk_widget_style_get (expander, "expander-size", &size,
			"expander-spacing", &spacing, NULL);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);

	check = get_fast_reboot_option (logout_dialog);
	align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_widget_show (align);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0,
			(size + 2 * spacing), 0);
	gtk_container_add (GTK_CONTAINER (align), check);
	gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 10);

	be = get_be_option (logout_dialog);
	align = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_widget_show (align);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0,
			(size + 2 * spacing), 0);
	gtk_container_add (GTK_CONTAINER (align), be);
	gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

	gtk_container_add (GTK_CONTAINER (expander), vbox);

	/*
	 * Find the right container to put the expander in.
	 * This is gtk dialog imeplementation specific.
	 * So if there is something wrong with the layout
	 * the code need to be udpated
	 */
	hbox = NULL;
	vbox = gtk_dialog_get_content_area (GTK_DIALOG
			(logout_dialog)); 
	list = gtk_container_get_children (GTK_CONTAINER (vbox));
	for (l = list; l != NULL; l = g_list_next (l)) {
		GtkWidget *widget;

		widget = l->data;
		if (GTK_IS_HBOX(widget)) {
			hbox = widget;
			break;
		}
	}
	g_list_free (list);
	g_assert (hbox != NULL);

	vbox = NULL;
	list = gtk_container_get_children (GTK_CONTAINER (hbox));
	for (l = list; l != NULL; l = g_list_next (l)) {
		GtkWidget *widget;

		widget = l->data;
		if (GTK_IS_VBOX(widget)) {
			vbox = widget;
			break;
		}
	}
	g_list_free (list);
	g_assert (vbox != NULL);

	if (vbox) {
		gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE,
				FALSE, 10);
	} else {
		gtk_widget_destroy (expander);
	}

	return;
}
#endif


static GtkWidget *
gsm_get_dialog (GsmDialogLogoutType type,
                GdkScreen          *screen,
                guint32             activate_time)
{
        GsmLogoutDialog *logout_dialog;
        GtkWidget       *dialog_image;
        const char      *primary_text;
        const char      *icon_name;

        if (current_dialog != NULL) {
                gtk_widget_destroy (GTK_WIDGET (current_dialog));
        }

        logout_dialog = g_object_new (GSM_TYPE_LOGOUT_DIALOG, NULL);

        current_dialog = logout_dialog;

        gtk_window_set_title (GTK_WINDOW (logout_dialog), "");

        logout_dialog->priv->type = type;

        icon_name = NULL;
        primary_text = NULL;

        switch (type) {
        case GSM_DIALOG_LOGOUT_TYPE_LOGOUT:
                icon_name    = GSM_ICON_LOGOUT;
                primary_text = _("Log out of this system now?");

                logout_dialog->priv->default_response = GSM_LOGOUT_RESPONSE_LOGOUT;

                if (gsm_logout_supports_switch_user (logout_dialog)) {
                        gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                               _("_Switch User"),
                                               GSM_LOGOUT_RESPONSE_SWITCH_USER);
                }

                gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL);

                gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                       _("_Log Out"),
                                       GSM_LOGOUT_RESPONSE_LOGOUT);

                break;
        case GSM_DIALOG_LOGOUT_TYPE_SHUTDOWN:
                icon_name    = GSM_ICON_SHUTDOWN;
                primary_text = _("Shut down this system now?");

                logout_dialog->priv->default_response = GSM_LOGOUT_RESPONSE_SHUTDOWN;

                if (gsm_logout_supports_system_suspend (logout_dialog)) {
                        gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                               _("S_uspend"),
                                               GSM_LOGOUT_RESPONSE_SLEEP);
                }

                if (gsm_logout_supports_system_hibernate (logout_dialog)) {
                        gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                               _("_Hibernate"),
                                               GSM_LOGOUT_RESPONSE_HIBERNATE);
                }

                if (gsm_logout_supports_reboot (logout_dialog)) {
#if defined(__x86) || defined(__x86__)
			/* fast reboot support */
			create_fastreboot_options(logout_dialog);
#endif
                        gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                               _("_Restart"),
                                               GSM_LOGOUT_RESPONSE_REBOOT);
                }

                gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL);

                if (gsm_logout_supports_shutdown (logout_dialog)) {
                        gtk_dialog_add_button (GTK_DIALOG (logout_dialog),
                                               _("_Shut Down"),
                                               GSM_LOGOUT_RESPONSE_SHUTDOWN);
                }
                break;
        default:
                g_assert_not_reached ();
        }

        dialog_image = gtk_message_dialog_get_image (GTK_MESSAGE_DIALOG (logout_dialog));

        gtk_image_set_from_icon_name (GTK_IMAGE (dialog_image),
                                      icon_name, GTK_ICON_SIZE_DIALOG);
        gtk_window_set_icon_name (GTK_WINDOW (logout_dialog), icon_name);
        gtk_window_set_position (GTK_WINDOW (logout_dialog), GTK_WIN_POS_CENTER_ALWAYS);
        gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (logout_dialog), primary_text);

        gtk_dialog_set_default_response (GTK_DIALOG (logout_dialog),
                                         logout_dialog->priv->default_response);

        gtk_window_set_screen (GTK_WINDOW (logout_dialog), screen);

        return GTK_WIDGET (logout_dialog);
}

gchar *
gsm_logout_dialog_get_restart_parameters (GsmLogoutDialog *logout_dialog)
{
	gchar *param = NULL;

	if (logout_dialog->priv->id < 0)
		return NULL;

	param = g_strdup_printf("id:%d fast:%d",
			logout_dialog->priv->id,
			logout_dialog->priv->fast);

	return param;
}

GtkWidget *
gsm_get_shutdown_dialog (GdkScreen *screen,
                         guint32    activate_time)
{
        return gsm_get_dialog (GSM_DIALOG_LOGOUT_TYPE_SHUTDOWN,
                               screen,
                               activate_time);
}

GtkWidget *
gsm_get_logout_dialog (GdkScreen *screen,
                       guint32    activate_time)
{
        return gsm_get_dialog (GSM_DIALOG_LOGOUT_TYPE_LOGOUT,
                               screen,
                               activate_time);
}
