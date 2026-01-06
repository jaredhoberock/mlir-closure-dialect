#pragma once

#include <memory>
#include <mlir/IR/BuiltinOps.h>
#include <mlir/Pass/Pass.h>

namespace mlir::closure {

struct ConvertClosureToFuncAndTuplePass : PassWrapper<ConvertClosureToFuncAndTuplePass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ConvertClosureToFuncAndTuplePass);

  inline StringRef getArgument() const final { return "convert-closure-to-func-and-tuple"; }
  inline StringRef getDescription() const final { return "Convert closures into func.func and captures into tuples."; }

  void runOnOperation() override;
};

std::unique_ptr<Pass> createConvertClosureToFuncAndTuplePass();

}  // end mlir::closure
