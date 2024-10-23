#include "../task_1/sim.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

int main() {
    LLVMContext context;
    Module *module = new Module("app.c", context);
    IRBuilder<> builder(context);

    // Declare simPutPixel function
    Type *voidType = Type::getVoidTy(context);
    ArrayRef<Type *> simPutPixelParamTypes = {Type::getInt32Ty(context),
                                              Type::getInt32Ty(context),
                                              Type::getInt32Ty(context)};
    FunctionType *simPutPixelType = FunctionType::get(voidType, simPutPixelParamTypes, false);
    FunctionCallee simPutPixelFunc = module->getOrInsertFunction("simPutPixel", simPutPixelType);

    // Declare simRand function
    FunctionType *simRandType = FunctionType::get(builder.getInt32Ty(), {}, false);
    FunctionCallee simRandFunc = module->getOrInsertFunction("simRand", simRandType);

    // Declare simFlush function
    FunctionType *simFlushType = FunctionType::get(voidType, {}, false);
    FunctionCallee simFlushFunc = module->getOrInsertFunction("simFlush", simFlushType);

    // Define draw_circle function
    ArrayRef<Type *> drawCircleParamTypes = {builder.getInt32Ty(), builder.getInt32Ty(),
                                             builder.getInt32Ty(), builder.getInt32Ty()};
    FunctionType *drawCircleFuncType = FunctionType::get(voidType, drawCircleParamTypes, false);
    Function *drawCircleFunc = Function::Create(drawCircleFuncType, Function::ExternalLinkage, "draw_circle", module);

    // Create basic blocks for draw_circle
    BasicBlock *DC_BB5 = BasicBlock::Create(context, "", drawCircleFunc);
    BasicBlock *DC_BB6 = BasicBlock::Create(context, "", drawCircleFunc);
    BasicBlock *DC_BB8 = BasicBlock::Create(context, "", drawCircleFunc);
    BasicBlock *DC_BB30 = BasicBlock::Create(context, "", drawCircleFunc);

    // Entry point of draw_circle function
    builder.SetInsertPoint(DC_BB5);
    Value *param0 = drawCircleFunc->getArg(0);
    Value *param1 = drawCircleFunc->getArg(1);
    Value *param2 = drawCircleFunc->getArg(2);
    Value *param3 = drawCircleFunc->getArg(3);

    // ; %5 = icmp slt i32 %2, 0
    Value *cond5 = builder.CreateICmpSLT(param2, builder.getInt32(0));

    // ; br i1 %5, label %30, label %6
    builder.CreateCondBr(cond5, DC_BB30, DC_BB6);

    // ; 6: %7 = sub nsw i32 1, %2
    builder.SetInsertPoint(DC_BB6);
    Value *val7 = builder.CreateNSWSub(builder.getInt32(1), param2);

    // ; br label %8
    builder.CreateBr(DC_BB8);

    // ; 8: phi nodes and pixel operations
    builder.SetInsertPoint(DC_BB8);
    PHINode *phi9 = builder.CreatePHI(builder.getInt32Ty(), 2);
    PHINode *phi10 = builder.CreatePHI(builder.getInt32Ty(), 2);
    PHINode *phi11 = builder.CreatePHI(builder.getInt32Ty(), 2);
    phi9->addIncoming(val7, DC_BB6);
    phi10->addIncoming(builder.getInt32(0), DC_BB6);
    phi11->addIncoming(param2, DC_BB6);

    // ; %12 = add nsw i32 %11, %0
    Value *val12 = builder.CreateNSWAdd(phi11, param0);

    // ; %13 = add nsw i32 %10, %1
    Value *val13 = builder.CreateNSWAdd(phi10, param1);

    // ; call simPutPixel(%12, %13, %3)
    Value *args1[] = {val12, val13, param3};
    builder.CreateCall(simPutPixelFunc, args1);

    // ; Handle the remaining simPutPixel calls with similar operations
    Value *val14 = builder.CreateNSWAdd(phi10, param0);
    Value *val15 = builder.CreateNSWAdd(phi11, param1);
    Value *args2[] = {val14, val15, param3};
    builder.CreateCall(simPutPixelFunc, args2);

    // Additional operations involving simPutPixel as per the IR instructions
    Value *val16 = builder.CreateNSWSub(param0, phi11);
    Value *args3[] = {val16, val13, param3};
    builder.CreateCall(simPutPixelFunc, args3);

    Value *val17 = builder.CreateNSWSub(param0, phi10);
    Value *args4[] = {val17, val15, param3};
    builder.CreateCall(simPutPixelFunc, args4);

    Value *val18 = builder.CreateNSWSub(param1, phi10);
    Value *args5[] = {val16, val18, param3};
    builder.CreateCall(simPutPixelFunc, args5);

    Value *val19 = builder.CreateNSWSub(param1, phi11);
    Value *args6[] = {val17, val19, param3};
    builder.CreateCall(simPutPixelFunc, args6);

    Value *args7[] = {val12, val18, param3};
    builder.CreateCall(simPutPixelFunc, args7);

    Value *args8[] = {val14, val19, param3};
    builder.CreateCall(simPutPixelFunc, args8);

    // ; Handle the phi updates and loop logic
    Value *val20 = builder.CreateNSWAdd(phi10, builder.getInt32(1));
    Value *cond21 = builder.CreateICmpSLT(phi9, builder.getInt32(1));
    Value *val22 = builder.CreateNSWSub(phi11, builder.getInt32(1));
    Value *val23 = builder.CreateSelect(cond21, phi11, val22);
    Value *val24 = builder.CreateSelect(cond21, builder.getInt32(0), val22);
    Value *val25 = builder.CreateNSWSub(val20, val24);
    Value *val26 = builder.CreateShl(val25, 1);
    Value *val27 = builder.CreateAdd(phi9, builder.getInt32(1));
    Value *val28 = builder.CreateAdd(val27, val26);
    Value *cond29 = builder.CreateICmpSLT(phi10, val23);
    builder.CreateCondBr(cond29, DC_BB8, DC_BB30);

    // ; Return from function
    builder.SetInsertPoint(DC_BB30);
    builder.CreateRetVoid();

    // Dump LLVM IR
    module->print(outs(), nullptr);

    // LLVM IR Interpreter
    outs() << "[EE] Run\n";
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    ExecutionEngine *ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
    ee->InstallLazyFunctionCreator([=](const std::string &fnName) -> void * {
        if (fnName == "simPutPixel") {
            return reinterpret_cast<void *>(simPutPixel);
        }
        if (fnName == "simRand") {
            return reinterpret_cast<void *>(simRand);
        }
        if (fnName == "simFlush") {
            return reinterpret_cast<void *>(simFlush);
        }
        return nullptr;
    });
    ee->finalizeObject();

    simInit();

    ArrayRef<GenericValue> noargs;
    GenericValue v = ee->runFunction(drawCircleFunc, noargs);
    outs() << "[EE] Result: " << v.IntVal << "\n";

    simExit();
    return EXIT_SUCCESS;
}
