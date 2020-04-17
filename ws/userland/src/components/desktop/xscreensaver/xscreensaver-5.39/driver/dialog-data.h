/* xscreensaver, Copyright (c) 1993-2008 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#ifndef __DIALOG_DATA_H__
#define __DIALOG_DATA_H__

#include <stdio.h>
#include "types.h"
#include "mlstring.h"

#define MAX_BYTES_PER_CHAR 8	/* UTF-8 uses no more than 3, I think */
#define MAX_PASSWD_CHARS   128	/* Longest possible passphrase */

struct passwd_dialog_data {

  saver_screen_info *prompt_screen;
  int previous_mouse_x, previous_mouse_y;

  /* "Characters" in the password may be a variable number of bytes long.
     typed_passwd contains the raw bytes.
     typed_passwd_char_size indicates the size in bytes of each character,
     so that we can make backspace work.
   */
  char typed_passwd [MAX_PASSWD_CHARS * MAX_BYTES_PER_CHAR];
  char typed_passwd_char_size [MAX_PASSWD_CHARS];

  XtIntervalId timer;
  int i_beam;

  float ratio;
  Position x, y;
  Dimension width;
  Dimension height;
  Dimension border_width;

  Bool echo_input;
  Bool show_stars_p; /* "I regret that I have but one asterisk for my country."
                        -- Nathan Hale, 1776. */

  char *heading_label;
  char *body_label;
  char *user_label;
  mlstring *info_label;
  /* The entry field shall only be displayed if prompt_label is not NULL */
  mlstring *prompt_label;
  char *date_label;
  char *passwd_string;
  Bool passwd_changed_p; /* Whether the user entry field needs redrawing */
  Bool caps_p;           /* Whether we saw a keypress with caps-lock on */
  char *unlock_label;
  char *login_label;
  char *uname_label;

  Bool show_uname_p;

#ifndef HAVE_XSCREENSAVER_LOCK
  XFontStruct *heading_font;
  XFontStruct *body_font;
  XFontStruct *label_font;
  XFontStruct *passwd_font;
  XFontStruct *date_font;
  XFontStruct *button_font;
  XFontStruct *uname_font;

  Pixel foreground;
  Pixel background;
  Pixel border;
  Pixel passwd_foreground;
  Pixel passwd_background;
  Pixel thermo_foreground;
  Pixel thermo_background;
  Pixel shadow_top;
  Pixel shadow_bottom;
  Pixel button_foreground;
  Pixel button_background;

  Dimension preferred_logo_width, logo_width;
  Dimension preferred_logo_height, logo_height;
  Dimension thermo_width;
  Dimension internal_border;
  Dimension shadow_width;

  Dimension passwd_field_x, passwd_field_y;
  Dimension passwd_field_width, passwd_field_height;

  Dimension unlock_button_x, unlock_button_y;
  Dimension unlock_button_width, unlock_button_height;

  Dimension login_button_x, login_button_y;
  Dimension login_button_width, login_button_height;

  Dimension thermo_field_x, thermo_field_y;
  Dimension thermo_field_height;

  Pixmap logo_pixmap;
  Pixmap logo_clipmask;
  int logo_npixels;
  unsigned long *logo_pixels;
#endif /* ! HAVE_XSCREENSAVER_LOCK */

  Cursor passwd_cursor;
  Bool unlock_button_down_p;
  Bool login_button_down_p;
  Bool login_button_p;
  Bool login_button_enabled_p;
  Bool button_state_changed_p; /* Refers to both buttons */

  Pixmap save_under;
  Pixmap user_entry_pixmap;

#ifdef HAVE_XSCREENSAVER_LOCK
  /* extern passwd dialog stuff */
  XtInputId stdout_input_id;
  int       stdin_fd;    /* child's stdin - parent writes to this */
  int       stdout_fd;   /* child's stdout - parent reads from this */
  FILE     *stdin_file;  /* child's stdin - parent writes to this */
  FILE     *stdout_file; /* child's stdout - parent reads from this */
  Bool      got_windowid;
  Bool      got_passwd;
#endif
};

#endif /* __DIALOG_DATA_H__ */
