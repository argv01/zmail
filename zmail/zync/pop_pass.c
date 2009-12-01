/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#ifdef HAVE_GETPWANAM
# include <sys/label.h>
# include <sys/audit.h>
# include <pwdadj.h>
#endif /* HAVE_GETPWANAM */
#ifdef HAVE_GETPRPWNAM
# ifdef HPUX
#  include <hpsecurity.h>
# else /* HPUX */
#  include <sys/security.h>
# endif /* HPUX */
# include <prot.h>
#endif /* HAVE_GETPRPWNAM */
#ifdef HAVE_GETSPNAM
# include <shadow.h>
#endif /* HAVE_GETSPNAM */

static const char pop_pass_rcsid[] =
    "$Id: pop_pass.c,v 1.27 1996/03/06 03:51:26 spencer Exp $";

extern char *crypt();

#if defined(HAVE_GETPRPWNAM) && !defined(HAVE_BIGCRYPT)
/*
 * I derived the bigcrypt() algorithm by observing how it behaves on other
 * machines, so this could well be wrong.  But it looks right.  Note it returns
 * a pointer to a static buffer that gets overwritten with each call.
 */
char *
bigcrypt(key, salt)
    const char *key, *salt;
{
    static char retbuf[AUTH_CIPHERTEXT_SIZE(AUTH_SEGMENTS(AUTH_MAX_PASSWD_LENGTH)) + 1]; /* same size as ufld.fd_encrypt */
    char *retp, s[3];
    int segs;

    s[0] = salt[0]; s[1] = salt[1]; s[2] = '\0';
    strcpy(retbuf, s); retp = &retbuf[2];
    segs = 0;

    while (1) {
	int i = 0;
	char buf[9];

	/* copy up to 8 characters */
	while (*key && i < 8)
	    buf[i++] = *key++;
	buf[i] = '\0';

	/* crypt this segment and add the ciphertext to the return buffer */
	strcpy(retp, (char *)(2+crypt(buf, s)));
	if (!*key || segs >= AUTH_SEGMENTS(AUTH_MAX_PASSWD_LENGTH))
	    break;

	/* copy the new salt */
	s[0] = *retp++; s[1] = *retp++;
	retp += AUTH_CIPHERTEXT_SEG_CHARS-2;
    }
    return retbuf;
}
#endif /* HAVE_GETPRPWNAM && !HAVE_BIGCRYPT */


int
pop_pass(p)
    POP *p;
{
    char *username;
    struct passwd *pw;
#ifdef HAVE_GETSPNAM
    struct spwd *shadow_pw;
#endif /* HAVE_GETSPNAM */
#ifdef HAVE_GETPWANAM
    struct passwd_adjunct *pwadj;
#endif /* HAVE_GETPWANAM */
#ifdef HAVE_GETPRPWNAM
    struct pr_passwd *prpw;
#endif /* HAVE_GETPRPWNAM */
    char *password;
    char *try_password;

    /* Look up the username in the password file. */
    username = p->user;

    if (!(pw = getpwnam(username)))
	goto nix;

    /* Check that the usernames match. */
    if (strcmp(pw->pw_name, username) != 0)
	goto nix;

#ifndef USE_ULTRIX_AUTH_USER
    /*
     * get passwords in increasing order of security, so we get the most
     * secure version that succeeds
     */
    password = pw->pw_passwd;

# ifdef HAVE_GETSPNAM
    shadow_pw = getspnam(username);
    if (shadow_pw != NULL)
	password = shadow_pw->sp_pwdp;
# endif /* HAVE_GETSPNAM */

# ifdef HAVE_GETPWANAM
    if (pwadj = getpwanam(username))
      password = pwadj->pwa_passwd;
# endif /* HAVE_GETPWANAM */

# ifdef HAVE_GETPRPWNAM
    if (prpw = getprpwnam(username))
      password = prpw->ufld.fd_encrypt;
# endif /* HAVE_GETPRPWNAM */

    /* We don't accept connections from users with null passwords. */
    if (password == NULL)
	goto nix;

# ifndef BLATANTLY_UNSECURE
    /* Encrypt the supplied password and compare it with the password file. */
#  ifdef HAVE_GETPRPWNAM /* seems to be only one that allows >8-char pw's */
    try_password = bigcrypt(&p->raw_command[5], password);
#  else /* HAVE_GETPRPWNAM */
    try_password = crypt(&p->raw_command[5], password);
#  endif /* HAVE_GETPRPWNAM */
    if (strncmp(try_password, password, strlen(password)) != 0)
	goto nix;
# endif /* BLATANTLY_UNSECURE */
#else /* USE_ULTRIX_AUTH_USER */
    if (authenticate_user(pw, &p->raw_command[5], NULL) < 0)
      goto nix;
#endif /* USE_ULTRIX_AUTH_USER */

    strcpy(p->homedir, pw->pw_dir);

    /* We used to do here what do_drop() now does */
    setuid(pw->pw_uid);

    pop_msg(p, POP_SUCCESS, "%s logged in", username);
    return POP_SUCCESS;

  nix:
    /* Note that we're deliberately coy about the reason for rejection. */
    pop_msg(p, POP_FAILURE,
	    "Password supplied for \"%s\" is incorrect.", username);
    return POP_FAILURE;
}

static int
dirp(name)
    char *name;
{
    struct stat statbuf;
    int retval = 0;

    if (stat(name, &statbuf))
	return (0);
    return ((statbuf.st_mode & S_IFMT) == S_IFDIR);
}

void
do_drop(pop)
    POP *pop;
{
    char *p, *q, expanded[MAXPATHLEN];

    if (pop->CurrentState != pretrans)
	return;

    /* Expand pop->dropname */
    p = pop->dropname;
    q = expanded;
    while (*p) {
	if (*p == '%') {
	    switch (*++p) {
	      case 'd':		/* default spool dir */
		strcpy(q, POP_MAILDIR);
		q += strlen(POP_MAILDIR);
		break;
	      case 'u':
		strcpy(q, pop->user);
		q += strlen(pop->user);
		break;
	      case 'h':
		strcpy(q, pop->homedir);
		q += strlen(pop->homedir);
		break;
	      default:
		*q++ = *p;
		break;
	    }
	    ++p;
	} else {
	    *q++ = *p++;
	}	
    }
    *q = '\0';
    strcpy(pop->dropname, expanded);
    while (dirp(pop->dropname)) {
	strcat(pop->dropname, "/");
	strcat(pop->dropname, pop->user);
    }

    /* Expand pop->tmpdropname */
    p = pop->tmpdropname;
    q = expanded;
    while (*p) {
	if (*p == '%') {
	    switch (*++p) {
	      case 'd':		/* default spool dir */
		strcpy(q, POP_MAILDIR);
		q += strlen(POP_MAILDIR);
		break;
	      case 's':		/* dir from pop->dropname */
		strcpy(q, pop->dropname);
		q = rindex(q, '/');
		break;
	      case 'u':
		strcpy(q, pop->user);
		q += strlen(pop->user);
		break;
	      case 'h':
		strcpy(q, pop->homedir);
		q += strlen(pop->homedir);
		break;
	      default:
		*q++ = *p;
		break;
	    }
	    ++p;
	} else {
	    *q++ = *p++;
	}
    }
    *q = '\0';
    strcpy(pop->tmpdropname, expanded);
    while (dirp(pop->tmpdropname)) {
	sprintf(pop->tmpdropname + strlen(pop->tmpdropname),
		"/.%s.pop",
		pop->user);
    }

    /* Make a temporary copy of the user's maildrop
       and set the group and user id. */
    pop_dropcopy(pop);

    /* Initialize the last-message-accessed number. */
    pop->orig_last_msg = 0;
    pop->last_msg = 0;

    /* Get information about the maildrop. */
    pop_dropinfo(pop);

#if 0
    /* Authorization completed successfully. */
    pop_msg(pop, POP_SUCCESS, "%s has %d %s (%d octets).",
	    pop->user, NUMMSGS(pop),
	    (NUMMSGS(pop) == 1 ? "message" : "messages"),
	    pop->drop_size);
#endif
}
