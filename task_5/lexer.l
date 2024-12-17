%{
#include "parser.tab.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

#define YYSTYPE_IS_DECLARED 1
#define YYSTYPE union YYSTYPE
extern YYSTYPE yylval;

extern int yylex();
extern void yyerror(const char*);

void print_token(const char* name);  // Helper for verbose output
%}

%option noyywrap
%option yylineno


%%

"func"          { print_token("FUNC"); return FUNC; }
"screen"        { print_token("SCREEN"); return SCREEN; }
"pixel"         { print_token("PIXEL"); return PIXEL; }
"circle"        { print_token("CIRCLE"); return CIRCLE; }
"rectangle"     { print_token("RECTANGLE"); return RECTANGLE; }
"flush"         { print_token("FLUSH"); return FLUSH; }
"rand"          { print_token("RAND"); return RAND; }
"const"         { print_token("CONST"); return CONST; }
"var"           { print_token("VAR"); return VAR; }
"main"          { print_token("MAIN"); return MAIN; }

"<<"            { print_token("STREAM"); return STREAM; }
"="             { print_token("ASSIGN"); return ASSIGN; }
";"             { print_token("SEMICOLON"); return SEMICOLON; }
","             { print_token("COMMA"); return COMMA; }
"("             { print_token("LPAREN"); return LPAREN; }
")"             { print_token("RPAREN"); return RPAREN; }
"{"             { print_token("LBRACE"); return LBRACE; }
"}"             { print_token("RBRACE"); return RBRACE; }

0[xX][0-9a-fA-F]+ { 
    yylval.intval = (int)strtoll(yytext, NULL, 16); 
    print_token("HEX_LITERAL");
    return INT_LITERAL;
}

[0-9]+          { 
    yylval.intval = atoi(yytext); 
    print_token("INT_LITERAL");
    return INT_LITERAL; 
}

[A-Za-z_][A-Za-z0-9_]* { 
    yylval.strval = strdup(yytext); 
    print_token("IDENTIFIER");
    return IDENTIFIER; 
}

[ \t\r\n]+      ;  // Ignore whitespace
[/][/].*        ;  // Ignore comments
.               { print_token("UNKNOWN"); return *yytext; }

%%

// Helper function for verbose Lex output
void print_token(const char* name) {
    printf("[LEX] Matched token: %s\n", name);
}
