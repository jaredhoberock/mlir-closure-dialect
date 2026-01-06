#ifndef CLOSURE_DIALECT_TD
#define CLOSURE_DIALECT_TD

include "mlir/IR/OpBase.td"

def ClosureDialect : Dialect {
  let name = "closure";
  let summary = "Closure dialect.";
  let cppNamespace = "::mlir::closure";

  let useDefaultTypePrinterParser = 1;

  let dependentDialects = [
    "mlir::func::FuncDialect",
    "mlir::tuple::TupleDialect"
  ];

  let extraClassDeclaration = [{
    void registerTypes();

    //===------------------------------------------------------------------===//
    // Identified closure type registry
    //===------------------------------------------------------------------===//

    mlir::LogicalResult defineOrCheckStaticType(
        llvm::function_ref<mlir::InFlightDiagnostic()> emitError,
        llvm::StringRef name,
        llvm::ArrayRef<mlir::Type> captureTypes,
        mlir::FunctionType callType);

  private:
    struct StaticSig {
      llvm::SmallVector<mlir::Type, 4> captureTypes;
      mlir::FunctionType callType;
    };

    llvm::sys::SmartRWMutex<true> staticTypeMu_;
    llvm::StringMap<StaticSig> staticTypeRegistry_;
  }];
}

class ClosureDialectOp<string mnemonic, list<Trait> traits = []> :
  Op<ClosureDialect, mnemonic, traits>;

#endif // CLOSURE_DIALECT_TD
