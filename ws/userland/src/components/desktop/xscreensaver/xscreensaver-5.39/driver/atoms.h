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

#ifndef _XSCREENSAVER_ATOMS_H_
#define _XSCREENSAVER_ATOMS_H_

struct atom_request {
    Atom *atomp;
    const char *name;
};

extern const struct atom_request *remote_control_atoms;
extern Status request_atoms ( Display *dpy,
			      const struct atom_request **request_lists );

/* Atoms to retrieve info from remote daemon */
extern Atom XA_SCREENSAVER, XA_SCREENSAVER_ID, XA_SCREENSAVER_VERSION,
  XA_SCREENSAVER_RESPONSE, XA_SCREENSAVER_STATUS;

/* Atoms to send commands to remote daemon */
extern Atom XA_ACTIVATE, XA_BLANK, XA_CYCLE, XA_DEACTIVATE, XA_DEMO,
  XA_EXIT, XA_LOCK, XA_NEXT, XA_PREFS, XA_PREV, XA_RESTART,
  XA_SELECT, XA_THROTTLE, XA_UNTHROTTLE;

#endif /* _XSCREENSAVER_ATOMS_H_ */
