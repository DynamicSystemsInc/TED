/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

  The contents of this file are subject to the terms of the
  GNU General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this file except in compliance with the License.

  You should have received a copy of the License along with this
  file; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/gpl-2.0.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <lastlog.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>
#include <strings.h>
#include <user_attr.h>
#include <priv.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <stropts.h>
#include <sys/contract/process.h>
#include <sys/ctfs.h>
#include <zone.h>
#include <userattr.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>

#include "tsolmotd.h"

#define MOTDFILE       "/etc/motd"
#define LASTLOGFILE    "/var/adm/wtmpx"

static char    *
get_lastlogin (uid_t uid, struct passwd * pw)
{
	struct utmpx   *up, log_info, previous;
	struct tm      *tm;
	char            timestrbuf[100];
	gchar          *utf8_timestrbuf = NULL;
	char           *msg;
	gboolean        found = 0;

	utmpxname (LASTLOGFILE);
	setutxent ();
	while (up = getutxent ()) {
		if (strcmp (pw->pw_name, up->ut_user) == 0) {
			found++;
			log_info = previous;
			previous = *up;
		}
	}
	endutxent ();

	if (found == 0) {
		return (NULL);
	}
	if (found == 1)
		log_info = previous;

	if (log_info.ut_tv.tv_sec == 0) {
		return (NULL);
	}
	tm = localtime (&log_info.ut_tv.tv_sec);

	if (strftime (timestrbuf, sizeof (timestrbuf), NULL, tm) == 0) {
		return (NULL);
	}
	utf8_timestrbuf = g_locale_to_utf8 (timestrbuf, -1, NULL, NULL, NULL);
	if (log_info.ut_syslen == 0 || strcmp (log_info.ut_host, ":0") == 0) {
		msg = g_strdup_printf (_("%s on %s"), utf8_timestrbuf, 
				       log_info.ut_line);
	} else {
		msg = g_strdup_printf (_("%s from %s"), utf8_timestrbuf,
				       log_info.ut_host);
	}
	g_free (utf8_timestrbuf);

	return (msg);
}

void
tsol_motd_dialog_destroy (GtkWidget * dialog)
{
	MotdData       *data = g_object_get_data (G_OBJECT (dialog), "motd_data");

	g_free (data->label);

	g_free (data);

	gtk_widget_destroy (dialog);
}

GtkWidget      *
left_aligned_label_new (char *text)
{
	GtkWidget      *label = gtk_label_new (text);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

	return label;
}


GtkWidget      *
create_attributes_table (uid_t uid, bslabel_t sl, bclear_t clr)
{
	GtkWidget      *table;
	GtkWidget      *widget;
	char           *str = NULL;
	char           *tmp = NULL;
	userattr_t     *u_ent = getuseruid (uid);
	struct passwd  *pw = getpwuid (uid);

	table = gtk_table_new (8, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 5);

	widget = left_aligned_label_new (_ ("Session Attributes for:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 0, 1);
	widget = left_aligned_label_new (pw->pw_gecos);
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 0, 1);

	widget = left_aligned_label_new (_ ("Last Login:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 1, 2);
	str = get_lastlogin (uid, pw);
	widget = left_aligned_label_new (str);
	g_free (str);
	str = NULL;
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 1, 2);

	widget = left_aligned_label_new (_ ("Rights:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 2, 3);
	if (!(str = gnome_tsol_get_usrattr_val (u_ent, USERATTR_PROFILES_KW))) {
		perror ("tsoljdslabel: Can't get rights");
		exit (1);
	}
	widget = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
	gtk_entry_set_text (GTK_ENTRY (widget), str);
	str = NULL;
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 2, 3);

	widget = left_aligned_label_new (_ ("Roles:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 3, 4);
	if (!(str = gnome_tsol_get_usrattr_val (u_ent, USERATTR_ROLES_KW))) {
		str = g_strdup (_ ("none"));
	}
	widget = gtk_entry_new ();
	gtk_entry_set_editable (GTK_ENTRY (widget), FALSE);
	gtk_entry_set_text (GTK_ENTRY (widget), str);
	g_free (str);
	str = NULL;
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 3, 4);

	widget = left_aligned_label_new (_ ("Minimum Label:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 4, 5);
	label_to_str (&sl, &str, M_LABEL, LONG_NAMES);
	widget = left_aligned_label_new (str);
	g_free (str);
	str = NULL;
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 4, 5);

	widget = left_aligned_label_new (_ ("Maximum Clearance:"));
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 5, 6);
	label_to_str (&clr, &str, M_LABEL, LONG_NAMES);
	widget = left_aligned_label_new (str);
	g_free (str);
	str = NULL;
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 5, 6);

	if (!(str = gnome_tsol_get_usrattr_val (u_ent, USERATTR_IDLECMD_KW))) {
		perror ("tsoljdslabel: Can't get idle command");
		exit (1);
	}
	if (!strcmp (USERATTR_IDLECMD_LOCK_KW, str)) {
		str = _("lock");
	}
	tmp = g_strdup_printf (_ ("Idle time before session %s"), str);
	str = NULL;
	widget = left_aligned_label_new (tmp);
	g_free (tmp);
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 6, 7);
	if (!(str = gnome_tsol_get_usrattr_val (u_ent, USERATTR_IDLETIME_KW))) {
		perror ("tsoljdslabel: Can't get idle time");
		exit (1);
	}
	tmp = g_strdup_printf ("%s %s", str, _("minutes"));
	widget = left_aligned_label_new (tmp);
	str = NULL;
	g_free (tmp);
	gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 6, 7);

	return table;
}

GtkWidget      *
tsol_motd_dialog_new (uid_t uid, bslabel_t lower_sl, bclear_t upper_clear, 
		      gboolean sl_only)
{
	GtkWidget      *motd_dialog;
	GtkWidget      *scroller;
	GtkWidget      *motd_text_view;
	GtkWidget      *widget;
	GtkWidget      *vbox;
	GtkWidget      *table;
	GtkWidget      *attr_table;
	char           *motd_msg;

	GtkTextBuffer  *motd_buffer;

	MotdData       *data = g_new0 (MotdData, 1);

	data->label = NULL;

	motd_dialog = gtk_dialog_new_with_buttons (_ ("Message Of The Day"),
						   NULL, GTK_DIALOG_MODAL,
						   GTK_STOCK_OK,
						   GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size (GTK_WINDOW (motd_dialog), 640, 480);
	gtk_window_set_position (GTK_WINDOW (motd_dialog),
				 GTK_WIN_POS_CENTER_ALWAYS);

	table = gtk_table_new (3, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (motd_dialog)->vbox), table,
			    FALSE, FALSE, 10);
	widget = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
					   GTK_ICON_SIZE_DIALOG);
	gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 0, 1, GTK_SHRINK,
			  GTK_SHRINK, 5, 5);
	vbox = gtk_vbox_new (FALSE, FALSE);
	gtk_table_attach (GTK_TABLE (table), vbox, 1, 2, 0, 1,
			  GTK_FILL | GTK_EXPAND,
			  GTK_FILL | GTK_EXPAND, 5, 5);

	motd_buffer = gtk_text_buffer_new (NULL);
	g_file_get_contents (MOTDFILE, &motd_msg, NULL, NULL);
	gtk_text_buffer_set_text (motd_buffer, motd_msg, -1);
	g_free (motd_msg);
	motd_text_view = gtk_text_view_new_with_buffer (motd_buffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (motd_text_view), FALSE);
	g_object_set (G_OBJECT (motd_text_view), "wrap-mode", GTK_WRAP_WORD, NULL);
	scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
					     GTK_SHADOW_ETCHED_IN);
	gtk_container_add (GTK_CONTAINER (scroller), motd_text_view);

	widget = left_aligned_label_new (_ ("Message Of The Day"));
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), scroller, TRUE, TRUE, 0);

	widget = gtk_image_new_from_stock (GTK_STOCK_DIALOG_AUTHENTICATION,
					   GTK_ICON_SIZE_DIALOG);
	gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 1, 2, GTK_SHRINK,
			  GTK_SHRINK, 5, 5);

	widget = create_attributes_table (uid, lower_sl, upper_clear);

	gtk_table_attach (GTK_TABLE (table), widget,
			  1, 2, 1, 2, GTK_FILL | GTK_EXPAND,
			  GTK_FILL | GTK_EXPAND, 5, 5);

	if (blequal (&lower_sl, &upper_clear)) {
		data->checkbutton = NULL;
		data->label = strdup (bcleartoh (&upper_clear));
	} else {
		widget = gtk_check_button_new_with_label (
				  _ ("Restrict Session To A Single Label"));
		data->checkbutton = widget;
		gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2,
					   2, 3);
		if (sl_only) { 
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->checkbutton), TRUE);
			gtk_widget_set_sensitive (data->checkbutton, FALSE);
		}
	}

	gtk_dialog_set_default_response (GTK_DIALOG (motd_dialog),
					 GTK_RESPONSE_OK);

	g_object_set_data (G_OBJECT (motd_dialog), "motd_data", data);

	return motd_dialog;
}
