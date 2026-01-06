# MLIR Closure Dialect

An MLIR dialect for statically-typed, first-class closures with explicit capture lists.

## Overview

This dialect represents closures as named types with explicit capture signatures:

```mlir
!closure.static<"name", [capture_types], call_signature>
```

Closures are created with `closure.make`, which specifies captured values and a body region:

```mlir
%lambda = closure.make "adder" [
  %x = %arg0 : i64
] (%y: i64) -> i64 {
  %sum = arith.addi %x, %y : i64
  closure.return %sum : i64
}
```

And invoked with `closure.call`:

```mlir
%result = closure.call %lambda(%five) : !closure.static<"adder", [i64], (i64) -> i64>
```

## Example

```mlir
func.func @make_counter(%init: i64) 
  -> !closure.static<"counter", [i64], () -> i64> {
  
  %counter = closure.make "counter" [
    %count = %init : i64
  ] () -> i64 {
    closure.return %count : i64
  }
  
  return %counter : !closure.static<"counter", [i64], () -> i64>
}
```

## Lowering

The `convert-closure-to-func-and-tuple` pass lowers closures to standard MLIR:
- Closure types become tuples of captures
- `closure.make` becomes `tuple.make` + a private function
- `closure.call` becomes `func.call` with the tuple as first argument
