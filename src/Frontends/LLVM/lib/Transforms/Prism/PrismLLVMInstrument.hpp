#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/Format.h"

inline void insertBeforeMain(llvm::Function &f) {
  if (f.getName() == "main") {
    llvm::format("main found!\n");
  }
}

inline void insertPrismEvents(llvm::BasicBlock &bb) {
}
