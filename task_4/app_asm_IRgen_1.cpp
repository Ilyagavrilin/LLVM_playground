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
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

// Constants for register file and stack sizes
constexpr int REG_FILE_SIZE = 16;
constexpr int REG_FP_INDEX = REG_FILE_SIZE;       // Index for Frame Pointer (FP)
constexpr int REG_SP_INDEX = REG_FILE_SIZE + 1;   // Index for Stack Pointer (SP)
constexpr int TOTAL_REG_SIZE = REG_FILE_SIZE + 2; // Total registers including FP and SP
constexpr int STACK_SIZE = 1024;

// Register file and stack memory
uint32_t REG_FILE[TOTAL_REG_SIZE] = {0}; // Initialize all registers to zero
uint32_t STACK[STACK_SIZE] = {0};        // Initialize stack memory to zero

// Function prototypes for assembler instructions
void do_MOV(int dst, int src);
void do_MOV_IMM(int dst, int imm);
void do_MOD(int dst, int src, int mod);
void do_ADD(int dst, int src1, int src2);
void do_ADD_IMM(int dst, int src, int imm);
void do_SUB(int dst, int src1, int src2);
void do_CMP(int result, int reg1, int reg2);
void do_PUSH(int reg);
void do_POP(int reg);
void do_PUSH_RETURN(int returnAddress);
int do_POP_RETURN();
void do_SIM_PUT_PIXEL(int x, int y, int color);
void do_SIM_RAND(int reg);
void do_SIM_FLUSH();

// Implementation of assembler instruction functions
void do_MOV(int dst, int src) {
    REG_FILE[dst] = REG_FILE[src];
}

void do_MOV_IMM(int dst, int imm) {
    REG_FILE[dst] = imm;
}

void do_MOD(int dst, int src, int mod) {
    REG_FILE[dst] = REG_FILE[src] % mod;
}

void do_ADD(int dst, int src1, int src2) {
    REG_FILE[dst] = REG_FILE[src1] + REG_FILE[src2];
}

void do_ADD_IMM(int dst, int src, int imm) {
    REG_FILE[dst] = REG_FILE[src] + imm;
}

void do_SUB(int dst, int src1, int src2) {
    REG_FILE[dst] = REG_FILE[src1] - REG_FILE[src2];
}

void do_CMP(int result, int reg1, int reg2) {
    REG_FILE[result] = (REG_FILE[reg1] >= REG_FILE[reg2]) ? 0 : 1;
}

void do_PUSH(int reg) {
    if (REG_FILE[REG_SP_INDEX] == 0) {
        std::cerr << "[ERROR] Stack overflow\n";
        exit(EXIT_FAILURE);
    }
    STACK[--REG_FILE[REG_SP_INDEX]] = REG_FILE[reg];
}

void do_POP(int reg) {
    if (REG_FILE[REG_SP_INDEX] >= STACK_SIZE) {
        std::cerr << "[ERROR] Stack underflow\n";
        exit(EXIT_FAILURE);
    }
    REG_FILE[reg] = STACK[REG_FILE[REG_SP_INDEX]++];
}

void do_PUSH_RETURN(int returnAddress) {
    if (REG_FILE[REG_SP_INDEX] == 0) {
        std::cerr << "[ERROR] Stack overflow\n";
        exit(EXIT_FAILURE);
    }
    STACK[--REG_FILE[REG_SP_INDEX]] = returnAddress;
}

int do_POP_RETURN() {
    if (REG_FILE[REG_SP_INDEX] >= STACK_SIZE) {
        std::cerr << "[ERROR] Stack underflow\n";
        exit(EXIT_FAILURE);
    }
    return STACK[REG_FILE[REG_SP_INDEX]++];
}

void do_SIM_PUT_PIXEL(int x, int y, int color) {
    simPutPixel(REG_FILE[x], REG_FILE[y], REG_FILE[color]);
}

void do_SIM_RAND(int reg) {
    REG_FILE[reg] = simRand();
}

void do_SIM_FLUSH() {
    simFlush();
}

// Function to load instructions, handling comments and labels
std::vector<std::string> loadInstructions(
    const std::string &filename,
    std::unordered_map<std::string, int> &labelMap) {

    std::vector<std::string> instructions;
    std::ifstream input(filename);
    std::string line;
    int lineNumber = 0;

    if (!input.is_open()) {
        std::cerr << "[ERROR] Can't open file " << filename << "\n";
        exit(EXIT_FAILURE);
    }

    while (std::getline(input, line)) {
        ++lineNumber;
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
        return EXIT_FAILURE;
    }

    // Initialize LLVM components
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    LLVMContext context;
    IRBuilder<> builder(context);
    auto module = std::make_unique<Module>("top", context);

    // Declare `regFile` as a zero-initialized global variable
    ArrayType *regFileType = ArrayType::get(builder.getInt32Ty(), TOTAL_REG_SIZE);
    GlobalVariable *regFile = new GlobalVariable(
        *module, regFileType, false, GlobalValue::ExternalLinkage,
        ConstantAggregateZero::get(regFileType), "regFile");

    // Create the main function
    FunctionType *mainFuncType = FunctionType::get(builder.getVoidTy(), false);
    Function *mainFunc = Function::Create(
        mainFuncType, Function::ExternalLinkage, "main", module.get());
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entryBB);

    // Load instructions from file with comments and labels
    std::unordered_map<std::string, int> labelMap;
    std::vector<std::string> instructions = loadInstructions(argv[1], labelMap);

    // Create a basic block for each instruction
    std::vector<BasicBlock *> instructionBBs;
    for (size_t i = 0; i < instructions.size(); ++i) {
        BasicBlock *bb = BasicBlock::Create(
            context, "inst_" + std::to_string(i), mainFunc);
        instructionBBs.push_back(bb);
    }

    // Map labels to their corresponding basic blocks
    std::unordered_map<std::string, BasicBlock *> labelBBMap;
    for (const auto &labelPair : labelMap) {
        int idx = labelPair.second;
        if (idx < static_cast<int>(instructions.size())) {
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
    if (!instructions.empty()) {
        builder.CreateBr(instructionBBs[0]);
    } else {
        builder.CreateRetVoid();
    }

    // Initialize SP and FP
    REG_FILE[REG_SP_INDEX] = STACK_SIZE; // SP points to the top of the stack
    REG_FILE[REG_FP_INDEX] = 0;          // Initialize FP to 0

    // Process each instruction
    for (size_t pc = 0; pc < instructions.size(); ++pc) {
        builder.SetInsertPoint(instructionBBs[pc]);
        std::string instr = instructions[pc];
        std::istringstream iss(instr);
        std::string opcode;
        iss >> opcode;

        bool terminatorAdded = false; // Flag to check if terminator was added

        // Skip empty instructions
        if (opcode.empty()) {
            if (!instructionBBs[pc]->getTerminator()) {
                if (pc + 1 < instructions.size()) {
                    builder.CreateBr(instructionBBs[pc + 1]);
                } else {
                    builder.CreateRetVoid();
                }
            }
            continue;
        }

        // Handle different opcodes
        if (opcode == "MOV") {
            // Handle MOV instruction
            std::string dst, src;
            iss >> dst >> src;

            int dstRegIndex = (dst == "FP") ? REG_FP_INDEX :
                              (dst == "SP") ? REG_SP_INDEX :
                              std::stoi(dst.substr(1));

            if (src[0] == 'R' || src == "FP" || src == "SP") {
                int srcRegIndex = (src == "FP") ? REG_FP_INDEX :
                                  (src == "SP") ? REG_SP_INDEX :
                                  std::stoi(src.substr(1));

                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV", funcType);
                builder.CreateCall(func, {builder.getInt32(dstRegIndex), builder.getInt32(srcRegIndex)});
            } else {
                int immediate = std::stoi(src);
                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_MOV_IMM", funcType);
                builder.CreateCall(func, {builder.getInt32(dstRegIndex), builder.getInt32(immediate)});
            }
        } else if (opcode == "ADD") {
            // Handle ADD instruction
            std::string dst, src1, src2;
            iss >> dst >> src1 >> src2;

            int dstRegIndex = std::stoi(dst.substr(1));
            int src1RegIndex = std::stoi(src1.substr(1));

            if (src2[0] == 'R') {
                int src2RegIndex = std::stoi(src2.substr(1));
                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_ADD", funcType);
                builder.CreateCall(func, {
                    builder.getInt32(dstRegIndex),
                    builder.getInt32(src1RegIndex),
                    builder.getInt32(src2RegIndex)
                });
            } else {
                int immediate = std::stoi(src2);
                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_ADD_IMM", funcType);
                builder.CreateCall(func, {
                    builder.getInt32(dstRegIndex),
                    builder.getInt32(src1RegIndex),
                    builder.getInt32(immediate)
                });
            }
        } else if (opcode == "SUB") {
            // Handle SUB instruction
            std::string dst, src1, src2;
            iss >> dst >> src1 >> src2;

            int dstRegIndex = std::stoi(dst.substr(1));
            int src1RegIndex = std::stoi(src1.substr(1));
            int src2RegIndex = std::stoi(src2.substr(1));

            FunctionType *funcType = FunctionType::get(
                builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_SUB", funcType);
            builder.CreateCall(func, {
                builder.getInt32(dstRegIndex),
                builder.getInt32(src1RegIndex),
                builder.getInt32(src2RegIndex)
            });
        } else if (opcode == "MOD") {
            // Handle MOD instruction
            std::string dst, src, modVal;
            iss >> dst >> src >> modVal;

            int dstRegIndex = std::stoi(dst.substr(1));
            int srcRegIndex = std::stoi(src.substr(1));
            int modValue = std::stoi(modVal);

            FunctionType *funcType = FunctionType::get(
                builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_MOD", funcType);
            builder.CreateCall(func, {
                builder.getInt32(dstRegIndex),
                builder.getInt32(srcRegIndex),
                builder.getInt32(modValue)
            });
        } else if (opcode == "CMP") {
            // Handle CMP instruction
            std::string result, reg1, reg2;
            iss >> result >> reg1 >> reg2;

            int resultRegIndex = std::stoi(result.substr(1));
            int reg1Index = std::stoi(reg1.substr(1));
            int reg2Index = std::stoi(reg2.substr(1));

            FunctionType *funcType = FunctionType::get(
                builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_CMP", funcType);
            builder.CreateCall(func, {
                builder.getInt32(resultRegIndex),
                builder.getInt32(reg1Index),
                builder.getInt32(reg2Index)
            });
        } else if (opcode == "PUSH") {
            // Handle PUSH instruction
            std::string reg;
            iss >> reg;

            int regIndex = (reg == "FP") ? REG_FP_INDEX :
                           (reg == "SP") ? REG_SP_INDEX :
                           std::stoi(reg.substr(1));

            FunctionType *funcType = FunctionType::get(
                builder.getVoidTy(), {builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_PUSH", funcType);
            builder.CreateCall(func, {builder.getInt32(regIndex)});
        } else if (opcode == "POP") {
            // Handle POP instruction
            std::string reg;
            iss >> reg;

            int regIndex = (reg == "FP") ? REG_FP_INDEX :
                           (reg == "SP") ? REG_SP_INDEX :
                           std::stoi(reg.substr(1));

            FunctionType *funcType = FunctionType::get(
                builder.getVoidTy(), {builder.getInt32Ty()}, false);
            FunctionCallee func = module->getOrInsertFunction("do_POP", funcType);
            builder.CreateCall(func, {builder.getInt32(regIndex)});
        } else if (opcode == "BR") {
            // Handle unconditional branch
            std::string label;
            iss >> label;

            if (labelBBMap.find(label) != labelBBMap.end()) {
                builder.CreateBr(labelBBMap[label]);
                terminatorAdded = true;
            } else {
                std::cerr << "[ERROR] Undefined label: " << label << "\n";
                exit(EXIT_FAILURE);
            }
        } else if (opcode == "BR_IF") {
            // Handle conditional branch
            std::string condReg, label;
            iss >> condReg >> label;

            int condRegIndex = std::stoi(condReg.substr(1));

            // Load condition register value
            Value *condPtr = builder.CreateInBoundsGEP(
                regFileType, regFile, {builder.getInt32(0), builder.getInt32(condRegIndex)});
            Value *condVal = builder.CreateLoad(builder.getInt32Ty(), condPtr);

            // Create condition (non-zero)
            Value *condition = builder.CreateICmpNE(condVal, builder.getInt32(0));

            BasicBlock *trueBB = labelBBMap[label];
            BasicBlock *falseBB = (pc + 1 < instructions.size()) ? instructionBBs[pc + 1] : nullptr;

            if (!trueBB) {
                std::cerr << "[ERROR] Undefined label: " << label << "\n";
                exit(EXIT_FAILURE);
            }

            if (!falseBB) {
                falseBB = BasicBlock::Create(context, "exit", mainFunc);
                builder.SetInsertPoint(falseBB);
                builder.CreateRetVoid();
            }

            builder.CreateCondBr(condition, trueBB, falseBB);
            terminatorAdded = true;
        } else if (opcode == "CALL") {
            // Handle CALL instruction
            std::string function;
            iss >> function;

            if (function == "SIM_PUT_PIXEL") {
                // Handle SIM_PUT_PIXEL call
                std::string xReg, yReg, colorReg;
                iss >> xReg >> yReg >> colorReg;

                int xIndex = std::stoi(xReg.substr(1));
                int yIndex = std::stoi(yReg.substr(1));
                int colorIndex = std::stoi(colorReg.substr(1));

                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_PUT_PIXEL", funcType);
                builder.CreateCall(func, {
                    builder.getInt32(xIndex),
                    builder.getInt32(yIndex),
                    builder.getInt32(colorIndex)
                });
            } else if (function == "SIM_RAND") {
                // Handle SIM_RAND call
                std::string reg;
                iss >> reg;

                int regIndex = std::stoi(reg.substr(1));

                FunctionType *funcType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty()}, false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_RAND", funcType);
                builder.CreateCall(func, {builder.getInt32(regIndex)});
            } else if (function == "SIM_FLUSH") {
                // Handle SIM_FLUSH call
                FunctionType *funcType = FunctionType::get(builder.getVoidTy(), false);
                FunctionCallee func = module->getOrInsertFunction("do_SIM_FLUSH", funcType);
                builder.CreateCall(func);
            } else {
                // Handle user-defined function calls
                if (labelBBMap.find(function) == labelBBMap.end()) {
                    std::cerr << "[ERROR] Undefined function: " << function << "\n";
                    exit(EXIT_FAILURE);
                }

                // Push return address
                int returnAddress = pc + 1;
                FunctionType *pushFuncType = FunctionType::get(
                    builder.getVoidTy(), {builder.getInt32Ty()}, false);
                FunctionCallee pushFunc = module->getOrInsertFunction("do_PUSH_RETURN", pushFuncType);
                builder.CreateCall(pushFunc, {builder.getInt32(returnAddress)});

                // Branch to function label
                builder.CreateBr(labelBBMap[function]);
                terminatorAdded = true;
            }
        } else if (opcode == "RET") {
            // Handle RET instruction
            FunctionType *popFuncType = FunctionType::get(builder.getInt32Ty(), false);
            FunctionCallee popFunc = module->getOrInsertFunction("do_POP_RETURN", popFuncType);
            Value *returnAddress = builder.CreateCall(popFunc);

            // Create switch for indirect branching
            BasicBlock *defaultBB = BasicBlock::Create(context, "default", mainFunc);
            builder.SetInsertPoint(defaultBB);
            builder.CreateRetVoid();

            builder.SetInsertPoint(instructionBBs[pc]);
            SwitchInst *switchInst = builder.CreateSwitch(returnAddress, defaultBB, instructionBBs.size());

            for (size_t i = 0; i < instructionBBs.size(); ++i) {
                switchInst->addCase(builder.getInt32(i), instructionBBs[i]);
            }
            terminatorAdded = true;
        } else if (opcode == "EXIT") {
            // Handle EXIT instruction
            builder.CreateRetVoid();
            terminatorAdded = true;
        } else {
            std::cerr << "[ERROR] Unknown opcode: " << opcode << "\n";
            exit(EXIT_FAILURE);
        }

        // At the end, if no terminator was added, add one
        if (!terminatorAdded && !instructionBBs[pc]->getTerminator()) {
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
        return EXIT_FAILURE;
    }

    // Create the ExecutionEngine
    std::string errorStr;
    ExecutionEngine *ee = EngineBuilder(std::move(module))
                              .setErrorStr(&errorStr)
                              .setEngineKind(EngineKind::JIT)
                              .create();

    if (!ee) {
        std::cerr << "[ERROR] Failed to create ExecutionEngine: " << errorStr << "\n";
        return EXIT_FAILURE;
    }

    // Map external functions
    ee->InstallLazyFunctionCreator([=](const std::string &fnName) -> void * {
        if (fnName == "do_MOV") return reinterpret_cast<void *>(&do_MOV);
        if (fnName == "do_MOV_IMM") return reinterpret_cast<void *>(&do_MOV_IMM);
        if (fnName == "do_MOD") return reinterpret_cast<void *>(&do_MOD);
        if (fnName == "do_ADD") return reinterpret_cast<void *>(&do_ADD);
        if (fnName == "do_ADD_IMM") return reinterpret_cast<void *>(&do_ADD_IMM);
        if (fnName == "do_SUB") return reinterpret_cast<void *>(&do_SUB);
        if (fnName == "do_CMP") return reinterpret_cast<void *>(&do_CMP);
        if (fnName == "do_PUSH") return reinterpret_cast<void *>(&do_PUSH);
        if (fnName == "do_POP") return reinterpret_cast<void *>(&do_POP);
        if (fnName == "do_SIM_PUT_PIXEL") return reinterpret_cast<void *>(&do_SIM_PUT_PIXEL);
        if (fnName == "do_SIM_RAND") return reinterpret_cast<void *>(&do_SIM_RAND);
        if (fnName == "do_SIM_FLUSH") return reinterpret_cast<void *>(&do_SIM_FLUSH);
        if (fnName == "do_PUSH_RETURN") return reinterpret_cast<void *>(&do_PUSH_RETURN);
        if (fnName == "do_POP_RETURN") return reinterpret_cast<void *>(&do_POP_RETURN);
        return nullptr;
    });

    ee->finalizeObject();

    // Initialize simulation
    simInit();

    // Run the main function
    ee->runFunctionAsMain(mainFunc, {}, nullptr);

    // Exit simulation
    simExit();
    return EXIT_SUCCESS;
}