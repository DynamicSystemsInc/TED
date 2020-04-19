/*Solaris Trusted Extensions GNOME desktop application.

  Copyright (C) 2009 Sun Microsystems, Inc. All Rights Reserved.

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

#include "privs.h"
#include <priv.h>
#include <secdb.h>
#include <user_attr.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void
drop_inherited_privs (void)
{
        priv_set_t *pset;
        userattr_t *uattr = NULL;
        char *value = NULL;

        pset = priv_allocset ();
        if ((uattr = getuseruid (getuid())) &&
            (value = kva_match (uattr->attr, USERATTR_DFLTPRIV_KW))) {
                pset = priv_str_to_set (value, ",", NULL);
		free_userattr (uattr);
        } else {
                pset = priv_str_to_set ("basic", ",", NULL);
        }

        setppriv (PRIV_SET, PRIV_INHERITABLE, pset);
        priv_freeset (pset);
}

void
escalate_inherited_privs (void)
{
        priv_set_t *pset;

        if ((pset = priv_allocset ()) == NULL) {
                perror ("Can't allocate priv set\n");
                exit (1);
        }
        if (getppriv (PRIV_PERMITTED, pset) == -1) {
                perror ("Can't get process privileges\n");
                exit (1);
        }
        if (setppriv (PRIV_SET, PRIV_INHERITABLE, pset) == -1) {
                perror ("Can't set process privileges\n");
                exit (1);
        }
}

gboolean
has_required_privileges (char *name)
{
        priv_set_t     *permitted_privs = priv_allocset ();

        /* Check that we have our expected privileges (full set) */
        if (getppriv (PRIV_PERMITTED, permitted_privs) == -1) {
                fprintf (stderr,
                         "%s: error getting permitted privilege set: %s\n",
			 name,
                         strerror (errno));
                priv_freeset (permitted_privs);
                return FALSE;
        }
        if (!priv_isfullset (permitted_privs)) {
                priv_freeset (permitted_privs);
                return FALSE;
        }
        priv_freeset (permitted_privs);

        return TRUE;
}
