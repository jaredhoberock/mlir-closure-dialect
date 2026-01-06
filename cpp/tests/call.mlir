// RUN: mlir-opt %s | FileCheck %s

// -----
// Test 1: No captures, no arguments, one result
// CHECK-LABEL: func.func @call_simple(
func.func @call_simple(
    %arg0: !closure.static<"test1", [], (i64, i1) -> i32>,
    %arg1: i64,
    %arg2: i1) -> i32 {
  // CHECK: closure.call %arg0(%arg1, %arg2) : !closure.static<"test1", [], (i64, i1) -> i32>
  %res = closure.call %arg0(%arg1, %arg2) : !closure.static<"test1", [], (i64, i1) -> i32>
  return %res : i32
}

// -----
// Test 2: closure.call with captures in the callee type
// CHECK-LABEL: func.func @call_with_captures(
func.func @call_with_captures(
    %arg0: !closure.static<"test2", [i64, i1], (i32, i32) -> (i64, i32)>,
    %arg1: i32,
    %arg2: i32) -> (i64, i32) {
  // CHECK: closure.call %arg0(%arg1, %arg2) : !closure.static<"test2", [i64, i1], (i32, i32) -> (i64, i32)>
  %r0, %r1 = closure.call %arg0(%arg1, %arg2) : !closure.static<"test2", [i64, i1], (i32, i32) -> (i64, i32)>
  return %r0, %r1 : i64, i32
}

// -----
// Test 3: No captures, no call operands, multi-result
// CHECK-LABEL: func.func @call_no_args_multi_result(
func.func @call_no_args_multi_result(
    %arg0: !closure.static<"test3", [], () -> (i64, i1)>) -> (i64, i1) {
  // CHECK: closure.call %arg0() : !closure.static<"test3", [], () -> (i64, i1)>
  %r0, %r1 = closure.call %arg0() : !closure.static<"test3", [], () -> (i64, i1)>
  return %r0, %r1 : i64, i1
}

// -----
// Test 4: closure.call with captures + empty call operand list + multi-result
// CHECK-LABEL: func.func @call_with_captures_no_args_multi_result(
func.func @call_with_captures_no_args_multi_result(
    %arg0: !closure.static<"test4", [i64], () -> (i64, i1)>) -> (i64, i1) {
  // CHECK: closure.call %arg0() : !closure.static<"test4", [i64], () -> (i64, i1)>
  %r0, %r1 = closure.call %arg0() : !closure.static<"test4", [i64], () -> (i64, i1)>
  return %r0, %r1 : i64, i1
}

// -----
// Test 5: nested closure type (callee captures a closure value)
// CHECK-LABEL: func.func @call_with_nested_closure_capture(
func.func @call_with_nested_closure_capture(
    %outer: !closure.static<"test5.outer",
                            [!closure.static<"test5.inner", [i64], (i64) -> i64>],
                            (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>,
    %x: i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64> {
  // CHECK: closure.call %{{.*}}(%{{.*}}) : !closure.static<"test5.outer", [!closure.static<"test5.inner", [i64], (i64) -> i64>], (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>
  %inner = closure.call %outer(%x)
    : !closure.static<"test5.outer",
                      [!closure.static<"test5.inner", [i64], (i64) -> i64>],
                      (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>
  return %inner : !closure.static<"test5.inner", [i64], (i64) -> i64>
}

// -----
// Test 6: zero-result call (no SSA results)
// CHECK-LABEL: func.func @call_no_results(
func.func @call_no_results(
    %arg0: !closure.static<"test6", [], (i64) -> ()>,
    %arg1: i64) {
  // CHECK: closure.call %arg0(%arg1) : !closure.static<"test6", [], (i64) -> ()>
  closure.call %arg0(%arg1) : !closure.static<"test6", [], (i64) -> ()>
  return
}
