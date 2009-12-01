
%{

/*
 *  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
 *           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
 *           RECOMMENDED FOR PRODUCTION USE.
 *
 *  This code has been produced for the C3 Task Force within
 *  TERENA (formerly RARE) by:
 *  
 *  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
 *     Royal Institute of Technology, Stockholm, Sweden
 *
 *  Use of this provisional source code is permitted and
 *  encouraged for testing and evaluation of the principles,
 *  software, and tableware of the C3 system.
 *
 *  More information about the C3 system in general can be
 *  found e.g. at
 *      <URL:http://www.nada.kth.se/i18n/c3/> and
 *      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
 *
 *  Questions, comments, bug reports etc. can be sent to the
 *  email address
 *      <c3-questions@nada.kth.se>
 *
 *  The version of this file and the date when it was last changed
 *  can be found on (or after) the line starting "static char file_id".
 *
 */

#define STRINGBUFLEN 10000
#define RET return
#undef PRI
#define PRI nothing
#undef ECHO
#define ECHO


#include "fh.h"
#include "util.h"

fhandle fh_in;

#undef input
#define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):fh_byte_read(fh_in))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)

static char file_id_lex[] = "$Id: parse.lex,v 1.3 1995/07/19 16:33:57 tom Exp $";

static long lint1;
/*** static byte by; ***/

#ifdef THINK_C
extern char *
strdup(
	const char *str
);
#endif

void
nothing(
)
{
}

%}


/* Regular definitions */

lwsp	[ \t]*
rwsp	[ \t]+
cr	\r
lf	\n
hexdig	[0-9a-fA-F]
hexnum	{hexdig}{hexdig}*
decnum  [0-9][0-9]*
identif		[A-Za-z][A-Za-z0-9_]*
charprio	\({decnum}\)
eol	{cr}|{lf}|{cr}{lf}
eostmt	{eol}/[^+ \t]
charspec  '({hexdig}{hexdig}|{hexdig}{hexdig}{hexdig}{hexdig}|{hexdig}{hexdig}{hexdig}{hexdig}{hexdig}{hexdig}{hexdig}{hexdig})
subrepchar	[^\]\}\{\|~^@`#$\"]
litt1		\"({subrepchar}|\"\")\"
litt		\"({subrepchar}|\"\")*\"
signal_symb	"&"
fallback_symb	"?"

%START dirline, ucsline

%%
aoza		{BEGIN ucsline; PRI(" AFILESTART"); RET(AFILESTART);}
aozz		{BEGIN ucsline; RET(EFILESTART);}
^%COMMENT_CCS   {BEGIN dirline; RET(D_COMMENT_CCS);}
^%RADIX		{BEGIN dirline; RET(D_RADIX);}
^%CONV_SYST	{BEGIN dirline; RET(D_CONV_SYST);}
^%APPROX_CONV 	{BEGIN dirline; RET(D_APPROX);}
^%CCS 		{BEGIN dirline; g_numlist.length = 0;
		 RET(D_CCS);}
^%FACTOR   	{BEGIN dirline; RET(D_FACTOR);}
^%CCS_WIDTH  	{BEGIN dirline; g_numlist.length = 0;
		RET(D_CCS_WIDTH);}
^%CCS_STATE   	{BEGIN dirline; RET(D_CCS_STATE);}
^%CONV_TYPES	{BEGIN dirline; RET(D_CONV_TYPES);}
^%REVERSIBLE_TYPE {BEGIN dirline; RET(D_REVERSIBLE_TYPE);}
^%FACTOR_SRC_ADD   {BEGIN dirline; RET(D_FACTOR_SRC_ADD);}
^%FACTOR_TGT_ADD   {BEGIN dirline; RET(D_FACTOR_TGT_ADD);}
^%FACTOR_SRC_RMV   {BEGIN dirline; RET(D_FACTOR_SRC_RMV);}
^%FACTOR_TGT_RMV   {BEGIN dirline; RET(D_FACTOR_TGT_RMV);}
^%FACTOR_VALUE_ADD	{BEGIN dirline; RET(D_FACTOR_VALUE_ADD);}
^%FACTOR_VALUE_RMV	{BEGIN dirline; RET(D_FACTOR_VALUE_RMV);}
^%END_OF_TABLE 	{BEGIN dirline; PRI(" END_OF_TABLE"); RET(D_END_OF_TABLE);}
INVARIANT   {RET(APPROX_INVARIANT);}

<dirline>{decnum} {
		    (void) sscanf(yytext, "%ld", &lint1);
		    yylval.num = lint1;
		    PRI(" D_NUM<%ld>", lint1);
		    RET(NUM);
		}
<dirline>{identif}	{
		    yylval.str=strdup(yytext);
		    PRI(" D_IDENTIFIER<%s>", yylval.str);
		    RET(D_IDENTIFIER);
		}
{charprio}	 {
		    (void) sscanf(yytext+1, "%ld", &lint1);
		    yylval.num = lint1;
		    PRI(" CHAR_PRIO<%ld>", lint1);
		    RET(CHAR_PRIO);
		}
{rwsp}		{PRI(" (RWSP)");}
{eostmt}	{PRI(" EOSTMT\n");
		 BEGIN ucsline;
		 RET(EOSTMT);}
{eol}           {PRI(" (EOL)\n");}
^{lwsp}\+	{PRI(" (CONTLIN)");}
{charspec}	{
	            enc_unit ucs;

		    

                    (void) sscanf(yytext+1, "%lx", &ucs);
		    yylval.eunit=ucs;
		    PRI(" CHAR<%x>", yylval.eunit);
		    RET(UCSCHARSPEC);
		}
{hexnum}	{
		    (void) sscanf(yytext, "%lx", &lint1);
		    yylval.eunit = lint1;
		    PRI(" EU<%lx>", lint1);
		    RET(EU);
		}
{litt1}		{
		    PRI(" LITT1-CHAR<%s>", yytext);
		    if (yyleng == 4) /* Must be doubled <"> */
		    {
		        yylval.eunit = (enc_unit) '"';
		    }
		    else
		    {
		        yylval.eunit = (enc_unit) *(yytext + 1);
		    }
		    RET(UCSCHARSPEC);
		}

{litt}		{
		    char *fromc=yytext+1;

		    PRI(" LITT<%s>", yytext);
		    yylval.eus = euseq_from_dupstr(NULL, 2, 16);
		    euseq_setepos(yylval.eus, -1);
		    while(fromc < yytext+yyleng-1) {
			euseq_puteu(yylval.eus, (enc_unit) *fromc);
			if(*fromc == '"' && *(fromc+1) == '"')
			    fromc++;
			fromc++;
		    }
		    RET(LITT);
		}
{signal_symb}	{
		    PRI(" SIGNAL");
		    yylval.eus = euseq_from_dupstr(" ", 2, 1);
		    euseq_setepos(yylval.eus, 0);
		    RET(SIGNAL);
		}
{fallback_symb}	{PRI(" FALLBACK"); RET(FALLBACK);}
"=>>"		{PRI(" APPROX_APPROX_OP"); RET(APPROX_APPROX_OP);}
"=>"		{PRI(" DEF_APPROX_OP"); RET(DEF_APPROX_OP);}
"="		{PRI(" EQUAL_OP"); RET(EQUAL_OP);}
:.*$		{PRI(" (COMM)");}
.		{ECHO; PRI(" DEFAULT<%s>", yytext); RET(*yytext);}
%%

