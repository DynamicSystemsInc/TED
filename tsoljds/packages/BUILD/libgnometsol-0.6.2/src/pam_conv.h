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


#ifndef GNOMETSOL_PAM_CONV_H
#define GNOMETSOL_PAM_CONV_H

#include <glib.h>
#include <security/pam_appl.h>

G_BEGIN_DECLS

/* Not defined in pam headers */
#define GNOME_TSOL_PAM_CANCEL	-1
#define GNOME_TSOL_PAM_SUCCESS	PAM_SUCCESS
#define GNOME_TSOL_PAM_CONV_ERR	PAM_CONV_ERR

/* Structure for appdata passed to conversation function */
typedef	struct conv_info {
	gboolean debug;				/* For debug tracing */
	gboolean sysmodal;			/* Makes the dialog system modal for volatile operations like workspace role su */
	char *prog_name;			/* For syslog and audit purposes */
	char *echoonmsg;			/* Extra message to display to user during PAM_PROMPT_ECHO_ON */
	char *echooffmsg;			/* Extra message to display to user during PAM_PROMPT_ECHO_OFF */
	GdkScreen *screen;  	 /* screen on which to display the dialog */
	int writefd;	 /* write file descriptor for cross zone prompting */
	int readfd;	 /* read  file descriptor for cross zone prompting */
} conv_info_t;

int
gnometsol_pam_conv(int num_msg, struct pam_message **msg,
           struct pam_response **resp, void *info);

G_END_DECLS

#endif /* GNOMETSOL_PAM_CONV_H */
