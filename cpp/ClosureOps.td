#ifndef CLOSURE_OPS_TD
#define CLOSURE_OPS_TD

include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/IR/OpAsmInterface.td"
include "mlir/IR/OpBase.td"
include "Closure.td"
include "ClosureTypes.td"

//===----------------------------------------------------------------------===//
// CallOp
//===----------------------------------------------------------------------===//

def CallOp : ClosureDialectOp<"call", [
  TypesMatchWith<"callee input types match argument types",
                 "callee", "callee_operands",
                 "::llvm::cast<::mlir::closure::StaticType>($_self).getCallType().getInputs()">,
  TypesMatchWith<"callee result types match result types",
                 "callee", "results",
                 "::llvm::cast<::mlir::closure::StaticType>($_self).getCallType().getResults()">
]> {
  let summary = "Call a statically-typed closure value";
  let description = [{
    Call to a closure value of type `!closure.static`.
    The closure type encodes the call signature, so result types are inferred
    from the callee type.
  }];

  let arguments = (ins StaticType:$callee, Variadic<AnyType>:$callee_operands);
  let results = (outs Variadic<AnyType>:$results);
  let assemblyFormat = "$callee `(` $callee_operands `)` attr-dict `:` qualified(type($callee))";
}

//===----------------------------------------------------------------------===//
// MakeOp
//===----------------------------------------------------------------------===//

def MakeOp : ClosureDialectOp<"make", [
  OpAsmOpInterface,
  IsolatedFromAbove,
  Pure
]> {
  let summary = "Create a statically-typed closure value";
  let description = [{
    Constructs a closure value with explicit captures and an isolated body region.
  }];

  let arguments = (ins Variadic<AnyType>:$captures);
  let results = (outs StaticType:$result);
  let regions = (region AnyRegion:$body);

  let hasCustomAssemblyFormat = 1;
  let hasVerifier = 1;

  let extraClassDeclaration = [{
    //===------------------------------------------------------------------===//
    // OpAsmOpInterface Methods
    //===------------------------------------------------------------------===//

    /// Allow the dialect prefix to be omitted.
    static StringRef getDefaultDialect() { return "closure"; }
  }];
}

//===----------------------------------------------------------------------===//
// ReturnOp
//===----------------------------------------------------------------------===//

def ReturnOp : ClosureDialectOp<"return", [
  HasParent<"MakeOp">,
  Pure,
  ReturnLike,
  Terminator
]> {
  let summary = "Return from a closure body";
  let description = [{
    Terminates the body region of a closure.make operation.

    This op is only valid inside the region of closure.make, and its operand
    types must match the result types encoded in the enclosing `!closure.static`
    type's call signature.
  }];

  let arguments = (ins Variadic<AnyType>:$operands);
  let builders = [OpBuilder<(ins), [{ /* nothing to do */ }]>];

  let assemblyFormat = "($operands^ `:` type($operands))? attr-dict";
  let hasVerifier = 1;
}

#endif // CLOSURE_OPS_TD
