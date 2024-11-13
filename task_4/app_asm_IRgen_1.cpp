#include "../task_1/sim.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

const int REG_FILE_SIZE = 16;
const int REG_FP_INDEX = REG_FILE_SIZE;     // Index for FP
const int REG_SP_INDEX = REG_FILE_SIZE + 1; // Index for SP
const int TOTAL_REG_SIZE = REG_FILE_SIZE + 2; // Total size including FP and SP

uint32_t REG_FILE[TOTAL_REG_SIZE]; // Include space for FP and SP
const int STACK_SIZE = 1024;
uint32_t STACK[STACK_SIZE];

// Declare assembler instruction functions

void do_MOV(int dst, int src) {
    if (dst == REG_SP_INDEX) {
        if (src == REG_SP_INDEX) {
            REG_FILE[REG_SP_INDEX] = REG_FILE[REG_SP_INDEX];
        } else {
            REG_FILE[REG_SP_INDEX] = REG_FILE[src];
        }
    } else if (dst == REG_FP_INDEX) {
        REG_FILE[REG_FP_INDEX] = REG_FILE[src];
    } else {
        if (src == REG_SP_INDEX) {
            REG_FILE[dst] = REG_FILE[REG_SP_INDEX];
        } else if (src == REG_FP_INDEX) {
            REG_FILE[dst] = REG_FILE[REG_FP_INDEX];
        } else {
            REG_FILE[dst] = REG_FILE[src];
        }
    }
}

void do_MOV_IMM(int dst, int imm) {
    if (dst == REG_SP_INDEX) {
        REG_FILE[REG_SP_INDEX] = imm;
    } else if (dst == REG_FP_INDEX) {
        REG_FILE[REG_FP_INDEX] = imm;
    } else {
        REG_FILE[dst] = imm;
    }
}
void do_MOD(int dst, int src, int mod) { REG_FILE[dst] = REG_FILE[src] % mod; }
void do_ADD(int dst, int src1, int src2) { REG_FILE[dst] = REG_FILE[src1] + REG_FILE[src2]; }
void do_ADD_IMM(int dst, int src, int imm) { REG_FILE[dst] = REG_FILE[src] + imm; }
void do_SUB(int dst, int src1, int src2) { REG_FILE[dst] = REG_FILE[src1] - REG_FILE[src2]; }
void do_CMP(int result, int reg1, int reg2) {
    REG_FILE[result] = (REG_FILE[reg1] >= REG_FILE[reg2]) ? 0 : 1;
}
void do_PUSH(int reg) {
    REG_FILE[REG_SP_INDEX]--;
    if (REG_FILE[REG_SP_INDEX] >= 0) {
        STACK[REG_FILE[REG_SP_INDEX]] = REG_FILE[reg];
    } else {
        std::cerr << "[ERROR] Stack overflow\n";
        exit(1);
    }
}
void do_POP(int reg) {
    if (REG_FILE[REG_SP_INDEX] < STACK_SIZE) {
        REG_FILE[reg] = STACK[REG_FILE[REG_SP_INDEX]];
        REG_FILE[REG_SP_INDEX]++;
    } else {
        std::cerr << "[ERROR] Stack underflow\n";
        exit(1);
    }
}
void do_SIM_PUT_PIXEL(int x, int y, int color) {
    simPutPixel(REG_FILE[x], REG_FILE[y], REG_FILE[color]);
}
void do_SIM_RAND(int reg) { REG_FILE[reg] = rand() % 1000; }
void do_SIM_FLUSH() { simFlush(); }

// Parsing function that loads instructions with comments and labels handling
std::vector<std::string> loadInstructions(
    const std::string &filename,
    std::unordered_map<std::string, int> &labelMap) {
    std::vector<std::string> instructions;
    std::ifstream input(filename);
    std::string line;

    if (!input.is_open()) {
        std::cerr << "[ERROR] Can't open file " << filename << "\n";
        exit(1);
    }

    while (getline(input, line)) {
        std::string cleanedLine;
        std::istringstream iss(line);
        bool isLabelLine = false;
        std::string labelName;

        while (iss) {
            std::string token;
            iss >> token;
            if (token.empty()) continue;

            // Skip comments starting with ';'
            if (token[0] == ';') break;

            // Remove inline comments
            size_t commentPos = token.find(';');
            if (commentPos != std::string::npos) {
                token = token.substr(0, commentPos);
                if (token.empty()) break;
            }

            // Store labels in the labelMap and mark this line as a label line
            if (token.back() == ':') {
                labelName = token.substr(0, token.size() - 1);
                isLabelLine = true;
                continue;
            }

            // Add the token to the cleaned line
            if (!cleanedLine.empty()) cleanedLine += " ";
            cleanedLine += token;
        }

        if (isLabelLine) {
            // Map label to the index of the next instruction
            labelMap[labelName] = instructions.size();
        }

        if (!cleanedLine.empty()) {
            instructions.push_back(cleanedLine);
        }
    }

    input.close();
    return instructions;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        outs() << "[ERROR] Need 1 argument: file with assembler code\n";
        return 1;
    }

    // Initialize LLVM target and execution engine
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    LLVMContext context;
    std::unique_ptr<Module> module = std::make_unique<Module>("top", context);
    IRBuilder<> builder(context);

    // Declare `regFile` as a zero-initialized global variable
    ArrayType *regFileType = ArrayType::get(builder.getInt32Ty(), REG_FILE_SIZE);
    module->getOrInsertGlobal("regFile", regFileType);
    GlobalVariable *regFile = module->getNamedGlobal("regFile");
    regFile->setLinkage(GlobalValue::ExternalLinkage);
    regFile->setInitializer(ConstantAggregateZero::get(regFileType));

    // Create main function
    FunctionType *mainFuncType = FunctionType::get(builder.getVoidTy(), false);
    Function *mainFunc =
        Function::Create(mainFuncType, Function::ExternalLinkage, "main", module.get());
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entryBB);

    // Load instructions from file with comments and labels
    std::unordered_map<std::string, int> labelMap;
    std::vector<std::string> instructions = loadInstructions(argv[1], labelMap);

    // Create a basic block for each instruction
    std::vector<BasicBlock *> instructionBBs;
    for (size_t i = 0; i < instructions.size(); ++i) {
        BasicBlock *bb = BasicBlock::Create(context, "inst_" + std::to_string(i), mainFunc);
        instructionBBs.push_back(bb);
    }

    // Map labels to their corresponding basic blocks
    std::unordered_map<std::string, BasicBlock *> labelBBMap;
    for (const auto &labelPair : labelMap) {
        int idx = labelPair.second;
        if (idx < instructions.size()) {
            labelBBMap[labelPair.first] = instructionBBs[idx];
        } else {
            // Label at the end of the code, create an exit block
            BasicBlock *exitBB = BasicBlock::Create(context, "exit", mainFunc);
            builder.SetInsertPoint(exitBB);
            builder.CreateRetVoid();
            labelBBMap[labelPair.first] = exitBB;
        }
    }

    // Start from entry block
    builder.CreateBr(instructionBBs[0]);

    // Stack to handle function calls
    std::stack<BasicBlock *> callStack;

    // Map instruction functions
    // (Function declarations are the same as in the previous code)

    // Populate instruction basic blocks
    for (size_t pc = 0; pc < instructions.size(); ++pc) {
        builder.SetInsertPoint(instructionBBs[pc]);

        std::string instr = instructions[pc];

        // If the instruction is empty (e.g., label-only line), proceed to next instruction
        if (instr.empty()) {
            if (!instructionBBs[pc]->getTerminator()) {
                if (pc + 1 < instructions.size()) {
                    builder.CreateBr(instructionBBs[pc + 1]);
                } else {
                    builder.CreateRetVoid();
                }
            }
            continue;
        }

        std::istringstream iss(instr);
        std::string opcode;
        iss >> opcode;
        std::cout << instr << std::endl;
        if (opcode == "MOV") {
            std::string arg1, arg2;
            iss >> arg1 >> arg2;

            int dstRegIndex;
            if (arg1 == "FP") {
                dstRegIndex = REG_FP_INDEX;
            } else if (arg1 == "SP") {
                dstRegIndex = REG_SP_INDEX;
            } else if (arg1[0] == 'R') {
                dstRegIndex = std::stoi(arg1.substr(1));
            } else {
                std::cerr << "[ERROR] Invalid destination in MOV: " << arg1 << "\n";
                return 1;
            }

            if (arg2 == "FP") {
                int srcRegIndex = REG_FP_INDEX;
                Value *dst = builder.getInt32(dstRegIndex);
                Value *src = builder.getInt32(srcRegIndex);
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(),
                    {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV", funcType);
                builder.CreateCall(func, {dst, src});
            } else if (arg2 == "SP") {
                int srcRegIndex = REG_SP_INDEX;
                Value *dst = builder.getInt32(dstRegIndex);
                Value *src = builder.getInt32(srcRegIndex);
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(),
                    {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV", funcType);
                builder.CreateCall(func, {dst, src});
            } else if (arg2[0] == 'R') {
                int srcRegIndex = std::stoi(arg2.substr(1));
                Value *dst = builder.getInt32(dstRegIndex);
                Value *src = builder.getInt32(srcRegIndex);
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(),
                    {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV", funcType);
                builder.CreateCall(func, {dst, src});
            } else {
                // Immediate value
                int imm = std::stoi(arg2);
                Value *dst = builder.getInt32(dstRegIndex);
                Value *immVal = builder.getInt32(imm);
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(),
                    {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV_IMM", funcType);
                builder.CreateCall(func, {dst, immVal});
            }
        } else if (opcode == "MOD") {
            std::string arg1, arg2, arg3;
            iss >> arg1 >> arg2 >> arg3;
            Value *dst = builder.getInt32(std::stoi(arg1.substr(1)));
            Value *src = builder.getInt32(std::stoi(arg2.substr(1)));
            int modValue = std::stoi(arg3);
            Value *mod = builder.getInt32(modValue);
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_MOD", funcType);
            builder.CreateCall(func, {dst, src, mod});
        } else if (opcode == "ADD") {
            std::string arg1, arg2, arg3;
            iss >> arg1 >> arg2 >> arg3;
            if (arg3[0] == 'R') {
                // ADD Rdst Rsrc1 Rsrc2
                Value *dst = builder.getInt32(std::stoi(arg1.substr(1)));
                Value *src1 = builder.getInt32(std::stoi(arg2.substr(1)));
                Value *src2 = builder.getInt32(std::stoi(arg3.substr(1)));
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_ADD", funcType);
                builder.CreateCall(func, {dst, src1, src2});
            } else {
                // ADD Rdst Rsrc IMM
                Value *dst = builder.getInt32(std::stoi(arg1.substr(1)));
                Value *src = builder.getInt32(std::stoi(arg2.substr(1)));
                int imm = std::stoi(arg3);
                Value *immVal = builder.getInt32(imm);
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_ADD_IMM", funcType);
                builder.CreateCall(func, {dst, src, immVal});
            }
        } else if (opcode == "SUB") {
            std::string arg1, arg2, arg3;
            iss >> arg1 >> arg2 >> arg3;
            Value *dst = builder.getInt32(std::stoi(arg1.substr(1)));
            Value *src1 = builder.getInt32(std::stoi(arg2.substr(1)));
            Value *src2 = builder.getInt32(std::stoi(arg3.substr(1)));
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_SUB", funcType);
            builder.CreateCall(func, {dst, src1, src2});
        } else if (opcode == "CMP") {
            std::string arg1, arg2, arg3;
            iss >> arg1 >> arg2 >> arg3;
            Value *result = builder.getInt32(std::stoi(arg1.substr(1)));
            Value *reg1 = builder.getInt32(std::stoi(arg2.substr(1)));
            Value *reg2 = builder.getInt32(std::stoi(arg3.substr(1)));
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_CMP", funcType);
            builder.CreateCall(func, {result, reg1, reg2});
        } else if (opcode == "PUSH") {
            std::string arg1;
            iss >> arg1;
            int regIndex;
            if (arg1 == "FP") {
                regIndex = REG_FP_INDEX;
            } else if (arg1[0] == 'R') {
                regIndex = std::stoi(arg1.substr(1));
            } else {
                std::cerr << "[ERROR] Invalid register in PUSH: " << arg1 << "\n";
                return 1;
            }
            Value *reg = builder.getInt32(regIndex);
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_PUSH", funcType);
            builder.CreateCall(func, {reg});
        } else if (opcode == "POP") {
            std::string arg1;
            iss >> arg1;
            int regIndex;
            if (arg1 == "FP") {
                regIndex = REG_FP_INDEX;
            } else if (arg1[0] == 'R') {
                regIndex = std::stoi(arg1.substr(1));
            } else {
                std::cerr << "[ERROR] Invalid register in POP: " << arg1 << "\n";
                return 1;
            }
            Value *reg = builder.getInt32(regIndex);
            FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_POP", funcType);
            builder.CreateCall(func, {reg});
        } else if (opcode == "BR") {
            std::string arg1;
            iss >> arg1;
            BasicBlock *targetBB = labelBBMap[arg1];
            builder.CreateBr(targetBB);
            continue;
        } else if (opcode == "CALL") {
            std::string arg1;
            iss >> arg1;
            if (arg1 == "SIM_PUT_PIXEL") {
                std::string arg2, arg3, arg4;
                iss >> arg2 >> arg3 >> arg4;
                Value *x = builder.getInt32(std::stoi(arg2.substr(1)));
                Value *y = builder.getInt32(std::stoi(arg3.substr(1)));
                Value *color = builder.getInt32(std::stoi(arg4.substr(1)));
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_PUT_PIXEL", funcType);
                builder.CreateCall(func, {x, y, color});
            } else if (arg1 == "SIM_RAND") {
                std::string arg2;
                iss >> arg2;
                Value *reg = builder.getInt32(std::stoi(arg2.substr(1)));
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_RAND", funcType);
                builder.CreateCall(func, {reg});
            } else if (arg1 == "SIM_FLUSH") {
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_FLUSH", funcType);
                builder.CreateCall(func);
            } else if (opcode == "RET") {
            if (!callStack.empty()) {
                BasicBlock *returnBB = callStack.top();
                callStack.pop();
                builder.CreateBr(returnBB);
            } else {
                builder.CreateRetVoid();
            }
            continue;
            }
        } else if (opcode == "BR_IF") {
            std::string conditionReg, targetLabel;
            iss >> conditionReg >> targetLabel;
            Value *condRegIndex = builder.getInt32(std::stoi(conditionReg.substr(1)));
            // Load the condition register
            Value *regPtr = builder.CreateInBoundsGEP(regFileType, regFile, {builder.getInt32(0), condRegIndex});
            Value *regValue = builder.CreateLoad(builder.getInt32Ty(), regPtr);
            // Compare with zero
            Value *cond = builder.CreateICmpNE(regValue, builder.getInt32(0));
            BasicBlock *targetBB = labelBBMap[targetLabel];
            BasicBlock *nextBB;
            if (pc + 1 < instructions.size()) {
                nextBB = instructionBBs[pc + 1];
            } else {
                nextBB = BasicBlock::Create(context, "exit", mainFunc);
                builder.CreateRetVoid();
            }
            builder.CreateCondBr(cond, targetBB, nextBB);
            continue;
        } else if (opcode == "EXIT") {
            builder.CreateRetVoid();
            break;
        } else {
            std::cerr << "[ERROR] Unknown opcode: " << opcode << "\n";
            return 1;
        }

        // Unconditional branch to next instruction if not already branched
        if (!instructionBBs[pc]->getTerminator()) {
            if (pc + 1 < instructions.size()) {
                builder.CreateBr(instructionBBs[pc + 1]);
            } else {
                builder.CreateRetVoid();
            }
        }
    }

    // Verify the module
    if (verifyModule(*module, &errs())) {
        errs() << "[ERROR] Module verification failed\n";
        return 1;
    }

    // Create the ExecutionEngine
    std::string errorStr;
    ExecutionEngine *ee = EngineBuilder(std::move(module))
                              .setErrorStr(&errorStr)
                              .setEngineKind(EngineKind::JIT)
                              .create();

    if (!ee) {
        std::cerr << "[ERROR] Failed to create ExecutionEngine: " << errorStr << "\n";
        return 1;
    }

    // Map `regFile` in LLVM to the `REG_FILE` array in C++
    ee->addGlobalMapping(regFile, (void *)REG_FILE);

    // Map external functions
        ee->InstallLazyFunctionCreator([=](const std::string &fnName) -> void * {
        if (fnName == "do_MOV") return (void *)&do_MOV;
        if (fnName == "do_MOV_IMM") return (void *)&do_MOV_IMM;
        if (fnName == "do_MOD") return (void *)&do_MOD;
        if (fnName == "do_ADD") return (void *)&do_ADD;
        if (fnName == "do_ADD_IMM") return (void *)&do_ADD_IMM;
        if (fnName == "do_SUB") return (void *)&do_SUB;
        if (fnName == "do_CMP") return (void *)&do_CMP;
        if (fnName == "do_PUSH") return (void *)&do_PUSH;
        if (fnName == "do_POP") return (void *)&do_POP;
        if (fnName == "do_SIM_PUT_PIXEL") return (void *)&do_SIM_PUT_PIXEL;
        if (fnName == "do_SIM_RAND") return (void *)&do_SIM_RAND;
        if (fnName == "do_SIM_FLUSH") return (void *)&do_SIM_FLUSH;
        return nullptr;
    });

    ee->finalizeObject();

    simInit();

    // Run the main function
    ee->runFunction(mainFunc, {});

    simExit();
    return 0;
}