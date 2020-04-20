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

#include "xutils.h"
#include <string.h>
#include <stdio.h>
enum {
	STRUT_LEFT = 0,
	STRUT_RIGHT = 1,
	STRUT_TOP = 2,
	STRUT_BOTTOM = 3,
	STRUT_LEFT_START = 4,
	STRUT_LEFT_END = 5,
	STRUT_RIGHT_START = 6,
	STRUT_RIGHT_END = 7,
	STRUT_TOP_START = 8,
	STRUT_TOP_END = 9,
	STRUT_BOTTOM_START = 10,
	STRUT_BOTTOM_END = 11
};

void
_tstripe_window_strut_set (GdkWindow * gdk_window,
			   guint32 strut,
			   guint32 strut_start,
			   guint32 strut_end)
{
	static Atom     net_wm_strut = None;
	static Atom     net_wm_strut_partial = None;
	static Atom     net_wm_window_type_dock = None;
	static Atom     net_wm_window_type = None;
	Display        *display;
	Window          window;
	gulong          struts[12] = {0,};
	Atom            atoms[2];
	int             i = 0;

	g_return_if_fail (GDK_IS_WINDOW (gdk_window));

	display = GDK_WINDOW_XDISPLAY (gdk_window);
	window = GDK_WINDOW_XID (gdk_window);

	if (net_wm_strut == None)
		net_wm_strut = XInternAtom (display, "_NET_WM_STRUT", False);
	if (net_wm_strut_partial == None)
		net_wm_strut_partial = XInternAtom (display, "_NET_WM_STRUT_PARTIAL", False);
	if (net_wm_window_type_dock == None)
		net_wm_window_type_dock = XInternAtom (display, "_NET_WM_WINDOW_TYPE_DOCK", False);
	if (net_wm_window_type == None)
		net_wm_window_type = XInternAtom (display, "_NET_WM_WINDOW_TYPE", False);

	atoms[i++] = net_wm_window_type_dock;

	struts[STRUT_TOP] = strut;
	struts[STRUT_TOP_START] = strut_start;
	struts[STRUT_TOP_END] = strut_end;

	gdk_error_trap_push ();
	XChangeProperty (display, window, net_wm_strut,
			 XA_CARDINAL, 32, PropModeReplace,
			 (guchar *) & struts, 4);
	XChangeProperty (display, window, net_wm_strut_partial,
			 XA_CARDINAL, 32, PropModeReplace,
			 (guchar *) & struts, 12);
	XChangeProperty (display, window, net_wm_window_type,
			 XA_ATOM, 32, PropModeReplace,
			 (guchar *) & atoms, i);

	gdk_error_trap_pop ();
}

gboolean
_tstripe_get_window (Window xwindow,
		     Atom atom,
		     Window * val)
{
	Atom            type;
	int             format;
	gulong          nitems;
	gulong          bytes_after;
	Window         *w;
	int             err, result;

	*val = 0;

	gdk_error_trap_push ();
	type = None;
	result = XGetWindowProperty (gdk_x11_get_default_xdisplay(),
				     xwindow,
				     atom,
				     0, G_MAXLONG,
				  False, XA_WINDOW, &type, &format, &nitems,
				     &bytes_after, (guchar **) & w);
	gdk_error_trap_pop ();
	if (result != Success)
		return FALSE;

	if (type != XA_WINDOW) {
		XFree (w);
		return FALSE;
	}
	*val = *w;

	XFree (w);

	return TRUE;
}

gboolean
_tstripe_get_cardinal (Window xwindow,
		       Atom atom,
		       int *val)
{
	Atom            type;
	int             format;
	gulong          nitems;
	gulong          bytes_after;
	gulong         *num;
	int             err, result;

	*val = 0;

	gdk_error_trap_push ();
	type = None;
	result = XGetWindowProperty (gdk_x11_get_default_xdisplay(),
				     xwindow,
				     atom,
				     0, G_MAXLONG,
				False, XA_CARDINAL, &type, &format, &nitems,
				     &bytes_after, (guchar **) & num);
	XSync (gdk_x11_get_default_xdisplay(), False);
	err = gdk_error_trap_pop ();
	if (err != Success ||
			result != Success)
		return FALSE;

	if (type != XA_CARDINAL) {
		XFree (num);
		return FALSE;
	}
	*val = *num;
	XFree (num);
	return TRUE;
}

void
_tstripe_set_cardinal (Window xwindow,
		       Atom atom,
		       int *val)
{
	unsigned long   data[1];

	data[0] = *val;

	gdk_error_trap_push ();
	XChangeProperty (gdk_x11_get_default_xdisplay(),
			 xwindow,
			 atom,
			 XA_CARDINAL,
			 32, PropModeReplace,
			 (guchar *) data, 1);

	XSync (gdk_x11_get_default_xdisplay(), False);
	gdk_error_trap_pop ();
}

gboolean
_tstripe_get_atom (Window xwindow,
		   Atom atom,
		   Atom * val)
{
	Atom            type;
	int             format;
	gulong          nitems;
	gulong          bytes_after;
	Atom           *a;
	int             err, result;

	*val = 0;

	gdk_error_trap_push ();
	type = None;
	result = XGetWindowProperty (gdk_x11_get_default_xdisplay(),
				     xwindow,
				     atom,
				     0, G_MAXLONG,
				     False, XA_ATOM, &type, &format, &nitems,
				     &bytes_after, (guchar **) & a);
	XSync (gdk_x11_get_default_xdisplay(), False);
	err = gdk_error_trap_pop ();
	if (err != Success ||
			result != Success)
		return FALSE;

	if (type != XA_ATOM) {
		XFree (a);
		return FALSE;
	}
	*val = *a;
	XFree (a);
	return TRUE;
}

char          **
_tstripe_get_utf8_list (Window xwindow,
			Atom atom)
{
	Atom            type;
	int             format;
	gulong          nitems;
	gulong          bytes_after;
	char           *val;
	int             err, result;
	Atom            utf8_string;
	char          **retval;
	int             i;
	int             n_strings;
	char           *p;

	utf8_string = _tstripe_atom_get ("UTF8_STRING");

	gdk_error_trap_push ();
	type = None;
	val = NULL;
	result = XGetWindowProperty (gdk_x11_get_default_xdisplay(),
				     xwindow,
				     atom,
				     0, G_MAXLONG,
				     False, utf8_string,
				     &type, &format, &nitems,
				     &bytes_after, (guchar **) & val);
	XSync (gdk_x11_get_default_xdisplay(), False);
	err = gdk_error_trap_pop ();

	if (err != Success ||
			result != Success)
		return NULL;

	if (type != utf8_string ||
			format != 8 ||
			nitems == 0) {
		if (val)
			XFree (val);
		return NULL;
	}
	/*
	 * I'm not sure this is right, but I'm guessing the property is
	 * nul-separated
	 */
	i = 0;
	n_strings = 0;
	while (i < nitems) {
		if (val[i] == '\0')
			++n_strings;
		++i;
	}

	if (val[nitems - 1] != '\0')
		++n_strings;

	/*
	 * we're guaranteed that val has a nul on the end by
	 * XGetWindowProperty
	 */

	retval = g_new0 (char *, n_strings + 1);
	p = val;
	i = 0;
	while (i < n_strings) {
		if (!g_utf8_validate (p, -1, NULL)) {
#if 0
			g_warning ("Property %s contained invalid UTF-8\n",
				   _tstripe_atom_name (atom));
#endif
			g_warning ("Poperty %ld contained invalid UTF-8\n",
				   atom);
			XFree (val);
			g_strfreev (retval);
			return NULL;
		}
		retval[i] = g_strdup (p);
		p = p + strlen (p) + 1;
		++i;
	}
	XFree (val);
	return retval;
}

void
_tstripe_set_utf8_list (Window xwindow,
			Atom atom,
			char **list)
{
	Atom            utf8_string;
	GString        *flattened;
	int             i;

	utf8_string = _tstripe_atom_get ("UTF8_STRING");

	/* flatten to nul-separated list */
	flattened = g_string_new ("");
	i = 0;

	/* The old method that escaped on NULL terminators */
	while (list[i] != NULL) {
		g_string_append_len (flattened, list[i],
				     strlen (list[i]) + 1);
		++i;
	}

	gdk_error_trap_push ();
	XChangeProperty (gdk_x11_get_default_xdisplay(),
			 xwindow,
			 atom,
			 utf8_string, 8, PropModeReplace,
			 (const guchar *) flattened->str,
			 flattened->len);

	XSync (gdk_x11_get_default_xdisplay(), False);
	gdk_error_trap_pop ();

	g_string_free (flattened, TRUE);
}

Atom
_tstripe_atom_get (const char *atom_name)
{
	g_return_val_if_fail (atom_name != NULL, None);

	return XInternAtom (gdk_x11_get_default_xdisplay(), atom_name, FALSE);
}
