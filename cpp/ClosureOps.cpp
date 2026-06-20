#include "Closure.hpp"
#include "ClosureOps.hpp"
#include "ClosureTypes.hpp"

#define GET_OP_CLASSES
#include <ClosureOps.cpp.inc>

namespace mlir::closure {

//===----------------------------------------------------------------------===//
// MakeOp
//===----------------------------------------------------------------------===//

LogicalResult MakeOp::verify() {
  auto staticTy = cast<StaticType>(getResult().getType());
  auto expectedCaptureTypes = staticTy.getCaptureTypes();
  auto expectedCallArgTypes = staticTy.getCallType().getInputs();

  // Capture operands must match the type's capture list.
  if (getCaptures().size() != expectedCaptureTypes.size()) {
    return emitOpError("has ")
           << getCaptures().size() << " capture operands but type expects "
           << expectedCaptureTypes.size();
  }

  for (auto [i, operand] : llvm::enumerate(getCaptures())) {
    Type expectedType = expectedCaptureTypes[i];
    Type foundType = operand.getType();
    if (foundType != expectedType) {
      return emitOpError("capture operand #")
             << i << " has type " << foundType
             << " but type expects " << expectedType;
    }
  }

  // Entry block args must be: captures first, then call args.
  Block &entry = getBody().front();
  unsigned numCaptures = expectedCaptureTypes.size();
  unsigned numCallArgs = expectedCallArgTypes.size();
  unsigned expectedNumArgs = numCaptures + numCallArgs;

  if (entry.getNumArguments() != expectedNumArgs) {
    return emitOpError("entry block must have ")
           << expectedNumArgs << " argument(s) (" << numCaptures
           << " captures + " << numCallArgs << " call args) but has "
           << entry.getNumArguments();
  }

  for (unsigned i = 0; i < numCaptures; ++i) {
    Type expectedType = expectedCaptureTypes[i];
    Type foundType = entry.getArgument(i).getType();
    if (foundType != expectedType) {
      return emitOpError("entry block capture arg #")
             << i << " has type " << foundType
             << " but type expects " << expectedType;
    }
  }

  for (unsigned i = 0; i < numCallArgs; ++i) {
    Type expectedType = expectedCallArgTypes[i];
    Type foundType = entry.getArgument(numCaptures + i).getType();
    if (foundType != expectedType) {
      return emitOpError("entry block call arg #")
             << i << " has type " << foundType
             << " but type expects " << expectedType;
    }
  }

  return success();
}

ParseResult MakeOp::parse(OpAsmParser &parser, OperationState &result) {
  // example:
  //   %lambda = closure.make "name" [
  //     %c0 = %outer0 : !C0,
  //     %c1 = %outer1 : !C1
  //   ] (%arg0: !A0, %arg1: !A1) -> !R {
  //     ...
  //     closure.return %res : !R
  //   }

  auto loc = parser.getCurrentLocation();
  MLIRContext *ctx = parser.getContext();

  // parse the closure name
  std::string name;
  if (parser.parseString(&name))
    return failure();

  SmallVector<OpAsmParser::UnresolvedOperand, 4> captureOperands;
  SmallVector<Type, 4> captureTypes;

  // entry block args are captures first, then call args
  SmallVector<OpAsmParser::Argument, 8> entryArgs;

  if (parser.parseLSquare())
    return failure();

  // allow empty capture list: []
  if (failed(parser.parseOptionalRSquare())) {
    // parse a comma-separated list of capture bindings until ']'
    while (true) {
      OpAsmParser::Argument capArg;
      OpAsmParser::UnresolvedOperand outer;
      Type capTy;

      // %cap = %outer : ty
      if (parser.parseArgument(capArg) || parser.parseEqual() ||
          parser.parseOperand(outer) || parser.parseColonType(capTy))
        return mlir::failure();

      capArg.type = capTy;
      entryArgs.push_back(capArg);

      captureOperands.push_back(outer);
      captureTypes.push_back(capTy);

      // either `,` more bindings, or `]`
      if (succeeded(parser.parseOptionalComma()))
        continue;

      if (parser.parseRSquare())
        return failure();
      break;
    }
  }

  // parse call arguments: func.func-style typed argument list in parens
  //
  //   (%arg0: !A0, %arg1: !A1, ...)
  SmallVector<Type, 8> callArgTypes;
  if (parser.parseLParen())
    return failure();

  // allow empty arg list: ()
  if (failed(parser.parseOptionalRParen())) {
    while (true) {
      OpAsmParser::Argument arg;
      Type argTy;

      // %arg: ty
      if (parser.parseArgument(arg) || parser.parseColonType(argTy))
        return failure();

      arg.type = argTy;
      callArgTypes.push_back(argTy);
      entryArgs.push_back(arg);

      // either `,` more args, or `)`
      if (succeeded(parser.parseOptionalComma()))
        continue;

      if (parser.parseRParen())
        return failure();
      break;
    }
  }

  // parse results: -> type-list
  SmallVector<mlir::Type, 4> resultTypes;
  if (parser.parseArrowTypeList(resultTypes))
    return failure();

  // optional attributes (func-like: attributes { ... }) before the region
  if (parser.parseOptionalAttrDictWithKeyword(result.attributes))
    return failure();

  // infer and set the result type: !closure.static<captures..., (args)->(rets)>
  auto callTy = FunctionType::get(ctx, callArgTypes, resultTypes);
  auto closureTy = StaticType::getChecked(
      [&] { return parser.emitError(loc); },
      ctx, name, captureTypes, callTy);
  if (!closureTy)
    return failure();

  result.addTypes(closureTy);

  // parse the body region, creating the entry block args we collected:
  //   (%captures..., %callArgs...)
  Region *body = result.addRegion();
  if (parser.parseRegion(*body, entryArgs))
    return failure();

  // resolve outer capture operands
  if (parser.resolveOperands(captureOperands, captureTypes, loc, result.operands))
    return failure();

  return success();
}

void MakeOp::print(OpAsmPrinter &printer) {
  auto staticTy = cast<StaticType>(getResult().getType());
  auto name = staticTy.getName();
  auto captureTypes = staticTy.getCaptureTypes();
  auto callTy = staticTy.getCallType();

  Block &entry = getBody().front();
  unsigned numCaptures = captureTypes.size();
  unsigned numCallArgs = callTy.getNumInputs();

  // print the name
  printer << " ";
  printer.printString(name);
  printer << " ";

  // Captures: [ %cap = %outer : ty, ... ]
  printer << " [";
  for (unsigned i = 0; i < numCaptures; ++i) {
    if (i)
      printer << ", ";

    Value capArg = entry.getArgument(i);
    printer.printOperand(capArg);
    printer << " = ";
    printer.printOperand(getCaptures()[i]);
    printer << " : ";
    printer.printType(capArg.getType());
  }
  printer << "] ";

  // Call args (func.func-like): (%arg0: !A0, %arg1: !A1)
  printer << "(";
  for (unsigned i = 0; i < numCallArgs; ++i) {
    if (i)
      printer << ", ";

    Value arg = entry.getArgument(numCaptures + i);
    printer.printOperand(arg);
    printer << ": ";
    printer.printType(arg.getType());
  }
  printer << ") ";

  // Results: -> type-list
  printer.printArrowTypeList(callTy.getResults());

  // Optional attributes: `attributes { ... }`
  printer.printOptionalAttrDictWithKeyword((*this)->getAttrs());

  // Body region. We already printed the entry block args above.
  printer << " ";
  printer.printRegion(getBody(),
                      /*printEntryBlockArgs=*/false,
                      /*printBlockTerminators=*/true);
}


//===----------------------------------------------------------------------===//
// ReturnOp
//===----------------------------------------------------------------------===//

LogicalResult ReturnOp::verify() {
  auto make = getOperation()->getParentOfType<mlir::closure::MakeOp>();
  if (!make)
    return emitOpError("must appear inside 'closure.make'");

  auto staticTy = cast<StaticType>(make.getResult().getType());
  auto expectedTypes = staticTy.getCallType().getResults();

  auto foundValues = getOperands();

  if (foundValues.size() != expectedTypes.size()) {
    return emitOpError("has ")
           << foundValues.size() << " operand(s) but enclosing closure expects "
           << expectedTypes.size() << " operand(s)";
  }

  for (auto [i, value] : llvm::enumerate(foundValues)) {
    Type expectedType = expectedTypes[i];
    Type foundType = value.getType();
    if (foundType != expectedType) {
      return emitOpError("operand #")
             << i << " has type " << foundType
             << " but enclosing closure expects " << expectedType;
    }
  }

  return success();
}

} // end mlir::closure
