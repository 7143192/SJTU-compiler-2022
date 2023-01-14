#ifndef TIGER_COMPILER_OUTPUT_H
#define TIGER_COMPILER_OUTPUT_H

#include <list>
#include <memory>
#include <string>

#include "tiger/canon/canon.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/regalloc/regalloc.h"
//#include "tiger/runtime/gc/roots/roots.h"
//#include <list>
//#include "tiger/frame/x64frame.cc"
//#include "tiger/codegen/assem.h"

namespace output {
//LAB7:GC
/*
void outPutPointerMap(FILE *out_) {
  if(GCRoots.size() != 0) {
      GCRoots.back().next_map = "0";
  }
  fprintf(out_, ".global GLOBAL_GC_ROOTS\n");
  fprintf(out_, ".data\n");
  fprintf(out_, "GLOBAL_GC_ROOTS:\n");
  for (auto i = 0; i < GCRoots.size(); ++i) {
    auto map = GCRoots[i];
    std::string label = map.label + ":\n";
    std::string ret_label = ".quad " + map.ret_label + "\n";
    std::string next_label = ".quad " + map.next_map + "\n";
    std::string frame_size = ".quad " + map.frame_size + "\n";
    std::string uppermost = ".quad " + map.uppermost + "\n";
    std::string map_str = label + ret_label + next_label + frame_size + uppermost;
    for(std::string &offset : map.offsets) {
	map_str += ".quad " + offset + "\n";
    }
    map_str += ".quad " + map.end + "\n";
    fprintf(out_, "%s", map_str.c_str());
  }
}

//this function is used to get the pointer-contained slot in the stack and regs in the formal list.
std::vector<int> collectEscapePointers(frame::Frame *frame_) {
  std::vector<int> offsets;
  std::list<frame::Access *> formal_list = frame_->formals;
  for (frame::Access *access : formal_list) {
    if (typeid(*access) == typeid(frame::InFrameAccess)) {
      frame::InFrameAccess* frame_access = static_cast<frame::InFrameAccess*>(access);
      if(frame_access->is_pointer == true) {
	  offsets.push_back(static_cast<frame::InFrameAccess *>(access)->offset);
      }
    }
  }
  return offsets;
}

//this function is used to gen pointer map.
assem::InstrList *AddNewPointerMap(assem::InstrList *il, frame::Frame *frame_,
                                 std::vector<int> offsets,
                                 temp::Map *color) {
  fg::FlowGraphFactory *graph = new fg::FlowGraphFactory(il);
  graph->AssemFlowGraph();//gen the control flow graph.
  fg::FGraphPtr fg = graph->GetFlowGraph();
  gc::Roots* new_root = new gc::Roots();
  new_root->instr_list = il;
  new_root->frame = frame_;
  new_root->flowgraph = fg;
  new_root->offsets = offsets;
  std::vector<gc::PointerMap> newMaps = new_root->GenPointerMaps();
  il = new_root->instr_list;
  if (GCRoots.size() != 0 && newMaps.size() != 0) {
    GCRoots.back().next_map = newMaps.front().label;
  }
  //GCRoots.insert(GCRoots.end(), newMaps.begin(), newMaps.end());
  for(size_t i = 0; i < newMaps.size(); ++i) {
      GCRoots.push_back(newMaps[i]);
  }
  return il;
}
*/

class AssemGen {
public:
  AssemGen() = delete;
  explicit AssemGen(std::string_view infile) {

    //printf("get into the AssemGen's constructor function!");

    std::string outfile = static_cast<std::string>(infile) + ".s";
    out_ = fopen(outfile.data(), "w");
  }
  AssemGen(const AssemGen &assem_generator) = delete;
  AssemGen(AssemGen &&assem_generator) = delete;
  AssemGen &operator=(const AssemGen &assem_generator) = delete;
  AssemGen &operator=(AssemGen &&assem_generator) = delete;
  ~AssemGen() { fclose(out_); }

  /**
   * Generate assembly
   */
  void GenAssem(bool need_ra);

private:
  FILE *out_; // Instream of source file
};

} // namespace output

#endif // TIGER_COMPILER_OUTPUT_H
