/* 
   Support for remote folder display and traversal. 
   See also motif/m_finder.c.
*/

#include <stdio.h>
#include <sys/param.h>

#include "imaplib.h"

#include "c-clientmail.h"

#if defined( TEST )
#define LATT_NOINFERIORS 1
#define LATT_NOSELECT 2
#endif

#if !defined( MAXPATHLEN )
#define MAXPATHLEN 1024
#endif

static Folders *tree = (Folders *) NULL;

static char delimiter = '/';	/* the default */

#define SCRUNCHLEN 24

void
scrunch( str )
char *str;
{
        int i, n, bound;
        char *p, *newstr;

        if ( !str )
                return;
        if ( ( n = strlen( str ) ) <= SCRUNCHLEN + 4 )
                return;

        newstr = (char *) malloc( SCRUNCHLEN + 4 );
        bound = SCRUNCHLEN / 2;

        for ( i = 0; i < bound; i++ )
                newstr[i] = str[ i ];

        newstr[ bound ] = newstr[ bound + 1 ] = newstr[ bound + 2 ] = '.';
        p = &str[ n - 1 ] - (bound - 1);
        for ( i = bound + 3, n = 0; n < bound; n++, p++, i++ )
            newstr[ i ] = *p;
        newstr[ i ] = '\0';
	    strcpy( str, newstr );
        free( newstr );
}


#if !defined( MOTIF ) && !defined( GUI )

int
InRemoveGUI()
{
	return( 0 );
}

int
UseIMAP()
{
	return( 0 );
}

int
GetUseIMAP()
{
	return( 0 );
}

#endif

int
FolderIsValid( file )
char *file;
{
	int	i, retval = 1;

	if ( !file || !strcmp( file, ".." ) )
		return( 0 );

	i = 0;
	while ( i < strlen( file ) ) {
		if ( isspace( file[i] ) || file[i] == delimiter )
			retval = 0;
		i++;
	} 
	return( retval );
}

int
FolderHasSlashes( file )
char *file;
{
	int	i, retval = 0;

	i = 0;
	while ( i < strlen( file ) ) {
		if ( file[i] == delimiter )
			retval = 1;
		i++;
	} 
	return( retval );
}

int
FolderHasSpaces( file )
char *file;
{
	int	i, retval = 0;

	i = 0;
	while ( i < strlen( file ) ) {
		if ( isspace( file[i] ) )
			retval = 1;
		i++;
	} 
	return( retval );
}

Folders *
GetTreePtr()
{
	return( tree );
}

char
GetDelimiter()
{
	return( ( UseIMAP() ? delimiter : '/' ) );
}

char *
GetDelimiterString()
{
	static char buf[ 16 ];

	sprintf( buf, "%c", GetDelimiter() );
	return( buf );
}

int
HaveTree()
{
	return( ( tree == (Folders *) NULL ? 0 : 1 ) );
}

int
IsRoot( p )
Folders *p;
{
	return( ( p == GetTreePtr() ? 1 : 0 ) );
}

static void
PurgeSubTree( tp )
Folders *tp;
{
	int	i;

	for ( i = 0; i < tp->nsibs; i++ )
		if ( strcmp( tp->sibs[i]->name, ".." ) )
			PurgeSubTree( tp->sibs[i] );
		else
			free( tp->sibs[i]->name );
	if ( tp->name )
		free( tp->name );
	if ( tp->sibs )
		free( tp->sibs );
	free( tp );
}

Folders *
GetParent( p )
Folders *p;
{
	if ( p )
		return( p->parent );
	return( (Folders *) NULL );
}

void
PurgeTree()
{
	if ( tree ) {
		PurgeSubTree( tree );
		tree = (Folders *) NULL;
	}
	delimiter = 0;
}

void
RemoveFolder( p )
Folders *p;
{
	Folders *q;
	int 	i, idx;

	/* remove parent reference to node as a sibling */

	q = p->parent;

	if ( q ) {
		idx = -1;
		for ( i = 0; i < q->nsibs; i++ ) {
			if ( q->sibs[i] == p ) {
				idx = i;
				break;
			}
		}

		/* if found, remove node and decrement sibling count */

		if ( idx >= 0 ) {
			if ( idx < q->nsibs - 1 )
				for ( i = idx; i < q->nsibs - 1; i++ )
					q->sibs[i] = q->sibs[i + 1];
			q->nsibs--;
		}
	}

	/* blow away the folder, and any children it may have */

	if ( p->attributes & LATT_NOSELECT ) {
		PurgeSubTree( p );
		if ( p == GetTreePtr() )
			tree = (Folders *) NULL;
	}
	else {
		if ( p->name ) free( p->name );
		free( p );
	}
}

Folders *
FolderHasItem( name, pFolder )
char	*name;
Folders *pFolder;
{
	int	i;
	Folders *ret = (Folders *) NULL;

	if ( pFolder == (Folders *) NULL )
		pFolder = GetTreePtr();
	if ( pFolder == (Folders *) NULL )
		return( pFolder );

	for ( i = 0; i < pFolder->nsibs; i++ )
		if ( !strcmp( pFolder->sibs[i]->name, name ) ) {
			ret = pFolder->sibs[i];
			break;
		}
	return( ret );
}

char *
GetNextComponent( p, delimiter )
char	*p;
char	delimiter;
{
	char	*q, *tmp;
	
	q = (char *) malloc( strlen( p ) + 1 );
	strcpy( q, p );
	tmp = q;
	while ( *q && *q != delimiter ) q++;
	*q = '\0';
	return( tmp );
}

Folders *
FolderByName( name )
char	*name;
{
	int	levels, i, j, hit;
	Folders *p;
	char	*next;

	p = tree;

	if ( !name || !strcmp( name, "" ) ) 
		return( p );

	levels = CountDelimiters( name, delimiter );

	for ( i = 0; i <= levels; i++ ) {
		next = GetNextComponent( name, delimiter );
		hit = 0;
		if ( next ) {
			for ( j = 0; !hit && j < p->nsibs; j++ ) {
				if ( !strcmp( p->sibs[j]->name, next ) ) {
					p = p->sibs[j];
					hit++;
				}
			}
		}

		if ( !hit ) {
			if ( next )
				free( next );
			return( (Folders *) NULL );
		}

        	name += strlen( next ) + 1;
		free( next );
	}
	if ( !strcmp( name, ".." ) )
		p = p->parent;
	return( p );
}

int
ChangeName( src, dst )
char	*src;
char	*dst;
{
	Folders *pSrc, *pDst;
	int	retval = 0;

	pSrc = FolderByName( src );
	pDst = FolderByName( dst );
	if ( !pDst && pSrc ) {
		if ( !pSrc->name )
			pSrc->name = malloc( strlen( dst ) + 1 );
		else
			pSrc->name = realloc( pSrc->name, strlen( dst ) + 1 );
		strcpy( pSrc->name, dst );
		retval = 1;
	}
	return( retval );
}

char *
GetFullPath( name, pFolder )
char	*name;
Folders	*pFolder;
{
	static char path[MAXPATHLEN], nameTmp[MAXPATHLEN];
	int	height = 0, i;
	Folders	*p, **pVec;
	char	dStr[2], *q;

	nameTmp[0] = path[0] = '\0';
	if ( name ) 
		strcpy( nameTmp, name );

	/* this happens the first time we're called */

	if ( pFolder == 0 ) {
		if ( !name )
			strcpy( path, "" );
		else
			strcpy( path, nameTmp );
		return( path );
	}

	p = pFolder;

	if ( !strcmp( nameTmp, ".." ) ) {
		p = p->parent;
		if ( p == tree )
			strcpy( nameTmp, "" );
		else
			strcpy( nameTmp, p->name );
	}

	/* if we're at the root, just return the name. otherwise, search 
	   bottom up */

	if ( p == tree ) 
		strcpy( path, nameTmp );
	else {
		pVec = (Folders **) malloc( sizeof(Folders *) );
		pVec[0] = (Folders *) NULL;

		/* check to see if it is a selected sibling */

		for ( i = 0; i < p->nsibs; i++ )
			if ( !strcmp( p->sibs[i]->name, nameTmp ) ) {
				pVec[0] = p->sibs[i];
				break;
			}

		if ( pVec[0] == (Folders *) NULL ) {

			/* we might be at the leaf */

			if ( !strcmp( nameTmp, p->name ) )
				pVec[0] = p;
			else {
				free( pVec );
				nameTmp[0] = '\0';
				return( nameTmp );
			}
		}	

		/* walk the tree back to the root */
	
		p = pVec[0];	
		while ( p->parent && p->parent != tree ) {
			if ( p->parent ) {
				height++;
				if ( !pVec )
				pVec = (Folders **) malloc( (height + 1) * sizeof( Folders * ) );
				else
				pVec = (Folders **) realloc( pVec, 
					(height + 1) * sizeof( Folders * ) );
				pVec[height] = p->parent;
			}
			p = p->parent;
		}

		/* build the path string */

		dStr[0] = delimiter;
		dStr[1] = '\0';	
		for ( i = height; i >= 0; i-- ) {
			if ( i == height )
				strcpy( path, pVec[i]->name );
			else
				strcat( path, pVec[i]->name );
			if ( i != 0 )
				strcat( path, dStr );
		}
		free ( pVec );
	}
	return( path );
}

char *
GetCurrentDirectory( pFolder )
Folders	*pFolder;
{
	return( GetFullPath( ( pFolder == tree || pFolder == 0 ? "" : pFolder->name ), ( pFolder == tree || pFolder == 0 ? 0 : pFolder->parent ) ) );
}

void
AddFolder( path, delim, type )
char 	*path;
char 	delim;
long	type;		/* attributes */
{
	char 	*q, *p = path;
	Folders *fp, *tp;
	int	found, i, levels, cnt;
	char	buf[ 256 ];

	if ( delim )
		delimiter = delim;

	if ( type == ROOT ) {
		fp = (Folders *) calloc( 1, sizeof( Folders ) );
		fp->attributes = type;
		fp->delimiter = delim;
		fp->name = (char *) NULL;
		tree = fp;
		return;
	}

	/* skip past the {host} stuff */

	while ( *p && *p != '}' ) p++;
	if ( *p == '}' ) 
		p++;
	else 
		p = path;	/* XXX can this happen? */

	/* count levels in the supplied path */

	levels = CountDelimiters( p, delim );

	/* if no delimiters, then add as child of / */

	if ( levels == 0 ) {

		/* make sure it doesn't already exist */

		found = 0;
		for ( i = 0; !found && i < tree->nsibs; i++ ) {
			if ( !strcmp( tree->sibs[i]->name, p ) )
				found++;
		}

		if ( !found ) { 
			fp = (Folders *) calloc( 1, sizeof( Folders ) );
			fp->attributes = type;
			fp->delimiter = delim;
			if ( type & LATT_NOSELECT ) {
                        	fp->nsibs = 1; /* for .. */
                        	fp->sibs = (Folders **) malloc( sizeof( Folders *) );
				fp->sibs[0] = (Folders *) malloc( sizeof( Folders ) );
				fp->sibs[0]->name = (char *) malloc( strlen( ".." ) + 1 );
				fp->sibs[0]->attributes = LATT_NOSELECT;
				fp->sibs[0]->nsibs = 0;
				fp->sibs[0]->delimiter = delim;
				fp->sibs[0]->parent = fp;
				strcpy( fp->sibs[0]->name, ".." );
			}
			tree->nsibs++;
			if ( !tree->sibs )
			tree->sibs = (Folders **) malloc( sizeof(Folders *) );
			else
			tree->sibs = (Folders **) realloc( tree->sibs, tree->nsibs * sizeof(Folders *) );
			fp->parent = tree;
			fp->name = (char *) malloc( strlen( p ) + 1 );
			strcpy( fp->name, p );
			tree->sibs[ tree->nsibs - 1 ] = fp;
		}
	}
	else {
		/* traverse the path, adding components as needed */

		cnt = levels;
		tp = tree;
		while ( strlen( p ) ) {
			q = GetNextComponent( p, delim );
			if ( q ) {
				p += strlen( q );
				found = 0;
				for ( i = 0; !found && i < tp->nsibs; i++ ) {
					if ( !strcmp( tp->sibs[i]->name, q ) ) {
						tp = tp->sibs[ i ];
						found++;
					}
				}
				if ( !found ) {

					/* add it */

					fp = (Folders *) calloc( 1, sizeof( Folders ) );
					if ( cnt != 0 )
						fp->attributes = LATT_NOSELECT;
					else
						fp->attributes = type;
					fp->delimiter = delim;

					if ( fp->attributes & LATT_NOSELECT || *p == delimiter ) {

						/* a folder */

						fp->nsibs = 1; /* for .. */
						fp->sibs = (Folders **) malloc( sizeof( Folders *) );
						fp->sibs[0] = (Folders *) calloc( 1, sizeof( Folders ) );
						fp->sibs[0]->name = (char *) malloc( strlen( ".." ) + 1 );
						strcpy( fp->sibs[0]->name, ".." );
						fp->sibs[0]->attributes = LATT_NOSELECT;
						fp->sibs[0]->nsibs = 0;
						fp->sibs[0]->delimiter = delim;
						fp->sibs[0]->parent = fp;
					}

					tp->nsibs++;
					if ( !tp->sibs )
					tp->sibs = (Folders **) malloc( sizeof(Folders *) );
					else
					tp->sibs = (Folders **) realloc( tp->sibs, tp->nsibs * sizeof(Folders *) );
					fp->parent = tp;
					fp->name = (char *) malloc( strlen( q ) + 1 );
					strcpy( fp->name, q );
					tp->sibs[ tp->nsibs - 1 ] = fp;
					tp = fp;
				}	

				/* skip the component and delimiter */

				free( q );
				if ( *p != '\0' ) p++;
			}
			else
				break; 
			cnt--;
		}
	}
}

int
FolderIsDir( p )
Folders *p;
{
	return( ( p->attributes & LATT_NOSELECT ? 1 : 0 ) );	
}

int
FolderIsFile( p )
Folders *p;
{
	return( ( p->attributes & LATT_NOINFERIORS ? 1 : 0 ) );	
}

int
IsADir( attrib )
unsigned int attrib;
{
	if ( attrib & LATT_NOSELECT )
		return( 1 );
	else
		return( 0 );
}

int
GetPathType( path )
char *path;
{
	Folders *p;
	int	type = -1;

	p = FolderByName( path );
	return( p->attributes );
}

int 
GetFolderCount( name )
char 	*name;
{
	Folders *p;

	p = FolderByName( name );
	return ( ( p ? p->nsibs : 0 ) );
}

char *
GetFolderName( which, prefix )
int     which;
char    *prefix;
{
        Folders *p;

        p = FolderByName( prefix );
	if ( which < p->nsibs ) 
		return( ( p ? p->sibs[which]->name : "" ) );
	else
		return( "" );
}

char *
GetAttributeStringByName( name, folder )
char *name;
Folders *folder;
{
	Folders *pFolder;
        static char attr[ 255 ];

	pFolder = FolderHasItem( name, folder );
        strcpy( attr, "" );
	if ( pFolder ) {
		if ( pFolder->attributes & LATT_NOINFERIORS && pFolder->attributes & LATT_NOSELECT )
			strcpy( attr, "[dir][file]" );
		else if ( pFolder->attributes & LATT_NOINFERIORS )
                        strcpy( attr, "[file]" );
                else if ( pFolder->attributes & LATT_NOSELECT )
                        strcpy( attr, "[directory]" );
                else
                        strcpy( attr, "[unknown]" );
	}
	return( attr );
}

char *
GetFolderAttributeString( which, name )
int     which;
char    *name;
{
        Folders *p;
        static char attr[ 255 ];

        p = FolderByName( name );
        strcpy( attr, "" );
        if ( p && which < p->nsibs ) {
		p = p->sibs[which];
                sprintf( attr, "%-30s", p->name );
		if ( p->attributes & LATT_NOINFERIORS && p->attributes & LATT_NOSELECT )
			strcat( attr, "[dir][file]" );
                else if ( p->attributes & LATT_NOINFERIORS )
                        strcat( attr, "[file]" );
                else if ( p->attributes & LATT_NOSELECT )
                        strcat( attr, "[directory]" );
                else
                        strcat( attr, "[unknown]" );
        }
        return( attr );
}

int
CountDelimiters( name, which )
char *name;
char which;
{
        int     count = 0;
        char    *p = name;

        while ( p && *p != '\0' ) {
                if ( *p == which ) count++;
                p++;
        }
        return( count );
}

#if !defined( MOTIF )	/* XXX */
void
SetFolderParent( p )
void *p;
{
}

void *
GetFoldersP()
{
	return( GetTreePtr() );
}

#endif

void
WalkSubTree( tp )
Folders *tp;
{
	int	i;
	char	*path;

	for ( i = 0; i < tp->nsibs; i++ )
		if ( strcmp( tp->sibs[i]->name, ".." ) )
			WalkSubTree( tp->sibs[i] );
		else {
			path = GetFullPath( tp->sibs[i]->name, tp->sibs[i] );
			fprintf( stderr, "%s\n", path );
		}
}

void
WalkTree()
{
	if ( tree ) 
		WalkSubTree( tree );
}

#if defined( TEST )
main( argc, argv )
int	argc;
char	*argv[];
{
	AddFolder( "", '/', ROOT );
	AddFolder( "{bedrock}IMAP", '/', LATT_NOINFERIORS );
	AddFolder( "{bedrock}mailhost", '/', LATT_NOINFERIORS );
	AddFolder( "{bedrock}yabba/dabba", '/', LATT_NOSELECT );
	AddFolder( "{bedrock}yabba/dabba/doo", '/', LATT_NOINFERIORS );
	AddFolder( "{bedrock}yabba/dabba/dee", '/', LATT_NOSELECT );
	AddFolder( "{bedrock}yabba/freak", '/', LATT_NOINFERIORS );
	AddFolder( "{bedrock}.foobah", '/', LATT_NOINFERIORS );
	FolderByName( "yabba/dabba" );
	FolderByName( "yabba/dabba/dee" );
	FolderByName( "yabba/yooba" );
	FolderByName( "yabba" );
	FolderByName( ".foobah" );
	PurgeTree();
}
#endif
