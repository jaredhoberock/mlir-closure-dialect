#include "ConvertToLLVM.hpp"
#include "Closure.hpp"
#include "ClosureOps.hpp"
#include <mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h>
#include <mlir/Conversion/LLVMCommon/TypeConverter.h>
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <mlir/Dialect/LLVMIR/LLVMDialect.h>
#include <mlir/Transforms/DialectConversion.h>

namespace mlir::closure {

struct MakeOpLowering : OpConversionPattern<MakeOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(MakeOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto staticTy = dyn_cast<StaticType>(op.getResult().getType());
    if (!staticTy)
      return rewriter.notifyMatchFailure(op, "expected StaticType");
    MLIRContext *ctx = op.getContext();
    Location loc = op.getLoc();

    // create or reuse the hoisted func.func callee
    // its signature uses converted types
    SmallVector<Type, 8> llvmInputTypes;
    SmallVector<Type, 4> llvmResultTypes;

    llvmInputTypes.reserve(staticTy.getCaptureTypes().size() +
                           staticTy.getCallType().getNumInputs());

    // captures first
    for (Type t : staticTy.getCallType().getInputs()) {
      Type ct = getTypeConverter()->convertType(t);
      if (!ct)
        return rewriter.notifyMatchFailure(op, "failed to convert capture type");
      llvmInputTypes.push_back(ct);
    }

    // then call args
    for (Type t : staticTy.getCallType().getInputs()) {
      Type ct = getTypeConverter()->convertType(t);
      if (!ct)
        return rewriter.notifyMatchFailure(op, "failed to convert call arg type");
      llvmInputTypes.push_back(ct);
    }

    // results
    for (Type t : staticTy.getCallType().getResults()) {
      Type ct = getTypeConverter()->convertType(t);
      if (!ct)
        return rewriter.notifyMatchFailure(op, "failed to convert result type");
      llvmResultTypes.push_back(ct);
    }

    auto calleeType = FunctionType::get(ctx, llvmInputTypes, llvmResultTypes);

    // pick a symbol name from the identified closure name
    std::string calleeName = ("__closure." + staticTy.getName()).str();

    func::FuncOp callee = module.lookupSymbol<func::FuncOp>(calleeName);
    if (callee) {
      if (callee.getFunctionType() != calleeType) {
        return op.emitOpError()
               << "hoisted callee @" << calleeName
               << " already exists with type " << callee.getFunctionType()
               << " but expected " << calleeType;
      }
    } else {
    }

    return rewriter.notifyMatchFailure(op, "MakeOpLowering::matchAndRewrite: TODO");
  }
};

void populateClosureToLLVMConversionPatterns(LLVMTypeConverter& typeConverter, RewritePatternSet& patterns) {
  typeConverter.addConversion([&](StaticType staticTy) -> std::optional<Type> {
    MLIRContext *ctx = staticTy.getContext();

    // convert capture field types
    SmallVector<Type, 4> llvmFieldTypes;
    llvmFieldTypes.reserve(staticTy.getCaptureTypes().size());

    for (Type t : staticTy.getCaptureTypes()) {
      Type converted = typeConverter.convertType(t);
      if (!converted)
        return std::nullopt;

      // reject things LLVM structs can't contain
      if (!LLVM::LLVMStructType::isValidElementType(converted))
        return std::nullopt;

      llvmFieldTypes.push_back(converted);
    }

    // use the closure's identified name to get an identified struct
    std::string envName = ("closure.env." + staticTy.getName()).str();

    auto envTy = LLVM::LLVMStructType::getIdentified(ctx, envName);

    if (failed(envTy.setBody(llvmFieldTypes, /*isPacked=*/false)))
      return std::nullopt;

    return envTy;
  });

  patterns.add<MakeOpLowering>(typeConverter, patterns.getContext());
}

} // end mlir::closure
