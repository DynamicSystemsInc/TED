/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

  The contents of this library are subject to the terms of the
  GNU Lesser General Public License version 2 (the "License")
  as published by the Free Software Foundation. You may not use
  this library except in compliance with the License.

  You should have received a copy of the License along with this
  library; see the file COPYING.  If not,you can obtain a copy
  at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html or by writing
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA. See the License for specific language
  governing permissions and limitations under the License.
*/

#include <config.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xtsol.h>
#include <gdk/gdkx.h>

#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <pwd.h>
#include <syslog.h>
#include <security/pam_appl.h>

#include "pam_conv.h"
#include "pam_dialog.h"
#include "message_dialog.h"

int 
gnometsol_pam_conv (int num_msg, struct pam_message ** msg,
		    struct pam_response ** response, void *info);

static void 
free_resp (int num_msg,
	   struct pam_response * pr);

static gchar*
prompt_in_global_zone (conv_info_t *info, gchar *msg, gchar *c)
{
	char buf[PAM_MAX_MSG_SIZE];
	int nbytes;
	write (info->writefd, c, 1);
	write (info->writefd, msg, strlen (msg));
	nbytes = read (info->readfd, buf, PAM_MAX_MSG_SIZE);
	buf[nbytes] = '\0';
	return g_strdup (buf);
}

/*
 * gnometsol_pam_conv():
 * 
 * This is a conv (conversation) function called from the PAM authentication
 * scheme.  It returns the user's password when requested by internal PAM
 * authentication modules and also logs any internal PAM error messages. ***************************************************************************
 */
/*
 * Note message strings (m->msg) are already localized may be PAM_MAX_MSG_SIZE
 * bytes long. Messages may contain multiple lines denoted by the newline
 * character ('\n') and other C language formatting characters such as alert
 * (bell, '\a'), backspace ('\b'), and tab ('\t'). When processing prompting
 * messages (PAM_PROMPT_ECHO_OFF, PAM_PROMPT_ECHO_ON), a final newline should
 * not be output. When processing error (PAM_ERROR_MSG) and informational
 * (PAM_TEXT_INFO) messages, a final newline should be output.  This
 * processing is noted in the code below.  GUI presentation may interpret
 * slightly differently.
 * 
 */
int
gnometsol_pam_conv (int num_msg, struct pam_message ** msg,
		    struct pam_response ** resp, void *info)
{
	GtkWidget      *pamdialog;
	GtkWidget      *msgdialog;
	GdkScreen      *screen = ((conv_info_t *) info)->screen;
	conv_info_t    *c_info = (conv_info_t *) info;

	if (!GDK_IS_SCREEN (screen))
		screen = gdk_screen_get_default ();

	struct pam_message *m = (struct pam_message *) * msg;
	struct pam_response *r;

	char           *password = NULL;
	int             i;
	gint            response;

	if (((conv_info_t *) info)->debug) {
		(void) fprintf (stderr, "PAM conversation %d messages\n",
				num_msg);
	}
	if (num_msg >= PAM_MAX_NUM_MSG) {
		syslog (LOG_AUTH | LOG_ERR, "too many PAM messages "
			"%d >= %d", num_msg, PAM_MAX_NUM_MSG);
		return (PAM_CONV_ERR);
	}
	if ((r = calloc (num_msg, sizeof (struct pam_response))) == NULL) {
		syslog (LOG_AUTH | LOG_ERR, "libgnometsol: unable "
			"to allocate PAM conversation memory %m");
		return (PAM_CONV_ERR);
	}
	*resp = r;
	(void) memset (*resp, 0, sizeof (struct pam_response));

	for (i = 0; i < num_msg; i++) {
		size_t          m_len = 0;
		gchar          *utf8_msg = NULL;
		gchar          *locale_msg = NULL;
		gchar          *p;
		gunichar        c;

		/* Bad message from service module */
		if (m->msg == NULL) {
			if (((conv_info_t *) info)->debug) {
				(void) fprintf (stderr, "PAM message[%d] "
				       " style %d NULL\n", i, m->msg_style);
			}
			syslog (LOG_AUTH | LOG_WARNING, "PAM message[%d] "
				"style %d NULL", i, m->msg_style);
			goto err;
		} else {
			utf8_msg = g_locale_to_utf8 (m->msg, -1, NULL, NULL, NULL);
			m_len = strlen (utf8_msg);
		}
		if (m_len > PAM_MAX_MSG_SIZE) {
			if (((conv_info_t *) info)->debug) {
				(void) fprintf (stderr, "PAM message[%d] "
					   " style %d length %d too long\n",
						i, m->msg_style, m_len);
			}
			syslog (LOG_AUTH | LOG_WARNING, "PAM message[%d] "
				"style %d length %d too long",
				i, m->msg_style, m_len);
			goto err;
		}
		r->resp = NULL;
		r->resp_retcode = 0;

		/*
		 * Fix up final newline: remove for prompts add back for
		 * messages
		 */
		p = g_utf8_prev_char (utf8_msg + m_len);
		c = g_utf8_get_char (p);
		if (c == '\n') {
			m_len = p - utf8_msg;
			*(utf8_msg + m_len) = '\0';
		}
		if (utf8_msg)
			locale_msg = g_locale_from_utf8 (utf8_msg, -1, NULL, NULL, NULL);
		if (((conv_info_t *) info)->debug) {
			(void) fprintf (stderr, "message[%d]/%d=%s\n",
					i, m->msg_style,
					locale_msg ? locale_msg : "(null)");
		}
		g_free (locale_msg);
		
		if (c_info->writefd == -1){/*cross zone prompting not required*/
			pamdialog = gnome_tsol_password_dialog_new ("", "", "", (((conv_info_t *) info)->sysmodal), TRUE);
			msgdialog = gnome_tsol_message_dialog_new (NULL,
							   GTK_DIALOG_MODAL,
							   GTK_MESSAGE_INFO,
							   GTK_BUTTONS_OK,
					 (((conv_info_t *) info)->sysmodal),
							   "",
							   NULL);
			gtk_window_set_screen (GTK_WINDOW (pamdialog), screen);
			gtk_window_set_screen (GTK_WINDOW (msgdialog), screen);
		}

		switch (m->msg_style) {
		case PAM_PROMPT_ECHO_ON:
			if (c_info->writefd != -1) { /* cross zone prompting */
				r->resp = prompt_in_global_zone(c_info,
						utf8_msg, "u");
				if (!strncmp (r->resp, "RESPONSE_CANCELLED",18))
					goto cancel;
			} else {
				/*Pop up a dialog to get user id from the user*/
				gnome_tsol_password_dialog_set_input_prompt (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), utf8_msg);
				gnome_tsol_password_dialog_set_input_visibility (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), TRUE);
				gnome_tsol_password_dialog_set_message (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), c_info->echoonmsg ? c_info->echoonmsg : "");
				response=gtk_dialog_run(GTK_DIALOG (pamdialog));

				if (response == GTK_RESPONSE_OK) {
					g_object_get (G_OBJECT (pamdialog), "input-text", &password, NULL);
					r->resp = g_strdup (password);
				} else goto cancel;
			}

			if (r->resp == NULL) {
				syslog (LOG_AUTH | LOG_ERR, "libgnometsol: unable "
					"to allocate PAM conversation response memory %m");
				*resp = NULL;
				goto err;
			}

			if (strlen (r->resp) > PAM_MAX_RESP_SIZE) {
				if (((conv_info_t *) info)->debug) {
					(void) fprintf (stderr, "PAM "
					 "message[%d] style %d response %d "
							"too long\n",
						    i, m->msg_style, m_len);
				}
				syslog (LOG_AUTH | LOG_WARNING, "PAM "
				"message[%d] style %d response %d too long",
					i, m->msg_style, m_len);
				goto err;
			}
			break;

		case PAM_PROMPT_ECHO_OFF:
			if (c_info->writefd != -1) { /* cross zone prompting */
				r->resp = prompt_in_global_zone(c_info,
						utf8_msg, "p");
				if (!strncmp (r->resp, "RESPONSE_CANCELLED",18))
					goto cancel;
			} else {
				/*
			 	* Pop up a dialog to get the password from
			 	* the user. Zap anything in the input field.
			 	*/
				gnome_tsol_password_dialog_set_input_prompt (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), utf8_msg);
				gnome_tsol_password_dialog_set_input_visibility (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), FALSE);
				gnome_tsol_password_dialog_set_message (GNOME_TSOL_PASSWORD_DIALOG (pamdialog), c_info->echooffmsg ? c_info->echooffmsg : "");
				response = gtk_dialog_run (GTK_DIALOG (pamdialog));

				if (response == GTK_RESPONSE_OK) {
					g_object_get (G_OBJECT (pamdialog), "input-text", &password, NULL);
					r->resp = g_strdup (password);
				} else goto cancel;
			}

			if (r->resp == NULL) {
				syslog (LOG_AUTH | LOG_ERR, "libgnometsol: unable "
					"to allocate PAM conversation response memory %m");
				*resp = NULL;
				goto err;
			}

			if (strlen (r->resp) > PAM_MAX_RESP_SIZE) {
				if (((conv_info_t *) info)->debug) {
					(void) fprintf (stderr, "PAM "
					 "message[%d] style %d response %d "
							"too long\n",
						    i, m->msg_style, m_len);
				}
				syslog (LOG_AUTH | LOG_WARNING, "PAM "
				"message[%d] style %d response %d too long",
					i, m->msg_style, m_len);
				goto err;
			}
			break;

		case PAM_ERROR_MSG:
			/* ensure newline for message */
			utf8_msg = g_renew (char, utf8_msg,  m_len + 2);
			g_strlcat (utf8_msg , "\n", m_len + 2);
			if (c_info->writefd != -1) { /* cross zone prompting */
				gchar *tmp;
				tmp = prompt_in_global_zone(c_info, 
							    utf8_msg, "e");
				g_free (tmp);
			} else {
				/*
			 	* Write information dialogue "utf8_msg" to user
			 	*/
				g_object_set (G_OBJECT (msgdialog), "message-type", GTK_MESSAGE_ERROR, NULL);
				gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (msgdialog), utf8_msg);
				gtk_widget_hide (GTK_WIDGET (pamdialog));
				gtk_dialog_run (GTK_DIALOG (msgdialog));
			}
			break;

		case PAM_TEXT_INFO:
			/* ensure newline for message */
			utf8_msg = g_renew (char, utf8_msg,  m_len + 2);
			g_strlcat (utf8_msg , "\n", m_len + 2);
			if (c_info->writefd != -1) { /* cross zone prompting */
				gchar *tmp;
				tmp = prompt_in_global_zone(c_info, 
							    utf8_msg, "i");
				g_free (tmp);
			} else {
				/*
			 	* write information dialogue "utf8_msg" to user
			 	*/
				g_object_set (G_OBJECT (msgdialog), "message-type", GTK_MESSAGE_INFO, NULL);
				gtk_message_dialog_set_markup (GTK_MESSAGE_DIALOG (msgdialog), utf8_msg);
				gtk_widget_hide (GTK_WIDGET (pamdialog));
				gtk_dialog_run (GTK_DIALOG (msgdialog));
			}
			break;

		default:
			syslog (LOG_AUTH | LOG_WARNING, "Invalid PAM "
				"message style %d", m->msg_style);
			goto err;
		}

		if (((conv_info_t *) info)->debug) {
			(void) fprintf (stderr, "response[%d]=", i);
			if (m->msg_style == PAM_PROMPT_ECHO_OFF) {
				(void) fprintf (stderr, "%s\n",
				      r->resp == NULL ? "NULL" : "xxxxxxx");
			} else {
				(void) fprintf (stderr, "%s\n",
					r->resp == NULL ? "NULL" : r->resp);
			}
		}
		/* next message/response */
		g_free (utf8_msg);
		m++;
		r++;
	}

	if (c_info->writefd == -1) {
		gtk_widget_hide (pamdialog);
		gtk_widget_destroy (pamdialog);
		gtk_widget_destroy (msgdialog);
	}
	return GNOME_TSOL_PAM_SUCCESS;
cancel:
	if (c_info->writefd == -1) {
		gtk_widget_hide (pamdialog);
		gtk_widget_destroy (pamdialog);
		gtk_widget_destroy (msgdialog);
	}
	free_resp (i, r);
	*resp = NULL;
	return GNOME_TSOL_PAM_CANCEL;
err:
	if (c_info->writefd == -1) {
		gtk_widget_hide (pamdialog);
		gtk_widget_destroy (pamdialog);
		gtk_widget_destroy (msgdialog);
	}
	free_resp (i, r);
	*resp = NULL;
	return GNOME_TSOL_PAM_CONV_ERR;
}

/*
 * Service modules don't clean up responses if an error is returned. Free
 * responses here.
 */
static void
free_resp (int num_msg, struct pam_response * pr)
{
	int             i;
	struct pam_response *r = pr;

	if (pr == NULL)
		return;

	for (i = 0; i < num_msg; i++, r++) {

		if (r->resp) {
			/*
			 * zap resp memory before freeing -- may be a
			 * password
			 */
			bzero (r->resp, strlen (r->resp));
			free (r->resp);
			r->resp = NULL;
		}
	}
	free (pr);
}
