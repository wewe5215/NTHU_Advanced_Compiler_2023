#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include <map>
#include  <stdio.h>
#include  <math.h>
#define DEBUG_MODE 1
using namespace llvm;
struct SolEquStru {
  int   is_there_solution;
  int   X0;
  int   XCoeffT;
  int   Y0;
  int   YCoeffT;
}  ;

struct TriStru{
  int   gcd;
  int   x;
  int   y;
} ;
int  Min(int a , int b)
{
 if (a <= b) {return a;}
 else {return b;}
}
int  Max(int a, int b)
{
 if (a >= b) {return a;}
 else {return b;}
}
using ValueVector = SmallVector<Value *, 16>;
std::map<Instruction*, int> instructionToLineNumberMap;
std::map<Instruction*, std::pair<int, int>> arrayIdx;
std::map<Instruction*, std::string> Arr_name;
std::map<std::string, std::pair<SolEquStru*, std::pair<int, int>>> flow_dep;
std::map<std::string, std::pair<SolEquStru*, std::pair<int, int>>> anti_dep;
std::map<std::string, std::pair<SolEquStru*, std::pair<int, int>>> output_dep;

struct TriStru 
Extended_Euclid(int a ,int b)
{
  struct TriStru Tri1,Tri2;
  if (b==0) {
    Tri1.gcd=a;
    Tri1.x=1;
    Tri1.y=0;
    return Tri1;
    }
  Tri2=Extended_Euclid( b , (a%b));
  Tri1.gcd=Tri2.gcd;
  Tri1.x=Tri2.y;
  Tri1.y=Tri2.x-(a/b)*Tri2.y;
  return Tri1;
}

struct SolEquStru 
diophatine_solver(int a, int b, int c)
{
  int   k;
  struct TriStru   Triple;
  struct SolEquStru s;

   Triple=Extended_Euclid( a , b );
   if ((c%Triple.gcd)==0) {
	k=c/Triple.gcd;
	s.X0=Triple.x*k;
	s.XCoeffT=(b/Triple.gcd );
	s.Y0=Triple.y*k;
	s.YCoeffT=((-a)/Triple.gcd );
        s.is_there_solution=1;
      }
  else s.is_there_solution=0;

  return(s);
}
void processLoadInst(LoadInst* LI, StringRef* IndVal){
  auto arr = dyn_cast<Instruction>(dyn_cast<Instruction>(LI->getOperand(0))->getOperand(0));
  auto idx = dyn_cast<Instruction>(dyn_cast<Instruction>(LI->getOperand(0))->getOperand(2));
  Arr_name[dyn_cast<Instruction>(LI)] = std::string(arr->getName());
  if(idx->getOperand(0)->getName() == *IndVal){
    std::pair<int, int> mapping = std::make_pair(1, 0);
    arrayIdx[dyn_cast<Instruction>(LI)] = mapping;
  }
  else if(BinaryOperator *Op = dyn_cast<BinaryOperator>(idx->getOperand(0))){
      // Create a std::pair<int, int> with values X and Y
      // for index : Xi + Y
      std::pair<int, int> mapping;
      if (Op->getOpcode() == Instruction::Sub || Op->getOpcode() == Instruction::Add) {
          // This is a subtraction/addition operation
          ConstantInt *Cst = dyn_cast<ConstantInt>(Op->getOperand(1));
          if (Cst) {
              int X = 1;
              int Y = Cst->getValue().getSExtValue();
              if(Op->getOpcode() == Instruction::Sub){
                Y = Y * (-1);
              }
              
              if(Op->getOperand(0)->getName() != *IndVal){
                   if(BinaryOperator *Op_mul = dyn_cast<BinaryOperator>(Op->getOperand(0))){
                        if(Op_mul->getOpcode() == Instruction::Mul){
                            ConstantInt *Cst_Mul = dyn_cast<ConstantInt>(Op_mul->getOperand(0));
                            if(Cst_Mul){
                                X = Cst_Mul->getValue().getSExtValue();
                            }
                        }
                  } 
              }
              // Map the instruction to the pair
              mapping = std::make_pair(X, Y);
              arrayIdx[dyn_cast<Instruction>(LI)] = mapping;
          }
              
      }
      else if(Op->getOpcode() == Instruction::Mul){
          ConstantInt *Cst_Mul = dyn_cast<ConstantInt>(Op->getOperand(0));
          if(Cst_Mul){
              int X = Cst_Mul->getValue().getSExtValue();
              int Y = 0;
              mapping = std::make_pair(X, Y);
              arrayIdx[dyn_cast<Instruction>(LI)] = mapping;
          }

      }
  }
  return;
}

void processStoreInst(StoreInst* SI, StringRef* IndVal){
  auto arr = dyn_cast<Instruction>(dyn_cast<Instruction>(SI->getOperand(1))->getOperand(0));
  auto idx = dyn_cast<Instruction>(dyn_cast<Instruction>(SI->getOperand(1))->getOperand(2));
  Arr_name[dyn_cast<Instruction>(SI)] = std::string(arr->getName());
  if(idx->getOperand(0)->getName() == *IndVal){
    std::pair<int, int> mapping = std::make_pair(1, 0);
    arrayIdx[dyn_cast<Instruction>(SI)] = mapping;
  }
  else if(BinaryOperator *Op = dyn_cast<BinaryOperator>(idx->getOperand(0))){
      // Create a std::pair<int, int> with values X and Y
      // for index : Xi + Y
      std::pair<int, int> mapping;
      if (Op->getOpcode() == Instruction::Sub || Op->getOpcode() == Instruction::Add) {
          // This is a subtraction/addition operation
          ConstantInt *Cst = dyn_cast<ConstantInt>(Op->getOperand(1));
          if (Cst) {
              int X = 1;
              int Y = Cst->getValue().getSExtValue();
              if(Op->getOpcode() == Instruction::Sub){
                Y = Y * (-1);
              }
              if(Op->getOperand(0)->getName() != *IndVal){
                   if(BinaryOperator *Op_mul = dyn_cast<BinaryOperator>(Op->getOperand(0))){
                        if(Op_mul->getOpcode() == Instruction::Mul){
                            ConstantInt *Cst_Mul = dyn_cast<ConstantInt>(Op_mul->getOperand(0));
                            if(Cst_Mul){
                                X = Cst_Mul->getValue().getSExtValue();
                            }
                        }
                  } 
              }
              // Map the instruction to the pair
              mapping = std::make_pair(X, Y);
              arrayIdx[dyn_cast<Instruction>(SI)] = mapping;
          }
              
      }
      else if(Op->getOpcode() == Instruction::Mul){
          ConstantInt *Cst_Mul = dyn_cast<ConstantInt>(Op->getOperand(0));
          if(Cst_Mul){
              int X = Cst_Mul->getValue().getSExtValue();
              int Y = 0;
              mapping = std::make_pair(X, Y);
              arrayIdx[dyn_cast<Instruction>(SI)] = mapping;
          }

      }
  }
  return;
}


void processInst(BasicBlock* BB, ValueVector* MemInstr, StringRef* IndVal){
    int line_num = 1;
    for (Instruction &I : *BB) {
      instructionToLineNumberMap[&I] = line_num;
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
          processLoadInst(LI, IndVal);
          MemInstr->push_back(LI);
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
          processStoreInst(SI, IndVal);
          MemInstr->push_back(SI);
          line_num++;
      }
    }
    return;
}
namespace {

struct HW1Pass : public PassInfoMixin<HW1Pass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

ValueVector MemInstr;
PreservedAnalyses HW1Pass::run(Function &F, FunctionAnalysisManager &FAM) {
  auto &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto &LI = FAM.getResult<LoopAnalysis>(F);
  ValueVector MemInstr;
  for (BasicBlock &BB : F) {
    if(BB.getName() == "for.body"){
      Loop *L = LI.getLoopFor(&BB);
      std::optional<Loop::LoopBounds> Bounds = L->getBounds(SE);
      ConstantInt *InitVal = dyn_cast<ConstantInt>(&Bounds->getInitialIVValue());
      StringRef IndVal = L->getInductionVariable(SE)->getName();
      ConstantInt *StepValue = dyn_cast_or_null<ConstantInt>(Bounds->getStepValue());
      ConstantInt *FinalVal = dyn_cast<ConstantInt>(&Bounds->getFinalIVValue());
      processInst(&BB, &MemInstr, &IndVal);
      ValueVector::iterator I, IE, J, JE;
      for (I = MemInstr.begin(), IE = MemInstr.end(); I != IE; ++I) {
        for (J = I, JE = MemInstr.end(); J != JE; ++J) {
          Instruction *Src = cast<Instruction>(*I);
          Instruction *Dst = cast<Instruction>(*J);
          std::string src_Arr_name = Arr_name[Src];
          std::string dst_Arr_name = Arr_name[Dst];
          if(isa<LoadInst>(Src) && isa<LoadInst>(Dst)){
            continue;
          }
          if(J == I)continue;
          if(src_Arr_name != dst_Arr_name){
            continue;
          }

          int src_first, src_second = 0;
          int dst_first, dst_second = 0;
          auto src_pair = arrayIdx.find(Src);
          auto dst_pair = arrayIdx.find(Dst);
          if(src_pair != arrayIdx.end()){
              std::pair<int, int> mapping = src_pair->second;
              src_first = mapping.first;
              src_second = mapping.second;
          }
          if(dst_pair != arrayIdx.end()){
              std::pair<int, int> mapping = dst_pair->second;
              dst_first = mapping.first;
              dst_second = mapping.second;
          }
          struct SolEquStru f;
          f = diophatine_solver( src_first, -dst_first, Max(src_second - dst_second, dst_second - src_second));
          if(!f.is_there_solution){
            continue;
          }

          std::pair<int, int> mapping_state = \
            std::make_pair(instructionToLineNumberMap[Src], instructionToLineNumberMap[Dst]);
          std::pair<SolEquStru*, std::pair<int, int>> mapping_sol = \
            std::make_pair(&f, mapping_state);
          // output dep
          if(isa<StoreInst>(Src) && isa<StoreInst>(Dst)){
              output_dep[src_Arr_name] = mapping_sol;
          }
          // flow dep && anti dep
          else{
              flow_dep[src_Arr_name] = mapping_sol;
              anti_dep[src_Arr_name] = mapping_sol;
          }
        }
      }
      //  print answer
      //  flow dep
      errs() << "====Flow Dependency====\n";
      for (auto it = flow_dep.begin(); it != flow_dep.end(); it++) {
          SolEquStru* sol = it->second.first;
          int state1 = it->second.second.first;
          int state2 = it->second.second.second;

          for (int i = InitVal->getSExtValue(); i < FinalVal->getSExtValue(); i++) {
              errs() << it->first << ":S" << state1 << " -----> S" << state2 << '\n';
              // X = Y -> a + bi = c + dj, j = (a + bi - c) / d
              int i0 = i;
              int i1 = (sol->X0 + sol->XCoeffT * i - sol->Y0) / sol->XCoeffT;
              if(i1 < FinalVal->getSExtValue())
                errs() << "(i=" << i0 << ",i=" << i1 << ")\n";
              else
                break;
          }
      }
      errs() << "====Anti-Dependency====\n";
      errs() << "====Output Dependency====\n";
      
    }
    
  }
  

  
  return PreservedAnalyses::all();
}

} // end anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HW1Pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hw1") {
                    FPM.addPass(HW1Pass());
                    return true;
                  }
                  return false;
                });
          }};
}