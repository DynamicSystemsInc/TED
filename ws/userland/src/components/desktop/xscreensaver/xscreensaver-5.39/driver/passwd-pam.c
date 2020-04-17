/* passwd-pam.c --- verifying typed passwords with PAM
 * (Pluggable Authentication Modules.)
 * written by Bill Nottingham <notting@redhat.com> (and jwz) for
 * xscreensaver, Copyright (c) 1993-2017 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * Some PAM resources:
 *
 *    PAM home page:
 *    http://www.us.kernel.org/pub/linux/libs/pam/
 *
 *    PAM FAQ:
 *    http://www.us.kernel.org/pub/linux/libs/pam/FAQ
 *
 *    PAM Application Developers' Guide:
 *    http://www.us.kernel.org/pub/linux/libs/pam/Linux-PAM-html/Linux-PAM_ADG.html
 *
 *    PAM Mailing list archives:
 *    http://www.linuxhq.com/lnxlists/linux-pam/
 *
 *    Compatibility notes, especially between Linux and Solaris:
 *    http://www.contrib.andrew.cmu.edu/u/shadow/pam.html
 *
 *    The Open Group's PAM API documentation:
 *    http://www.opengroup.org/onlinepubs/8329799/pam_start.htm
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef NO_LOCKING  /* whole file */

#include <stdlib.h>
#include <xscreensaver-intl.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef __sun
# include <deflt.h>
# include <bsm/adt.h>
# include <bsm/adt_event.h>
#endif

extern char *blurb(void);


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <security/pam_appl.h>
#include <signal.h>
#include <errno.h>
#include <X11/Intrinsic.h>

#include <sys/stat.h>

#include "dialog-data.h"
#include "auth.h"

extern sigset_t block_sigchld (void);
extern void unblock_sigchld (void);

/* blargh */
#undef  Bool
#undef  True
#undef  False
#define Bool  int
#define True  1
#define False 0

#undef countof
#define countof(x) (sizeof((x))/sizeof(*(x)))

static struct pam_response *reply = 0; /*making it global so we can free it */
static int replies = 0;

/* Some time between Red Hat 4.2 and 7.0, the words were transposed 
   in the various PAM_x_CRED macro names.  Yay!
 */
#if !defined(PAM_REFRESH_CRED) && defined(PAM_CRED_REFRESH)
# define PAM_REFRESH_CRED PAM_CRED_REFRESH
#endif
#if !defined(PAM_REINITIALIZE_CRED) && defined(PAM_CRED_REINITIALIZE)
# define PAM_REINITIALIZE_CRED PAM_CRED_REINITIALIZE
#endif

static int pam_conversation (int nmsgs,
#ifndef __sun
                             const
#endif
                             struct pam_message **msg,
                             struct pam_response **resp,
                             void *closure);

void pam_try_unlock(saver_info *si, Bool verbose_p,
	       Bool (*valid_p)(const char *typed_passwd, Bool verbose_p));

Bool pam_priv_init (int argc, char **argv, Bool verbose_p);

#ifdef HAVE_PAM_FAIL_DELAY
   /* We handle delays ourself.*/
   /* Don't set this to 0 (Linux bug workaround.) */
# define PAM_NO_DELAY(pamh) pam_fail_delay ((pamh), 1)
#else  /* !HAVE_PAM_FAIL_DELAY */
# define PAM_NO_DELAY(pamh) /* */
#endif /* !HAVE_PAM_FAIL_DELAY */


/* On SunOS 5.6, and on Linux with PAM 0.64, pam_strerror() takes two args.
   On some other Linux systems with some other version of PAM (e.g.,
   whichever Debian release comes with a 2.2.5 kernel) it takes one arg.
   I can't tell which is more "recent" or "correct" behavior, so configure
   figures out which is in use for us.  Shoot me!
 */
#ifdef PAM_STRERROR_TWO_ARGS
# define PAM_STRERROR(pamh, status) pam_strerror((pamh), (status))
#else  /* !PAM_STRERROR_TWO_ARGS */
# define PAM_STRERROR(pamh, status) pam_strerror((status))
#endif /* !PAM_STRERROR_TWO_ARGS */


/* PAM sucks in that there is no way to tell whether a particular service
   is configured at all.  That is, there is no way to tell the difference
   between "authentication of the FOO service is not allowed" and "the
   user typed the wrong password."

   On RedHat 5.1 systems, if a service name is not known, it defaults to
   being not allowed (because the fallback service, /etc/pam.d/other, is
   set to `pam_deny'.)

   On Solaris 2.6 systems, unknown services default to authenticating normally.

   So, we could simply require that the person who installs xscreensaver
   set up an "xscreensaver" PAM service.  However, if we went that route,
   it would have a really awful failure mode: the failure mode would be that
   xscreensaver was willing to *lock* the screen, but would be unwilling to
   *unlock* the screen.  (With the non-PAM password code, the analagous
   situation -- security not being configured properly, for example do to the
   executable not being installed as setuid root -- the failure mode is much
   more palettable, in that xscreensaver will refuse to *lock* the screen,
   because it can know up front that there is no password that will work.)

   Another route would be to have the service name to consult be computed at
   compile-time (perhaps with a configure option.)  However, that doesn't
   really solve the problem, because it means that the same executable might
   work fine on one machine, but refuse to unlock when run on another
   machine.

   Another alternative would be to look in /etc/pam.conf or /etc/pam.d/ at
   runtime to see what services actually exist.  But I think that's no good,
   because who is to say that the PAM info is actually specified in those
   files?  Opening and reading those files is not a part of the PAM client
   API, so it's not guarenteed to work on any given system.

   An alternative I tried was to specify a list of services to try, and to
   try them all in turn ("xscreensaver", "xlock", "xdm", and "login").
   This worked, but it was slow (and I also had to do some contortions to
   work around bugs in Linux PAM 0.64-3.)

   So what we do today is, try PAM once, and if that fails, try the usual
   getpwent() method.  So if PAM doesn't work, it will at least make an
   attempt at looking up passwords in /etc/passwd or /etc/shadow instead.

   This all kind of blows.  I'm not sure what else to do.
 */


/* On SunOS 5.6, the `pam_conv.appdata_ptr' slot seems to be ignored, and
   the `closure' argument to pc.conv always comes in as random garbage.
   So we get around this by using a global variable instead.  Shoot me!

   (I've been told this is bug 4092227, and is fixed in Solaris 7.)
   (I've also been told that it's fixed in Solaris 2.6 by patch 106257-05.)
 */
static void *suns_pam_implementation_blows = 0;

#ifdef __sun
#include <syslog.h>
#include <bsm/adt.h>
#include <bsm/adt_event.h>

static Bool audit_flag_global = True;

/*
 * audit_lock - audit entry to screenlock
 *
 *      Entry   Process running with appropriate privilege to generate
 *                      audit records and real uid of the user.
 *
 *      Exit    ADT_screenlock audit record written.
 */
void
audit_lock(void)
{
  adt_session_data_t      *ah;          /* audit session handle */
  adt_event_data_t        *event;       /* audit event handle */

  /* Audit start of screen lock -- equivalent to logout ;-) */
  if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0)
    {
      syslog(LOG_AUTH | LOG_ALERT, "adt_start_session: %m");
      return;
    }
  if ((event = adt_alloc_event(ah, ADT_screenlock)) == NULL)
    {
      syslog(LOG_AUTH | LOG_ALERT, "adt_alloc_event(ADT_screenlock): %m");
    } else {
      if (adt_put_event(event, ADT_SUCCESS, ADT_SUCCESS) != 0)
        {
          syslog(LOG_AUTH | LOG_ALERT, "adt_put_event(ADT_screenlock): %m");
        }
      adt_free_event(event);
    }
  (void) adt_end_session(ah);
}

/*
 * audit_unlock - audit screen unlock
 *
 *      Entry   Process running with appropriate privilege to generate
 *                      audit records and real uid of the user.
 *              pam_status = PAM error code; reason for failure.
 *
 *      Exit    ADT_screenunlock audit record written.
 */
static void
audit_unlock(int pam_status)
{
  adt_session_data_t      *ah;          /* audit session handle */
  adt_event_data_t        *event;       /* audit event handle */

  if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0)
    {
      syslog(LOG_AUTH | LOG_ALERT,
             "adt_start_session(ADT_screenunlock): %m");
      return;
    }
  if ((event = adt_alloc_event(ah, ADT_screenunlock)) == NULL)
    {
      syslog(LOG_AUTH | LOG_ALERT,
             "adt_alloc_event(ADT_screenunlock): %m");
    } else {
      if (adt_put_event(event,
                        pam_status == PAM_SUCCESS ? ADT_SUCCESS : ADT_FAILURE,
                        pam_status == PAM_SUCCESS ? ADT_SUCCESS
                                                  : ADT_FAIL_PAM + pam_status)
          != 0)
        {
          syslog(LOG_AUTH | LOG_ALERT,
                 "adt_put_event(ADT_screenunlock(%s): %m",
                 pam_strerror(NULL, pam_status));
        }
      adt_free_event(event);
    }
  (void) adt_end_session(ah);
}

/*
 * audit_passwd - audit password change
 *      Entry   Process running with appropriate privilege to generate
 *                      audit records and real uid of the user.
 *              pam_status = PAM error code; reason for failure.
 *
 *      Exit    ADT_passwd audit record written.
 */
static void
audit_passwd(int pam_status)
{
  adt_session_data_t      *ah;          /* audit session handle */
  adt_event_data_t        *event;       /* audit event handle */

  if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0)
    {
      syslog(LOG_AUTH | LOG_ALERT, "adt_start_session(ADT_passwd): %m");
      return;
    }
  if ((event = adt_alloc_event(ah, ADT_passwd)) == NULL)
    {
      syslog(LOG_AUTH | LOG_ALERT, "adt_alloc_event(ADT_passwd): %m");
    } else {
      if (adt_put_event(event,
                        pam_status == PAM_SUCCESS ? ADT_SUCCESS : ADT_FAILURE,
                        pam_status == PAM_SUCCESS ? ADT_SUCCESS
                                                  : ADT_FAIL_PAM + pam_status)
          != 0)
        {
          syslog(LOG_AUTH | LOG_ALERT, "adt_put_event(ADT_passwd(%s): %m",
                 pam_strerror(NULL, pam_status));
        }
      adt_free_event(event);
    }
  (void) adt_end_session(ah);
}
#endif /* sun */

/**
 * This function is the PAM conversation driver. It conducts a full
 * authentication round by invoking the GUI with various prompts.
 */
void
pam_try_unlock(saver_info *si, Bool verbose_p,
	       Bool (*valid_p)(const char *typed_passwd, Bool verbose_p))
{
  const char *service = PAM_SERVICE_NAME;
  pam_handle_t *pamh = 0;
  int status = -1;
  struct pam_conv pc;
# ifdef HAVE_SIGTIMEDWAIT
  sigset_t set;
  struct timespec timeout;
# endif /* HAVE_SIGTIMEDWAIT */
  int pam_auth_status = 0;  /* Specific for pam_authenticate() status*/
  int acct_rc, setcred_rc, chauth_rc;
  int pam_flags = 0;

  uid_t euid = geteuid();

  pc.conv = &pam_conversation;
  pc.appdata_ptr = (void *) si;

  /* On SunOS 5.6, the `appdata_ptr' slot seems to be ignored, and the
     `closure' argument to pc.conv always comes in as random garbage. */
  suns_pam_implementation_blows = (void *) si;

#ifdef __sun
  if (verbose_p)
    fprintf (stderr, "Before uid=%d euid=%d \n\n", getuid(), geteuid());

  if (seteuid (0) != 0)
    {
      if (verbose_p)
        perror("Could not change euid to root, pam may not work!\n");
    }

  if (verbose_p)
    {
      fprintf (stderr, "After seteuid(0) uid=%d euid=%d \n\n",
               getuid(), geteuid());
      fprintf (stderr, "PAM is using SERVICE_NAME=\"%s\"\n\n", service);
    }
#endif

  /* Initialize PAM.
   */
  status = pam_start (service, si->user, &pc, &pamh);
  if (verbose_p)
    fprintf (stderr, "%s: pam_start (\"%s\", \"%s\", ...) ==> %d (%s)\n",
             blurb(), service, si->user,
             status, PAM_STRERROR (pamh, status));

#ifdef __sun
  if (audit_flag_global) /* We want one audit lock log per lock */
    audit_lock ();
#endif /**sun*/

  if (status != PAM_SUCCESS) goto DONE;

#ifdef __sun
  /* Check /etc/default/login to see if we should add
     PAM_DISALLOW_NULL_AUTHTOK to pam_flags */
  if (defopen("/etc/default/login") == 0) {
    char *ptr;
    int flags = defcntl(DC_GETFLAGS, 0);

    TURNOFF(flags, DC_CASE);
    (void) defcntl(DC_SETFLAGS, flags);
    if ((ptr = defread("PASSREQ=")) != NULL &&
        strcasecmp("YES", ptr) == 0)
      {
        pam_flags |= PAM_DISALLOW_NULL_AUTHTOK;
      }

    (void) defopen((char *)NULL); /* close current file */
  }
#endif

  /* #### We should set PAM_TTY to the display we're using, but we
     don't have that handy from here.  So set it to :0.0, which is a
     good guess (and has the bonus of counting as a "secure tty" as
     far as PAM is concerned...)
   */

/* From the pam trace and log file, it is found out that the
   Sun pam modules can drive itself.
*/
#ifndef __sun
  {
    char *tty = strdup (":0.0");
    status = pam_set_item (pamh, PAM_TTY, tty);
    if (verbose_p)
      fprintf (stderr, "%s:   pam_set_item (p, PAM_TTY, \"%s\") ==> %d (%s)\n",
               blurb(), tty, status, PAM_STRERROR(pamh, status));
    free (tty);
  }
#endif

  /* Try to authenticate as the current user.
     We must turn off our SIGCHLD handler for the duration of the call to
     pam_authenticate(), because in some cases, the underlying PAM code
     will do this:

        1: fork a setuid subprocess to do some dirty work;
        2: read a response from that subprocess;
        3: waitpid(pid, ...) on that subprocess.

    If we (the ignorant parent process) have a SIGCHLD handler, then there's
    a race condition between steps 2 and 3: if the subprocess exits before
    waitpid() was called, then our SIGCHLD handler fires, and gets notified
    of the subprocess death; then PAM's call to waitpid() fails, because the
    process has already been reaped.

    I consider this a bug in PAM, since the caller should be able to have
    whatever signal handlers it wants -- the PAM documentation doesn't say
    "oh by the way, if you use PAM, you can't use SIGCHLD."
   */

  PAM_NO_DELAY(pamh);

  if (verbose_p)
    fprintf (stderr, "%s:   pam_authenticate (...) ...\n", blurb());

# ifdef HAVE_SIGTIMEDWAIT
  timeout.tv_sec = 0;
  timeout.tv_nsec = 1;
  set =
# endif /* HAVE_SIGTIMEDWAIT */
    block_sigchld();
  pam_auth_status = pam_authenticate (pamh, pam_flags);
# ifdef HAVE_SIGTIMEDWAIT
  sigtimedwait (&set, NULL, &timeout);
  /* #### What is the portable thing to do if we don't have it? */
# endif /* HAVE_SIGTIMEDWAIT */
  unblock_sigchld();

#ifdef __sun
  audit_unlock(pam_auth_status);
  if (pam_auth_status == PAM_SUCCESS)
    audit_flag_global = True;
  else
    audit_flag_global = False;
#endif /*sun*/

#ifdef HAVE_XSCREENSAVER_LOCK
  /* Send status message to unlock dialog */
  if (pam_auth_status == PAM_SUCCESS)
    {
      if (verbose_p)
        write_to_child (si, "ul_ok", PAM_STRERROR (pamh, pam_auth_status));
    }
  else if (si->unlock_state != ul_cancel && si->unlock_state != ul_time)
    {
      write_to_child (si, "ul_fail", PAM_STRERROR (pamh, pam_auth_status));
    }
  if (verbose_p)
    sleep (1);
#endif

  if (verbose_p)
      fprintf (stderr, "after calling pam_authenticate state is: %s\n",
               si->unlock_state == ul_success ? "ul_success" : "ul_fail");

  if (verbose_p)
    fprintf (stderr, "%s:   pam_authenticate (...) ==> %d (%s)\n",
             blurb(), pam_auth_status, PAM_STRERROR(pamh, pam_auth_status));

  if (pam_auth_status == PAM_SUCCESS)  /* Win! */
    {
      /* perform PAM account validation procedures for login user only */
      acct_rc   = pam_acct_mgmt(pamh, pam_flags);

      /******************************************************************
           ignore other cases for the time being
           PAM_USER_UNKNOWN, PAM_AUTH_ERR, PAM_ACCT_EXPIRED
           (password mgn service module)
           same as pam_setcred(), focus on auth. service module only
       *****************************************************************/

      if (verbose_p)
        fprintf (stderr, "%s:   pam_acct_mgmt (...) ==> %d (%s)\n",
                 blurb(), acct_rc, PAM_STRERROR(pamh, acct_rc));

#ifdef HAVE_XSCREENSAVER_LOCK
      /* Send status message to unlock dialog ***/
      if (acct_rc == PAM_SUCCESS)
        {
          if (verbose_p)
            write_to_child (si, "ul_acct_ok", PAM_STRERROR(pamh, acct_rc));
        }
      else
        {
#ifdef __sun
          /* Only in failure of pam_acct_mgmt case we call audit */
          audit_unlock (acct_rc);
#endif /*sun*/

          write_to_child (si, "ul_acct_fail", PAM_STRERROR(pamh, acct_rc));
        }
      if (verbose_p)
        sleep (1);
#endif

      /* HPUX for some reason likes to make PAM defines different from
       * everyone else's. */
#ifdef PAM_AUTHTOKEN_REQD
      if (acct_rc == PAM_AUTHTOKEN_REQD)
#else
      if (acct_rc == PAM_NEW_AUTHTOK_REQD)
#endif
        {
          int i;
          for (i = 0; i < 3; i++)
            {
              chauth_rc  = pam_chauthtok (pamh, PAM_CHANGE_EXPIRED_AUTHTOK);
              if (chauth_rc == PAM_AUTHTOK_ERR ||
                  chauth_rc  == PAM_TRY_AGAIN )
                {
                  i = 0;
                  si->unlock_state = ul_read;
                }
              else break; /* get out of the loop */
            }

          if (verbose_p)
            fprintf (stderr, "%s: pam_chauthtok (...) ==> %d (%s)\n",
                     blurb(), chauth_rc, PAM_STRERROR(pamh, chauth_rc));

#ifdef __sun
          audit_passwd (chauth_rc);
#endif /* sun */

          if (chauth_rc != PAM_SUCCESS)
            {
              pam_auth_status = chauth_rc;
              goto DONE;
            }
        }
      else if (acct_rc != PAM_SUCCESS)
        {
          pam_auth_status = acct_rc;
          write_to_child (si, "ul_acct_fail", PAM_STRERROR(pamh, acct_rc));
          sleep (3);
          goto DONE;
        }

      /* Each time we successfully authenticate, refresh credentials,
         for Kerberos/AFS/DCE/etc.  If this fails, just ignore that
         failure and blunder along; it shouldn't matter.
       */

#if defined(__linux__)
      /* Apparently the Linux PAM library ignores PAM_REFRESH_CRED and only
         refreshes credentials when using PAM_REINITIALIZE_CRED. */
      setcred_rc = pam_setcred (pamh, PAM_REINITIALIZE_CRED);
#else
      /* But Solaris requires PAM_REFRESH_CRED or extra prompts appear. */
      setcred_rc = pam_setcred (pamh, PAM_REFRESH_CRED);
#endif

      if (verbose_p)
        fprintf (stderr, "%s:   pam_setcred (...) ==> %d (%s)\n",
                 blurb(), setcred_rc, PAM_STRERROR(pamh, setcred_rc));

#ifdef HAVE_XSCREENSAVER_LOCK
      /* Send status message to unlock dialog ***/
      if (setcred_rc == PAM_SUCCESS)
        {
          if (verbose_p)
            write_to_child (si, "ul_setcred_ok", PAM_STRERROR(pamh, setcred_rc));
        }
      else
        {
#ifdef __sun
          /* Only in failure of pam_setcred() case we call audit. */
          audit_unlock (setcred_rc);
#endif /*sun*/
          write_to_child (si, "ul_setcred_fail", PAM_STRERROR(pamh, setcred_rc));
        }
      if (verbose_p)
        sleep (1);
#endif
    }

 DONE:
  if (pamh)
    {
      int status2 = pam_end (pamh, pam_auth_status);
      pamh = NULL;
      if (verbose_p)
        fprintf (stderr, "%s: pam_end (...) ==> %d (%s)\n",
                 blurb(), status2,
                 (status2 == PAM_SUCCESS ? "Success" : "Failure"));
    }

  if (seteuid (euid) != 0)
    {
      if (verbose_p)
        perror("Error pam could not revert euid to user running as euid root,"
               " locking may not work now\n");
    }

  if (verbose_p)
    fprintf (stderr,
             "<--end of pam_authenticate() returning ok_to_unblank = %d\n",
             (int) ((pam_auth_status == PAM_SUCCESS) ? True : False));

  if (si->pw_data->passwd_string)
    {
      memset(si->pw_data->passwd_string, 0,
             strlen(si->pw_data->passwd_string));
      free (si->pw_data->passwd_string);
      si->pw_data->passwd_string = NULL;
    }

  if (pam_auth_status == PAM_SUCCESS)
    si->unlock_state = ul_success;	     /* yay */
  else if (si->unlock_state == ul_cancel ||
           si->unlock_state == ul_time)
    ;					     /* more specific failures ok */
  else
    si->unlock_state = ul_fail;		     /* generic failure */
}


Bool 
pam_priv_init (int argc, char **argv, Bool verbose_p)
{
  /* We have nothing to do at init-time.
     However, we might as well do some error checking.
     If "/etc/pam.d" exists and is a directory, but "/etc/pam.d/xlock"
     does not exist, warn that PAM probably isn't going to work.

     This is a priv-init instead of a non-priv init in case the directory
     is unreadable or something (don't know if that actually happens.)
   */
  const char   dir[] = "/etc/pam.d";
  const char  file[] = "/etc/pam.d/" PAM_SERVICE_NAME;
  const char file2[] = "/etc/pam.conf";
  struct stat st;

#ifdef __sun
  if (! verbose_p)      /* SUN addition: only print warnings in verbose mode */
    {                   /* since they are rarely useful and mostly just      */
      return True;      /* cause confusion when users see them.		     */
    }
#endif

# ifndef S_ISDIR
#  define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
# endif

  if (stat (dir, &st) == 0 && S_ISDIR(st.st_mode))
    {
      if (stat (file, &st) != 0)
        fprintf (stderr,
                 "%s: warning: %s does not exist.\n"
                 "%s: password authentication via PAM is unlikely to work.\n",
                 blurb(), file, blurb());
    }
  else if (stat (file2, &st) == 0)
    {
      FILE *f = fopen (file2, "r");
      if (f)
        {
          Bool ok = False;
          char buf[255];
          while (fgets (buf, sizeof(buf), f))
            if (strstr (buf, PAM_SERVICE_NAME))
              {
                ok = True;
                break;
              }
          fclose (f);

#ifndef __sun  /* disable the misleading message */
          if (!ok)
            {
              fprintf (stderr,
                  "%s: warning: %s does not list the `%s' service.\n"
                  "%s: password authentication via PAM is unlikely to work.\n",
                       blurb(), file2, PAM_SERVICE_NAME, blurb());
            }
#endif
        }
      /* else warn about file2 existing but being unreadable? */
    }
#ifndef __sun  /* disable the misleading message */
  else
    {
      fprintf (stderr,
               "%s: warning: neither %s nor %s exist.\n"
               "%s: password authentication via PAM is unlikely to work.\n",
               blurb(), file2, file, blurb());
    }
#endif

  /* Return true anyway, just in case. */
  return True;
}


int
pam_conversation (int nmsgs,
#ifndef __sun
                  const
#endif
		  struct pam_message **msg,
		  struct pam_response **resp,
		  void *vsaver_info)
{
  int i, ret = -1;
  struct auth_message *messages = 0;
  struct auth_response *authresp = 0;
  struct pam_response *pam_responses;
  saver_info *si = (saver_info *) vsaver_info;
  Bool verbose_p;
  size_t msg_len = 0;

  /* On SunOS 5.6, the `closure' argument always comes in as random garbage. */
  si = (saver_info *) suns_pam_implementation_blows;

  verbose_p = si->prefs.verbose_p;

  /* Converting the PAM prompts into the XScreenSaver native format.
   * It was a design goal to collapse (INFO,PROMPT) pairs from PAM
   * into a single call to the unlock_cb function. The unlock_cb function
   * does that, but only if it is passed several prompts at a time. Most PAM
   * modules only send a single prompt at a time, but because there is no way
   * of telling whether there will be more prompts to follow, we can only ever
   * pass along whatever was passed in here.
   */

  if (nmsgs >= PAM_MAX_NUM_MSG)
    {
      if (verbose_p)
        {
          fprintf (stderr, "Too many PAM messages "
                  "%d >= %d\n", nmsgs, PAM_MAX_NUM_MSG);
        }

      syslog (LOG_AUTH | LOG_ERR, "Too many PAM messages "
               "%d >= %d", nmsgs, PAM_MAX_NUM_MSG);
      *resp = NULL;
      return (PAM_CONV_ERR);
    }

  messages = calloc(nmsgs, sizeof(struct auth_message));
  pam_responses = calloc(nmsgs, sizeof(*pam_responses));
  
  if (!pam_responses || !messages)
    goto end;

  if (verbose_p)
    fprintf (stderr, "%s:     pam_conversation (", blurb());

  for (i = 0; i < nmsgs; ++i)
    {

      if (msg[i]->msg == NULL)
        {
          if (verbose_p)
            {
              fprintf (stderr, "PAM message[%d] "
                      "style %d NULL\n", i, msg[i]->msg_style);
            }
          syslog (LOG_AUTH | LOG_WARNING, "PAM message[%d] "
                  "style %d NULL", i, msg[i]->msg_style);
          goto end;
        }

        msg_len = strlen (msg[i]->msg);

        if (msg_len > PAM_MAX_MSG_SIZE)
          {
            if (verbose_p)
              {
                fprintf (stderr, "PAM message[%d] "
                        " style %d length %d too long\n",
                        i, msg[i]->msg_style, msg_len);
              }
             syslog (LOG_AUTH | LOG_WARNING, "PAM message[%d] "
                     "style %d length %d too long",i, msg[i]->msg_style, msg_len);
             goto end;
          }

      if (verbose_p && i > 0) fprintf (stderr, ", ");

      messages[i].msg = msg[i]->msg;

      switch (msg[i]->msg_style) {
      case PAM_PROMPT_ECHO_OFF: messages[i].type = AUTH_MSGTYPE_PROMPT_NOECHO;
        if (verbose_p) fprintf (stderr, "ECHO_OFF");
        break;
      case PAM_PROMPT_ECHO_ON:  messages[i].type = AUTH_MSGTYPE_PROMPT_ECHO;
        if (verbose_p) fprintf (stderr, "ECHO_ON");
        break;
      case PAM_ERROR_MSG:       messages[i].type = AUTH_MSGTYPE_ERROR;
        if (verbose_p) fprintf (stderr, "ERROR_MSG");
        break;
      case PAM_TEXT_INFO:       messages[i].type = AUTH_MSGTYPE_INFO;
        if (verbose_p) fprintf (stderr, "TEXT_INFO");
        break;
      default:
         if (verbose_p)
           {
             fprintf (stderr, "Invalid PAM "
                     "message style %d\n", msg[i]->msg_style);
           }
          syslog (LOG_AUTH | LOG_WARNING, "Invalid PAM "
                 "message style %d", msg[i]->msg_style);

          goto end;
      }

      if (verbose_p) 
        fprintf (stderr, "=\"%s\"", msg[i]->msg ? msg[i]->msg : "(null)");
    }

  if (verbose_p)
    fprintf (stderr, ") ...\n");

  ret = si->unlock_cb(nmsgs, messages, &authresp, si);

  /* #### If the user times out, or hits ESC or Cancel, we return PAM_CONV_ERR,
          and PAM logs this as an authentication failure.  It would be nice if
          there was some way to indicate that this was a "cancel" rather than
          a "fail", so that it wouldn't show up in syslog, but I think the
          only options are PAM_SUCCESS and PAM_CONV_ERR.  (I think that
          PAM_ABORT means "internal error", not "cancel".)  Bleh.
   */

  if (ret == 0)
    {
      msg_len = 0;
      for (i = 0; i < nmsgs; ++i)
      {
        if (authresp[i].response)
            msg_len = strlen (authresp[i].response);

        if (msg_len > PAM_MAX_RESP_SIZE)
          {
            if (verbose_p)
              {
                fprintf (stderr, "PAM "
                        "message[%d] style %d response %d too long\n",
                        i, msg[i]->msg_style, msg_len);
              }
            syslog (LOG_AUTH | LOG_WARNING, "PAM "
                    "message[%d] style %d response %d too long",
                    i, msg[i]->msg_style, msg_len);
            ret = -1;
            goto end;
          }
        pam_responses[i].resp = authresp[i].response;
      }
    }

end:
  if (messages)
    free(messages);

  if (authresp)
    free(authresp);

  if (verbose_p)
    fprintf (stderr, "%s:     pam_conversation (...) ==> %s\n", blurb(),
             (ret == 0 ? "PAM_SUCCESS" : "PAM_CONV_ERR"));

  if (ret == 0)
    {
      *resp = pam_responses;
      return PAM_SUCCESS;
    }

  /* Failure only */
    if (pam_responses)
      free(pam_responses);

    *resp == NULL;
    return PAM_CONV_ERR;
}

#endif /* NO_LOCKING -- whole file */
