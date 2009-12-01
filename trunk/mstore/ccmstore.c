#include <cc_mail.h>

/* The following are all symbols that must be defined by the caller of
 * the C-Client library.  They are callbacks used by C-Client to notify
 * the calling program of asynchronous IMAP4 events and the like.
 *
 * Some of these have commented-out sample bodies, lifted from mtest.c
 * in the C-Client sources.
 */

void mm_searched (MAILSTREAM *stream,long number)
{
}


void mm_exists (MAILSTREAM *stream,long number)
{
}


void mm_expunged (MAILSTREAM *stream,long number)
{
}


void mm_flags (MAILSTREAM *stream,long number)
{
}


void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
}


void mm_list (MAILSTREAM *stream,char delimiter,char *mailbox,long attributes)
{
  /*
  putchar (' ');
  if (delimiter) putchar (delimiter);
  else fputs ("NIL",stdout);
  putchar (' ');
  fputs (mailbox,stdout);
  if (attributes & LATT_NOINFERIORS) fputs (", no inferiors",stdout);
  if (attributes & LATT_NOSELECT) fputs (", no select",stdout);
  if (attributes & LATT_MARKED) fputs (", marked",stdout);
  if (attributes & LATT_UNMARKED) fputs (", unmarked",stdout);
  putchar ('\n');
  */
}


void mm_lsub (MAILSTREAM *stream,char delimiter,char *mailbox,long attributes)
{
  /*
  putchar (' ');
  if (delimiter) putchar (delimiter);
  else fputs ("NIL",stdout);
  putchar (' ');
  fputs (mailbox,stdout);
  if (attributes & LATT_NOINFERIORS) fputs (", no inferiors",stdout);
  if (attributes & LATT_NOSELECT) fputs (", no select",stdout);
  if (attributes & LATT_MARKED) fputs (", marked",stdout);
  if (attributes & LATT_UNMARKED) fputs (", unmarked",stdout);
  putchar ('\n');
  */
}


void mm_log (char *string,long errflg)
{
  /*
  switch ((short) errflg) {
  case NIL:
    printf ("[%s]\n",string);
    break;
  case PARSE:
  case WARN:
    printf ("%%%s\n",string);
    break;
  case ERROR:
    printf ("?%s\n",string);
    break;
  }
  */
}


void mm_dlog (char *string)
{
  /* puts (string); */
}


void mm_login (char *host,char *user,char *pwd,long trial)
{
  /*
  char tmp[MAILTMPLEN];
  if (curhst) fs_give ((void **) &curhst);
  curhst = (char *) fs_get (1+strlen (host));
  strcpy (curhst,host);
  sprintf (tmp,"{%s} username: ",host);
  prompt (tmp,user);
  if (curusr) fs_give ((void **) &curusr);
  curusr = (char *) fs_get (1+strlen (user));
  strcpy (curusr,user);
  prompt ("password: ",pwd);
  */
}


void mm_critical (MAILSTREAM *stream)
{
}


void mm_nocritical (MAILSTREAM *stream)
{
}


long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
#ifdef UNIX
  kill (getpid (),SIGSTOP);
#else
  abort ();
#endif
  return NIL;
}


void mm_fatal (char *string)
{
  /* printf ("?%s\n",string); */
}
