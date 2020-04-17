/* lock-Gtk.c -- a GTK+ password dialog for xscreensaver
 * xscreensaver, Copyright (c) 1993-1998 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

/* GTK+ locking code written by Jacob Berkman  <jacob@ximian.com> for
 *  Sun Microsystems.
 *
 * Copyright (c) 2002, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_GTK2 /* whole file */

#include <xscreensaver-intl.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

/* AT-enabled */
#include <stdio.h>
#include <ctype.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xmu/WinUtil.h>

#include <gconf/gconf-client.h>
#include <atk/atkobject.h>

#include "remote.h"
#include "atoms.h"

#if GTK_CHECK_VERSION(2,14,0)
# define GET_WINDOW(w)          gtk_widget_get_window (w)
#else
# define GET_WINDOW(w)          ((w)->window)
#endif

static Atom XA_UNLOCK_RATIO;

typedef struct {
  GtkWidget *dialog;
  GtkWidget *user_prompt_label;
  GtkWidget *user_input_entry;
  GtkWidget *progress;
  GtkWidget *button;
  GtkWidget *msg_label;
  GtkWidget *pam_message_label;
} PasswdDialog;

/*Global info */
#define MAXRAISEDWINS  2

char *progname = 0;
FILE *parent_file = NULL; /* child writes to parent on this */

#define FD_TO_PARENT  9

/* Send a command to the xscreensaver parent daemon
   Arguments:
    - msg - type of message - "input", "raise_wid", etc.
    - data - data for message
    - flush - whether to flush now or allow stdio to buffer
   Message format sent to parent:
    "msg\n" if no data, otherwise "msg=data\n"

   Can be used to flush previously buffered messages by calling
   with NULL msg & data, and TRUE for flush.
 */
static int
write_to_parent (const char* msg, const char *data, gboolean flush)
{
  int len = 0;

  /*
  fprintf (stderr, "-->Child write_to_parent() string to send is: %s=%s\n",
           msg, data ? data : "(null)");
  fflush (stderr);
  */

  if (msg)
    {
      if (data)
        len = fprintf (parent_file, "%s=%s\n", msg, data);
      else
        len = fprintf (parent_file, "%s\n", msg);
    }

  if (flush)
    fflush (parent_file);

  return len;
}

/* Send parent a message with a window id as the data */
static void
write_windowid (const char* msg, Window w)
{
  char s[16]; /* more than long enough to hold a 32-bit integer + '\0' */

  snprintf(s, sizeof(s), "0x%lx", w);
  write_to_parent(msg, s, FALSE);
}

static GtkWidget *
load_unlock_logo_image (void)
{
  const char *logofile;
  struct stat statbuf;

  logofile = DEFAULT_ICONDIR "/unlock-logo.png";

  if (stat (logofile, &statbuf) != 0)
    {
      logofile = DEFAULT_ICONDIR "/logo-180.gif"; /* fallback */
    }

  return gtk_image_new_from_file (logofile);
}

/* Create unlock dialog */
static PasswdDialog *
make_dialog (gboolean center_pos)
{
  GtkWidget *dialog;
  AtkObject *atk_dialog;
  GtkWidget *frame1, *frame2;
  GtkWidget *vbox;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *bbox;
  GtkWidget *vbox2;
  GtkWidget *entry;
  AtkObject *atk_entry;
  GtkWidget *title_label, *msg_label, *prompt_label,
    *user_label, *date_label, *pam_msg_label;
  AtkObject *atk_title_label, *atk_prompt_label;
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *progress;
  char *version;
  char *user;
  char *host;
  char *s;
  gchar *format_string_locale, *format_string_utf8;
  PasswdDialog *pwd;

  /* taken from lock.c */
  char buf[256];
  gchar *utf8_format;
  time_t now = time (NULL);
  struct tm* tm;

  server_xscreensaver_version (GDK_DISPLAY (), &version, &user, &host);

  if (!version)
    {
      fprintf (stderr, "%s: no xscreensaver running on display %s, exiting.\n",
               progname, gdk_get_display ());
      exit (1);
    }

  /* PUSH */
  gtk_widget_push_colormap (gdk_rgb_get_cmap ());

  pwd = g_new0 (PasswdDialog, 1);

  dialog = gtk_window_new (GTK_WINDOW_POPUP);
  pwd->dialog = dialog;

  /*
  ** bugid: 5077989(P2)Bug 147580: password input dialogue obscures GOK
     gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
     bugid: 5002244:  scr unlock dialog incompatible with MAG technique
  ** 6182506: scr dialog is obscured by MAG window
  */
  if (center_pos)
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ALWAYS);
  else
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE); /*mali99 irritating*/

  /* AT-enabled dialog role = frame */
  atk_dialog = gtk_widget_get_accessible (dialog);
  atk_object_set_description (atk_dialog, _("screen unlock dialog"));

  /* frame */
  frame1 = g_object_new (GTK_TYPE_FRAME,
                         "shadow-type", GTK_SHADOW_OUT,
                         NULL);
  gtk_container_add (GTK_CONTAINER (dialog), frame1);
  /* AT role = panel */

  /* vbox */
  vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_container_add (GTK_CONTAINER (frame1), vbox);
  /* AT role= filler(default) */

  /* hbox */
  hbox1 = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox1,
                      TRUE, TRUE, 0);

  /* image frame */
  frame2 = g_object_new (GTK_TYPE_FRAME,
                         "shadow-type", GTK_SHADOW_ETCHED_IN,
                         NULL);
  gtk_box_pack_start (GTK_BOX (hbox1), frame2,
                      TRUE, TRUE, 0);
  /* AT role= filler(default) */

  /* image */
  image = load_unlock_logo_image ();
  /* AT role = icon */
  gtk_container_add (GTK_CONTAINER (frame2), image);

  /* progress thingie */
  progress = g_object_new (GTK_TYPE_PROGRESS_BAR,
                           "orientation", GTK_PROGRESS_BOTTOM_TO_TOP,
                           "fraction", 1.0,
                           NULL);
  gtk_box_pack_start (GTK_BOX (hbox1), progress,
                      FALSE, FALSE, 0);
  pwd->progress = progress;
  atk_object_set_description (gtk_widget_get_accessible (progress),
            _("Percent of time you have left to unlock the screen."));

  /* text fields */
  vbox2 = gtk_vbox_new (FALSE, 20);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2,
                      TRUE, TRUE, 0);
  /* AT role =filler */

  s = g_markup_printf_escaped ("<span size=\"xx-large\"><b>%s </b></span>",
                               _("Screensaver"));
  /* XScreenSaver foo label */
  title_label = g_object_new (GTK_TYPE_LABEL,
                              "use-markup", TRUE,
                              "label", s,
                              NULL);
  g_free (s);
  gtk_box_pack_start (GTK_BOX (vbox2), title_label,
                      FALSE, FALSE, 0);
  /* AT role = label prog name */
  atk_title_label = gtk_widget_get_accessible (title_label);
  atk_object_add_relationship (atk_title_label, ATK_RELATION_LABEL_FOR,
                               atk_dialog);
  atk_object_add_relationship (atk_dialog, ATK_RELATION_LABELLED_BY,
                               atk_title_label);

  /* This display is locked. */
  msg_label = g_object_new (GTK_TYPE_LABEL,
                            "use-markup", TRUE,
                            "label", _("<b>This display is locked.</b>"),
                            NULL);
  pwd->msg_label = msg_label;
  gtk_box_pack_start (GTK_BOX (vbox2), msg_label,
                      FALSE, FALSE, 0);

  /* User information */
  s = g_strdup_printf (_("User: %s"), user ? user : "");
  user_label = g_object_new (GTK_TYPE_LABEL,
                             "label", s,
                             "use_underline", TRUE,
                             NULL);
  g_free(s);
  gtk_label_set_width_chars (GTK_LABEL (user_label), 35);
  gtk_box_pack_start (GTK_BOX (vbox2), user_label, FALSE, FALSE, 0);

  /* User input */
  hbox2 = gtk_widget_new (GTK_TYPE_HBOX,
                          "border_width", 5,
                          "visible", TRUE,
                          "homogeneous", FALSE,
                          "spacing", 1,
                          NULL);

  /* PAM prompt */
  prompt_label = g_object_new (GTK_TYPE_LABEL,
                               /* blank space for prompt */
                               "label", _("         "),
                               "use_underline", TRUE,
                               "use_markup", FALSE,
                               "justify", GTK_JUSTIFY_CENTER,
                               "wrap", FALSE,
                               "selectable", FALSE,
                               "xalign", 1.0,
                               "xpad", 0,
                               "ypad", 0,
                               "visible", FALSE,
                               NULL);
  pwd->user_prompt_label = prompt_label;

  entry = g_object_new (GTK_TYPE_ENTRY,
                        "activates-default", TRUE,
                        "visible", TRUE,
                        "editable", TRUE,
                        "visibility", FALSE,
                        "can_focus", TRUE,
                        NULL);
  pwd->user_input_entry = entry;
  /* gtk_widget_grab_focus (entry); */
  atk_entry = gtk_widget_get_accessible (entry);
  atk_object_set_role (atk_entry, ATK_ROLE_PASSWORD_TEXT);

  /* AT role = label for input widget */
  atk_prompt_label = gtk_widget_get_accessible (prompt_label);
  atk_object_add_relationship (atk_prompt_label, ATK_RELATION_LABEL_FOR,
                               atk_entry);
  atk_object_add_relationship (atk_entry, ATK_RELATION_LABELLED_BY,
                               atk_prompt_label);

  gtk_box_pack_start (GTK_BOX (hbox2), prompt_label, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);

  pam_msg_label = g_object_new (GTK_TYPE_LABEL,
                                NULL);
  pwd->pam_message_label = pam_msg_label;

  gtk_box_pack_start (GTK_BOX (vbox2), pam_msg_label, FALSE, FALSE, 0);

  /* date string */
  tm = localtime (&now);
  memset (buf, 0, sizeof (buf));
  format_string_utf8 = _("%d-%b-%y (%a); %I:%M %p");
  format_string_locale = g_locale_from_utf8 (format_string_utf8, -1,
                                             NULL, NULL, NULL);
  strftime (buf, sizeof (buf) - 1, format_string_locale, tm);
  g_free (format_string_locale);

  utf8_format = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  s = g_markup_printf_escaped ("<small>%s</small>", utf8_format);
  g_free (utf8_format);

  date_label = g_object_new (GTK_TYPE_LABEL,
                             "use-markup", TRUE,
                             "label", s,
                             NULL);
  g_free (s);
  gtk_box_pack_start (GTK_BOX (vbox2), date_label,
                      FALSE, FALSE, 0);

  /* button box */
  bbox = g_object_new (GTK_TYPE_HBUTTON_BOX,
                       "layout-style", GTK_BUTTONBOX_END,
                       "spacing", 10,
                       NULL);

  /* Ok button */
  button = g_object_new (GTK_TYPE_BUTTON, "visible", FALSE, "label",
                         "Dismiss",  "can_focus", TRUE, NULL) ;
  pwd->button = button;

  gtk_box_pack_end (GTK_BOX (bbox), button,
                    FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), bbox, FALSE, FALSE, 0);

  free (user);
  free (version);
  free (host);

  /* POP */
  gtk_widget_pop_colormap ();

  return pwd;
}

/* Callback for when user has finished entering input, even though
   we don't display an "OK" button for them to click on */
static void
ok_clicked_cb (GtkWidget *button, PasswdDialog *pwd)
{
  const char *s;

  if (GTK_IS_BUTTON (button) && gtk_widget_get_visible (button)) /* Is it Dismiss Dialog Box */
    {
      write_to_parent ("dismiss", "true", TRUE);
      gtk_widget_hide (button);
    }
  else
    {
      g_object_set (pwd->msg_label, "label", _("<b>Checking...</b>"), NULL);

      s = gtk_entry_get_text (GTK_ENTRY (pwd->user_input_entry));
      write_to_parent ("input", s, TRUE);

      /* Reset password field to blank, else passwd field shows old passwd *'s,
         visible when passwd is expired, and pam is walking the user to change
         old passwd.
       */
      gtk_editable_delete_text (GTK_EDITABLE (pwd->user_input_entry), 0, -1);
      gtk_widget_hide (pwd->user_input_entry);
      gtk_widget_hide (pwd->user_prompt_label);
    }
}

static void
connect_signals (PasswdDialog *pwd)
{
  g_signal_connect (pwd->button, "clicked",
                    G_CALLBACK (ok_clicked_cb),
                    pwd);

  g_signal_connect (pwd->user_input_entry, "activate",
                    G_CALLBACK (ok_clicked_cb),
                    pwd);

  g_signal_connect (pwd->dialog, "delete-event",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  g_signal_connect (pwd->button, "button-press-event",
                    G_CALLBACK (ok_clicked_cb),
                    pwd);
}

static GdkFilterReturn
dialog_filter_func (GdkXEvent *xevent, GdkEvent *gevent, gpointer data)
{
  PasswdDialog *pwd = data;
  XEvent *event = xevent;
  gdouble ratio;

  if ((event->xany.type != ClientMessage ||
       event->xclient.message_type != XA_UNLOCK_RATIO))
    return GDK_FILTER_CONTINUE;

  ratio = event->xclient.data.l[0] / (gdouble)100.0;

  /* CR 6176524 passwdTimeoutEnable for disabled user */
  if (event->xclient.data.l[1] == 0)
    g_object_set (pwd->progress, "fraction", ratio, NULL);

  return GDK_FILTER_REMOVE;
}

static gboolean
handle_input (GIOChannel *source, GIOCondition cond, gpointer data)
{
  PasswdDialog *pwd = data;
  GIOStatus status;
  char *str;
  char *label;
  char *hmsg = NULL;  /* This is the heading of lock dialog..shows status */

  if (cond & G_IO_HUP) /* daemon crashed/exited/was killed */
    gtk_main_quit ();

  do
    {
      status = g_io_channel_read_line (source, &str, NULL, NULL, NULL);
    }
  while (status == G_IO_STATUS_AGAIN);

/* debug only
  if (status == G_IO_STATUS_ERROR)
      g_message ("handle input() status_error %s\n",str);
  if (status == G_IO_STATUS_EOF)
      g_message ("handle input() status_eof %s\n",str);
  if (status == G_IO_STATUS_NORMAL)
      g_message ("handle input() status_normal %s\n",str);
  Most likely, the returned error msg of g_io_channel_read_line(),
  i.e str will not be translated into other locales ...
*/

  if (str)
    {
      /* strip trailing newline */
      char *nl = strrchr(str, '\n');
      if (nl)
        *nl = 0;

      /*
      fprintf (stderr,">>>>>Child..in handle_input..string is:%s\n",str);
      fflush (stderr);
      */

      /* Handle commands from parent daemon */

      if (((strncmp (str, "ul_", 3)) == 0))
        {
          /* search for =, and if found, split into two strings there */
          char *msgstr = strchr(str, '='); /* Data sent with command */
          if (msgstr)
            *msgstr++ = 0;

          if ((strcmp (str, "ul_ok") == 0))
            {
              hmsg = _("Authentication Successful!");
            }
          else if ((strcmp (str, "ul_acct_ok") == 0))
            {
              hmsg = _("PAM Account Management Also Successful!");
            }
          else if ((strcmp (str, "ul_setcred_fail") == 0))
            {
              hmsg = _("Just a Warning PAM Set Credential Failed!");
            }
          else if ((strcmp (str, "ul_setcred_ok") == 0))
            {
              hmsg = _("PAM Set Credential Also Successful!");
            }
          else if ((strcmp (str, "ul_acct_fail") == 0))
            {
              hmsg = _("Your Password has expired.");
            }
          else if ((strcmp (str, "ul_fail") == 0))
            {
              hmsg = _("Sorry!");
            }
          else if ((strcmp (str, "ul_read") == 0))
            {
              hmsg = _("Waiting for user input!");
            }
          else if ((strcmp (str, "ul_time") == 0))
            {
              hmsg = _("Timed Out!");
            }
          else if ((strcmp (str, "ul_null") == 0))
            {
              hmsg = _("Still Checking!");
            }
          else if ((strcmp (str, "ul_cancel") == 0))
            {
              hmsg = _("Authentication Cancelled!");
            }
          else if ((strcmp (str, "ul_pamprompt") == 0))
            {
              gtk_label_set_text (GTK_LABEL (pwd->user_prompt_label), msgstr);
              gtk_widget_show (pwd->user_prompt_label);
              gtk_widget_hide (pwd->button);
              msgstr = NULL; /* clear message so we don't show it twice */
            }
          else if ((strcmp (str, "ul_prompt_echo") == 0))
            {
              if ((strcmp (msgstr, "true") == 0))
                {
                  gtk_entry_set_visibility
                    (GTK_ENTRY (pwd->user_input_entry), TRUE);
                }
              else
                {
                  if ((strcmp (msgstr, "stars") == 0))
                    /* reset to default display of "*" or bullet */
                    gtk_entry_unset_invisible_char
                      (GTK_ENTRY (pwd->user_input_entry));
                  else
                    /* set to no display */
                    gtk_entry_set_invisible_char
                       (GTK_ENTRY (pwd->user_input_entry), 0);

                  gtk_entry_set_visibility
                    (GTK_ENTRY (pwd->user_input_entry), FALSE);
                }
              msgstr = NULL; /* clear message so we don't show it to user */
              /* Show the entry field */
              gtk_widget_show (pwd->user_input_entry);
              gtk_widget_grab_focus (pwd->user_input_entry);
              gdk_display_sync
		  (gtk_widget_get_display (pwd->user_input_entry));
            }
          else if ((strcmp (str, "ul_message") == 0))
            {
              hmsg = NULL; /* only show msg */
            }
          else if ((strcmp (str, "ul_pam_msg") == 0))
            {
              GTK_WIDGET_SET_FLAGS (pwd->button,GTK_CAN_DEFAULT);
              gtk_widget_show (pwd->button);
              gtk_widget_grab_default (pwd->button);
              gtk_widget_grab_focus (pwd->button);
              hmsg = NULL; /* only show msg */
            }
          else
            {
              /* Should not be others, but if so just show it */
              hmsg = str;
            }

          if (hmsg)
            {
              label = g_markup_printf_escaped ("<b>%s</b>", hmsg);
              g_object_set (pwd->msg_label, "label", label, NULL);
              g_free (label);
            }

          if (msgstr)
            {
              gtk_label_set_text (GTK_LABEL (pwd->pam_message_label), msgstr);
            }
        }
      else if ((strcmp (str, "cmd_exit") == 0))
        {
          gtk_main_quit ();
        }
      else /* something came through that didn't start with ul_ */
        {
          gtk_label_set_text (GTK_LABEL (pwd->pam_message_label), str);
        }

      g_free (str);
    }

  return (status != G_IO_STATUS_EOF);
}

int
main (int argc, char *argv[])
{
  GIOChannel *ioc;
  PasswdDialog *pwd;
  char *s;
  char *real_progname = argv[0];
  GConfClient *client;
  const char *modulesptr = NULL;
  int i;
  const char *locale = NULL;

  gboolean  at_enable  = FALSE; /* accessibility mode enabled ? */
  gboolean center_position = TRUE; /* center dialog on screen? */

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

#ifdef HAVE_GTK2
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#else /* ! HAVE_GTK2 */
  if (!setlocale (LC_ALL, ""))
    fprintf (stderr, "%s: locale not supported by C library\n", real_progname);
#endif /* ! HAVE_GTK2 */
#endif /* ENABLE_NLS */

  s = strrchr (real_progname, '/');
  if (s) real_progname = s+1;
  progname = real_progname;

  parent_file = fdopen(FD_TO_PARENT, "w");
  if (!parent_file)
    {
      fprintf (stderr, "%s: can't communicate with parent, exiting.\n",
               progname);
      exit (1);
    }

  gtk_init (&argc, &argv);

  /* Intern the atoms that xscreensaver_command() needs.
   */
  {
    Display *dpy = gdk_x11_get_default_xdisplay();

    const struct atom_request unlock_atoms[] =
      {
	{ &XA_UNLOCK_RATIO, "UNLOCK_RATIO" },
	{ NULL, NULL } /* Must be last to terminate list */
      };

    const struct atom_request *atom_lists[3] = { NULL, NULL, NULL };
    atom_lists[0] = remote_control_atoms;
    atom_lists[1] = unlock_atoms;
    request_atoms (dpy, atom_lists);
  }

  /* bugid 6346056(P1):
     ATOK pallet sometimes appears in screensave/lock-screen mode
  */
  putenv ("GTK_IM_MODULE=gtk-im-context-simple");


  /* accessibility mode enabled ? */
  client = gconf_client_get_default ();
  at_enable = gconf_client_get_bool (client,
                                     "/desktop/gnome/interface/accessibility",
                                     NULL);
  if (at_enable)
    {

      /* GTK Accessibility Module initialized */
      modulesptr = g_getenv ("GTK_MODULES");
      if (!modulesptr || modulesptr [0] == '\0')
        putenv ("GTK_MODULES=gail:atk-bridge");

      /*
       * 6182506: unlock dialog can be obscured by the magnifier window
       * if it's always centered, so don't force that if any accessibility
       * helpers are present
       */
      center_position = FALSE;
    } /* accessibility enabled */

  pwd = make_dialog (center_position);
  connect_signals (pwd);

  gtk_widget_show_all (pwd->dialog);
  gtk_window_present (GTK_WINDOW (pwd->dialog));
  gtk_widget_map (pwd->dialog);

  gdk_display_sync (gtk_widget_get_display (pwd->dialog));

  gdk_window_add_filter (GET_WINDOW (pwd->dialog), dialog_filter_func, pwd);
  write_windowid ("dialog_win", GDK_WINDOW_XID (GET_WINDOW (pwd->dialog)));

  /* Flush dialog window ids & any messages about login helpers to parent */
  write_to_parent(NULL, NULL, TRUE);

  gtk_widget_grab_focus (pwd->user_input_entry);

  ioc = g_io_channel_unix_new (0);
  g_get_charset (&locale);
  g_io_channel_set_encoding(ioc, locale, NULL);
  g_io_add_watch (ioc, G_IO_IN | G_IO_HUP, handle_input, pwd);

  gtk_main ();

  return 0;
}
#endif /* HAVE_GTK2 */
