add_llvm_loadable_module( LLVMPrism
  PrismLLVMInstrument.cpp

  DEPENDS
  intrinsics_gen

  PLUGIN_TOOL
  opt
  )
