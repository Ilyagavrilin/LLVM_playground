#include "../task_1/sim.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <sstream>

using namespace llvm;

const int REG_FILE_SIZE = 16;
uint32_t REG_FILE[REG_FILE_SIZE];
std::stack<uint32_t> stack;
std::stack<int> callStack;

// Define functions for each instruction
void do_MOV(int dst, int src) { REG_FILE[dst] = REG_FILE[src]; }
void do_MOD(int dst, int src, int mod) { REG_FILE[dst] = REG_FILE[src] % mod; }
void do_ADD(int dst, int src1, int src2) { REG_FILE[dst] = REG_FILE[src1] + REG_FILE[src2]; }
void do_SUB(int dst, int src1, int src2) { REG_FILE[dst] = REG_FILE[src1] - REG_FILE[src2]; }
void do_CMP(int result, int reg1, int reg2) { REG_FILE[result] = (REG_FILE[reg1] >= REG_FILE[reg2]) ? 0 : 1; }
void do_PUSH(int reg) { stack.push(REG_FILE[reg]); }
void do_POP(int reg) { if (!stack.empty()) { REG_FILE[reg] = stack.top(); stack.pop(); } }
void do_SIM_PUT_PIXEL(int x, int y, int color) { simPutPixel(REG_FILE[x], REG_FILE[y], REG_FILE[color]); }
void do_SIM_RAND(int reg) { REG_FILE[reg] = rand() % 1000; }
void do_SIM_FLUSH() { simFlush(); }
void do_RET(int &pc) { if (!callStack.empty()) { pc = callStack.top(); callStack.pop(); } }
void do_CALL(int &pc, int targetIndex) { callStack.push(pc + 1); pc = targetIndex; }
void do_BR(int &pc, int targetIndex) { pc = targetIndex; }
void do_BR_IF(int &pc, int condition, int targetIndex) { if (REG_FILE[condition] != 0) { pc = targetIndex; } }

std::vector<std::string> loadInstructions(const std::string &filename, std::unordered_map<std::string, int> &labelMap) {
    std::vector<std::string> instructions;
    std::ifstream input(filename);
    std::string line;
    int lineIndex = 0;

    if (!input.is_open()) {
        std::cerr << "[ERROR] Can't open file " << filename << "\n";
        exit(1);
    }

    while (getline(input, line)) {
        std::string cleanedLine;
        std::istringstream iss(line);

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

            // Store labels in the labelMap and skip further processing for them
            if (token.back() == ':') {
                labelMap[token.substr(0, token.size() - 1)] = lineIndex;
                continue;
            }

            // Add the token to the cleaned line
            if (!cleanedLine.empty()) cleanedLine += " ";
            cleanedLine += token;
        }

        if (!cleanedLine.empty()) {
            instructions.push_back(cleanedLine);
            lineIndex++;
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

    LLVMContext context;
    Module *module = new Module("top", context);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    IRBuilder<> builder(context);

    ArrayType *regFileType = ArrayType::get(builder.getInt32Ty(), REG_FILE_SIZE);
    module->getOrInsertGlobal("regFile", regFileType);
    GlobalVariable *regFile = module->getNamedGlobal("regFile");

    FunctionType *voidFuncType = FunctionType::get(builder.getVoidTy(), false);
    FunctionType *int32x3FuncType = FunctionType::get(builder.getVoidTy(),
                                                      {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);

    Function *do_MOV_Func = cast<Function>(module->getOrInsertFunction("do_MOV", int32x3FuncType).getCallee());
    Function *do_MOD_Func = cast<Function>(module->getOrInsertFunction("do_MOD", int32x3FuncType).getCallee());
    Function *do_ADD_Func = cast<Function>(module->getOrInsertFunction("do_ADD", int32x3FuncType).getCallee());
    Function *do_SUB_Func = cast<Function>(module->getOrInsertFunction("do_SUB", int32x3FuncType).getCallee());
    Function *do_CMP_Func = cast<Function>(module->getOrInsertFunction("do_CMP", int32x3FuncType).getCallee());
    Function *do_PUSH_Func = cast<Function>(module->getOrInsertFunction("do_PUSH", int32x3FuncType).getCallee());
    Function *do_POP_Func = cast<Function>(module->getOrInsertFunction("do_POP", int32x3FuncType).getCallee());
    Function *do_SIM_PUT_PIXEL_Func = cast<Function>(module->getOrInsertFunction("do_SIM_PUT_PIXEL", int32x3FuncType).getCallee());
    Function *do_SIM_RAND_Func = cast<Function>(module->getOrInsertFunction("do_SIM_RAND", int32x3FuncType).getCallee());
    Function *do_SIM_FLUSH_Func = cast<Function>(module->getOrInsertFunction("do_SIM_FLUSH", voidFuncType).getCallee());

    ExecutionEngine *ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
    ee->InstallLazyFunctionCreator([=](const std::string &fnName) -> void * {
        if (fnName == "do_MOV") return reinterpret_cast<void *>(do_MOV);
        if (fnName == "do_MOD") return reinterpret_cast<void *>(do_MOD);
        if (fnName == "do_ADD") return reinterpret_cast<void *>(do_ADD);
        if (fnName == "do_SUB") return reinterpret_cast<void *>(do_SUB);
        if (fnName == "do_CMP") return reinterpret_cast<void *>(do_CMP);
        if (fnName == "do_PUSH") return reinterpret_cast<void *>(do_PUSH);
        if (fnName == "do_POP") return reinterpret_cast<void *>(do_POP);
        if (fnName == "do_SIM_PUT_PIXEL") return reinterpret_cast<void *>(do_SIM_PUT_PIXEL);
        if (fnName == "do_SIM_RAND") return reinterpret_cast<void *>(do_SIM_RAND);
        if (fnName == "do_SIM_FLUSH") return reinterpret_cast<void *>(do_SIM_FLUSH);
        return nullptr;
    });

    ee->addGlobalMapping(regFile, (void *)REG_FILE);
    ee->finalizeObject();

    std::unordered_map<std::string, int> labelMap;
    std::vector<std::string> instructions = loadInstructions(argv[1], labelMap);
    int pc = 0;

    while (pc < instructions.size()) {
        std::istringstream iss(instructions[pc]);
        std::string opcode, arg1, arg2, arg3;
        iss >> opcode;

        if (opcode == "CALL") {
            iss >> arg1 >> arg2;
            if (arg1 == "SIM_RAND") {
                GenericValue argVal;
                argVal.IntVal = APInt(32, std::stoi(arg2.substr(1)));
                ee->runFunction(do_SIM_RAND_Func, {argVal});
            } else if (arg1 == "SIM_FLUSH") {
                ee->runFunction(do_SIM_FLUSH_Func, {});
            } else {
                do_CALL(pc, labelMap[arg1]);
            }
        } else if (opcode == "MOD") {
            iss >> arg1 >> arg2 >> arg3;
            GenericValue args[3];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)));
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)));
            args[2].IntVal = APInt(32, std::stoi(arg3));
            ee->runFunction(do_MOD_Func, args);
        } else if (opcode == "MOV") {
            iss >> arg1 >> arg2;
            GenericValue args[2];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)));
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)));
            ee->runFunction(do_MOV_Func, args);
        } else if (opcode == "ADD") {
            iss >> arg1 >> arg2 >> arg3;
            GenericValue args[3];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)));
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)));
            args[2].IntVal = APInt(32, std::stoi(arg3.substr(1)));
            ee->runFunction(do_ADD_Func, args);
        } else if (opcode == "SUB") {
            iss >> arg1 >> arg2 >> arg3;
            GenericValue args[3];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)));
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)));
            args[2].IntVal = APInt(32, std::stoi(arg3.substr(1)));
            ee->runFunction(do_SUB_Func, args);
        } else if (opcode == "SIM_PUT_PIXEL") {
            iss >> arg1 >> arg2 >> arg3;
            GenericValue args[3];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)));
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)));
            args[2].IntVal = APInt(32, std::stoi(arg3.substr(1)));
            ee->runFunction(do_SIM_PUT_PIXEL_Func, args);
        } else if (opcode == "RET") {
            do_RET(pc);
            continue;
        } else if (opcode == "BR") {
            iss >> arg1;
            do_BR(pc, labelMap[arg1]);
            continue;
        } else if (opcode == "BR_IF") {
            iss >> arg1 >> arg2;
            do_BR_IF(pc, std::stoi(arg1.substr(1)), labelMap[arg2]);
            continue;
        }

        pc++;
    }

    return 0;
}