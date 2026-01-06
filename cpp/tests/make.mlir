// RUN: mlir-opt %s | FileCheck %s

// ---- Test 1: Two captures, single-arg, single-result ----
// CHECK-LABEL: func @make_two_captures
// CHECK: %[[LAMBDA:.+]] = closure.make "test1" [%[[C0:.+]] = %arg0 : i64, %[[C1:.+]] = %arg1 : i1] (%[[X:.+]]: i64) -> i64 {
// CHECK:   return %[[C0]] : i64
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test1", [i64, i1], (i64) -> i64>
func.func @make_two_captures(%arg0: i64, %arg1: i1) -> !closure.static<"test1", [i64, i1], (i64) -> i64> {
  %lambda = closure.make "test1" [
    %c0 = %arg0 : i64,
    %c1 = %arg1 : i1
  ] (%x: i64) -> i64 {
    return %c0 : i64
  }
  return %lambda : !closure.static<"test1", [i64, i1], (i64) -> i64>
}

// ---- Test 2: One capture, two-arg, multi-result ----
// CHECK-LABEL: func @make_multi_result
// CHECK: %[[LAMBDA:.+]] = closure.make "test2" [%[[C0:.+]] = %arg0 : i64] (%[[A:.+]]: i32, %[[B:.+]]: i32) -> (i64, i32) {
// CHECK:   return %[[C0]], %[[A]] : i64, i32
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test2", [i64], (i32, i32) -> (i64, i32)>
func.func @make_multi_result(%arg0: i64) -> !closure.static<"test2", [i64], (i32, i32) -> (i64, i32)> {
  %lambda = closure.make "test2" [
    %c0 = %arg0 : i64
  ] (%a: i32, %b: i32) -> (i64, i32) {
    return %c0, %a : i64, i32
  }
  return %lambda : !closure.static<"test2", [i64], (i32, i32) -> (i64, i32)>
}

// ---- Test 3: No captures, empty arg list, empty result list ----
// CHECK-LABEL: func @make_unit
// CHECK: %[[LAMBDA:.+]] = closure.make "test3" [] () -> () {
// CHECK:   return
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test3", [], () -> ()>
func.func @make_unit() -> !closure.static<"test3", [], () -> ()> {
  %lambda = closure.make "test3" [] () -> () {
    return
  }
  return %lambda : !closure.static<"test3", [], () -> ()>
}

// ---- Test 4: One capture, empty call arg list, multi-result ----
// CHECK-LABEL: func @make_capture_no_args_multi_result
// CHECK: %[[LAMBDA:.+]] = closure.make "test4" [%[[C0:.+]] = %arg0 : i64] () -> (i64, i1) {
// CHECK:   return %[[C0]], %[[C1:.+]] : i64, i1
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test4", [i64], () -> (i64, i1)>
func.func @make_capture_no_args_multi_result(%arg0: i64) -> !closure.static<"test4", [i64], () -> (i64, i1)> {
  %lambda = closure.make "test4" [
    %c0 = %arg0 : i64
  ] () -> (i64, i1) {
    %c1 = arith.constant 1 : i1
    return %c0, %c1 : i64, i1
  }
  return %lambda : !closure.static<"test4", [i64], () -> (i64, i1)>
}

// ---- Test 5: Nested closure (closure captured as a value) ----
// CHECK-LABEL: func @make_nested_closure
// CHECK: %[[INNER:.+]] = closure.make "test5.inner" [%[[IC0:.+]] = %arg0 : i64] (%[[IX:.+]]: i64) -> i64 {
// CHECK:   return %[[IC0]] : i64
// CHECK: }
// CHECK: %[[OUTER:.+]] = closure.make "test5.outer" [%[[OC0:.+]] = %[[INNER]] : !closure.static<"test5.inner", [i64], (i64) -> i64>] (%[[OX:.+]]: i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64> {
// CHECK:   return %[[OC0]] : !closure.static<"test5.inner", [i64], (i64) -> i64>
// CHECK: }
// CHECK: return %[[OUTER]] : !closure.static<"test5.outer", [!closure.static<"test5.inner", [i64], (i64) -> i64>], (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>
func.func @make_nested_closure(%arg0: i64)
  -> !closure.static<"test5.outer", [!closure.static<"test5.inner", [i64], (i64) -> i64>], (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>> {

  %inner = closure.make "test5.inner" [
    %c0 = %arg0 : i64
  ] (%x: i64) -> i64 {
    return %c0 : i64
  }

  %outer = closure.make "test5.outer" [
    %c1 = %inner : !closure.static<"test5.inner", [i64], (i64) -> i64>
  ] (%y: i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64> {
    return %c1 : !closure.static<"test5.inner", [i64], (i64) -> i64>
  }

  return %outer
    : !closure.static<"test5.outer", [!closure.static<"test5.inner", [i64], (i64) -> i64>], (i64) -> !closure.static<"test5.inner", [i64], (i64) -> i64>>
}

// ---- Test 6: Capturing multiple values and returning them in a different order ----
// CHECK-LABEL: func @make_reorder_captures
// CHECK: %[[LAMBDA:.+]] = closure.make "test6" [%[[C0:.+]] = %arg0 : i32, %[[C1:.+]] = %arg1 : i64, %[[C2:.+]] = %arg2 : i1] (%[[X:.+]]: i64) -> (i1, i32, i64) {
// CHECK:   return %[[C2]], %[[C0]], %[[C1]] : i1, i32, i64
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test6", [i32, i64, i1], (i64) -> (i1, i32, i64)>
func.func @make_reorder_captures(%arg0: i32, %arg1: i64, %arg2: i1)
  -> !closure.static<"test6", [i32, i64, i1], (i64) -> (i1, i32, i64)> {

  %lambda = closure.make "test6" [
    %c0 = %arg0 : i32,
    %c1 = %arg1 : i64,
    %c2 = %arg2 : i1
  ] (%x: i64) -> (i1, i32, i64) {
    return %c2, %c0, %c1 : i1, i32, i64
  }

  return %lambda : !closure.static<"test6", [i32, i64, i1], (i64) -> (i1, i32, i64)>
}

// ---- Test 7: Multi-block body with multiple exits (cf.cond_br) ----
// CHECK-LABEL: func @make_multi_block_two_returns
// CHECK: %[[LAMBDA:.+]] = closure.make "test7" [%[[CAP:.+]] = %arg0 : i64] (%[[PRED:.+]]: i1) -> i64 {
// CHECK:   cf.cond_br %[[PRED]], ^bb1, ^bb2
// CHECK: ^bb1:
// CHECK:   return %[[CAP]] : i64
// CHECK: ^bb2:
// CHECK:   return %[[CAP]] : i64
// CHECK: }
// CHECK: return %[[LAMBDA]] : !closure.static<"test7", [i64], (i1) -> i64>
func.func @make_multi_block_two_returns(%arg0: i64) -> !closure.static<"test7", [i64], (i1) -> i64> {
  %lambda = closure.make "test7" [
    %cap = %arg0 : i64
  ] (%pred: i1) -> i64 {
    cf.cond_br %pred, ^bb1, ^bb2

  ^bb1:
    return %cap : i64

  ^bb2:
    return %cap : i64
  }
  return %lambda : !closure.static<"test7", [i64], (i1) -> i64>
}
