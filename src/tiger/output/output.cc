#include "tiger/output/output.h"
#include <cstdio>
#include "tiger/output/logger.h"
#include "tiger/runtime/gc/roots/roots.h"
#include <list>
#include "tiger/frame/x64frame.cc"
#include "tiger/codegen/assem.h"
extern frame::RegManager *reg_manager;
extern frame::Frags *frags;

//extern gc::Roots* roots;
extern std::vector<gc::PointerMap> GCRoots; 

namespace output {
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
  new_root->escapes = offsets;
  new_root->coloring = color;
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

void AssemGen::GenAssem(bool need_ra) {
  frame::Frag::OutputPhase phase;

  // Output proc
  phase = frame::Frag::Proc;
  fprintf(out_, ".text\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  // Output string
  phase = frame::Frag::String;
  fprintf(out_, ".section .rodata\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  //LAB7:
  outPutPointerMap(out_);
  /*
  //fprintf(out_, ".section .data\n");
  std::list<std::string> map_list = roots->pointer_maps;
  for(auto it : map_list) {
      const char* cc = it.c_str();
      fprintf(out_, cc);
  }
  fprintf(out_, ".globl GLOBAL_GC_ROOTS\n");
  //fprintf(out_, "\n");
  */
}

} // namespace output

namespace frame {

void ProcFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  std::unique_ptr<canon::Traces> traces;
  std::unique_ptr<cg::AssemInstr> assem_instr;
  std::unique_ptr<ra::Result> allocation;

  // When generating proc fragment, do not output string assembly
  if (phase != Proc)
    return;

  TigerLog("-------====IR tree=====-----\n");
  TigerLog(body_);

  {
    // Canonicalize
    TigerLog("-------====Canonicalize=====-----\n");
    canon::Canon canon(body_);

    // Linearize to generate canonical trees
    TigerLog("-------====Linearlize=====-----\n");
    tree::StmList *stm_linearized = canon.Linearize();
    TigerLog(stm_linearized);

    // Group list into basic blocks
    TigerLog("------====Basic block_=====-------\n");
    canon::StmListList *stm_lists = canon.BasicBlocks();
    TigerLog(stm_lists);

    // Order basic blocks into traces_
    TigerLog("-------====Trace=====-----\n");
    tree::StmList *stm_traces = canon.TraceSchedule();
    TigerLog(stm_traces);

    traces = canon.TransferTraces();
  }

  temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
  {
    // Lab 5: code generation
    TigerLog("-------====Code generate=====-----\n");
    cg::CodeGen code_gen(frame_, std::move(traces));
    code_gen.Codegen();
    assem_instr = code_gen.TransferAssemInstr();
    TigerLog(assem_instr.get(), color);
  }

  assem::InstrList *il = assem_instr.get()->GetInstrList();
  
  //LAB7:
  std::vector<int> offsets = output::collectEscapePointers(frame_);

  if (need_ra) {
    // Lab 6: register allocation
    TigerLog("----====Register allocate====-----\n");
    ra::RegAllocator reg_allocator(frame_, std::move(assem_instr));
    reg_allocator.RegAlloc();
    allocation = reg_allocator.TransferResult();
    il = allocation->il_;
    color = temp::Map::LayerMap(reg_manager->temp_map_, allocation->coloring_);
  }

  //LAB7:
  il = output::AddNewPointerMap(il, frame_, offsets, color);

  TigerLog("-------====Output assembly for %s=====-----\n",
           frame_->name_->Name().data());

  assem::Proc *proc = frame::ProcEntryExit3(frame_, il);
  
  std::string proc_name = frame_->GetLabel();

  fprintf(out, ".globl %s\n", proc_name.data());
  fprintf(out, ".type %s, @function\n", proc_name.data());
  // prologue
  fprintf(out, "%s", proc->prolog_.data());
  // body
  proc->body_->Print(out, color);
  // epilog_
  fprintf(out, "%s", proc->epilog_.data());
  fprintf(out, ".size %s, .-%s\n", proc_name.data(), proc_name.data());
}

void StringFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  // When generating string fragment, do not output proc assembly
  if (phase != String)
    return;

  fprintf(out, "%s:\n", label_->Name().data());
  int length = static_cast<int>(str_.size());
  // It may contain zeros in the middle of string. To keep this work, we need
  // to print all the charactors instead of using fprintf(str)
  fprintf(out, ".long %d\n", length);
  fprintf(out, ".string \"");
  for (int i = 0; i < length; i++) {
    if (str_[i] == '\n') {
      fprintf(out, "\\n");
    } else if (str_[i] == '\t') {
      fprintf(out, "\\t");
    } else if (str_[i] == '\"') {
      fprintf(out, "\\\"");
    } else {
      fprintf(out, "%c", str_[i]);
    }
  }
  fprintf(out, "\"\n");
}
} // namespace frame
