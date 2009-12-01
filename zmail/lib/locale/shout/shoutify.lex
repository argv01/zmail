%%

^\$.*	ECHO;

\\.	ECHO;

%[-+ #O]*[0-9]*(\.([0-9]*|\*))?(h|l|ll|L)?.	ECHO;

[a-z]	putc( yytext[0] - 'a' + 'A', yyout );

%%

int main()
{
  return yylex();
}
