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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include "ui-view.h"
#include "menus.h"
#include "ui-controller.h"
#include "xagent-management.h"
#include "privs.h"
#include <glib/gi18n.h>

#define SELECTION_NAME "_TSOLJDS_TSTRIPE"

static          gboolean
get_lock (void)
{
	Display *display = gdk_x11_get_default_xdisplay();
	static GtkWidget *selection_window;
	Atom            selection_atom = gdk_x11_get_xatom_by_name (SELECTION_NAME);

	if (XGetSelectionOwner(display, selection_atom) != None)
		return FALSE;

	selection_window = gtk_invisible_new ();
	gtk_widget_show (selection_window);

	if (!gtk_selection_owner_set (selection_window,
				    gdk_atom_intern (SELECTION_NAME, FALSE),
				      GDK_CURRENT_TIME)) {
		gtk_widget_destroy (selection_window);
		selection_window = NULL;
		return FALSE;
	}
	return TRUE;
}

void 
setup_xagent ()
{
	Window          rootw = None;
	Display        *x_dpy;
	GdkDisplay     *dpy;
	GdkWindow      *default_root;
	struct sigaction sa;

	dpy = gdk_display_get_default ();
	x_dpy = GDK_DISPLAY_XDISPLAY (dpy);
	rootw = DefaultRootWindow (x_dpy);

	/* This ignores ALL X server errors */
	XSetErrorHandler (TsolErrorHandler);

	start_xagent_for_workspaces ();

	XInternAtom (x_dpy, _XA_LABEL_EXEC_COMMAND, False);
	XInternAtom (x_dpy, "_SAVE_LOCAL_ZONE_SESSION", False);
	get_exec_command_prop (x_dpy, rootw);

	tsol_login_defaults ();

	/*
	 * FIXME - Maybe this can be merged with the existing filter in the
	 * trusted stripe
	 */
	default_root = gdk_screen_get_root_window (gdk_display_get_default_screen (dpy));

	gdk_window_add_filter (default_root, process_label_command, (gpointer) rootw);

	/* Child signal handler */
	sa.sa_handler = ChildDieHandler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset (&sa.sa_mask);
	(void) sigaction (SIGCLD, &sa, (struct sigaction *) 0);

}

int
main (int argc, char *argv[])
{
	Display        *x_dpy;
        GdkDisplay     *dpy;
	/*
	int delay = 1;

	while(delay) {
		sleep (1);
	}
	*/

	XInitThreads();
	gdk_disable_multidevice();
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* initialise gtk or die if we cant connect to the xserver */
	gtk_init (&argc, &argv);

	if (!get_lock ()) {
		fprintf (stderr, "%s is already running!\n", GETTEXT_PACKAGE);
		exit (2);
	}
	if (!has_required_privileges (GETTEXT_PACKAGE)) {
		fprintf (stderr, "Insufficient priviliges to run. All priviliges are required to run\n");
		exit (3);
	}

	/* wait until the WM tells us the workspace props are setup */
        dpy = gdk_display_get_default ();
        x_dpy = GDK_DISPLAY_XDISPLAY (dpy);
	while (XInternAtom (x_dpy, "TX_WS_SETUP_DONE", TRUE) == None) {
		sleep (1);
	}

	/* set up graphical bits (trusted stripe) */
	setup_trusted_stripes ();

	/* Set up local zone xagent management */
	if (!getenv ("TSTRIPE_GFX_ONLY"))
		setup_xagent ();

	gtk_main ();

	return 0;
}
