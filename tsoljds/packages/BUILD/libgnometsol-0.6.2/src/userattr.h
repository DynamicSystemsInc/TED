/* Solaris Trusted Extensions GNOME desktop library.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.

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

#ifndef USERATTR_H
#define USERATTR_H

#include <user_attr.h>

#define USERATTR_CLEARANCE              "clearance"
#define USERATTR_LABELVIEW              "labelview"
#define USERATTR_LABELVIEW_EXTERNAL     "external"
#define USERATTR_LABELVIEW_HIDESL       "hidesl"
#define USERATTR_HIDESL                 USERATTR_LABELVIEW_HIDESL
#define USERATTR_LABELVIEW_INTERNAL     "internal"
#define USERATTR_LABELVIEW_SHOWSL       "showsl"
#define USERATTR_LABELTRANS             "labeltrans"
#define USERATTR_LOCK                   "lock_after_retries"
#define USERATTR_LOCK_NO                "no"
#define USERATTR_LOCK_YES               "yes"
#define USERATTR_MINLABEL               "min_label"
#define USERATTR_PASSWD                 "password"
#define USERATTR_PASSWD_AUTOMATIC       "automatic"
#define USERATTR_PASSWD_MANUAL          "manual"
#define USERATTR_TYPE_ROLE              USERATTR_TYPE_NONADMIN_KW

char *gnome_tsol_get_usrattr_val (userattr_t *uattr, char *keywd);

#endif /* USERATTR_H */
