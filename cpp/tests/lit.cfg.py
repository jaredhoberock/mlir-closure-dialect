import os
import lit.formats

config.name = "Closure Dialect Tests"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.mlir']
config.test_source_root = os.path.dirname(__file__)

closure_plugin_path = os.path.join(os.path.dirname(__file__), '..', 'libclosure_dialect.so')
tuple_plugin_path = os.path.join(os.path.dirname(__file__), '/home/jhoberock/dev/git/mlir-tuple-dialect/cpp', 'libtuple_dialect.so')
trait_plugin_path = os.path.join(os.path.dirname(__file__), '/home/jhoberock/dev/git/mlir-trait-dialect/cpp', 'libtrait_dialect.so')

llvm_bin = os.path.join(os.path.expanduser("~"), "dev/git/llvm-project-22/build/bin")
mlir_opt = os.path.join(llvm_bin, "mlir-opt")
config.substitutions.append(('mlir-opt', f'{mlir_opt} --load-dialect-plugin={trait_plugin_path} --load-dialect-plugin={tuple_plugin_path} --load-dialect-plugin={closure_plugin_path}'))
