use melior::Context;
use mlir_sys::MlirContext;

unsafe extern "C" {
    fn closureRegisterDialect(ctx: MlirContext);
}

pub fn register(context: &Context) {
    unsafe { closureRegisterDialect(context.to_raw()) }
}
