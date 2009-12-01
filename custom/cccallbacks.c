#if defined( SUN414 )
#if !defined( const )
#define const
#endif
#endif

#include "c-clientmail.h"

#include "zmail.h"
#include "mailserv.h"

/* The following are all symbols that must be defined by the caller of
 * the C-Client library.  They are callbacks used by C-Client to notify
 * the calling program of asynchronous IMAP4 events and the like.
 *
 * Some of these have commented-out sample bodies, lifted from mtest.c
 * in the C-Client sources.
 */

typedef struct _fQ {
	struct _fQ *next;
	unsigned long number;
} FQ;

unsigned long hasFlags = 0;
unsigned long hasDeleted = 0;

FQ *fList = (FQ *) NULL;
FQ *dList = (FQ *) NULL;

int
FQueueAdd( list, number )
FQ **list;
unsigned long number;
{
	FQ *foo;

	foo = (FQ *) malloc( sizeof ( FQ ) );

	foo->number = number;
	foo->next = (FQ *) NULL;
	if ( !*list ) {
		*list = foo;
	}
	else {
		foo->next = *list;
		*list = foo;
	}
}

unsigned long
FQNext( which )
FQ **which;
{
	unsigned long foo;
	FQ *sav, *sav2;

	if ( !*which )
		return( 0 );
	foo = (*which)->number;
	sav = *which;
	*which = (*which)->next;
	free( sav );
	return( foo );
}

unsigned long 
GetDeleted()
{
	return( FQNext( &dList ) );
}

unsigned long
GetFlags()
{
	return( FQNext( &fList ) );
}

#if defined( ANSI )
void mm_searched (MAILSTREAM *stream, unsigned long number)
#else
void mm_searched (stream, number)
MAILSTREAM *stream;
unsigned long number;
#endif
{
}

#if defined( ANSI )
void mm_exists (MAILSTREAM *stream, unsigned long number)
#else
void mm_exists (stream, number)
MAILSTREAM *stream;
unsigned long number;
#endif
{
}

static int ec = 0;

int
InExpungeCallback()
{
	return( ec );
}

void
SetExpungeCallback( val )
int val;
{
	ec = val;
}

#if defined( ANSI )
void mm_expunged (MAILSTREAM *stream, unsigned long number)
#else
void mm_expunged (stream, number)
MAILSTREAM *stream;
unsigned long number;
#endif
{
	if ( !boolean_val( VarImapShared ) )
		return;
	if ( number < 0 || number > msg_cnt )
		return;
	FQueueAdd( &dList, current_folder->mf_msgs[number - 1]->uid );
	hasDeleted = 1;
	if ( boolean_val( VarImapShared ) ) {
		SetExpungeCallback( 1 );
		HandleFlagsAndDeletes( 2 );
		SetExpungeCallback( 0 );
	}
}

#if defined( ANSI )
void mm_flags (MAILSTREAM *stream, unsigned long number)
#else
void mm_flags (stream, number)
MAILSTREAM *stream;
unsigned long number;
#endif
{
	if ( number < 0 || number > msg_cnt )
		return;
	FQueueAdd( &fList, current_folder->mf_msgs[number - 1]->uid );
	hasFlags = 1;
}

#if defined( ANSI )
void mm_notify (MAILSTREAM *stream,char *string,long errflg)
#else
void mm_notify (stream,string,errflg)
MAILSTREAM *stream;
char *string;
long errflg;
#endif
{
	extern MAILSTREAM *gIMAPh;

	switch( errflg ) {
	case BYE:
		gIMAPh = (MAILSTREAM *) NULL;
		imap_initialized = 0;
		break;
	case ERROR:
		strcpy( imap_error, string );
		zimap_display_error();

		/* assume fatal */

		gIMAPh = (MAILSTREAM *) NULL;
		imap_initialized = 0;
		break;
	case WARN:
		strcpy( imap_error, string );
		zimap_display_warning();
		break;
	case NIL:
#if 0
		strcpy( imap_error, string );
		zimap_display_info();
#endif
		break;
	}
}

static MAILSTATUS statusCache;

#if defined( ANSI ) 
void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
#else
void mm_status (stream,mailbox,status)
MAILSTREAM *stream;
char *mailbox;
MAILSTATUS *status;
#endif
{
	if ( status )
		memcpy( &statusCache, status, sizeof( MAILSTATUS ) );		
	return;
}

unsigned long
#if defined( ANSI )
mm_getRecent( void )
#else
mm_getRecent()
#endif
{
	if ( statusCache.flags & SA_RECENT )
		return( statusCache.recent );
	return( 0 );
}

unsigned long
#if defined( ANSI )
mm_getCount( void )
#else
mm_getCount()
#endif
{
	if ( statusCache.flags & SA_MESSAGES )
		return( statusCache.messages );
	return( 0 );
}

#if defined( ANSI )
void mm_list (MAILSTREAM *stream,int delimiter,char *mailbox,long attributes)
#else
void mm_list (stream, delimiter,mailbox, attributes)
MAILSTREAM *stream;
int delimiter; 
char *mailbox; 
long attributes;
#endif
{
  if ( attributes & LATT_NOSELECT && attributes & LATT_NOINFERIORS )

	/* nonsense */

	return;

  if ( mailbox ) {
	if ( attributes == 0 ) attributes = LATT_NOSELECT | LATT_NOINFERIORS;
	AddFolder( mailbox, delimiter, attributes ); 
  }
}

#if defined( ANSI )
void mm_lsub (MAILSTREAM *stream,int delimiter,char *mailbox,long attributes)
#else
void mm_lsub (stream, delimiter,mailbox, attributes)
MAILSTREAM *stream;
int delimiter; 
char *mailbox; 
long attributes;
#endif
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

#if defined( ANSI )
void mm_log (char *string,long errflg)
#else
void mm_log (string, errflg)
char *string;
long errflg;
#endif
{
  mm_notify( NIL, string, errflg );

#if 0
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
#endif
}

#if defined( ANSI )
void mm_dlog (char *string)
#else
void mm_dlog (string)
char *string;
#endif
{
  /* puts (string); */
}

#if defined( ANSI )
void mm_login (NETMBX *mb,char *user,char *pwd,long trial)
#else
void mm_login (mb,user,pwd,trial)
NETMBX *mb;
char *user;
char *pwd;
long trial;
#endif
{
  /* we already have our username and password. stuff it in the
     fields and return */

  strcpy( user, mailserver_GetUsername(mserv) );
  strcpy( pwd, mailserver_GetPassword(mserv) ); 
}

#if defined( ANSI )
void mm_critical (MAILSTREAM *stream)
#else
void mm_critical (stream)
MAILSTREAM *stream;
#endif
{
}

#if defined( ANSI )
void mm_nocritical (MAILSTREAM *stream)
#else
void mm_nocritical (stream)
MAILSTREAM *stream;
#endif
{
}

#if defined( ANSI )
long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
#else
long mm_diskerror (stream, errcode, serious)
MAILSTREAM *stream;
long errcode;
long serious;
#endif
{
#ifdef UNIX
  kill (getpid (),SIGSTOP);
#else
  abort ();
#endif
  return NIL;
}

#if defined( ANSI )
void mm_fatal (char *string)
#else
void mm_fatal (string)
char *string;
#endif
{
  /* printf ("?%s\n",string); */
}
