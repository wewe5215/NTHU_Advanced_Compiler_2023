#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include <map>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <algorithm>
#define DEBUG_MODE 0
using namespace llvm;

struct Var_set {
  int is_pointer;
  int level_of_pointer;
  std::string name;
};

struct TREF {
    std::vector<std::string> element;
};
struct TGEN {
    std::vector<std::string> element;
};
struct TEQ {
    int elem1_deref_level;
    std::string elem1_name;
    std::string elem2;
    std::vector<std::string> alias;
};
struct DEP{
    std::string VAR_name;
    std::string src;
    std::string dst;
};
std::map<std::string, Var_set*> all_variables;
std::vector<TREF*> all_TREF;
std::vector<TGEN*> all_TGEN;
std::vector<std::string> all_LHS;
std::map<std::string, TEQ*> TEQIV;
std::map<std::string, std::string> TDEF;
std::vector<std::vector<DEP*>> all_flow_dep;
std::vector<std::vector<DEP*>> all_out_dep;
void handling_var(AllocaInst* AI) {
    // new variables
    Var_set* new_var = new Var_set;
    std::string var_name = std::string(AI->getName());
    Type *type = AI->getAllocatedType();
    if (type->isPointerTy()) {
        new_var->is_pointer = 1;
        if(var_name == "p"){
            new_var->level_of_pointer = 1;
            new_var->name = "*p";
        }
        else if(var_name == "pp"){
            new_var->level_of_pointer = 2;
            new_var->name = "**pp";
        }
    }
    else {
        new_var->is_pointer = 0;
        new_var->level_of_pointer = 0;
        new_var->name = var_name;
    }
    all_variables[var_name] = new_var;
    // if(DEBUG_MODE){
    //     errs() << "name = " << var_name << ", is_pointer = " << all_variables[var_name]->is_pointer;
    //     errs() << ", level_of_pointer = " << all_variables[var_name]->level_of_pointer;
    //     errs() << ", name = " << all_variables[var_name]->name << "\n";
    // }
    return;
}

void handling_alias(std::string src){
    std::string elem2_str = "*" + TEQIV[src]->elem2;
    if(all_variables[TEQIV[src]->elem2]->is_pointer && (all_variables[TEQIV[src]->elem1_name]->level_of_pointer == 2)){
        if(TEQIV.find(elem2_str) != TEQIV.end()){
            if(std::find(TEQIV[elem2_str]->alias.begin(), TEQIV[elem2_str]->alias.end(), "*" + src) == TEQIV[elem2_str]->alias.end()){
                TEQIV[elem2_str]->alias.push_back("*" + src);
                if(TEQIV.find("*" + src) == TEQIV.end()){
                    TEQ* new_TEQ = new TEQ;
                    new_TEQ->elem1_deref_level = TEQIV[src]->elem1_deref_level++;
                    new_TEQ->elem1_name = TEQIV[src]->elem1_name;
                    new_TEQ->elem2 = TEQIV[elem2_str]->elem2;
                    new_TEQ->alias.push_back(elem2_str);
                    TEQIV["*" + src] = new_TEQ;
                }
                if(DEBUG_MODE){
                    errs() << "elem2_str = " << elem2_str << ", alias: *" << src << "\n";
                }
            }
        }
    }
    return;
}

void update_equiv(std::string src){
    if(TEQIV[src]->alias.empty())return;

    for(int i = 0; i < TEQIV[src]->alias.size(); i ++){
        TEQIV[TEQIV[src]->alias[i]]->elem2 = TEQIV[src]->elem2;
    }
}

void print_output(int stmt_cnt){
    errs() << "S" << stmt_cnt << ":--------------------\n";
    errs() << "TREF: {";
    if(!all_TREF[stmt_cnt - 1]->element.empty()){
        for(int i = 0; i < all_TREF[stmt_cnt - 1]->element.size(); i ++){
            errs() << all_TREF[stmt_cnt - 1]->element[i];
            if(i != all_TREF[stmt_cnt - 1]->element.size() - 1){
                errs() << ", ";
            }
        }
        
    }
    errs() << "}\n";
    errs() << "TGEN: {";
    if(!all_TGEN[stmt_cnt - 1]->element.empty()){
        for(int i = 0; i < all_TGEN[stmt_cnt - 1]->element.size(); i ++){
            errs() << all_TGEN[stmt_cnt - 1]->element[i];
            if(i != all_TGEN[stmt_cnt - 1]->element.size() - 1){
                errs() << ", ";
            }
        }
        
    }
    errs() << "}\n";
    errs() << "DEP:{\n";
    for(int i = 0; i < all_flow_dep[stmt_cnt - 1].size(); i ++){
        errs() << "    " << all_flow_dep[stmt_cnt - 1][i]->VAR_name << ": ";
        errs() << all_flow_dep[stmt_cnt - 1][i]->src << "--->";
        errs() << all_flow_dep[stmt_cnt - 1][i]->dst << "\n";
    }

    for(int i = 0; i < all_out_dep[stmt_cnt - 1].size(); i ++){
        errs() << "    " << all_out_dep[stmt_cnt - 1][i]->VAR_name << ": ";
        errs() << all_out_dep[stmt_cnt - 1][i]->src << "-O->";
        errs() << all_out_dep[stmt_cnt - 1][i]->dst << "\n";
    }
    
    errs() << "}\n";
    errs() << "TDEF:{";
    for (auto it = TDEF.begin(); it != TDEF.end(); ++it){
        errs() << "(" << it->first << ", " << it->second << ")";
        if (std::next(it) != TDEF.end()) {
            errs() << ", ";
        }
    }
    errs() << "}\n";
    errs() << "TEQUIV:{";
    for (auto it = TEQIV.begin(); it != TEQIV.end(); ++it){
        errs() << "(" << it->first << ", " << it->second->elem2 << ")";
        if (std::next(it) != TEQIV.end()) {
            errs() << ", ";
        }
    }
    errs() << "}\n";
}
void handling_TGEN(int stmt_cnt){
    if(!all_TGEN[stmt_cnt - 1]->element.empty()){
        for(int i = 0; i < all_TGEN[stmt_cnt - 1]->element.size(); i ++){
            if(TEQIV.find(all_TGEN[stmt_cnt - 1]->element[i]) != TEQIV.end()){
                if(std::find(all_TGEN[stmt_cnt - 1]->element.begin(), \
                    all_TGEN[stmt_cnt - 1]->element.end(), \
                    TEQIV[all_TGEN[stmt_cnt - 1]->element[i]]->elem2) == all_TGEN[stmt_cnt - 1]->element.end()){
                    all_TGEN[stmt_cnt - 1]->element.push_back(TEQIV[all_TGEN[stmt_cnt - 1]->element[i]]->elem2);
                }
            }
            
        }
    }
}
void handling_TDEF(int stmt_cnt){
    if(!all_TGEN[stmt_cnt - 1]->element.empty()){
        for(int i = 0; i < all_TGEN[stmt_cnt - 1]->element.size(); i ++){
            TDEF[all_TGEN[stmt_cnt - 1]->element[i]] = "S" + std::to_string(stmt_cnt);
        }
    }
}
void handling_TREF(StoreInst* SI, int stmt_cnt){
    std::string RHS_name, LHS_name;
    TREF* new_TREF = new TREF;
    Value *valueToBeStored = SI->getOperand(0);
    Value *storeLocation = SI->getOperand(1);
    // RHS, for TREF, TEQUIV
    if(isa<LoadInst>(valueToBeStored)){
        Instruction* LI1 = dyn_cast<Instruction>(valueToBeStored);
        int level_of_deref = 0;
        while(isa<LoadInst>(LI1)){
            if(isa<AllocaInst>(LI1->getOperand(0))){
                RHS_name = std::string(LI1->getOperand(0)->getName());
                if(DEBUG_MODE)errs() << "rhs_name = " << RHS_name << "\n";
                break;
            }
            LI1 = dyn_cast<Instruction>(LI1->getOperand(0));
            level_of_deref++;
        }
        std::string ref_name = "";
        while (level_of_deref--)
        {
            ref_name += "*";
        }
        ref_name += RHS_name;
        
        new_TREF->element.push_back(ref_name);
    }
    else if(isa<AllocaInst>(valueToBeStored)){
        RHS_name = std::string(dyn_cast<AllocaInst>(valueToBeStored)->getName());
        if(DEBUG_MODE)errs() << "rhs_name = " << RHS_name << "\n";
    }

    // LHS, for TREF, TGEN
    TGEN* new_TGEN = new TGEN;
    int level_of_deref = 0;
    if(isa<LoadInst>(storeLocation)){
        level_of_deref++;
        Instruction* LI1 = dyn_cast<Instruction>(storeLocation);
        while(isa<LoadInst>(LI1)){
            if(isa<AllocaInst>(LI1->getOperand(0))){
                LHS_name = std::string(LI1->getOperand(0)->getName());
                if(DEBUG_MODE)errs() << "lhs_name = " << LHS_name << ", with multiple load\n";
                break;
            }
            LI1 = dyn_cast<Instruction>(LI1->getOperand(0));
            level_of_deref++;
        }
        std::string ref_name = "", gen_name = "*";
        int tmp = level_of_deref - 1;
        while (tmp--)
        {
            ref_name += "*";
            gen_name += "*";
        }
        ref_name += LHS_name;
        gen_name += LHS_name;
        
        new_TREF->element.push_back(ref_name);
        new_TGEN->element.push_back(gen_name);
    }
    else if(isa<AllocaInst>(storeLocation)){
        LHS_name = std::string(dyn_cast<AllocaInst>(storeLocation)->getName());
        new_TGEN->element.push_back(LHS_name);
        if(DEBUG_MODE)errs() << "lhs_name = " << LHS_name << "\n";
    }
    all_TREF.push_back(new_TREF);
    all_TGEN.push_back(new_TGEN);

    if(storeLocation->getType()->isPointerTy() && valueToBeStored->getType()->isPointerTy() && \
        (level_of_deref < all_variables[LHS_name]->level_of_pointer)){
        int cnt = level_of_deref + 1;
        std::string elem1 = "";
        while(cnt --){
            elem1 += "*";
        }
        elem1 += LHS_name;
        if(TEQIV.find(elem1) == TEQIV.end()){
            TEQ* new_TEQ = new TEQ;
            new_TEQ->elem1_deref_level = level_of_deref;
            new_TEQ->elem1_name = LHS_name;
            new_TEQ->elem2 = RHS_name;
            TEQIV[elem1] = new_TEQ;
        }
        else{
            TEQIV[elem1]->elem1_deref_level = level_of_deref;
            TEQIV[elem1]->elem1_name = LHS_name;
            TEQIV[elem1]->elem2 = RHS_name;
        }

        handling_alias(elem1);
        update_equiv(elem1);
    }
    if(DEBUG_MODE){
        errs() << "stmt count = " << stmt_cnt << "\n";
        errs() << "TREF: " << "\n";
        if(!all_TREF[stmt_cnt - 1]->element.empty()){
            for(int i = 0; i < all_TREF[stmt_cnt - 1]->element.size(); i ++){
                errs() << all_TREF[stmt_cnt - 1]->element[i] << ", ";
            }
            errs() << "\n";
        }
        errs() << "TGEN: " << "\n";
        if(!all_TGEN[stmt_cnt - 1]->element.empty()){
            for(int i = 0; i < all_TGEN[stmt_cnt - 1]->element.size(); i ++){
                errs() << all_TGEN[stmt_cnt - 1]->element[i] << ", ";
            }
            errs() << "\n";
        }
    }
    all_LHS.push_back(LHS_name);
    return;
}
void handling_dependency(int stmt_cnt){
    std::vector<DEP*> flow_dep;
    std::vector<DEP*> out_dep;
    // flow dependency : TREF(Si) and TDEFi is taken
    for(int i = 0; i < all_TREF[stmt_cnt - 1]->element.size(); i ++){
        if(TDEF.find(all_TREF[stmt_cnt - 1]->element[i]) != TDEF.end()){
            DEP* new_dep = new DEP;
            new_dep->VAR_name = all_TREF[stmt_cnt - 1]->element[i];
            new_dep->src = TDEF[all_TREF[stmt_cnt - 1]->element[i]];
            new_dep->dst = "S" + std::to_string(stmt_cnt);
            flow_dep.push_back(new_dep);
        }
    }
    // output dependency : TDEFi and TGEN(Si) is taken
    for(int i = 0; i < all_TGEN[stmt_cnt - 1]->element.size(); i ++){
        if(TDEF.find(all_TGEN[stmt_cnt - 1]->element[i]) != TDEF.end()){
            DEP* new_dep = new DEP;
            new_dep->VAR_name = all_TGEN[stmt_cnt - 1]->element[i];
            new_dep->src = TDEF[all_TGEN[stmt_cnt - 1]->element[i]];
            new_dep->dst = "S" + std::to_string(stmt_cnt);
            out_dep.push_back(new_dep);
        }
    }
    all_flow_dep.push_back(flow_dep);
    all_out_dep.push_back(out_dep);
}

namespace {

struct HW2Pass : public PassInfoMixin<HW2Pass> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

PreservedAnalyses HW2Pass::run(Function &F, FunctionAnalysisManager &FAM) {
    for (BasicBlock &BB : F) {
        int stmt_cnt = 0;
        for (Instruction &I : BB) {
            if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
                handling_var(AI);
            }
            else if(auto *SI = dyn_cast<StoreInst>(&I)){
                stmt_cnt++;
                handling_TREF(SI, stmt_cnt);
                handling_TGEN(stmt_cnt);
                handling_dependency(stmt_cnt);
                handling_TDEF(stmt_cnt);
                print_output(stmt_cnt);
            }
            else continue;
        
        }
    }
  return PreservedAnalyses::all();
}

} // end anonymous namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HW2Pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hw2") {
                    FPM.addPass(HW2Pass());
                    return true;
                  }
                  return false;
                });
          }};
}