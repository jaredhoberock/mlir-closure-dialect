ifndef MLIR_SYS_220_PREFIX
$(error MLIR_SYS_220_PREFIX must be set; point it at your LLVM/MLIR 22 install)
endif

BUILD_DIR := build
TUPLE_DIALECT_SOURCE_DIR := ../../mlir-tuple-dialect/cpp
TUPLE_DIALECT_INCLUDE_DIR := ../../mlir-tuple-dialect/cpp/build
TRAIT_DIALECT_SOURCE_DIR := ../../mlir-trait-dialect/cpp
TRAIT_DIALECT_INCLUDE_DIR := ../../mlir-trait-dialect/cpp/build
LLVM_BIN_PATH := $(MLIR_SYS_220_PREFIX)/bin
LLVM_CONFIG := $(LLVM_BIN_PATH)/llvm-config
TBLGEN := $(LLVM_BIN_PATH)/mlir-tblgen

CXX := clang++
CXXFLAGS := -Wall -Werror -fPIC $(shell $(LLVM_CONFIG) --cxxflags)
INCLUDES := -I$(BUILD_DIR) -I. -I$(MLIR_SYS_220_PREFIX)/include -I$(TUPLE_DIALECT_SOURCE_DIR) -I$(TUPLE_DIALECT_INCLUDE_DIR) -I$(TRAIT_DIALECT_SOURCE_DIR) -I$(TRAIT_DIALECT_INCLUDE_DIR)

DIALECT_SOURCES := c_api.cpp Closure.cpp ClosureOps.cpp ClosureTypes.cpp ConvertToFuncAndTuple.cpp
DIALECT_OBJECTS := $(addprefix $(BUILD_DIR)/,$(DIALECT_SOURCES:.cpp=.o))
PLUGIN_OBJECTS := $(DIALECT_OBJECTS) $(BUILD_DIR)/Plugin.o
TD_FILES := $(wildcard *.td)

GENERATED := \
  $(BUILD_DIR)/Closure.hpp.inc \
  $(BUILD_DIR)/Closure.cpp.inc \
  $(BUILD_DIR)/ClosureOps.hpp.inc \
  $(BUILD_DIR)/ClosureOps.cpp.inc \
  $(BUILD_DIR)/ClosureTypes.hpp.inc \
  $(BUILD_DIR)/ClosureTypes.cpp.inc

.PHONY: all clean test

all: $(BUILD_DIR)/libclosure_dialect.a $(BUILD_DIR)/libclosure_dialect.so

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/Closure.hpp.inc: Closure.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-dialect-decls $(INCLUDES) $< -o $@

$(BUILD_DIR)/Closure.cpp.inc: Closure.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-dialect-defs $(INCLUDES) $< -o $@

$(BUILD_DIR)/ClosureOps.hpp.inc: ClosureOps.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-op-decls $(INCLUDES) $< -o $@

$(BUILD_DIR)/ClosureOps.cpp.inc: ClosureOps.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-op-defs $(INCLUDES) $< -o $@

$(BUILD_DIR)/ClosureTypes.hpp.inc: ClosureTypes.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-typedef-decls $(INCLUDES) $< -o $@

$(BUILD_DIR)/ClosureTypes.cpp.inc: ClosureTypes.td $(TD_FILES) | $(BUILD_DIR)
	$(TBLGEN) --gen-typedef-defs $(INCLUDES) $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(GENERATED) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/libclosure_dialect.a: $(DIALECT_OBJECTS)
	ar rcs $@ $^

$(BUILD_DIR)/libclosure_dialect.so: $(PLUGIN_OBJECTS)
	$(CXX) -shared $^ -o $@

test: $(BUILD_DIR)/libclosure_dialect.so
	CLOSURE_DIALECT_PLUGIN=$(CURDIR)/$(BUILD_DIR)/libclosure_dialect.so lit tests

clean:
	rm -rf $(BUILD_DIR)
