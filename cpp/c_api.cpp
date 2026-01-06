#include "c_api.h"
#include "Closure.hpp"
#include "ClosureOps.hpp"
#include "ClosureTypes.hpp"
#include <mlir/CAPI/IR.h>
#include <mlir/CAPI/Pass.h>
#include <mlir/IR/Builders.h>

using namespace mlir;
using namespace mlir::closure;

extern "C" {

void closureRegisterDialect(MlirContext context) {
  unwrap(context)->loadDialect<ClosureDialect>();
}

} // end extern "C"
