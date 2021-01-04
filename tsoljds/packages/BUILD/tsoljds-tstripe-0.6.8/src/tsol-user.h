/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef TSTRIPE_TSOL_USER_H
#define TSTRIPE_TSOL_USER_H

#include <pwd.h>
#include <user_attr.h>
#include <glib-2.0/glib.h>
#include <gdk/gdk.h>
#include <pwd.h>
#include <bsm/adt.h>

G_BEGIN_DECLS
#define PW_BUF_LEN 256
#define MAX_USERS 130

typedef struct _User
{
	gboolean authenticated;
	char		*pw_buffer;
	struct	passwd	*p;
	userattr_t	*u;
} User;

/* FIXME - see if we can make this a non-global */
/* List of valid roles (including users own WorkstationOwner UID) for the user */
GSList *users;

void _tstripe_users_init ();
User* _tstripe_user_find_user_by_name (const char *username);
int _tstripe_user_get_user_index (User *user);
int _tstripe_user_count_get ();
int _tstripe_authenticate_role 	(User 	*user, GdkScreen *screen, int r, int w);
int _tstripe_solaris_setcred    (const char* prog_name, const char* user, uid_t uid, gid_t gid);
int	_tstripe_solaris_chauthtok  (const char* prog_name, const char* user, uid_t uid, gid_t gid, GdkScreen *screen, int readfd, int writefd);

G_END_DECLS

#endif /* TSTRIPE_TSOL_USER_H */
