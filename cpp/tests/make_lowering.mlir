// RUN: mlir-opt -split-input-file --pass-pipeline="builtin.module(convert-closure-to-func-and-tuple)" %s | FileCheck %s

// -----
// Test 1: Two captures, single-arg, single-result
// CHECK-LABEL: func.func private @__closure.test1(%{{.*}}: tuple<i64, i1>, %{{.*}}: i64) -> i64
// CHECK: tuple.get {{.*}}, 0 : tuple<i64, i1> -> i64
// CHECK: return {{.*}} : i64
// CHECK-LABEL: func.func @make_two_captures(%{{.*}}: i64, %{{.*}}: i1) -> tuple<i64, i1>
// CHECK: tuple.make
// CHECK: return
func.func @make_two_captures(%arg0: i64, %arg1: i1) -> !closure.static<"test1", [i64, i1], (i64) -> i64> {
  %lambda = closure.make "test1" [
    %c0 = %arg0 : i64,
    %c1 = %arg1 : i1
  ] (%x: i64) -> i64 {
    return %c0 : i64
  }
  return %lambda : !closure.static<"test1", [i64, i1], (i64) -> i64>
}

// -----
// Test 2: One capture, two-arg, multi-result
// CHECK-LABEL: func.func private @__closure.test2(%{{.*}}: tuple<i64>, %{{.*}}: i32, %{{.*}}: i32) -> (i64, i32)
// CHECK: %{{.*}} = tuple.get %{{.*}}, 0 : tuple<i64> -> i64
// CHECK: return %{{.*}}, %{{.*}} : i64, i32
// CHECK-LABEL: func.func @make_multi_result(%{{.*}}: i64) -> tuple<i64>
// CHECK: tuple.make
// CHECK: return
func.func @make_multi_result(%arg0: i64) -> !closure.static<"test2", [i64], (i32, i32) -> (i64, i32)> {
  %lambda = closure.make "test2" [
    %c0 = %arg0 : i64
  ] (%a: i32, %b: i32) -> (i64, i32) {
    return %c0, %a : i64, i32
  }
  return %lambda : !closure.static<"test2", [i64], (i32, i32) -> (i64, i32)>
}

// -----
// Test 3: No captures, empty arg list, empty result list
// CHECK-LABEL: func.func private @__closure.test3(%{{.*}}: tuple<>) {
// CHECK: return
// CHECK-LABEL: func.func @make_unit() -> tuple<>
// CHECK: tuple.make : tuple<>
// CHECK: return %{{.*}} : tuple<>
func.func @make_unit() -> !closure.static<"test3", [], () -> ()> {
  %lambda = closure.make "test3" [] () -> () {
    return
  }
  return %lambda : !closure.static<"test3", [], () -> ()>
}

// -----
// Test 4: One capture, empty call arg list, multi-result
// CHECK-LABEL: func.func private @__closure.test4(%{{.*}}: tuple<i64>) -> (i64, i1)
// CHECK: %{{.*}} = tuple.get %{{.*}}, 0 : tuple<i64> -> i64
// CHECK: %{{.*}} = arith.constant true
// CHECK: return %{{.*}}, %{{.*}} : i64, i1
// CHECK-LABEL: func.func @make_capture_no_args_multi_result(%{{.*}}: i64) -> tuple<i64>
// CHECK: %{{.*}} = tuple.make(%{{.*}} : i64) : tuple<i64>
// CHECK: return %{{.*}} : tuple<i64>
func.func @make_capture_no_args_multi_result(%arg0: i64) -> !closure.static<"test4", [i64], () -> (i64, i1)> {
  %lambda = closure.make "test4" [
    %c0 = %arg0 : i64
  ] () -> (i64, i1) {
    %c1 = arith.constant 1 : i1
    return %c0, %c1 : i64, i1
  }
  return %lambda : !closure.static<"test4", [i64], () -> (i64, i1)>
}

// -----
// Test 5: Nested closure (closure captured as a value)
// CHECK-LABEL: func.func private @__closure.test5.outer(%{{.*}}: tuple<tuple<i64>>, %{{.*}}: i64) -> tuple<i64>
// CHECK:       tuple.get {{.*}}, 0 : tuple<tuple<i64>> -> tuple<i64>
// CHECK:       return {{.*}} : tuple<i64>

// CHECK-LABEL: func.func private @__closure.test5.inner(%{{.*}}: tuple<i64>, %{{.*}}: i64) -> i64
// CHECK:       tuple.get {{.*}}, 0 : tuple<i64> -> i64
// CHECK:       return {{.*}} : i64

// CHECK-LABEL: func.func @make_nested_closure(%{{.*}}: i64) -> tuple<tuple<i64>>
// CHECK:       %{{.*}} = tuple.make(%{{.*}} : i64) : tuple<i64>
// CHECK:       %{{.*}} = tuple.make(%{{.*}} : tuple<i64>) : tuple<tuple<i64>>
// CHECK:       return {{.*}} : tuple<tuple<i64>>
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

// -----
// Test 6: Capturing multiple values and returning them in a different order
// CHECK-LABEL: func.func private @__closure.test6(%{{.*}}: tuple<i32, i64, i1>, %{{.*}}: i64) -> (i1, i32, i64)
// CHECK: %[[C0:.*]] = tuple.get %{{.*}}, 0 : tuple<i32, i64, i1> -> i32
// CHECK: %[[C1:.*]] = tuple.get %{{.*}}, 1 : tuple<i32, i64, i1> -> i64
// CHECK: %[[C2:.*]] = tuple.get %{{.*}}, 2 : tuple<i32, i64, i1> -> i1
// CHECK: return %[[C2]], %[[C0]], %[[C1]] : i1, i32, i64
// CHECK-LABEL: func.func @make_reorder_captures(%{{.*}}: i32, %{{.*}}: i64, %{{.*}}: i1) -> tuple<i32, i64, i1>
// CHECK: tuple.make
// CHECK: return
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

// -----
// Test 7: Multi-block body with multiple exits (cf.cond_br)
// CHECK-LABEL: func.func private @__closure.test7(%{{.*}}: tuple<i64>, %{{.*}}: i1) -> i64
// CHECK: %[[CAP:.*]] = tuple.get %{{.*}}, 0 : tuple<i64> -> i64
// CHECK: cf.cond_br %{{.*}}, ^[[BB1:.*]], ^[[BB2:.*]]
// CHECK: ^[[BB1]]:
// CHECK: return %[[CAP]] : i64
// CHECK: ^[[BB2]]:
// CHECK: return %[[CAP]] : i64
// CHECK-LABEL: func.func @make_multi_block_two_returns(%{{.*}}: i64) -> tuple<i64>
// CHECK: tuple.make
// CHECK: return
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
