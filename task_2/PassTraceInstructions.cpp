#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Compiler.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

struct TraceInstructionPass : public PassInfoMixin<TraceInstructionPass> {
  static unsigned InstructionCounter;

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    Module *M = F.getParent();
    LLVMContext &Ctx = M->getContext();

    // Получаем или создаем декларации функций логирования
    FunctionCallee InstructionLogger = M->getOrInsertFunction(
        "instructionLogger",
        FunctionType::get(Type::getVoidTy(Ctx),
                          {Type::getInt8Ty(Ctx)->getPointerTo(), Type::getInt64Ty(Ctx)},
                          false));

    FunctionCallee UsesLogger = M->getOrInsertFunction(
        "usesLogger",
        FunctionType::get(Type::getVoidTy(Ctx),
                          {Type::getInt8Ty(Ctx)->getPointerTo(), Type::getInt8Ty(Ctx)->getPointerTo()},
                          false));

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (isa<PHINode>(&I))
          continue;

        IRBuilder<> Builder(&I);

        std::string InstName = I.getOpcodeName();

        uint64_t InstID = InstructionCounter++;

        Value *InstStrConst = Builder.CreateGlobalStringPtr(InstName);
        Value *InstIDConst = ConstantInt::get(Type::getInt64Ty(Ctx), InstID);

        Builder.CreateCall(InstructionLogger, {InstStrConst, InstIDConst});

        for (auto &Use : I.uses()) {
          User *U = Use.getUser();
          if (Instruction *UserInst = dyn_cast<Instruction>(U)) {
            
            std::string LhsName = I.getOpcodeName();
            std::string RhsName = UserInst->getOpcodeName();


            Value *LhsNameConst = Builder.CreateGlobalStringPtr(LhsName);
            Value *RhsNameConst = Builder.CreateGlobalStringPtr(RhsName);

            Builder.CreateCall(UsesLogger, {LhsNameConst, RhsNameConst});
          }
        }
      }
    }

    bool Broken = verifyFunction(F, &errs());
    if (Broken) {
      errs() << "Function " << F.getName() << " is broken!\n";
      return PreservedAnalyses::none();
    }

    return PreservedAnalyses::all();
  }
};

unsigned TraceInstructionPass::InstructionCounter = 0;

extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "TraceInstructionPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "trace-instruction") {
                    FPM.addPass(TraceInstructionPass());
                    return true;
                  }
                  return false;
                });
          }};
}