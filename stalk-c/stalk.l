%{
#include <string.h>
#include "stalk.h"
#include "parse.h"
#include "parse.tab.h"
#include "syntax.h"

#define SAVE_STRING yylval->sl_p_string = strdup(yytext);
#define LINE_COUNT  int i = 0; while(yytext[i] != '\0') { yylineno++; i += 1; }

int yylineno_extern = 1;
#define YY_USER_ACTION yylineno_extern = yylineno;
%}

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
  yylval->sl_p_float = atof(yytext);
  return SL_P_DECIMAL;
}
{integer} {
  yylval->sl_p_int = atoi(yytext);
  return SL_P_INTEGER;
}

{id}":"       { SAVE_STRING; return SL_P_KEYWORD; }
{id}          { SAVE_STRING; return SL_P_IDENT; }
":"{id}       { SAVE_STRING; return SL_P_SYMBOL; }
{comment}     { }
"\\([ \t\n]*" { LINE_COUNT; return SL_P_LPAREN; }
"[ \t\n]*\\)" { LINE_COUNT; return SL_P_RPAREN; }
"\\{[ \t\n]*" { LINE_COUNT; return SL_P_RBRACK; }
"[ \t\n]*\\}" { LINE_COUNT; return SL_P_LBRACK; }
"\\[[ \t\n]*" { LINE_COUNT; return SL_P_LSQ; }
"[ \t\n]*\\]" { LINE_COUNT; return SL_P_RSQ; }
"[ \t]+\\\\"{comment}?"\n[ \t]*" { LINE_COUNT; return SL_P_CONT; }
"[ \t]+"      { return SL_P_SWS; }
",[ \t\n]*"   { LINE_COUNT; return SL_P_COMMA; }
"[ \t]*\n[ \t\n]*" { LINE_COUNT; return SL_P_TERMINAL; }
"\\\"[^\\\"]*\\\"" { LINE_COUNT; SAVE_STRING; return SL_P_STRING; }
"="           { SAVE_STRING; return SL_P_ASSIGN; }
"[+\\-*/\\^]" { SAVE_STRING; return SL_P_OPERATOR; }

