//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include "PrismLLVMInstrument.hpp"
#include "PrismLLVMAppIface.hpp"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Instrumentation.h"


namespace {
/// Prism: instrument the code to generate Prism events
struct Prism : public llvm::ModulePass {
  Prism() : ModulePass(ID) {}
  llvm::StringRef getPassName() const override;
  bool runOnModule(llvm::Module &M) override;

  static char ID; // Pass identification, replacement for typeid.
};

/// GlobalValues expected to be linked into main module.
llvm::Function *PrismCurEvFunction(llvm::Module &M) {
  llvm::LLVMContext &cxt = M.getContext();
  // Create an external declaration in the given module
  return llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getPointerElementType(cxt),
                                                        std::vector<llvm::Type*>(),
                                                        false),
                                llvm::Function::ExternalLinkage,
                                "prism_cur_ev_get",
                                &M);
}
} // end namespace.

llvm::StringRef Prism::getPassName() const {
  return "Prism";
}


bool Prism::runOnModule(llvm::Module &M) {
  bool modified = false;

  llvm::LLVMContext &cxt = M.getContext();
  llvm::IRBuilder<> Builder(cxt);

  // Insert instrumentation for each event type.
  for (llvm::Function &f : M.functions()) {
    for (llvm::BasicBlock &bb : f.getBasicBlockList()) {
      // Create a new basic block for sending event data to Prism.
      // The new basic block will always execute before original
      // applicaton basic blocks.
      auto newbb = llvm::BasicBlock::Create(cxt, "", &f, &bb);

      { // load thread local channel info
      }
      
      for (llvm::Instruction &instr : bb) {
        if (instr.getOpcode() == llvm::Instruction::Add) {
          // example checking for an add
        }
        // group instrumentation per basic block
      }
    }
  }

  return modified;
}

char Prism::ID = 0;
static llvm::RegisterPass<Prism> X("prism", "Prism Event Generation Pass");
