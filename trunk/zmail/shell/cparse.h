#ifndef _CPARSE_H_
#define _CPARSE_H_

#define cpOn  1
#define cpOff 2

typedef struct {
    int opt;
    u_long flags, bit;
} cpDescRec;
typedef cpDescRec *cpDesc;

typedef struct {
    cpDesc desc;
    VPTR value;
} cpOptRec;
typedef cpOptRec *cpOpt;

typedef struct {
    char **argv;
    u_long onbits, offbits;
    char opts[256];
    int optct, optslots, optptr;
    cpOpt optlist;
    cpDesc descs;
} cpDataRec;
typedef cpDataRec *cpData;

#define CP_ALL_ARGS        ULBIT(0)

#define CP_STR_ARG	   ULBIT(0)
#define CP_INT_ARG	   ULBIT(1)
#define CP_POSITIONAL	   ULBIT(2)
#define CP_MULTIPLE	   ULBIT(3)
#define CP_OPT_ARG	   ULBIT(4)

extern int cpParseOpts P((cpData,char**,cpDesc,char*(*)[2],u_long));
extern VPTR cpGetOpt P((cpData,int));
extern void cpAddOpt P((cpData,cpDesc,VPTR));
extern void cpAddOptBool P((cpData,cpDesc,int));
extern int cpOtherOpts P((cpData,char*));
extern int cpHasOpts P((cpData,char*));
extern int cpNextOpt P((cpData,int*,char**,int*));
extern cpDesc cpGetDesc P((cpData,int));
#define cpGetOptStr(X,Y) ((char *) cpGetOpt((X),(Y)))
#define cpGetOptInt(X,Y) ((int) cpGetOpt((X),(Y)))
#define cpGetFlagsOn(X)  ((X)->onbits)
#define cpGetFlagsOff(X) ((X)->offbits)
#define cpOptIsOn(X,Y) ((X)->opts[(Y)] == cpOn)

#endif /* !_CPARSE_H_ */
