#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include  <iostream>
#include  <unordered_set>
#include  <string>


using namespace llvm;

namespace {

struct HW1Pass : public PassInfoMixin<HW1Pass> {
  std::unordered_set<std::string> branch{"br", "switch", "indirectbr"};
  std::unordered_set<std::string> integer{"add", "sub", "mul", "udiv", "sdiv", "urem", "shl", "lshr", "ashr", "and", "or", "xor", "icmp", "srem"};
  std::unordered_set<std::string> floating_point{"fadd", "fsub", "fmul", "fdiv", "frem", "fcmp"};
  std::unordered_set<std::string> memory{"alloca", "load", "store", "getelementptr", "fence", "atomiccmpxchg", "atomicrmw"};

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    // These variables contain the results of some analysis passes.
    // Go through the documentation to see how they can be used.
    llvm::BlockFrequencyAnalysis::Result &bfi = FAM.getResult<BlockFrequencyAnalysis>(F);
    llvm::BranchProbabilityAnalysis::Result &bpi = FAM.getResult<BranchProbabilityAnalysis>(F);

    double num_insts = 0;
    double counts[6] = {0}; // integer, float, mem, biased_br, unbiased_br, other

    for(auto it = F.begin(); it != F.end(); it++){ // through basic blocks
      double freq = bfi.getBlockProfileCount(&(*it)).value_or(0);

      for(auto inst=it->begin(); inst != it->end(); inst++){ // through instructions
        num_insts += freq;
        auto opcode = inst->getOpcodeName();
        if(branch.find(opcode) != branch.end()){
          bool biased = false;
          for(int i=0;i<inst->getNumSuccessors();i++){
            auto p = bpi.getEdgeProbability(&(*it), i);
            biased = (p.getNumerator()/(double)p.getDenominator()) > .8;
            if(biased)
              break;
          }
          if(biased)
            counts[3] += freq;
          else
            counts[4] += freq;
        }else{
          if(integer.find(opcode) != integer.end()){
            counts[0] += freq;
          }else if(floating_point.find(opcode) != floating_point.end()){
            counts[1] += freq;
          }else if(memory.find(opcode) != memory.end()){
            counts[2] += freq;
          }else{
            counts[5] += freq;
          }
        } 
      }
    }

    errs() << F.getName();
    errs() << ", " << (uint64_t)num_insts;
    for (int i=0;i<6;i++) { // iterate all categories
      if (num_insts == 0) errs() << ", " << format("%.3f", 0);
      else errs() << ", " << format("%.3f", counts[i]/num_insts);
    }
    errs() << "\n";

    return PreservedAnalyses::all();
  }
};

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "HW1Pass", "v0.1",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
        ArrayRef<PassBuilder::PipelineElement>) {
          if(Name == "hw1"){
            FPM.addPass(HW1Pass());
            return true;
          }
          return false;
        }
      );
    }
  };
}
}
