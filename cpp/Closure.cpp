#include "ConvertToLLVM.hpp"
#include "Closure.hpp"
#include "ClosureOps.hpp"
#include "ClosureTypes.hpp"
#include <mlir/Dialect/Func/IR/FuncOps.h>
#include <Tuple.hpp>

#include "Closure.cpp.inc"

namespace mlir::closure {

void ClosureDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "ClosureOps.cpp.inc"
  >();

  registerTypes();
}

LogicalResult ClosureDialect::defineOrCheckStaticType(
    function_ref<InFlightDiagnostic()> emitError,
    StringRef name,
    ArrayRef<Type> captureTypes,
    FunctionType callType) {

  if (name.empty()) {
    emitError() << "closure name must not be empty";
    return failure();
  }

  if (!callType) {
    emitError() << "closure name \"" << name
                << "\" has invalid call signature (expected function type)";
    return failure();
  }

  llvm::sys::SmartScopedWriter<true> lock(staticTypeMu_);

  auto it = staticTypeRegistry_.find(name);
  if (it == staticTypeRegistry_.end()) {
    StaticSig sig;
    sig.captureTypes.assign(captureTypes.begin(), captureTypes.end());
    sig.callType = callType;
    staticTypeRegistry_.try_emplace(name, std::move(sig));
    return success();
  }

  const StaticSig &expected = it->second;

  // Capture list size.
  if (expected.captureTypes.size() != captureTypes.size()) {
    return emitError()
           << "closure name \"" << name
           << "\" capture list mismatch: expected "
           << expected.captureTypes.size() << " capture type(s) but found "
           << captureTypes.size();
  }

  // Capture list contents.
  if (!llvm::equal(expected.captureTypes, captureTypes)) {
    auto diag = emitError();
    diag << "closure name \"" << name
         << "\" capture types mismatch: expected [";
    llvm::interleaveComma(expected.captureTypes, diag,
                          [&](Type t) { diag << t; });
    diag << "] but found [";
    llvm::interleaveComma(captureTypes, diag,
                          [&](Type t) { diag << t; });
    diag << "]";
    return failure();
  }

  // Call signature.
  if (expected.callType != callType) {
    return emitError()
           << "closure name \"" << name
           << "\" call signature mismatch: expected " << expected.callType
           << " but found " << callType;
  }

  return success();
}

}
