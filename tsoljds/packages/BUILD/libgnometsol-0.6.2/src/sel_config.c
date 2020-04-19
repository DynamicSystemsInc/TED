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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sel_config.h"

#define SETTINGS_FILE          "/usr/share/gnome/sel_config"

#define AUTO_REPLY_WORD         16	/* index in the array */
#define REPLY_TYPE_WORD         17	/* index in the array */
#define HIDDEN_IL_WORD          18	/* index in the array */

char           *sel_keywords[] = {
	"downgradesl.downgradeil",
	"downgradesl.equalil",
	"downgradesl.upgradeil",
	"downgradesl.disjointil",
	"equalsl.downgradeil",
	"equalsl.equalil",
	"equalsl.upgradeil",
	"equalsl.disjointil",
	"upgradesl.downgradeil",
	"upgradesl.equalil",
	"upgradesl.upgradeil",
	"upgradesl.disjointil",
	"disjointsl.downgradeil",
	"disjointsl.equalil",
	"disjointsl.upgradeil",
	"disjointsl.disjointil",
	"autoreply",
	"replytype",
	"hidden_il_action",
	NULL
};

char           *no_il_sel_keywords[] = {
	"",
	"downgradesl",		/* Treat as synonym for downgradesl.equalil */
	"",
	"",
	"",
	"equalsl",		/* Treat as synonym for equalsl.equalil */
	"",
	"",
	"",
	"upgradesl",		/* Treat as synonym for upgradesl.equalil */
	"",
	"",
	"",
	"disjointsl",		/* Treat as synonym for disjointsl.equalil */
	"",
	"",
	"",
	"",
	"",
	NULL
};

char           *
search_settings (char *inbuf, int *cmd, char *do_confirm, char *do_cancel)
{
	register int    i;
	char           *p;
	char           *first_param;

	/* set default parameter values */
	*cmd = -1;
	*do_confirm = 'n';
	*do_cancel = 'n';

	/* Find the keyword terminator and replace with a null for later */
	if ((p = strchr (inbuf, ':')) == NULL)
		return NULL;
	*p++ = '\0';

	/* now look up the keyword */
	for (i = 0; sel_keywords[i] != NULL; i++) {
		if (strcmp (inbuf, sel_keywords[i]) == 0)
			*cmd = i;
		else if (strcmp (inbuf, no_il_sel_keywords[i]) == 0)
			*cmd = i;
	}

	/*
	 * find first parameter, set param as index to first character if
	 * there is no first character set the index to point to the null at
	 * the end of the inbuf.
	 * 
	 * set do_confirm to first character of first parameter if it is null,
	 * leave do_confirm as default value
	 */

	while (isspace (*p))
		p++;

	if (*p == '\0')
		return NULL;	/* end of line reached - no first parameter */

	first_param = p;
	*do_confirm = *p;

	/*
	 * get next parameter (after comma) and set do_cancel to first char if
	 * it is null, leave do_cancel as default value
	 */

	if ((p = strchr (p, ',')) == NULL)
		return first_param;

	p++;			/* skip the comma */

	while (isspace (*p))
		p++;

	if (*p == '\0')
		return first_param;	/* end of line reached */

	*do_cancel = *p;

	return first_param;	/* end of line reached */
}

int
tsol_load_sel_config (AutoConfirm * confirm, AutoReply * reply)
{
	FILE           *fp;
	char            buf[80];
	char            do_confirm, do_cancel, hidden_il_action;
	int             cmd;	/* cmd index */
	char           *param;	/* first parameter */
	int             len;

	/*
	 * Note that if confirm or reply are set to NULL those sections of
	 * the config file will be skipped. This is normally used only by
	 * nautilus for the reply parameter.
	 */

	if ((fp = fopen (SETTINGS_FILE, "r")) == NULL)
		return -1;

	if (reply != NULL)
		reply->count = 0;

	confirm->hidden_il_action = TS_IL_CONFIRM;

	while (fgets (buf, (sizeof (buf) - 1), fp) != NULL) {
		if (buf[strlen (buf) - 1] == '\n')
			buf[strlen (buf) - 1] = '\0';

		param = search_settings (buf, &cmd, &do_confirm, &do_cancel);

		switch (cmd) {
		case (-1):
			continue;	/* unknown type */
			break;
		case HIDDEN_IL_WORD:
			if (reply == NULL)
				continue;
			if (do_confirm == 'c')
				confirm->hidden_il_action = TS_IL_CONFIRM;
			else if (do_confirm == 'd')
				confirm->hidden_il_action = TS_IL_FROM_DST;
			else
				confirm->hidden_il_action = TS_IL_ADLOW;
			break;
		case AUTO_REPLY_WORD:
			if (reply == NULL)
				continue;
			if (do_confirm == 'y')
				reply->enabled = 1;
			else
				reply->enabled = 0;
			break;
		case REPLY_TYPE_WORD:
			if (reply == NULL)
				continue;
			if (param == NULL)
				continue;
			if (reply->count == MAX_AUTO_REPLY)
				continue;
			if (reply->selection[reply->count])
				free (reply->selection[reply->count]);
			reply->selection[reply->count] = strdup (param);
			reply->count++;
			break;
		default:
			/* all others are for auto_confirm settings */
			if (confirm == NULL)
				continue;
			if (do_confirm == 'y')
				confirm->do_auto_confirm[cmd] = 1;
			else
				confirm->do_auto_confirm[cmd] = 0;

			if (do_cancel == 'y')
				confirm->do_auto_cancel[cmd] = 1;
			else
				confirm->do_auto_cancel[cmd] = 0;

		}
	}
	return 0;
}

TransferType
tsol_check_transfer_type (bslabel_t * src_sl, bslabel_t * dst_sl)
{
	TransferParts   sl_relationship;

	if (blstrictdom (src_sl, dst_sl))
		sl_relationship = DGSL;
	else if (blequal (src_sl, dst_sl))
		sl_relationship = EQSL;
	else if (blstrictdom (dst_sl, src_sl))
		sl_relationship = UGSL;
	else
		sl_relationship = DJSL;

	return ((TransferType) sl_relationship + EQIL);
}
