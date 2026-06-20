import os
import lit.formats

config.name = "Closure Dialect Tests"
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.mlir']
config.test_source_root = os.path.dirname(__file__)

mlir_prefix = os.environ.get('MLIR_SYS_220_PREFIX', '/home/jhoberock/dev/git/llvm-project-22/install-release-asserts')
llvm_bin = os.path.join(mlir_prefix, 'bin')
fallback_llvm_bin = '/home/jhoberock/dev/git/llvm-project-22/build/bin'

def tool(name):
    installed = os.path.join(llvm_bin, name)
    if os.path.exists(installed):
        return installed
    return os.path.join(fallback_llvm_bin, name)

trait_plugin = os.environ.get(
    'TRAIT_DIALECT_PLUGIN',
    '/home/jhoberock/dev/git/mlir-trait-dialect/cpp/build/libtrait_dialect.so',
)
tuple_plugin = os.environ.get(
    'TUPLE_DIALECT_PLUGIN',
    '/home/jhoberock/dev/git/mlir-tuple-dialect/cpp/build/libtuple_dialect.so',
)
closure_plugin = os.environ.get(
    'CLOSURE_DIALECT_PLUGIN',
    os.path.join(os.path.dirname(__file__), '..', 'build', 'libclosure_dialect.so'),
)

plugins = f'--load-dialect-plugin={trait_plugin} --load-dialect-plugin={tuple_plugin} --load-dialect-plugin={closure_plugin}'
config.substitutions.append(('mlir-opt', f'{tool("mlir-opt")} {plugins}'))
config.substitutions.append(('FileCheck', tool('FileCheck')))
