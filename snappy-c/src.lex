/* Convert google comment and identifier style to linux style. */
%x comment studlycaps ccomment
%{
#include <string.h>
#include <ctype.h>

int lastindent;

int indent(void)
{
	int space = strspn(yytext, " \t");
	printf("%.*s", space, "");
	lastindent = space;
	return space;
}

#define P(x) putchar(x)

%}
%option noyywrap
%%

"/*"                            { BEGIN(ccomment); ECHO; }
<ccomment>.                     { ECHO; }
<ccomment>"*/"                  { BEGIN(INITIAL); ECHO; }

[ \t]*"//".*\n[\ t]*"//".*\n    { BEGIN(comment);
	                          int n = indent(); 
                                  printf("/*\n");
                                  indent();
				  char *s = yytext + n + 2;
				  int llen = strcspn(s, "\n");
                                  printf(" *%.*s\n", llen, s);
				  indent();
				  s += llen + 1;
				  s += strspn(s, " \t");
				  s += 2;
				  printf(" * %s", s);
                                }
<comment>[ \t]*"//".*\n	        { int n = indent(); printf(" *%s", yytext + n + 2); }
<comment>.*\n                   { printf("%.*s */\n", lastindent, ""); 
                                  BEGIN(INITIAL); 
				  yyless(0); 
                                }

"//".*\n			{ printf("/*%.*s */\n", yyleng - 3, yytext + 2); }


	/*
[a-zA-Z_][a-zA-Z_0-9]*_         { yyleng--; ECHO; }
	*/

[A-Z][a-z]                      { BEGIN(studlycaps); P(tolower(yytext[0])); P(yytext[1]); }
<studlycaps>[A-Z]		{ P('_'); P(tolower(yytext[0])); }
<studlycaps>[a-z0-9_]		{ ECHO; }
<studlycaps>.                   { BEGIN(INITIAL); ECHO; }

%%

int main(void)
{
	return yylex();
}
