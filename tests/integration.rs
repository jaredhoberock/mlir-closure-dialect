use melior::{
    Context,
    dialect::{arith, func, DialectRegistry},
    ExecutionEngine,
    ir::{
        attribute::{IntegerAttribute, StringAttribute, TypeAttribute},
        Attribute,
        r#type::{FunctionType, IntegerType},
        Block, BlockLike, Location, Module, Operation, Region, RegionLike,
    },
    pass::{self, PassManager},
    utility::{register_all_dialects},
};
use closure_dialect as closure;

fn build_test_func<'c>(
    context: &'c Context,
    loc: Location<'c>
) -> Operation<'c> {
    // build a func.func @test:
    //
    // func.func @test() -> i1 {
    //   %res = arith.constant 1 : i1
    //   return %res : i1
    // }

    // build the function body
    let i1_ty = IntegerType::new(context, 1).into();
    let region = {
        let block = Block::new(&[]);
        let res = block.append_operation(arith::constant(
            context,
            IntegerAttribute::new(i1_ty, 1).into(),
            loc,
        )).result(0).unwrap().into();

        block.append_operation(func::r#return(
            &[res],
            loc,
        ));

        let region = Region::new();
        region.append_block(block);
        region
    };

    // build the function
    let function_type = FunctionType::new(
        &context,
        &[],
        &[IntegerType::new(&context, 1).into()]
    );

    let mut func_op = func::func(
        &context,
        StringAttribute::new(&context, "test"),
        TypeAttribute::new(function_type.into()),
        region,
        &[],
        loc,
    );
    func_op.set_attribute("llvm.emit_c_interface", Attribute::unit(&context));
    func_op
}

#[test]
fn test_closure_jit() {
    // create a dialect registry and register all dialects
    let registry = DialectRegistry::new();
    register_all_dialects(&registry);
    let context = Context::new();
    context.append_dialect_registry(&registry);
    closure::register(&context);

    // make all the dialects available
    context.load_all_available_dialects();

    // create a module
    let loc = Location::unknown(&context);
    let mut module = Module::new(loc);

    // build two functions @test1 and @test2
    module.body().append_operation(
        build_test_func(&context, loc)
    );

    assert!(module.as_operation().verify(), "MLIR module verification failed");

    // Lower to LLVM
    let pass_manager = PassManager::new(&context);
    pass_manager.add_pass(pass::conversion::create_to_llvm());
    assert!(pass_manager.run(&mut module).is_ok());

    // JIT compile the module
    let engine = ExecutionEngine::new(&module, 0, &[], false);

    // test
    unsafe {
        let mut result: bool = false;
        let mut packed_args: [*mut (); 1] = [
            &mut result as *mut bool as *mut ()
        ];

        engine.invoke_packed("test", &mut packed_args)
            .expect("test1 JIT invocation failed");

        assert_eq!(result, true);
    }
}
