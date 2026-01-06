#include "Closure.hpp"
#include "ConvertToFuncAndTuple.hpp"
#include <mlir/Pass/PassManager.h>
#include <mlir/Tools/Plugins/DialectPlugin.h>
#include <mlir/Tools/Plugins/PassPlugin.h>

static void registerPlugin(mlir::DialectRegistry* registry) {
  registry->insert<mlir::closure::ClosureDialect>();
  ::mlir::PassRegistration<::mlir::closure::ConvertClosureToFuncAndTuplePass>();
}

extern "C" ::mlir::DialectPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
mlirGetDialectPluginInfo() {
  return {
    MLIR_PLUGIN_API_VERSION,
    "ClosureDialectPlugin", 
    "v0.1", 
    registerPlugin
  };
}
