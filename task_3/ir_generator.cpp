// ir_generator.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

// LLVM headers
#include <llvm/ADT/StringRef.h>
#include <llvm/AsmParser/Parser.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/AssemblyAnnotationWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>

using namespace llvm;

// Helper function to mangle function names
std::string MangleName(const std::string &Name) {
    std::string MangledName = "func_" + Name;
    std::replace(MangledName.begin(), MangledName.end(), '.', '_');
    std::replace(MangledName.begin(), MangledName.end(), '@', '_');
    return MangledName;
}

// Helper function to mangle variable names
std::string MangleVariable(const std::string &Name) {
    std::string MangledName = "var_" + Name;
    for (char &c : MangledName) {
        if (!isalnum(c) && c != '_') {
            c = '_';
        }
    }
    return MangledName;
}

// Helper function to get LLVM type as a string
std::string GetLLVMTypeAsString(Type *Ty, LLVMContext &Context) {
    std::string TypeStr;
    raw_string_ostream RSO(TypeStr);
    Ty->print(RSO);
    return TypeStr;
}

// Helper function to get the LLVM type
std::string GetLLVMType(Type *Ty, LLVMContext &Context) {
    if (Ty->isIntegerTy()) {
        unsigned BitWidth = Ty->getIntegerBitWidth();
        return "Type::getInt" + std::to_string(BitWidth) + "Ty(Context)";
    } else if (Ty->isFloatTy()) {
        return "Type::getFloatTy(Context)";
    } else if (Ty->isDoubleTy()) {
        return "Type::getDoubleTy(Context)";
    } else if (Ty->isVoidTy()) {
        return "Type::getVoidTy(Context)";
    } else if (Ty->isPointerTy()) {
        Type *ElementType = Ty->getArrayElementType();
        return "PointerType::getUnqual(" + GetLLVMType(ElementType, Context) + ")";
    } else {
        return "Type::getVoidTy(Context)"; // Default to void type
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./ir_generator <input.ll>\n";
        return 1;
    }

    LLVMContext Context;
    SMDiagnostic Err;
    std::string InputFile = argv[1];

    // Parse the IR file
    std::unique_ptr<Module> SourceModule = parseIRFile(InputFile, Err, Context);
    if (!SourceModule) {
        Err.print("IR Generator", errs());
        return 1;
    }

    // Output file for generated C++ code
    std::ofstream OutFile("generated_code.cpp");
    if (!OutFile.is_open()) {
        std::cerr << "Failed to open output file for generated code.\n";
        return 1;
    }

    // Write includes and using statements
    OutFile << "// Generated C++ code using LLVM C++ API\n";
    OutFile << "#include <llvm/IR/IRBuilder.h>\n";
    OutFile << "#include <llvm/IR/LLVMContext.h>\n";
    OutFile << "#include <llvm/IR/Module.h>\n";
    OutFile << "#include <llvm/IR/Verifier.h>\n";
    OutFile << "#include <llvm/Support/TargetSelect.h>\n";
    OutFile << "#include <llvm/ExecutionEngine/ExecutionEngine.h>\n";
    OutFile << "#include <llvm/ExecutionEngine/MCJIT.h>\n";
    OutFile << "#include <llvm/ExecutionEngine/GenericValue.h>\n";
    OutFile << "#include <iostream>\n";
    OutFile << "using namespace llvm;\n\n";

    // Start main function
    OutFile << "int main() {\n";
    OutFile << "    LLVMContext Context;\n";
    OutFile << "    Module *ModulePtr = new Module(\"GeneratedModule\", Context);\n";
    OutFile << "    IRBuilder<> builder(Context);\n\n";

    // Initialize JIT
    OutFile << "    InitializeNativeTarget();\n";
    OutFile << "    InitializeNativeTargetAsmPrinter();\n";
    OutFile << "    InitializeNativeTargetAsmParser();\n\n";

    // Maps to keep track of values and functions
    std::unordered_map<Value *, std::string> ValueMap;
    std::unordered_map<BasicBlock *, std::string> BlockMap;
    std::unordered_map<std::string, std::string> FunctionMap;

    // Function declarations
    OutFile << "    // Declare functions\n";
    for (Function &Func : SourceModule->functions()) {
        std::string FuncName = Func.getName().str();
        std::string MangledFuncName = MangleName(FuncName);
        FunctionMap[FuncName] = MangledFuncName;

        // Get function return type
        std::string ReturnType = GetLLVMType(Func.getReturnType(), Context);

        // Get function argument types
        std::vector<std::string> ArgTypes;
        for (Argument &Arg : Func.args()) {
            ArgTypes.push_back(GetLLVMType(Arg.getType(), Context));
        }

        // Generate function type
        OutFile << "    std::vector<Type*> " << MangledFuncName << "_args;\n";
        for (const std::string &ArgType : ArgTypes) {
            OutFile << "    " << MangledFuncName << "_args.push_back(" << ArgType << ");\n";
        }
        OutFile << "    FunctionType *" << MangledFuncName << "_type = FunctionType::get(" << ReturnType << ", " << MangledFuncName << "_args, false);\n";
        OutFile << "    Function *" << MangledFuncName << " = Function::Create(" << MangledFuncName << "_type, Function::ExternalLinkage, \"" << FuncName << "\", ModulePtr);\n\n";
    }

    // Function definitions
    for (Function &Func : SourceModule->functions()) {
        if (Func.isDeclaration()) {
            continue; // Skip declarations
        }

        std::string FuncName = Func.getName().str();
        std::string MangledFuncName = FunctionMap[FuncName];
        OutFile << "    // Define function: " << FuncName << "\n";

        // Map function arguments
        unsigned ArgIndex = 0;
        for (Argument &Arg : Func.args()) {
            std::string ArgName = Arg.getName().str();
            if (ArgName.empty()) {
                ArgName = "arg" + std::to_string(ArgIndex);
            }
            std::string MangledArgName = MangleVariable(FuncName + "_" + ArgName);
            OutFile << "    Value *" << MangledArgName << " = " << MangledFuncName << "->getArg(" << ArgIndex << ");\n";
            ValueMap[&Arg] = MangledArgName;
            ArgIndex++;
        }

        // Create basic blocks
        for (BasicBlock &BB : Func) {
            std::string BBName = BB.getName().str();
            if (BBName.empty()) {
                BBName = "bb_" + std::to_string(reinterpret_cast<uintptr_t>(&BB));
            }
            std::string MangledBBName = MangleVariable(FuncName + "_" + BBName);
            BlockMap[&BB] = MangledBBName;
            OutFile << "    BasicBlock *" << MangledBBName << " = BasicBlock::Create(Context, \"" << BBName << "\", " << MangledFuncName << ");\n";
        }

        // Instruction handling
        for (BasicBlock &BB : Func) {
            std::string MangledBBName = BlockMap[&BB];
            OutFile << "    // Basic block: " << BB.getName().str() << "\n";
            OutFile << "    builder.SetInsertPoint(" << MangledBBName << ");\n";

            for (Instruction &Inst : BB) {
                // Get original IR instruction
                std::string IRString;
                {
                    std::string str;
                    raw_string_ostream rso(str);
                    Inst.print(rso);
                    IRString = rso.str();
                }
                // Add as comment
                OutFile << "    // " << IRString << "\n";

                if (AllocaInst *AI = dyn_cast<AllocaInst>(&Inst)) {
                    std::string VarName = MangleVariable(FuncName + "_" + AI->getName().str());
                    std::string AllocaType = GetLLVMType(AI->getAllocatedType(), Context);
                    OutFile << "    AllocaInst *" << VarName << " = builder.CreateAlloca(" << AllocaType << ");\n";
                    ValueMap[AI] = VarName;
                }
                else if (StoreInst *SI = dyn_cast<StoreInst>(&Inst)) {
                    Value *Val = SI->getValueOperand();
                    Value *Ptr = SI->getPointerOperand();
                    std::string ValName;
                    if (ConstantInt *CI = dyn_cast<ConstantInt>(Val)) {
                        std::string ConstType = GetLLVMType(CI->getType(), Context);
                        ValName = "ConstantInt::get(" + ConstType + ", " + std::to_string(CI->getSExtValue()) + ")";
                    } else {
                        ValName = ValueMap[Val];
                    }
                    std::string PtrName = ValueMap[Ptr];
                    OutFile << "    builder.CreateStore(" << ValName << ", " << PtrName << ");\n";
                }
                else if (LoadInst *LI = dyn_cast<LoadInst>(&Inst)) {
                    Value *Ptr = LI->getPointerOperand();
                    std::string PtrName = ValueMap[Ptr];
                    std::string VarName = MangleVariable(FuncName + "_" + LI->getName().str());
                    OutFile << "    Value *" << VarName << " = builder.CreateLoad(" << PtrName << ");\n";
                    ValueMap[LI] = VarName;
                }
                else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&Inst)) {
                    std::string OpName = BO->getOpcodeName();
                    Value *Op1 = BO->getOperand(0);
                    Value *Op2 = BO->getOperand(1);
                    std::string Op1Name = ValueMap[Op1];
                    std::string Op2Name = ValueMap[Op2];
                    std::string VarName = MangleVariable(FuncName + "_" + BO->getName().str());
                    OutFile << "    Value *" << VarName << " = builder.Create" << OpName << "(" << Op1Name << ", " << Op2Name << ");\n";
                    ValueMap[BO] = VarName;
                }
                else if (ICmpInst *ICmp = dyn_cast<ICmpInst>(&Inst)) {
                    Value *Op1 = ICmp->getOperand(0);
                    Value *Op2 = ICmp->getOperand(1);
                    std::string Op1Name = ValueMap[Op1];
                    std::string Op2Name = ValueMap[Op2];
                    std::string VarName = MangleVariable(FuncName + "_" + ICmp->getName().str());
                    std::string PredicateStr = ICmpInst::getPredicateName(ICmp->getPredicate()).str();
                    OutFile << "    Value *" << VarName << " = builder.CreateICmp(" << PredicateStr << ", " << Op1Name << ", " << Op2Name << ");\n";
                    ValueMap[ICmp] = VarName;
                }
                else if (BranchInst *BI = dyn_cast<BranchInst>(&Inst)) {
                    if (BI->isUnconditional()) {
                        BasicBlock *Dest = BI->getSuccessor(0);
                        std::string DestName = BlockMap[Dest];
                        OutFile << "    builder.CreateBr(" << DestName << ");\n";
                    } else {
                        Value *Cond = BI->getCondition();
                        BasicBlock *TrueDest = BI->getSuccessor(0);
                        BasicBlock *FalseDest = BI->getSuccessor(1);
                        std::string CondName = ValueMap[Cond];
                        std::string TrueDestName = BlockMap[TrueDest];
                        std::string FalseDestName = BlockMap[FalseDest];
                        OutFile << "    builder.CreateCondBr(" << CondName << ", " << TrueDestName << ", " << FalseDestName << ");\n";
                    }
                }
                else if (ReturnInst *RI = dyn_cast<ReturnInst>(&Inst)) {
                    if (RI->getNumOperands() == 0) {
                        OutFile << "    builder.CreateRetVoid();\n";
                    } else {
                        Value *RetVal = RI->getReturnValue();
                        std::string RetValName;
                        if (ConstantInt *CI = dyn_cast<ConstantInt>(RetVal)) {
                            std::string ConstType = GetLLVMType(CI->getType(), Context);
                            RetValName = "ConstantInt::get(" + ConstType + ", " + std::to_string(CI->getSExtValue()) + ")";
                        } else {
                            RetValName = ValueMap[RetVal];
                        }
                        OutFile << "    builder.CreateRet(" << RetValName << ");\n";
                    }
                }
                else if (PHINode *PN = dyn_cast<PHINode>(&Inst)) {
                    Type *PhiType = PN->getType();
                    std::string PhiTypeStr = GetLLVMType(PhiType, Context);
                    std::string VarName = MangleVariable(FuncName + "_" + PN->getName().str());
                    OutFile << "    PHINode *" << VarName << " = builder.CreatePHI(" << PhiTypeStr << ", " << PN->getNumIncomingValues() << ");\n";
                    ValueMap[PN] = VarName;

                    for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
                        Value *IncomingVal = PN->getIncomingValue(i);
                        BasicBlock *IncomingBB = PN->getIncomingBlock(i);
                        std::string IncomingValName = ValueMap[IncomingVal];
                        std::string IncomingBBName = BlockMap[IncomingBB];
                        OutFile << "    " << VarName << "->addIncoming(" << IncomingValName << ", " << IncomingBBName << ");\n";
                    }
                }
                else if (SelectInst *SI = dyn_cast<SelectInst>(&Inst)) {
                    Value *Cond = SI->getCondition();
                    Value *TrueVal = SI->getTrueValue();
                    Value *FalseVal = SI->getFalseValue();
                    std::string CondName = ValueMap[Cond];
                    std::string TrueValName = ValueMap[TrueVal];
                    std::string FalseValName = ValueMap[FalseVal];
                    std::string VarName = MangleVariable(FuncName + "_" + SI->getName().str());
                    OutFile << "    Value *" << VarName << " = builder.CreateSelect(" << CondName << ", " << TrueValName << ", " << FalseValName << ");\n";
                    ValueMap[SI] = VarName;
                }
                else {
                    OutFile << "    // Unhandled instruction\n";
                }
            }
        }
    }

    // Verify module
    OutFile << "\n    verifyModule(*ModulePtr, &errs());\n\n";

    // Create execution engine
    OutFile << "    std::string ErrStr;\n";
    OutFile << "    ExecutionEngine *EE = EngineBuilder(std::unique_ptr<Module>(ModulePtr)).setErrorStr(&ErrStr).create();\n";
    OutFile << "    if (!EE) {\n";
    OutFile << "        errs() << \"Failed to construct ExecutionEngine: \" << ErrStr << \"\\n\";\n";
    OutFile << "        return 1;\n";
    OutFile << "    }\n\n";

    // Execute 'main' function if it exists
    OutFile << "    Function *MainFunc = ModulePtr->getFunction(\"main\");\n";
    OutFile << "    if (MainFunc) {\n";
    OutFile << "        std::vector<GenericValue> Args;\n";
    OutFile << "        GenericValue Result = EE->runFunction(MainFunc, Args);\n";
    OutFile << "        std::cout << \"Program exited with code: \" << Result.IntVal << std::endl;\n";
    OutFile << "    }\n\n";

    // Shutdown
    OutFile << "    delete EE;\n";
    OutFile << "    llvm_shutdown();\n";
    OutFile << "    return 0;\n";
    OutFile << "}\n";

    OutFile.close();

    std::cout << "Generated code has been written to 'generated_code.cpp'.\n";
    std::cout << "You can compile it using the provided instructions.\n";

    return 0;
}