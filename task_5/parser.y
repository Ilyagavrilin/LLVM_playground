%{
#include <iostream>
#include <map>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"

using namespace llvm;

// Logging
#define LOG_BISON
#if defined(LOG_BISON)
    #define LOG_BISON_MSG(fmt, ...) fprintf(stderr, "[BISON] " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_BISON_MSG(fmt, ...)
#endif

extern int yylex();
extern void yyerror(const char* s);

LLVMContext TheContext;
IRBuilder<> Builder(TheContext);
std::unique_ptr<Module> TheModule;

std::map<std::string, Value*> Variables;
std::map<std::string, BasicBlock*> Labels;

Function* SimPutPixelFunc;
Function* SimFlushFunc;
Function* SimDrawCircleFunc;
Function* SimDrawRectFunc;
%}

%define parse.error verbose

%union {
    long intval;           // For integer literals
    char* strval;          // For identifiers
    llvm::Value* value;    // Use fully-qualified 'llvm::Value' here
}

%token <intval> INT_LITERAL
%token <strval> IDENTIFIER
%token FUNC SCREEN PIXEL CIRCLE RECTANGLE FLUSH RAND VAR STREAM ASSIGN SEMICOLON COMMA LPAREN RPAREN LBRACE RBRACE
%token IF ELSE GOTO COLON MAIN

%type <value> Expression Simple Summand Factor Statements

%%

Program:
    MainFunction FunctionList {
        LOG_BISON_MSG("Program parsed successfully.");
        // Verify the module
        if (verifyModule(*TheModule, &errs())) {
            errs() << "[VERIFICATION] FAIL\n";
            exit(1);
        }
        // Save the resulting LLVM IR to a file
        std::error_code EC;
        raw_fd_ostream out("output.ll", EC, sys::fs::OF_Text);
        if (EC) {
            fprintf(stderr, "[ERROR] Could not open output file: %s\n", EC.message().c_str());
            exit(1);
        }
        TheModule->print(out, nullptr);
        out.flush();
    }
    | MainFunction {
        LOG_BISON_MSG("Program parsed with only main function.");
        // Verify the module
        if (verifyModule(*TheModule, &errs())) {
            errs() << "[VERIFICATION] FAIL\n";
            exit(1);
        }
        // Save the resulting LLVM IR to a file
        std::error_code EC;
        raw_fd_ostream out("output.ll", EC, sys::fs::OF_Text);
        if (EC) {
            fprintf(stderr, "[ERROR] Could not open output file: %s\n", EC.message().c_str());
            exit(1);
        }
        TheModule->print(out, nullptr);
        out.flush();
    }
    ;

MainFunction:
    FUNC MAIN LBRACE {
        // Create the main function and set the insertion point before parsing the statements
        FunctionType* FT = FunctionType::get(Builder.getVoidTy(), false);
        Function* MainFunction = Function::Create(FT, Function::ExternalLinkage, "main", TheModule.get());
        BasicBlock* Entry = BasicBlock::Create(TheContext, "entry", MainFunction);
        Builder.SetInsertPoint(Entry);
    } Statements RBRACE {
        // After statements have been generated, finish the main function
        Builder.CreateCall(SimFlushFunc); // auto flush
        Builder.CreateRetVoid();
    }
    ;

FunctionList:
    FunctionList Function
    | Function
    ;

Function:
    FUNC IDENTIFIER LBRACE Statements RBRACE {
        LOG_BISON_MSG("Defining function: %s", $2);
        FunctionType* FT = FunctionType::get(Builder.getVoidTy(), false);
        Function* Func = Function::Create(FT, Function::ExternalLinkage, $2, TheModule.get());
        BasicBlock* Entry = BasicBlock::Create(TheContext, "entry", Func);
        Builder.SetInsertPoint(Entry);
        // The statements body has been processed during parsing
        Builder.CreateRetVoid();
        free($2);
    }
    ;

Statements:
    /* empty */
    | Statements Statement
    ;

Statement:
    SCREEN STREAM PIXEL LPAREN Expression COMMA Expression COMMA Expression RPAREN SEMICOLON {
        // Draw a pixel using simPutPixel
        LOG_BISON_MSG("Drawing pixel at runtime");
        Value* Args[] = { $5, $7, $9 };
        Builder.CreateCall(SimPutPixelFunc, Args);
    }
    | SCREEN STREAM CIRCLE LPAREN Expression COMMA Expression COMMA Expression COMMA Expression RPAREN SEMICOLON {
        // Draw a circle using SimDrawCircle
        LOG_BISON_MSG("Drawing pixel at runtime");
        Value* Args[] = { $5, $7, $9, $11 };
        Builder.CreateCall(SimDrawCircleFunc, Args);
    }
    | SCREEN STREAM RECTANGLE LPAREN Expression COMMA Expression COMMA Expression COMMA Expression COMMA Expression RPAREN SEMICOLON {
        // Draw a rectangle using SimDrawRect
        LOG_BISON_MSG("Drawing pixel at runtime");
        Value* Args[] = { $5, $7, $9, $11, $13 };
        Builder.CreateCall(SimDrawRectFunc, Args);
    }
    | FLUSH SEMICOLON {
        // Flush the screen
        LOG_BISON_MSG("Flushing screen at runtime");
        Builder.CreateCall(SimFlushFunc);
    }
    | VAR IDENTIFIER ASSIGN Expression SEMICOLON {
        // Variable assignment
        LOG_BISON_MSG("Assigning variable '%s'", $2);
        if (Variables.find($2) == Variables.end()) {
            Function* F = Builder.GetInsertBlock()->getParent();
            IRBuilder<> TmpB(&F->getEntryBlock(), F->getEntryBlock().begin());
            Variables[$2] = TmpB.CreateAlloca(Builder.getInt32Ty(), nullptr, $2);
        }
        Builder.CreateStore($4, Variables[$2]);
        free($2);
    }
    | IF LPAREN Expression RPAREN LBRACE Statements RBRACE ELSE LBRACE Statements RBRACE {
        // If-else statement
        LOG_BISON_MSG("If-else statement");
        Value* Cond = Builder.CreateICmpNE($3, Builder.getInt32(0));
        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        BasicBlock* ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
        BasicBlock* ElseBB = BasicBlock::Create(TheContext, "else", TheFunction);
        BasicBlock* MergeBB = BasicBlock::Create(TheContext, "merge", TheFunction);

        Builder.CreateCondBr(Cond, ThenBB, ElseBB);

        Builder.SetInsertPoint(ThenBB);
        // 'then' statements processed
        Builder.CreateBr(MergeBB);

        Builder.SetInsertPoint(ElseBB);
        // 'else' statements processed
        Builder.CreateBr(MergeBB);

        Builder.SetInsertPoint(MergeBB);
    }
    | GOTO IDENTIFIER SEMICOLON {
        // Goto statement
        LOG_BISON_MSG("Goto: %s", $2);
        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        if (Labels.find($2) == Labels.end()) {
            Labels[$2] = BasicBlock::Create(TheContext, $2, TheFunction);
        }
        Builder.CreateBr(Labels[$2]);
        // Create a new block after the goto
        BasicBlock* NewBlock = BasicBlock::Create(TheContext, "after_goto", TheFunction);
        Builder.SetInsertPoint(NewBlock);
        free($2);
    }
    | IDENTIFIER COLON {
        // Label
        LOG_BISON_MSG("Label: %s", $1);
        Function* TheFunction = Builder.GetInsertBlock()->getParent();
        if (Labels.find($1) == Labels.end()) {
            Labels[$1] = BasicBlock::Create(TheContext, $1, TheFunction);
        }
        Builder.CreateBr(Labels[$1]);
        Builder.SetInsertPoint(Labels[$1]);
        free($1);
    }
    ;

Expression:
    Simple { $$ = $1; }
    | Expression '>' Simple { $$ = Builder.CreateZExt(Builder.CreateICmpSGT($1, $3), Builder.getInt32Ty()); }
    | Expression '<' Simple { $$ = Builder.CreateZExt(Builder.CreateICmpSLT($1, $3), Builder.getInt32Ty()); }
    ;

Simple:
    Summand { $$ = $1; }
    | Simple '+' Summand { $$ = Builder.CreateAdd($1, $3); }
    ;

Summand:
    Factor { $$ = $1; }
    | Summand '*' Factor { $$ = Builder.CreateMul($1, $3); }
    ;

Factor:
    INT_LITERAL { $$ = Builder.getInt32($1); }
    | RAND { $$ = Builder.getInt32(rand()); }
    | IDENTIFIER {
        // Load a variable
        if (Variables.find($1) == Variables.end()) {
            Function* F = Builder.GetInsertBlock()->getParent();
            IRBuilder<> TmpB(&F->getEntryBlock(), F->getEntryBlock().begin());
            Variables[$1] = TmpB.CreateAlloca(Builder.getInt32Ty(), nullptr, $1);
        }
        $$ = Builder.CreateLoad(Builder.getInt32Ty(), Variables[$1]);
        free($1);
    }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "[ERROR] %s\n", s);
}

int main() {
    srand(time(nullptr));
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    TheModule = std::make_unique<Module>("generated_program", TheContext);

    // Declare external functions
    FunctionType* PixelFuncType = FunctionType::get(Type::getVoidTy(TheContext),
                                                    { Type::getInt32Ty(TheContext),
                                                      Type::getInt32Ty(TheContext),
                                                      Type::getInt32Ty(TheContext) }, false);
    SimPutPixelFunc = Function::Create(PixelFuncType, Function::ExternalLinkage, "simPutPixel", TheModule.get());

    FunctionType* FlushFuncType = FunctionType::get(Type::getVoidTy(TheContext), false);
    
    SimFlushFunc = Function::Create(FlushFuncType, Function::ExternalLinkage, "simFlush", TheModule.get());
        // simDrawCircle(int x0, int y0, int radius, int argb)
    FunctionType* CircleFuncType = FunctionType::get(Type::getVoidTy(TheContext),
                                                    { Type::getInt32Ty(TheContext),
                                                      Type::getInt32Ty(TheContext),
                                                      Type::getInt32Ty(TheContext),
                                                      Type::getInt32Ty(TheContext) },
                                                      false);
    SimDrawCircleFunc = Function::Create(CircleFuncType, Function::ExternalLinkage, "simDrawCircle", TheModule.get());

    // simDrawRectangle(int x, int y, int width, int height, int argb)
    FunctionType* RectFuncType = FunctionType::get(Type::getVoidTy(TheContext),
                                                  { Type::getInt32Ty(TheContext),
                                                    Type::getInt32Ty(TheContext),
                                                    Type::getInt32Ty(TheContext),
                                                    Type::getInt32Ty(TheContext),
                                                    Type::getInt32Ty(TheContext) },
                                                    false);
    SimDrawRectFunc = Function::Create(RectFuncType, Function::ExternalLinkage, "simDrawRectangle", TheModule.get());

    yyparse();
    return 0;
}