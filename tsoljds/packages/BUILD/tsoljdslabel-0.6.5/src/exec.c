/*
   Copyright 2012, Oracle and/or its affiliates. All rights reserved.
*/

#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <wait.h>

int
exec_command(char *cmd[], int cleanenv)
{
        int              pid;
        int              w;
        int              status;
        char            *env[] = {
                                "PATH=/usr/bin:/usr/lib",
                                "IFS=' '", NULL
                        };
        int              flags, i;
        struct           rlimit rlim;

        flags = FD_CLOEXEC;

        getrlimit (RLIMIT_NOFILE, &rlim);
        for (i = 3; i < rlim.rlim_cur; i++)
                fcntl (i, F_SETFD, flags);

        pid = fork();
        switch ( pid ) {
                case 0: /* Child */
			if (cleanenv) execve(cmd[0], cmd, env);
			else execv(cmd[0], cmd);
                        _exit(4); /* Should never reach here */
                        break;
                case -1:        /* Error */
                        fprintf(stderr, "%s failed to start\n", cmd[0]);
                        _exit(127);
                        break ;
                default:        /* Parent */
                        w = waitpid(pid, &status, 0);
                        return ( WEXITSTATUS(status) );
                        break;
        }
}

