%{
#include "llvm/IR/Value.h"
#include "parser.tab.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define YYSTYPE_IS_DECLARED 1
#define YYSTYPE union YYSTYPE
extern YYSTYPE yylval;

extern int yylex();
extern void yyerror(const char*);
#define LOG_LEXER
#if defined(LOG_LEXER)
#define LOG_LEX(fmt, ...) printf("[LEXER] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_LEX(fmt, ...)
#endif
%}

%option noyywrap
%option yylineno

%%

"func"          { LOG_LEX("Matched FUNC: %s", yytext); return FUNC; }
"screen"        { LOG_LEX("Matched SCREEN: %s", yytext); return SCREEN; }
"pixel"         { LOG_LEX("Matched PIXEL: %s", yytext); return PIXEL; }
"circle"        { LOG_LEX("Matched CIRCLE: %s", yytext); return CIRCLE; }
"rectangle"     { LOG_LEX("Matched RECTANGLE: %s", yytext); return RECTANGLE; }
"flush"         { LOG_LEX("Matched FLUSH: %s", yytext); return FLUSH; }
"rand"          { LOG_LEX("Matched RAND: %s", yytext); return RAND; }
"var"           { LOG_LEX("Matched VAR: %s", yytext); return VAR; }
"if"            { LOG_LEX("Matched IF: %s", yytext); return IF; }
"else"          { LOG_LEX("Matched ELSE: %s", yytext); return ELSE; }
"goto"          { LOG_LEX("Matched GOTO: %s", yytext); return GOTO; }
"main"          { LOG_LEX("Matched MAIN: %s", yytext); return MAIN;}

":"             { LOG_LEX("Matched COLON: %s", yytext); return COLON; }
"<<"            { LOG_LEX("Matched STREAM: %s", yytext); return STREAM; }
"="             { LOG_LEX("Matched ASSIGN: %s", yytext); return ASSIGN; }
";"             { LOG_LEX("Matched SEMICOLON: %s", yytext); return SEMICOLON; }
","             { LOG_LEX("Matched COMMA: %s", yytext); return COMMA; }
"("             { LOG_LEX("Matched LPAREN: %s", yytext); return LPAREN; }
")"             { LOG_LEX("Matched RPAREN: %s", yytext); return RPAREN; }
"{"             { LOG_LEX("Matched LBRACE: %s", yytext); return LBRACE; }
"}"             { LOG_LEX("Matched RBRACE: %s", yytext); return RBRACE; }

0[xX][0-9a-fA-F]+ { 
    yylval.intval = strtoll(yytext, NULL, 16); 
    LOG_LEX("Matched HEX_LITERAL: %s", yytext);
    return INT_LITERAL;
}
[0-9]+          { yylval.intval = strtol(yytext, NULL, 10); LOG_LEX("Matched INT_LITERAL: %s", yytext); return INT_LITERAL; }
[A-Za-z_][A-Za-z0-9_]* { yylval.strval = strdup(yytext); LOG_LEX("Matched IDENTIFIER: %s", yytext); return IDENTIFIER; }
[ \t\r\n]+      ;  // Ignore whitespace
[/][/].*        ;  // Ignore comments
.               { LOG_LEX("Matched UNKNOWN: %s", yytext); return *yytext; }

%%