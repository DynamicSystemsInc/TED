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

#ifndef TSTRIPE_XUTILS_H
#define TSTRIPE_XUTILS_H

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS

void	 _tstripe_window_strut_set	(GdkWindow        *gdk_window,
					 guint32           strut,
					 guint32           strut_start,
					 guint32           strut_end);
gboolean  _tstripe_get_window		(Window  xwindow,
					 Atom    atom,
					 Window *val);
gboolean _tstripe_get_cardinal		(Window  xwindow,
                              		 Atom    atom,
                              		 int    *val);
void     _tstripe_set_cardinal		(Window  xwindow,
                              		 Atom    atom,
                              		 int    *val);
gboolean _tstripe_get_atom			(Window  xwindow,
                          	  		 Atom    atom,
                               		 Atom   *val);
char**   _tstripe_get_utf8_list		(Window   xwindow,
                               		 Atom     atom);
void     _tstripe_set_utf8_list		(Window   xwindow,
                               		 Atom     atom,
                               		 char   **list);
Atom     _tstripe_atom_get			(const char *atom_name);

G_END_DECLS

#endif /* TSTRIPE_XUTILS_H */
