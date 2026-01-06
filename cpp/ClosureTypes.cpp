#include "Closure.hpp"
#include "ClosureTypes.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/TypeSwitch.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/DialectImplementation.h>

using namespace mlir;
using namespace mlir::closure;

#define GET_TYPEDEF_CLASSES
#include "ClosureTypes.cpp.inc"

void ClosureDialect::registerTypes() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "ClosureTypes.cpp.inc"
  >();
}

//===----------------------------------------------------------------------===//
// StaticType
//===----------------------------------------------------------------------===//

LogicalResult StaticType::verifyInvariants(function_ref<InFlightDiagnostic()> emitError,
                                          StringRef name,
                                          ArrayRef<Type> captureTypes,
                                          FunctionType callType) {
  if (name.empty())
    return emitError() << "closure.static name must not be empty", failure();

  if (!callType)
    return emitError() << "closure.static<\"" << name
                       << "\"> expected function type for call signature",
           failure();

  // Keep the diagnostic you wanted and avoid confusing "expected non-function"
  // errors when someone accidentally puts a function type in the capture list.
  for (Type t : captureTypes) {
    if (mlir::isa<FunctionType>(t)) {
      return emitError() << "closure.static<\"" << name
                         << "\"> expected non-function type in capture list",
             failure();
    }
  }

  MLIRContext *ctx = callType.getContext();

  // Ensure the dialect is available so we can consult the registry.
  ctx->loadDialect<ClosureDialect>();
  auto *dialect = ctx->getLoadedDialect<ClosureDialect>();
  if (!dialect)
    return emitError() << "closure dialect is not loaded", failure();

  // Single source of truth for name/signature consistency.
  return dialect->defineOrCheckStaticType(emitError, name, captureTypes, callType);
}

StaticType StaticType::get(MLIRContext *ctx,
                           StringRef name,
                           ArrayRef<Type> captureTypes,
                           FunctionType callType) {
  // Route through checked construction so callers cannot bypass the registry.
  return StaticType::getChecked(
      [&]() {
        return mlir::emitError(UnknownLoc::get(ctx))
               << "invalid !closure.static type";
      },
      ctx, name, captureTypes, callType);
}

StaticType StaticType::getChecked(function_ref<InFlightDiagnostic()> emitError,
                                  MLIRContext *ctx,
                                  StringRef name,
                                  ArrayRef<Type> captureTypes,
                                  FunctionType callType) {
  // Let the generated checked builder call verifyInvariants().
  return Base::getChecked(emitError, ctx, name, captureTypes, callType);
}

Type StaticType::parse(AsmParser &parser) {
  MLIRContext *ctx = parser.getContext();
  SmallVector<Type, 4> captureTypes;

  // < "name", [ ... ] , (A...) -> (R...) >
  if (parser.parseLess())
    return Type();

  std::string name;
  if (parser.parseString(&name))
    return Type();

  if (parser.parseComma())
    return Type();

  if (parser.parseLSquare())
    return Type();

  // Allow empty capture list: []
  if (failed(parser.parseOptionalRSquare())) {
    while (true) {
      Type ty;
      if (parser.parseType(ty))
        return Type();

      captureTypes.push_back(ty);

      if (succeeded(parser.parseOptionalComma()))
        continue;

      if (parser.parseRSquare())
        return Type();
      break;
    }
  }

  if (parser.parseComma())
    return Type();

  Type callTypeTy;
  if (parser.parseType(callTypeTy))
    return Type();

  auto callType = mlir::dyn_cast<FunctionType>(callTypeTy);
  if (!callType) {
    parser.emitError(parser.getCurrentLocation(),
                     "expected function type for call signature");
    return Type();
  }

  if (parser.parseGreater())
    return Type();

  return StaticType::getChecked(
      [&]() { return parser.emitError(parser.getCurrentLocation()); },
      ctx, name, captureTypes, callType);
}

void StaticType::print(AsmPrinter &printer) const {
  printer << "<";
  printer.printString(getName());
  printer << ", [";
  llvm::interleaveComma(getCaptureTypes(), printer,
                        [&](Type t) { printer.printType(t); });
  printer << "], ";
  printer.printType(getCallType());
  printer << ">";
}
