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

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <priv.h>

#include <zone.h>

#include <gdk/gdkx.h>
#include <glib/gslist.h>

#if 0
/* Problems with my system configuration.  should be fixed shortly 
   but don't include until it is fixed. */
#include <sys/param.h>
#include <bsm/libbsm.h>
#include <user_attr.h>

#include <bsm/audit.h>
#include <bsm/auditwrite.h>
#include <bsm/audit_uevents.h>

#include <tsol/label.h>
#include <sys/tsol/label_macro.h>
#include <X11/extensions/Xtsol.h>
#endif

#include <auth_attr.h>
#include <secdb.h>
#include <pwd.h>

#include <libgnomeui/libgnomeui.h>

#include "devmgr-dialog.h"
#include "devmgr-help.h"

GtkTable *expander_table;

GtkDialog *
dev_mgr_init (void) 
{
	GtkDialog *dialog;

	dialog = (GtkDialog *)dev_mgr_dialog_new ();
	gtk_widget_show_all(GTK_WIDGET(dialog));
	gtk_widget_hide(GTK_WIDGET(expander_table));

	return ( dialog );
}

static gboolean
started_from_trusted_path(char *progname)
{
	gboolean ret = TRUE;
	
	if (getzoneid() != 0 || setuid(0) == -1)
	{
		fprintf(stderr, _("%s must be started from the trusted path\n"), progname);
		ret = FALSE ;	
	}
	
	return ( ret );
}

#ifdef CONSOLE
FILE *console;
#endif /* CONSOLE */

int
main (int argc, char **argv)
{
	GnomeClient *client;
	GtkDialog *dialog;
	gint resp;
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			    NULL);

#ifdef CONSOLE
	if ((console = fopen("/dev/console", "r+")) == NULL)
	{
		printf("Error: Unable to open /dev/console\n");
		console = stdout;
	}
#endif /* CONSOLE */

	if (! started_from_trusted_path(argv[0]) )
		exit ( -1 );
		
	dialog = dev_mgr_init();
	do 
	{
		switch ((resp = gtk_dialog_run(dialog)))
	        {
	        case GTK_RESPONSE_HELP:
			devmgr_show_help (GTK_WIDGET (dialog));
			break;
 	        case GTK_RESPONSE_CANCEL:
                        break;
                case GTK_RESPONSE_OK:
                	/* Commit the Allocations */
                        break;
                default:
                        break;
		}
	} while (resp != GTK_RESPONSE_CANCEL && resp != GTK_RESPONSE_OK);
	
        gtk_widget_hide_all(GTK_WIDGET(dialog));

#ifdef CONSOLE
	if (console != NULL)
		fclose(console);
#endif /* CONSOLE */
}

