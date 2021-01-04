/*
   Copyright 2007, 2012, Oracle and/or its affiliates. All rights reserved.
*/

#include <config.h>

#include <priv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "exec.h"

int
main (int argc, char *argv[])
{
	priv_set_t     *pset;
	uid_t           uid;

	argv[0] = "/usr/bin/tsoljdslabel-ui";

	uid = getuid ();

	setuid (0);

	/*
	 * Make Inheritable same as Permitted
	 */
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
	/* Change uid back to the user */
	if (setuid (uid) == -1) {
		perror ("Can't make real & effective uid same\n");
		exit (1);
	}
	/* kick off tsoljdslabel-ui with new privileges */

	exec_command (argv, 0);

}
