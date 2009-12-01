/* Define TTY_t, Stty(int, TTY_t *), and Gtty(int, TTY_t *) */
/* This logic duplicates a big chunk of zmtty.h, but since I */
/* stole it from Bob, it must be ok. */

#ifdef TERM_USE_TERMIOS
# include <termios.h>
typedef struct termios TTY_t;
# define VMAX NCCS
# ifdef HAVE_TCSETATTR
#  define Stty(fd,buf) (tcsetattr((fd), TCSADRAIN, (buf)))
# else /* HAVE_TCSETATTR */
#  ifdef TCSETS
#   define Stty(fd,buf) (ioctl((fd), (TCSETS), (buf)))
#  else /* TCSETS */
#   ifdef TCSETAW
#    define Stty(fd,buf) (ioctl((fd), (TCSETAW), (buf)))
#   else /* TCSETAW */
#    define Stty(fd,buf) (ioctl(fd, TIOCSETN, buf))
#   endif /* TCSETAW */
#  endif /* TCSETS */
# endif /* HAVE_TCSETATTR */
# ifdef HAVE_TCGETATTR
#  define Gtty(fd,buf) (tcgetattr((fd), (buf)))
# else
#  ifdef TCGETS
#   define Gtty(fd,buf) (ioctl((fd), (TCGETS), (buf)))
#  else /* TCGETS */
#   define Gtty(fd,buf) (ioctl((fd), (TCGETA), (buf)))
#  endif /* TCGETS */
# endif /* HAVE_TCGETATTR */
#else /* TERM_USE_TERMIOS */
# ifdef TERM_USE_TERMIO
#  include <termio.h>
typedef struct termio TTY_t;
#  define VMAX NCC
#  ifdef TCSETS
#   define Stty(fd,buf) (ioctl((fd), (TCSETS), (buf)))
#  else /* TCSETS */
#   ifdef TCSETAW
#    define Stty(fd,buf) (ioctl((fd), (TCSETAW), (buf)))
#   else /* TCSETAW */
#    ifdef TCSETATTRD
#     define Stty(fd,buf) (ioctl((fd), (TCSETATTRD), (buf)))
#    else /* TCSETATTRD */
#     define Stty(fd,buf) (ioctl((fd), (TIOCSETN), (buf)))
#    endif /* TCSETATTRD */
#   endif /* TCSETAW */
#  endif /* TCSETS */
#  ifdef TCGETA
#   define Gtty(fd,buf) (ioctl((fd), (TCGETA), (buf)))
#  else /* TCGETA */
#   define Gtty(fd,buf) (ioctl((fd), (TCGETATTR), (buf)))
#  endif /* TCGETA */
# else /* TERM_USE_TERMIO */
#  ifdef TERM_USE_SGTTYB
#   include <sgtty.h>
typedef struct sgttyb TTY_t;
#   define Stty(fd,buf) (stty((fd),(buf)))
#   define Gtty(fd,buf) (gtty((fd),(buf)))
#  endif /* TERM_USE_SGTTYB */
# endif /* TERM_USE_TERMIO */
#endif /* TERM_USE_TERMIOS */

#define MAXCHARSPERKEY  1024
#define MAYKEYNAMELEN   1024
#define NUMDIALOGKEYS 11

/* Struct holding each key, that is name and the keys it spits out when hit */
typedef struct KeyEntry {
    char Name[MAYKEYNAMELEN];
    unsigned char Keys[MAXCHARSPERKEY];
    int NumKeys;
} KeyEntryType;

/* as above, with fields so I can make a linked list of them */
typedef struct KeyEntryLL {
    KeyEntryType entry;
    struct KeyEntryLL *next;
} KeyEntryLLType;

/* whole thing, function keys, keys from the multikey dialog in Lite */
/* and as many other keys as the user has on their keyboard */
typedef struct KeyHolder {
    int	NumFunctionKeys;
    KeyEntryType	*FunctionKeys;
    int NumOurKeys;
    KeyEntryType OurKeys[NUMDIALOGKEYS];
    KeyEntryLLType*	TheirKeys;
} KeyHolderType;

extern void getOtherKeys P((KeyHolderType *));
extern void getFunctionKeys P((KeyHolderType *));
extern void getDialogKeys P((KeyHolderType *));
extern FILE * tryHomeDir P((char *, char *));
extern void dumpKeybindings P((KeyHolderType *, char*));
extern void ttySetup P((TTY_t *));
extern void ttyRevert P((TTY_t *));
extern void printkey P((KeyEntryType *, FILE *));
extern void readkey P((KeyEntryType *));
extern char * duplicateKey P((KeyEntryType *, KeyHolderType *));


