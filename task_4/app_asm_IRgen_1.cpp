#include "../task_1/sim.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/Error.h"
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

// Define assembler instruction functions
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

// Parsing function that loads instructions with comments and labels handling
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

    // Initialize LLVM target and execution engine
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    LLVMContext context;
    std::unique_ptr<Module> module = std::make_unique<Module>("top", context);
    IRBuilder<> builder(context);

    // Declare `regFile` as a zero-initialized global variable
    ArrayType *regFileType = ArrayType::get(builder.getInt32Ty(), REG_FILE_SIZE);
    GlobalVariable *regFile = new GlobalVariable(
        *module, regFileType, false, GlobalValue::ExternalLinkage,
        ConstantAggregateZero::get(regFileType), "regFile");

    FunctionType *voidFuncType = FunctionType::get(builder.getVoidTy(), false);
    FunctionType *int32x3FuncType = FunctionType::get(builder.getVoidTy(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);

    std::string errorStr;
    ExecutionEngine *ee = EngineBuilder(std::move(module))
                          .setErrorStr(&errorStr)
                          .create();

    if (!ee) {
        std::cerr << "[ERROR] Failed to create ExecutionEngine: " << errorStr << "\n";
        return 1;
    }

    // Map `regFile` in LLVM to the `REG_FILE` array in C++
    ee->addGlobalMapping(regFile, REG_FILE);

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

    ee->finalizeObject();

    // Load instructions from file with comments and labels
    std::unordered_map<std::string, int> labelMap;
    std::vector<std::string> instructions = loadInstructions(argv[1], labelMap);
    int pc = 0;
    simInit();
    // Execution loop
    while (pc < instructions.size()) {
        std::istringstream iss(instructions[pc]);
        std::string opcode, arg1, arg2, arg3;
        iss >> opcode;

        if (opcode == "CALL") {
            iss >> arg1 >> arg2 >> arg3;
            if (arg1 == "SIM_PUT_PIXEL") {
                GenericValue args[3];
                args[0].IntVal = APInt(32, std::stoi(arg2.substr(1)), true);
                args[1].IntVal = APInt(32, std::stoi(arg3.substr(1)), true);
                args[2].IntVal = APInt(32, std::stoi(arg3.substr(1)), true);
                ee->runFunction(ee->FindFunctionNamed("do_SIM_PUT_PIXEL"), args);
            } else if (arg1 == "SIM_RAND") {
                GenericValue arg;
                arg.IntVal = APInt(32, std::stoi(arg2.substr(1)), true);
                ee->runFunction(ee->FindFunctionNamed("do_SIM_RAND"), {arg});
            } else if (arg1 == "SIM_FLUSH") {
                ee->runFunction(ee->FindFunctionNamed("do_SIM_FLUSH"), {});
            } else {
                do_CALL(pc, labelMap[arg1]);
            }
        } else if (opcode == "MOD") {
            iss >> arg1 >> arg2 >> arg3;
            GenericValue args[3];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)), true);
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)), true);
            args[2].IntVal = APInt(32, std::stoi(arg3), true);
            ee->runFunction(ee->FindFunctionNamed("do_MOD"), args);
        } else if (opcode == "MOV") {
            iss >> arg1 >> arg2;
            GenericValue args[2];
            args[0].IntVal = APInt(32, std::stoi(arg1.substr(1)), true);
            args[1].IntVal = APInt(32, std::stoi(arg2.substr(1)), true);
            ee->runFunction(ee->FindFunctionNamed("do_MOV"), args);
        } // Add other opcodes as needed

        pc++;
    }
    simExit();
    return 0;
}
