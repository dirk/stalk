%{
#include <string.h>
#include "stalk.h"
#include "parse.h"
#include "parse.tab.h"
#include "syntax.h"

#define SAVE_STRING yylval->p_string = strdup(yytext);

int yylineno_extern = 1;
#define YY_USER_ACTION yylineno_extern = yylineno;
%}

%option header-file="stalk.yy.h"
%option nounput
%option yylineno
%option reentrant noyywrap
%option bison-bridge

integer     -?0|([1-9][0-9]*)
fractional  \.[0-9]+
id          [A-Za-z][A-Za-z0-9_]*
comment     [ \t]*#[^\n]*

%%
{integer}{fractional} {
  yylval->p_float = atof(yytext);
  return SL_P_DECIMAL;
}
{integer} {
  yylval->p_int = atoi(yytext);
  return SL_P_INTEGER;
}

^[ \t\n]       { }
"def:"         {              return SL_P_DEF; }
{id}":"        { SAVE_STRING; return SL_P_KEYWORD; }
{id}           { SAVE_STRING; return SL_P_IDENT; }
":"{id}        { SAVE_STRING; return SL_P_SYMBOL; }
{comment}      { SAVE_STRING; return SL_P_COMMENT; }
[ \t\n]*")"    {              return SL_P_RPAREN; }
[ \t\n]*"}"    {              return SL_P_RBRACK; }
[ \t\n]*"]"    {              return SL_P_RSQ; }
"("[ \t\n]*({comment}"\n")* { return SL_P_LPAREN; }
"{"[ \t\n]*({comment}"\n")* { return SL_P_LBRACK; }
"["[ \t\n]*({comment}"\n")* { return SL_P_LSQ; }
","[ \t\n]*({comment}"\n")* { return SL_P_COMMA; }
"\""[^\"]*"\"" {              SAVE_STRING; return SL_P_STRING; }
[+\-*/\^]      { SAVE_STRING; return SL_P_OPERATOR; }
"="            {              return SL_P_ASSIGN; }
[ \t]+("\\"{comment}?"\n"[ \n\t]*)? { return SL_P_SEP; }
[ \t]*[\n;][ \t\n]* {         return SL_P_TERMINAL; }
