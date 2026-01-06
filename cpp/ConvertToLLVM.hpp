#pragma once

namespace mlir {

class LLVMTypeConverter;
class RewritePatternSet;

namespace closure {

void populateClosureToLLVMConversionPatterns(LLVMTypeConverter& typeConverter,
                                             RewritePatternSet& patterns);
}
}
