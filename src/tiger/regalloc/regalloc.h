#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <set>
namespace ra {

class Result {
public:
  temp::Map *coloring_;//mapping the color and the reg.
  assem::InstrList *il_;//the rewrited instr list.

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result() = default;
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
  public:
      frame::Frame* frame_;
      assem::InstrList* instr_list_;//instrList of the current frame.
      //std::list<temp::Temp*> newTemps;//used to store the temp created in the RewriteProgram function.
      std::unique_ptr<ra::Result> result_;//used to store the result of the reg-alloc.
      RegAllocator(frame::Frame* f, std::unique_ptr<cg::AssemInstr> assem_instr) {
          frame_ = f;
	  instr_list_ = assem_instr->GetInstrList();
	  result_ = std::make_unique<ra::Result>();
	  //newTemps = {};
	  //K = 15;//there are 15 kinds of the colors.
      }
      void RegAlloc();//used to alloc the registers.
      std::unique_ptr<ra::Result> TransferResult();//get the result.
      std::set<temp::Temp*> RewriteProgram(live::INodeListPtr spilledNodes);//rewrite the codes after the reg-alloc with the param spilledNodes.
      live::LiveGraph LivenessAnalysis(assem::InstrList* instrList);//do the liveness analysis.
std::string debug_Format(std::string_view assem, temp::TempList *dst,
                          temp::TempList *src, assem::Targets *jumps, temp::Map *m) {
  std::string result;
  for (std::string::size_type i = 0; i < assem.size(); i++) {
    char ch = assem.at(i);
    if (ch == '`') {
      i++;
      switch (assem.at(i)) {
      case 's': {
        i++;
        int n = assem.at(i) - '0';
        std::string *s = m->Look(src->NthTemp(n));
        result += *s;
      } break;
      case 'd': {
        i++;
        int n = assem.at(i) - '0';
        std::string *s = m->Look(dst->NthTemp(n));
        result += *s;
      } break;
      case 'j': {
        i++;
        assert(jumps);
        std::string::size_type n = assem.at(i) - '0';
        std::string s = temp::LabelFactory::LabelString(jumps->labels_->at(n));
        result += s;
      } break;
      case '`': {
        result += '`';
      } break;
      default:
        assert(0);
      }
    } else {
      result += ch;
    }
  }
  return result;
}

      void DEBUG(assem::InstrList* list, temp::Map* coloring_) {
          for(auto it : list->GetList()) {
	      if(typeid(*it) == typeid(assem::OperInstr)) {
	          assem::OperInstr* instr = static_cast<assem::OperInstr*>(it);
		  std::string ans = debug_Format(instr->assem_, instr->dst_, instr->src_, instr->jumps_, coloring_);
		  printf("OPER: %s\n", ans.c_str());
	      }
	      if(typeid(*it) == typeid(assem::LabelInstr)) {
	          assem::LabelInstr* instr = static_cast<assem::LabelInstr*>(it);
		  std::string ans = debug_Format(instr->assem_, nullptr, nullptr, nullptr, coloring_);
		  printf("LABEL: %s\n", ans.c_str());
	      }
              if(typeid(*it) == typeid(assem::MoveInstr)) {
	          assem::MoveInstr* instr = static_cast<assem::MoveInstr*>(it);
                  if(!(instr->dst_) && !(instr->src_)) {
                      std::size_t srcpos = (instr->assem_).find_first_of('%');
                      if (srcpos != std::string::npos) {
                          std::size_t dstpos = (instr->assem_).find_first_of('%', srcpos + 1);
                          if (dstpos != std::string::npos) {
                              if (((instr->assem_)[srcpos + 1] == (instr->assem_)[dstpos + 1]) &&
                                  ((instr->assem_)[srcpos + 2] == (instr->assem_)[dstpos + 2]) &&
                                  ((instr->assem_)[srcpos + 3] == (instr->assem_)[dstpos + 3]))
                                  continue;
                          }
                      }
                  }  
		  std::string ans = debug_Format(instr->assem_, instr->dst_, instr->src_, nullptr, coloring_);
		  printf("MOVE: %s\n", ans.c_str());
	      }
	  }
      }
      /*
      live::INodeListPtr simplifyWorkList;//the list of the nodes that will be simplified.
      live::INodeListPtr freezeWorkList;//the list of the nodes that can be simplified but are move-related.
      live::INodeListPtr spillWorkList;//the list of the nodes that are marked to be spilled.
      
      std::stack<live::INodePtr> selectStack;//the stack used to store the nodes simplified from the graph.
     
      live::MoveList WorkListMoves;//the moveList of the nodes that can be colasece.
      live::MoveList activeMoves;//the moveList of the nodes that are not ready to do the colasece.
      live::MoveList frozenMoves;//the moveList of the nodes that won't be coalesced.
      live::MoveList constrainedMoves;//the moveList of the nodes that are interferd but are also move-related between src_ and the dst_
      live::MoveList coalescedMoves;//the moveList of the nodes that are already coalesced.
      
      live::INodeListPtr coalescedNodes;
      live::INodeListPtr coloredNodes;
      
      std::map<live::INodePtr, live::MoveList> moveList;//the map between the node and its related moves.
      std::map<live::INodePtr, live::INodePtr> alias;//the alias(the chosen leader) of the coalesced pair of the nodes.
      std::map<live::INodePtr, std::string> precolored;
      std::map<live::INodePtr, int> degree;//the mapping between the node and its degree(note that the interf_graph is a undirected graph.)
      std::set<std::pair<live::INodePtr, live::INodePtr>> adjSet;//the set recording the pair of the interfering edges.
      std::map<live::INodePtr, live::INodeListPtr> adjList;//the mapping between a node and its corresponding edges.
      temp::Map* colored_;//the map used to record the color and its coloring node pair.
      
      int K;
      */
};

} // namespace ra

#endif
