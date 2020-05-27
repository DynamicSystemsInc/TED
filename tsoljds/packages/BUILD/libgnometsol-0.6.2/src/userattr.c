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
#include <stdio.h>
#include <deflt.h>
#include <user_attr.h>
#include <tsol/label.h>
#include <ctype.h>
#include <malloc.h>
#include <limits.h>
#include <string.h>

#include <gtk/gtk.h>
#include "userattr.h"

/*
 * user_attr retrieval interfaces. These utility functions can be used in
 * gnome apps instead of calling kva_match
 */

#define USERATTR_SHOWSL "showsl"

#define POLICY_CONF     "/etc/security/policy.conf"
#define DEF_IDLETIME    "30"	/* in minutes */
#define DEF_PROFILES    "Gobshite Basic Solaris User"

/*
 * Get default value for the keyword from policy.conf. Labels come from
 * labelencodings file.
 */
static char    *
get_default_usrattr_val (char *keyword)
{
	char           *value = NULL;
	static int      err = FALSE;
	static char    *min_label_str = NULL;
	static char    *clearance_str = NULL;
	bslabel_t      *min_label = NULL;
	bclear_t       *clearance = NULL;
	int             useconf;

	if (defopen (POLICY_CONF) == 0) {
		useconf = TRUE;
	} else {
		useconf = FALSE;
	}

	/* Get the label defaults */
	if (min_label_str == NULL && !err) {
		min_label = blabel_alloc ();
		clearance = blabel_alloc ();
		if (userdefs (min_label, clearance) == -1) {
			err = TRUE;
		} else {
			min_label_str = (char *) strdup (bsltoh (min_label));
			clearance_str = (char *) strdup (bcleartoh (clearance));
		}
	}
	/* min_label */
	if (strcmp (keyword, USERATTR_MINLABEL) == 0) {
		value = min_label_str;
	} else if (strcmp (keyword, USERATTR_CLEARANCE) == 0) {
		value = clearance_str;
	} else if (strcmp (keyword, USERATTR_TYPE_KW) == 0) {
		value = USERATTR_TYPE_NORMAL_KW;
	} else if (strcmp (keyword, USERATTR_PASSWD) == 0) {
		if (useconf)
			value = defread ("PASSWORD=");
		if (value == NULL)
			value = USERATTR_PASSWD_MANUAL;
	}
	if (strcmp (keyword, USERATTR_LABELVIEW) == 0) {
		if (useconf)
			value = defread ("LABELVIEW=");
		if (value == NULL)
			value = USERATTR_SHOWSL;
	}
	if (strcmp (keyword, USERATTR_LOCK_KW) == 0) {
		if (useconf)
			value = defread ("LOCK=");
		if (value == NULL)
			value = USERATTR_LOCK_OPEN_KW;
	}
	if (strcmp (keyword, USERATTR_IDLECMD_KW) == 0) {
		if (useconf)
			value = defread ("IDLECMD=");
		if (value == NULL)
			value = USERATTR_IDLECMD_LOCK_KW;
	}
	if (strcmp (keyword, USERATTR_IDLETIME_KW) == 0) {
		if (useconf)
			value = defread ("IDLETIME=");
		if (value == NULL)
			value = DEF_IDLETIME;
	}
	if (strcmp (keyword, USERATTR_PROFILES_KW) == 0) {
		if (useconf)
			value = defread ("PROFS_GRANTED=");
		if (value == NULL)
			value = DEF_PROFILES;
	}
	(void) defopen (NULL);

	return value;
}

/*
 * Returns a value from uattr for the given key. If there is no value in
 * user_attr, then it returns the system wide default from policy.conf or
 * labelencodings as appropriate.
 */
char           *
gnome_tsol_get_usrattr_val (userattr_t * uattr, char *keyword)
{
	char           *value;

	if (uattr != NULL) {
		value = kva_match (uattr->attr, keyword);
		if (value != NULL)
			return value;	/* found from user_attr */
	}
	return get_default_usrattr_val (keyword);
}
