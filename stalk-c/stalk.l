%{
#include <string.h>
#include "stalk.h"
#include "parse.tab.h"
#include "syntax.h"

#define SAVE_STRING yylval->p_string = strdup(yytext);

int yylineno_extern = 1;
#define YY_USER_ACTION yylineno_extern = yylineno;
%}

%option header-file="stalk.yy.h"
%option yylineno
%option noyywrap
%option reentrant
%option nounput
%option warn
%option never-interactive
%option bison-bridge

integer     -?0|([1-9][0-9]*)
fractional  \.[0-9]+
id          [A-Za-z][A-Za-z0-9_]*
comment     [ \t]*#[^\n]*

%%
{comment}      { }
^[ \t]*        { }

{integer}{fractional} {
  yylval->p_float = atof(yytext);
  return SL_T_DECIMAL;
}
{integer} {
  yylval->p_int = atoi(yytext);
  return SL_T_INTEGER;
}

"def:"         {              return SL_T_DEF; }
{id}":"        { SAVE_STRING; return SL_T_KEYWORD; }
{id}           { SAVE_STRING; return SL_T_IDENT; }
":"{id}        { SAVE_STRING; return SL_T_SYMBOL; }
[ \t\n]*")"    {              return SL_T_RPAREN; }
[ \t\n]*"}"    {              return SL_T_RBRACK; }
[ \t\n]*"]"    {              return SL_T_RSQ; }
"("[ \t\n]*    {              return SL_T_LPAREN; }
"{"[ \t\n]*    {              return SL_T_LBRACK; }
"["[ \t\n]*    {              return SL_T_LSQ; }
","[ \t\n]*    {              return SL_T_COMMA; }
"\""[^\"]*"\"" { SAVE_STRING; return SL_T_STRING; }
[+\-*/\^%&><]  { SAVE_STRING; return SL_T_OPERATOR; }
"="            {              return SL_T_ASSIGN; }
[ \t]+"\\\n"   {              return SL_T_SEP;  }
[ \t]*"\n"+    {              return SL_T_TERMINAL; }
[ \t]+         {              return SL_T_SEP;  }



