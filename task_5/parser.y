%{
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

extern int yylex();
extern void yyerror(const char* s);

LLVMContext context;
IRBuilder<> *builder;
Module *module;

std::map<std::string, Value*> variables;

void log(const char* msg) {
    std::cout << "[LOG] " << msg << "\n";
}

#define YYDEBUG 1  // Enable Bison debugging
%}

%union {
    int intval;
    char* strval;
}

%token <intval> INT_LITERAL
%token <strval> IDENTIFIER
%token FUNC MAIN SCREEN PIXEL CIRCLE RECTANGLE FLUSH RAND CONST VAR STREAM ASSIGN SEMICOLON COMMA LPAREN RPAREN LBRACE RBRACE

%type <intval> Argument

%%

Program:
    FunctionList { log("Program parsed successfully."); }
    ;

FunctionList:
    FunctionList Function
    | Function
    ;

Function:
    FUNC MAIN LBRACE Statements RBRACE {
        log("Main function parsed.");
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
    | FlushStatement
    ;

VariableDeclaration:
    VAR IDENTIFIER ASSIGN INT_LITERAL SEMICOLON {
        log(("Variable declared: " + std::string($2) + " = " + std::to_string($4)).c_str());
    }
    ;

Assignment:
    IDENTIFIER ASSIGN INT_LITERAL SEMICOLON {
        log(("Variable assigned: " + std::string($1) + " = " + std::to_string($3)).c_str());
    }
    ;

GraphicsStatement:
    SCREEN STREAM GraphicsObject SEMICOLON {
        log("Graphics statement parsed.");
    }
    ;

GraphicsObject:
    PIXEL LPAREN Argument COMMA Argument COMMA Argument RPAREN {
        log("Pixel drawn.");
    }
    | CIRCLE LPAREN Argument COMMA Argument COMMA Argument COMMA Argument RPAREN {
        log("Circle drawn.");
    }
    | RECTANGLE LPAREN Argument COMMA Argument COMMA Argument COMMA Argument COMMA Argument RPAREN {
        log("Rectangle drawn.");
    }
    ;

Argument:
    INT_LITERAL { $$ = $1; }
    | IDENTIFIER { log(("Argument identifier: " + std::string($1)).c_str()); $$ = 0; /* Placeholder */ }
    ;

FlushStatement:
    FLUSH SEMICOLON {
        log("Screen flushed.");
    }

%%

int main(int argc, char** argv) {
    module = new Module("MiniGraphLang", context);
    builder = new IRBuilder<>(context);

    log("Parsing started.");
    yydebug = 1;  // Enable debug output
    yyparse();
    module->print(outs(), nullptr);

    log("Parsing completed.");
    return 0;
}

void yyerror(const char* s) {
    std::cerr << "Error: " << s << "\n";
}
