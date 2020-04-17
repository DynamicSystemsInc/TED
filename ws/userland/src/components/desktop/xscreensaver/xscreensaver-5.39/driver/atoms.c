/* xscreensaver, Copyright (c) 1991-2010 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <X11/Xproto.h>		/* for CARD32 */
#include <X11/Xlib.h>
#include <X11/Xos.h>

#include "atoms.h"

/* Atoms to retrieve info from remote daemon */
Atom XA_SCREENSAVER, XA_SCREENSAVER_ID, XA_SCREENSAVER_VERSION,
   XA_SCREENSAVER_RESPONSE, XA_SCREENSAVER_STATUS;

/* Atoms to send commands to remote daemon */
Atom XA_ACTIVATE, XA_BLANK, XA_CYCLE, XA_DEACTIVATE, XA_DEMO,
  XA_EXIT, XA_LOCK, XA_NEXT, XA_PREFS, XA_PREV, XA_RESTART,
  XA_SELECT, XA_THROTTLE, XA_UNTHROTTLE;

static const struct atom_request remote_control_atom_list[] =
{
    { &XA_SCREENSAVER, "SCREENSAVER" },
    { &XA_SCREENSAVER_ID, "_SCREENSAVER_ID" },
    { &XA_SCREENSAVER_VERSION, "_SCREENSAVER_VERSION" },
    { &XA_SCREENSAVER_RESPONSE, "_SCREENSAVER_RESPONSE" },
    { &XA_SCREENSAVER_STATUS, "_SCREENSAVER_STATUS" },
    { &XA_ACTIVATE, "ACTIVATE" },
    { &XA_BLANK, "BLANK" },
    { &XA_CYCLE, "CYCLE" },
    { &XA_DEACTIVATE, "DEACTIVATE" },
    { &XA_DEMO, "DEMO" },
    { &XA_EXIT, "EXIT" },
    { &XA_LOCK, "LOCK" },
    { &XA_NEXT, "NEXT" },
    { &XA_PREFS, "PREFS" },
    { &XA_PREV, "PREV" },
    { &XA_RESTART, "RESTART" },
    { &XA_SELECT, "SELECT" },
    { &XA_THROTTLE, "THROTTLE" },
    { &XA_UNTHROTTLE, "UNTHROTTLE" },
    { NULL, NULL } /* Must be last to terminate list */
};

const struct atom_request *remote_control_atoms = remote_control_atom_list;

/* Load a list of atoms in a single round trip to the X server instead of
   waiting for a synchronous round trip for each and every atom */
Status request_atoms ( Display *dpy,
		       const struct atom_request **request_lists )
{
  int atom_count, n;
  Status result;
  const struct atom_request **l, *r;
  Atom *atoms;
  const char **names;

  /* Count the number of items across all the lists passed in */
  atom_count = 0;
  for (l = request_lists; l != NULL && *l != NULL; l++)
    {
      for (r = *l; r != NULL && r->name != NULL; r++)
	{
	  atom_count++;
	}
    }

  atoms = calloc(atom_count, sizeof(Atom));
  names = calloc(atom_count, sizeof(char *));
  if (!atoms || !names)
    return -1;

  n = 0;
  for (l = request_lists; l != NULL && *l != NULL; l++)
    {
      for (r = *l; r != NULL && r->name != NULL; r++)
	{
	  names[n++] = r->name;
	}
    }
  result = XInternAtoms( dpy, (char **) names, atom_count, False, atoms );

  n = 0;
  for (l = request_lists; l != NULL && *l != NULL; l++)
    {
      for (r = *l; r != NULL && r->name != NULL; r++)
	{
#if DEBUG_ATOMS
	  fprintf (stderr, "atom: %s => %d\n", names[n], atoms[n]);
#endif
	  *(r->atomp) = atoms[n++];
	}
    }

  free(atoms);
  free(names);

  return result;
}
