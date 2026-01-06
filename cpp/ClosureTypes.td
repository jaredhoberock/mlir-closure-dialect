#ifndef CLOSURE_TYPES_TD
#define CLOSURE_TYPES_TD

include "mlir/IR/AttrTypeBase.td"
include "Closure.td"

def StaticType : TypeDef<ClosureDialect, "Static"> {
  let mnemonic = "static";
  let summary = "Statically-typed closure with explicit name, capture types, and call signature";
  let description = [{
    A statically-typed closure value identified by a name.

    Parameters:
      - name: unique closure identity (checked for consistency across all spellings)
      - captureTypes: ordered list of capture field types
      - callType: function type describing the call signature
  }];

  let parameters = (ins
    StringRefParameter<"closure name">:$name,
    ArrayRefParameter<"Type", "capture field types">:$captureTypes,
    TypeParameter<"FunctionType", "call signature">:$callType
  );

  let skipDefaultBuilders = 1;

  let extraClassDeclaration = [{
    static StaticType get(MLIRContext* ctx, StringRef name,
                          ArrayRef<Type> captureTypes, FunctionType callType);

    static StaticType getChecked(function_ref<InFlightDiagnostic()> emitError,
                                 MLIRContext* ctx, StringRef name,
                                 ArrayRef<Type> captureTypes, FunctionType callType);

    static LogicalResult verifyInvariants(function_ref<InFlightDiagnostic()> emitError,
                                          StringRef name,
                                          ArrayRef<Type> captureTypes,
                                          FunctionType callType);
    using Base::verifyInvariants;
  }];

  let hasCustomAssemblyFormat = 1;
}

#endif // CLOSURE_TYPES_TD
