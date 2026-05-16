ifndef MLIR_SYS_220_PREFIX
$(error MLIR_SYS_220_PREFIX must be set; point it at your LLVM/MLIR 22 install)
endif

LLVM_BIN_PATH := $(MLIR_SYS_220_PREFIX)/bin

LLVM_CONFIG := $(LLVM_BIN_PATH)/llvm-config
TBLGEN := $(LLVM_BIN_PATH)/mlir-tblgen
OPT := $(LLVM_BIN_PATH)/mlir-opt

# Compiler flags
CXX := clang++
CXXFLAGS := -Wall -Werror -g -fPIC `$(LLVM_CONFIG) --cxxflags`

# LLVM/MLIR libraries
MLIR_INCLUDE := $(MLIR_SYS_220_PREFIX)/include

# Trait dialect include
TRAIT_DIALECT_INCLUDE = ../../mlir-trait-dialect/cpp

# Tuple dialect include
TUPLE_DIALECT_INCLUDE = ../../mlir-tuple-dialect/cpp

INCLUDES := -I $(MLIR_INCLUDE) -I $(TRAIT_DIALECT_INCLUDE) -I $(TUPLE_DIALECT_INCLUDE)

# Dialect library sources
DIALECT_SOURCES := c_api.cpp Closure.cpp ClosureOps.cpp ClosureTypes.cpp ConvertToFuncAndTuple.cpp
DIALECT_OBJECTS := $(DIALECT_SOURCES:.cpp=.o)

# Generated files
GENERATED := Closure.hpp.inc Closure.cpp.inc ClosureOps.hpp.inc ClosureOps.cpp.inc ClosureTypes.hpp.inc ClosureTypes.cpp.inc

.PHONY: all clean

all: libclosure_dialect.a libclosure_dialect.so

# TableGen rules
Closure.hpp.inc: Closure.td
	$(TBLGEN) --gen-dialect-decls $(INCLUDES) $< -o $@

Closure.cpp.inc: Closure.td
	$(TBLGEN) --gen-dialect-defs $(INCLUDES) $< -o $@

ClosureOps.hpp.inc: ClosureOps.td
	$(TBLGEN) --gen-op-decls $(INCLUDES) $< -o $@

ClosureOps.cpp.inc: ClosureOps.td
	$(TBLGEN) --gen-op-defs $(INCLUDES) $< -o $@

ClosureTypes.hpp.inc: ClosureTypes.td
	$(TBLGEN) --gen-typedef-decls $(INCLUDES) $< -o $@

ClosureTypes.cpp.inc: ClosureTypes.td
	$(TBLGEN) --gen-typedef-defs $(INCLUDES) $< -o $@

# Object file rules
%.o: %.cpp $(GENERATED)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

libclosure_dialect.a: $(DIALECT_OBJECTS)
	ar rcs $@ $^

PLUGIN_OBJECTS := $(DIALECT_OBJECTS) Plugin.o

libclosure_dialect.so: $(PLUGIN_OBJECTS)
	$(CXX) -shared $^ -o $@

.PHONY: test
test: libclosure_dialect.so
	@echo "Running closure dialect tests..."
	lit tests

clean:
	rm -f *.o *.inc *.a *.so

