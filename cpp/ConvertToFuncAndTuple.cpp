#include "Closure.hpp"
#include "ClosureOps.hpp"
#include "ConvertToFuncAndTuple.hpp"
#include <mlir/Dialect/Func/Transforms/FuncConversions.h>
#include <mlir/Pass/Pass.h>
#include <mlir/Transforms/DialectConversion.h>
#include <TraitTypes.hpp>
#include <TupleOps.hpp>

namespace mlir::closure {

/// Build a "deep" type converter that rewrites closure types anywhere they occur.
/// Target representation: builtin tuple of capture field types.
static TypeConverter makeClosureToFuncAndTupleTypeConverter() {
  TypeConverter tc;

  // One conversion: deep-rewrite the type by recursively replacing any
  // !closure.static occurrences.
  tc.addConversion([&](Type root) -> std::optional<Type> {
    AttrTypeReplacer replacer;

    // Replace !closure.static with tuple<captureTypes...>, recursively converted.
    replacer.addReplacement([&](Type t) -> std::optional<Type> {
      auto st = dyn_cast<StaticType>(t);
      if (!st)
        return std::nullopt;

      SmallVector<Type, 4> elts;
      elts.reserve(st.getCaptureTypes().size());
      for (Type cap : st.getCaptureTypes())
        elts.push_back(replacer.replace(cap)); // deep conversion

      return TupleType::get(t.getContext(), elts);
    });

    return replacer.replace(root);
  });

  return tc;
}


struct CallOpLowering : public OpConversionPattern<CallOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult matchAndRewrite(CallOp op, OpAdaptor adaptor,
                                ConversionPatternRewriter &rewriter) const override {
    MLIRContext *ctx = op.getContext();
    Location loc = op.getLoc();
    ModuleOp module = op->getParentOfType<ModuleOp>();
    if (!module)
      return rewriter.notifyMatchFailure(op, "expected parent ModuleOp");

    auto staticTy = dyn_cast<StaticType>(op.getCallee().getType());
    if (!staticTy)
      return rewriter.notifyMatchFailure(op, "expected !closure.static callee type");

    // @__closure.<name>
    std::string funcName = ("__closure." + staticTy.getName()).str();

    const TypeConverter *tc = getTypeConverter();

    // build lowered call operand list: (env, call_operands...)
    SmallVector<Value, 8> callOperands;
    callOperands.push_back(adaptor.getCallee()); // converted env tuple
    llvm::append_range(callOperands, adaptor.getCalleeOperands());

    // convert result types (from the closure's call signature)
    SmallVector<Type, 8> convertedResults;
    if (failed(tc->convertTypes(staticTy.getCallType().getResults(), convertedResults)))
      return rewriter.notifyMatchFailure(op, "failed to convert call result types");

    // ensure the hoisted callee exists; if not, create a private declaration.
    func::FuncOp func = module.lookupSymbol<func::FuncOp>(funcName);
    if (!func) {
      SmallVector<Type, 8> inputTypes;
      inputTypes.reserve(callOperands.size());
      for (Value v : callOperands)
        inputTypes.push_back(v.getType());

      FunctionType funcTy = FunctionType::get(ctx, inputTypes, convertedResults);

      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());

      func = func::FuncOp::create(rewriter, loc, funcName, funcTy);
      func.setPrivate();
    } else {
      // the existing type must match what this call implies.
      SmallVector<Type, 8> inputTypes;
      inputTypes.reserve(callOperands.size());
      for (Value v : callOperands)
        inputTypes.push_back(v.getType());
      FunctionType expectedTy = FunctionType::get(ctx, inputTypes, convertedResults);

      if (func.getFunctionType() != expectedTy) {
        return op.emitOpError()
               << "hoisted function @" << funcName << " has type "
               << func.getFunctionType() << " but call expects " << expectedTy;
      }
    }

    // lower to func.call
    rewriter.replaceOpWithNewOp<func::CallOp>(op, funcName, convertedResults, callOperands);
    return success();
  }
};


struct MakeOpLowering : public OpConversionPattern<MakeOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(MakeOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    MLIRContext *ctx = op.getContext();
    Location loc = op.getLoc();
    ModuleOp module = op->getParentOfType<ModuleOp>();
    if (!module) return rewriter.notifyMatchFailure(op, "expected parent ModuleOp");

    auto staticTy = dyn_cast<StaticType>(op.getResult().getType());
    if (!staticTy) return op.emitOpError() << "expected !closure.static result type";
    StringRef name = staticTy.getName();

    // hoisted function name derived from closure identity
    std::string funcName = ("__closure." + name).str();

    // build the hoisted (unconverted) function type: (staticTy, callInputs...) -> callResults...
    SmallVector<Type> inputTypes;
    inputTypes.push_back(staticTy);
    inputTypes.append(staticTy.getCallType().getInputs().begin(),
                      staticTy.getCallType().getInputs().end());
    auto resultTypes = staticTy.getCallType().getResults();
    FunctionType unconvertedfuncTy = FunctionType::get(ctx, inputTypes, resultTypes);

    // convert to the lowered function type
    const TypeConverter *tc = getTypeConverter();
    FunctionType funcTy = dyn_cast_or_null<FunctionType>(tc->convertType(unconvertedfuncTy));
    if (!funcTy) return rewriter.notifyMatchFailure(op, "failed to convert hoisted function type");

    // get or create the hoisted func.func
    func::FuncOp func = module.lookupSymbol<func::FuncOp>(funcName);
    if (!func) {
      // create the hoisted func.func at module scope
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      func = func::FuncOp::create(rewriter, loc, funcName, funcTy);
      func.setPrivate();
    } else {
      // if it already exists, ensure the type matches
      auto existingTy = func.getFunctionType();
      if (existingTy != funcTy)
        return op.emitOpError()
               << "closure \"" << name << "\" hoisted function @" << funcName
               << " already exists with type " << existingTy
               << " but expected " << funcTy;
    }

    // only define the function body once
    if (func.isDeclaration()) {
      OpBuilder::InsertionGuard guard(rewriter);

      Region &body = op.getBody();
      if (body.empty())
        return rewriter.notifyMatchFailure(op, "expected non-empty closure body");

      // convert the region signature
      TypeConverter::SignatureConversion sigConv(body.front().getNumArguments());
      for (auto [i, arg] : llvm::enumerate(body.front().getArguments())) {
        Type oldTy = arg.getType();
        Type newTy = tc->convertType(oldTy);
        if (!newTy)
          return rewriter.notifyMatchFailure(op, "failed to convert closure body arg type");
        sigConv.addInputs(i, newTy);
      }
      if (failed(rewriter.convertRegionTypes(&body, *tc, &sigConv)))
        return rewriter.notifyMatchFailure(op, "convertRegionTypes failed");

      // create destination entry block and inline body
      Block *dstEntry = func.addEntryBlock();
      Block *srcEntry = &body.front(); // capture pointer before inlining
      rewriter.inlineRegionBefore(body, func.getBody(), func.getBody().end());

      // map closure block args -> (captures extracted from env) + (call args)
      Value env = dstEntry->getArgument(0);
      unsigned numCaptures = staticTy.getCaptureTypes().size();
      unsigned numCallArgs = staticTy.getCallType().getNumInputs();

      SmallVector<Value, 8> mapped;
      mapped.reserve(numCaptures + numCallArgs);

      // captures = tuple.get env, i
      {
        OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(dstEntry);
        for (unsigned i = 0; i < numCaptures; ++i)
          mapped.push_back(tuple::GetOp::create(rewriter, loc, env, i));
      }

      // call args = dstEntry args after env
      for (unsigned i = 0; i < numCallArgs; ++i)
        mapped.push_back(dstEntry->getArgument(1 + i));

      // merge the inlined entry block into the real entry block, replacing args
      rewriter.mergeBlocks(srcEntry, dstEntry, mapped);

      // collect closure.return ops that are immediate children of the func
      SmallVector<ReturnOp, 8> returns;
      for (Block &b : func.getBody().getBlocks())
        if (auto r = dyn_cast<ReturnOp>(b.getTerminator()))
          returns.push_back(r);

      // rewrite each closure.return -> func.return
      for (ReturnOp r : returns) {
        rewriter.setInsertionPoint(r);
        rewriter.replaceOpWithNewOp<func::ReturnOp>(r, r.getOperands());
      }
    } else {
      // MakeOpLowering is the only thing allowed to define the function's body
      return op.emitOpError()
             << "multiple definitions for closure \"" << name
             << "\"; hoisted function @" << funcName << " already has a body";
    }

    // replace closure.make with tuple.make
    rewriter.replaceOpWithNewOp<tuple::MakeOp>(op, adaptor.getCaptures());
    return success();
  }
};


LogicalResult convertClosureToFuncAndTuple(ModuleOp module) {
  MLIRContext *ctx = module.getContext();

  ConversionTarget target(*ctx);

  // all closure dialect ops are illegal
  target.addIllegalDialect<ClosureDialect>();

  // otherwise, an op is legal if it does not mention a !closure.static type
  target.markUnknownOpDynamicallyLegal([](Operation *op) {
    return !trait::opMentionsType<StaticType>(op);
  });

  // create a TypeConverter to (recursively) convert !closure.static -> tuple
  TypeConverter tc = makeClosureToFuncAndTupleTypeConverter();

  // convert all closure ops
  RewritePatternSet patterns(ctx);
  patterns.add<
    CallOpLowering,
    MakeOpLowering
  >(tc, ctx);

  // populate conversion patterns for func dialect ops
  populateFunctionOpInterfaceTypeConversionPattern<func::FuncOp>(patterns, tc);
  populateCallOpTypeConversionPattern(patterns, tc);
  populateReturnOpTypeConversionPattern(patterns, tc);

  return applyPartialConversion(module, target, std::move(patterns));
}

void ConvertClosureToFuncAndTuplePass::runOnOperation() {
  if (failed(convertClosureToFuncAndTuple(getOperation())))
    signalPassFailure();
}

std::unique_ptr<Pass> createConvertClosureToFuncAndTuplePass() {
  return std::make_unique<ConvertClosureToFuncAndTuplePass>();
}

} // end mlir::closure
