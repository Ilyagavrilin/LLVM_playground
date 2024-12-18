%{
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>

#if defined(LOG_BISON) || defined(LOG_BISON_FULL)
    #define LOG_BISON_MSG(fmt, ...) printf("[BISON] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_BISON_MSG(fmt, ...)
#endif

#ifdef LOG_BISON_FULL
    #define LOG_BISON_FULL_MSG(fmt, ...) printf("[BISON_FULL] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_BISON_FULL_MSG(fmt, ...)
#endif

extern int yylex();
extern void yyerror(const char* s);

std::map<std::string, long> variables;
std::map<std::string, bool> functions; // Tracks declared functions
%}
%define parse.lac full
%define parse.error verbose

%union {
    long intval;
    char* strval;
}

%token <intval> INT_LITERAL
%token <strval> IDENTIFIER
%token FUNC SCREEN PIXEL CIRCLE RECTANGLE FLUSH RAND CONST VAR STREAM ASSIGN SEMICOLON COMMA LPAREN RPAREN LBRACE RBRACE
%token IF ELSE GOTO COLON

%type <intval> Argument
%type <intval> GraphicsObject

%%

Program:
    FunctionList { LOG_BISON_MSG("Program parsed successfully.\n"); }
    ;

FunctionList:
    FunctionList Function
    | Function
    ;

Function:
    FUNC IDENTIFIER LBRACE Statements RBRACE {
        if (functions.find($2) != functions.end()) {
            yyerror(("Function redefined: " + std::string($2)).c_str());
        } else {
            functions[$2] = true;
            LOG_BISON_MSG("Function defined: %s\n", $2);
        }
    }
    ;

Statements:
    Statements Statement
    | Statement
    ;

Statement:
    VariableDeclaration
    | Assignment
    | GraphicsStatement
    | ConditionalStatement
    | GoToStatement
    | LabelStatement
    | FunctionCall
    | FlushStatement
    ;

VariableDeclaration:
    VAR IDENTIFIER ASSIGN Argument SEMICOLON {
        variables[$2] = $4;
        LOG_BISON_MSG("Variable declared: %s = %ld\n", $2, $4);
    }
    ;

Assignment:
    IDENTIFIER ASSIGN Argument SEMICOLON {
        variables[$1] = $3;
        LOG_BISON_MSG("Variable assigned: %s = %ld\n", $1, $3);
    }
    ;

GraphicsStatement:
    SCREEN STREAM GraphicsObject SEMICOLON {
        LOG_BISON_FULL_MSG("Graphics statement parsed.\n");
    }
    ;

GraphicsObject:
    PIXEL LPAREN Argument COMMA Argument COMMA Argument RPAREN {
        LOG_BISON_FULL_MSG("Pixel drawn with (%ld, %ld) and color %ld.\n", $3, $5, $7);
    }
    | CIRCLE LPAREN Argument COMMA Argument COMMA Argument COMMA Argument RPAREN {
        LOG_BISON_FULL_MSG("Circle drawn at (%ld, %ld) with radius %ld and color %ld.\n", $3, $5, $7, $9);
    }
    | RECTANGLE LPAREN Argument COMMA Argument COMMA Argument COMMA Argument COMMA Argument RPAREN {
        LOG_BISON_FULL_MSG("Rectangle drawn at (%ld, %ld) with size (%ld, %ld) and color %ld.\n", $3, $5, $7, $9, $11);
    }
    ;

ConditionalStatement:
    IF LPAREN Argument RPAREN LBRACE Statements RBRACE {
        LOG_BISON_MSG("If statement evaluated.\n");
    }
    | IF LPAREN Argument RPAREN LBRACE Statements RBRACE ELSE LBRACE Statements RBRACE {
        LOG_BISON_MSG("If-Else statement evaluated.\n");
    }
    ;

GoToStatement:
    GOTO IDENTIFIER SEMICOLON {
        LOG_BISON_MSG("Goto statement: Jump to label %s\n", $2);
    }
    ;

LabelStatement:
    IDENTIFIER COLON {
        LOG_BISON_MSG("Label defined: %s\n", $1);
    }
    ;

FunctionCall:
    IDENTIFIER LPAREN RPAREN SEMICOLON {
        if (functions.find($1) == functions.end()) {
            yyerror(("Function not declared: " + std::string($1)).c_str());
        } else {
            LOG_BISON_MSG("Function called: %s\n", $1);
        }
    }
    ;

FlushStatement:
    FLUSH SEMICOLON {
        LOG_BISON_FULL_MSG("Screen flushed.\n");
    }

Argument:
    INT_LITERAL {
        $$ = $1;
        LOG_BISON_FULL_MSG("Argument: INT_LITERAL = %ld\n", $1);
    }
    | IDENTIFIER {
        if (variables.find($1) != variables.end()) {
            $$ = variables[$1];
            LOG_BISON_FULL_MSG("Argument: IDENTIFIER %s = %ld\n", $1, $$);
        } else {
            yyerror(("Undeclared variable: " + std::string($1)).c_str());
            $$ = 0;
        }
    }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "[BISON_ERROR] %s\n", s);
}

int main(int argc, char** argv) {
    printf("[INFO] Parsing started.\n");
    yyparse();
    printf("[INFO] Parsing completed.\n");
    return 0;
}