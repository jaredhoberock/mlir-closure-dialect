// RUN: mlir-opt -split-input-file --pass-pipeline="builtin.module(convert-closure-to-func-and-tuple)" %s | FileCheck %s

// -----
// Test 1: Basic closure.call lowers to a direct call of the hoisted symbol
// CHECK-LABEL: func.func @call_lowering_simple(
// CHECK: %[[R:.*]] = call @__closure.test1(%{{.*}}, %{{.*}}, %{{.*}}) : (tuple<>, i64, i1) -> i32
func.func @call_lowering_simple(
    %callee: !closure.static<"test1", [], (i64, i1) -> i32>,
    %x: i64,
    %p: i1) -> i32 {
  %r = closure.call %callee(%x, %p) : !closure.static<"test1", [], (i64, i1) -> i32>
  return %r : i32
}

// -----
// Test 2: closure.call with captures lowers to a multi-result direct call
// CHECK-LABEL: func.func @call_lowering_with_captures(
// CHECK: %[[R:.*]]:2 = call @__closure.test2(%{{.*}}, %{{.*}}, %{{.*}}) : (tuple<i64, i1>, i32, i32) -> (i64, i32)
func.func @call_lowering_with_captures(
    %callee: !closure.static<"test2", [i64, i1], (i32, i32) -> (i64, i32)>,
    %a: i32,
    %b: i32) -> (i64, i32) {
  %r0, %r1 = closure.call %callee(%a, %b)
    : !closure.static<"test2", [i64, i1], (i32, i32) -> (i64, i32)>
  return %r0, %r1 : i64, i32
}

// -----
// Test 3: No captures, no call operands, multi-result
// CHECK-LABEL: func.func @call_lowering_no_args_multi_result(
// CHECK: %[[R:.*]]:2 = call @__closure.test3(%{{.*}}) : (tuple<>) -> (i64, i1)
// CHECK: return %[[R]]#0, %[[R]]#1 : i64, i1
func.func @call_lowering_no_args_multi_result(
    %callee: !closure.static<"test3", [], () -> (i64, i1)>) -> (i64, i1) {
  %r0, %r1 = closure.call %callee()
    : !closure.static<"test3", [], () -> (i64, i1)>
  return %r0, %r1 : i64, i1
}

// -----
// Test 4: closure.call with captures + empty call operand list + multi-result
// CHECK-LABEL: func.func @call_lowering_with_captures_no_args_multi_result(
// CHECK: %[[R:.*]]:2 = call @__closure.test4(%arg0) : (tuple<i64>) -> (i64, i1)
// CHECK: return %[[R]]#0, %[[R]]#1 : i64, i1
func.func @call_lowering_with_captures_no_args_multi_result(
    %callee: !closure.static<"test4", [i64], () -> (i64, i1)>) -> (i64, i1) {
  %r0, %r1 = closure.call %callee()
    : !closure.static<"test4", [i64], () -> (i64, i1)>
  return %r0, %r1 : i64, i1
}

// -----
// Test 5: nested closure type (callee captures a closure value)
// CHECK-LABEL: func.func private @__closure.test5.outer(tuple<tuple<i64>>, i64) -> tuple<i64>
// CHECK-LABEL: func.func @call_lowering_with_nested_closure_capture(%{{.*}}: tuple<tuple<i64>>, %{{.*}}: i64) -> tuple<i64>
// CHECK: %[[R:.*]] = call @__closure.test5.outer(%{{.*}}, %{{.*}}) : (tuple<tuple<i64>>, i64) -> tuple<i64>
// CHECK: return %[[R]] : tuple<i64>
func.func @call_lowering_with_nested_closure_capture(
    %outer: !closure.static<"test5.outer",
                            [!closure.static<"test5.inner", [i64], (i64) -> i64>],
                            (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>,
    %x: i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64> {
  %inner = closure.call %outer(%x)
    : !closure.static<"test5.outer",
                      [!closure.static<"test5.inner", [i64], (i64) -> i64>],
                      (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>
  return %inner : !closure.static<"test5.inner", [i64], (i64) -> i64>
}

// -----
// Test 6: No captures, one arg, zero results
// CHECK-LABEL: func.func private @__closure.test6(tuple<>, i64)
// CHECK-LABEL: func.func @call_lowering_no_results(%arg0: tuple<>, %arg1: i64)
// CHECK: call @__closure.test6(%arg0, %arg1) : (tuple<>, i64) -> ()
module {
  func.func @call_lowering_no_results(
      %arg0: !closure.static<"test6", [], (i64) -> ()>,
      %arg1: i64) {
    closure.call %arg0(%arg1) : !closure.static<"test6", [], (i64) -> ()>
    return
  }
}
