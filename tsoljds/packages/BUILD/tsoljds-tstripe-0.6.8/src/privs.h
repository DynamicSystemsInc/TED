/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2009 Sun Microsystems, Inc. All Rights Reserved.
  Copyright (c) 2020, Dynamic Systems, Inc.

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

#ifndef TSTRIPE_PRIVS_H
#define TSTRIPE_PRIVS_H

#include <glib.h>

G_BEGIN_DECLS

void escalate_inherited_privs (void);
void drop_inherited_privs (void);
gboolean has_required_privileges (char *name);

G_END_DECLS

#endif /* TSTRIPE_PRIVS_H */
