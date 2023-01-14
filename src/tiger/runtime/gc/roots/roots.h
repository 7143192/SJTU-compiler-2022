#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>
#include <map>
#include <list>
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/translate/tree.h"
#include "tiger/liveness/liveness.h"
#include <vector>

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

struct PointerMap {
  std::string label;//the label of the pointer map, should be "L + retLabel"
  std::string ret_label;//the ret label after one callq instr.
  std::string next_map;//the next pointer map's label.
  std::string frame_size;//the frame size of the current callq frame.
  std::string uppermost = "0";//used to mark if this map is the "tigermain".
  std::vector<std::string> offsets;
  std::string end = "-1";//used to mark the end of one pointer map.
};

class Roots {
  // TODO(lab7): define some member and methods here to keep track of gc roots;
  public:
      std::map<temp::Temp*, int> temp_map;//used to record the temps that point to a record/an array.
      std::map<std::string, std::vector<int>> func_pointers;//used to map a func to its pointer in its frame.(not sure).
      std::map<std::string, std::vector<int>> pointers_type;//used to record the pointer-containing stack slot's pointer-type.
      std::list<std::string> pointer_maps;//used to record all the pointerMaps in a list structure.
      std::map<std::string, std::vector<temp::Label*>> ret_map;//used to map a called func to its ret_labels.

      assem::InstrList* instr_list;//the final instr list.
      frame::Frame *frame;//the frame of the current call.
      fg::FGraphPtr flowgraph;//the control flow graph of the instrs.
      std::vector<int> escapes;
      temp::Map *coloring;//the regalloc res, can be got at the output / regalloc level.
      std::map<fg::FNodePtr, temp::TempList *> temp_in;
      std::map<fg::FNodePtr, std::vector<int>> address_in;
      std::map<fg::FNodePtr, std::vector<int>> address_out;
      std::map<assem::Instr *, std::vector<int>> addr_map;
      std::map<assem::Instr *, std::vector<std::string>> temp_map_;//a map mapping a ret label to its alive temps in its pointer map.

      Roots();
      //Roots(assem::InstrList* il, frame::Frame* f, fg::FGraphPtr graph, std::vector<int> e);
      std::vector<gc::PointerMap> GenPointerMaps();//gen all pointer maps for the current frame.
      std::vector<int> GetUseOff(assem::Instr* instr);
      std::vector<int> GetDefOff(assem::Instr* instr);//these two funcs are used to get the offsets from one movq instr.
      void GenAliveAddr();
      //void GenAliveTemp();
      void AliveAfterCall();
      void Rewrite();
      std::vector<int> Union(std::vector<int> v1, std::vector<int> v2);
      std::vector<int> Diff(std::vector<int> v1, std::vector<int> v2);
      
      void GenPointerMap(frame::Frame* f);
      //the above function is used to gen pointer map for the function.every call of same function should have a new retLabel and a similar pointerMap.
};

}

#endif // TIGER_RUNTIME_GC_ROOTS_H
