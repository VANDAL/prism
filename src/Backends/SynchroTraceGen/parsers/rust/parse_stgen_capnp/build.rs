extern crate capnpc;

fn main() {
    let schema_dir = "../../../STEventTraceSchemas";
	::capnpc::CompilerCommand::new()
        .src_prefix(schema_dir)
		.file(format!("{}{}", schema_dir, "/STEventTraceUncompressed.capnp"))
		.file(format!("{}{}", schema_dir, "/STEventTraceCompressed.capnp"))
		.run()
		.expect("compiling schema");
}
