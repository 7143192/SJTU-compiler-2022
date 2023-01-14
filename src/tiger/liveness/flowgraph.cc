#include "tiger/liveness/flowgraph.h"

namespace fg {
//this function is used to generate the control flow graph.
void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  std::list<assem::Instr*> list = instr_list_->GetList();
  FNodePtr from = nullptr;//used to record the from node of every new edge.
  for(assem::Instr* it : list) {
      FNodePtr node = flowgraph_->NewNode(it);//create a new node in the graph based on a instruction.
      if(from == nullptr) {
          from = node;
	  AddLabelMap(node);//add the label-map
	  continue;//the case of the first instruction.
      }
      //flowgraph_->AddEdge(from, node);//remember we can't create a self-cycle.
      //we should handle the jump and the CJump cases specially.
      //int jump_flag = 0;
      if(typeid(*(from->NodeInfo())) == typeid(assem::OperInstr)) {
	  assem::OperInstr* instr = static_cast<assem::OperInstr*>(from->NodeInfo());
	  if((instr->assem_).find("jmp") == std::string::npos) {//try to find the "jmp" substring
	      //there are no jump targets
	      flowgraph_->AddEdge(from, node);
	  }
      }
      else flowgraph_->AddEdge(from, node);
      AddLabelMap(node);//add the label-map
      from = node;
  }
  //then try to add edges between jump_src and the jump_dsts.
  std::list<FNodePtr> nodeList = flowgraph_->Nodes()->GetList();
  for(FNodePtr it : nodeList) {
      assem::Instr* instr = it->NodeInfo();
      if(typeid(*instr) == typeid(assem::OperInstr)) {
          assem::OperInstr* op = static_cast<assem::OperInstr*>(instr);
	  if(op->jumps_ != nullptr) {
	      std::vector<temp::Label*>* labelPointer = (op->jumps_->labels_);//get the jump labels.
	      if(labelPointer != nullptr) {
		  const auto & labels = *labelPointer;
	          for(size_t i = 0; i < labels.size(); ++i) {
	              FNodePtr dst = label_map_->Look(labels[i]);//this line indicates why we need to add the label first.similar to the solution of the type/funcDec.
		      if(dst != nullptr) flowgraph_->AddEdge(it, dst);//add a edge between the current node and the jump_dst node(s).
	          }
	      }
	  }
      }
  }
}

void FlowGraphFactory::AddLabelMap(FNodePtr node) {//put the labels and the corresponding instrs into the graph-attr.
    if(typeid(*(node->NodeInfo())) == typeid(assem::LabelInstr)) {
        //if it's a LabelInstr.
	assem::LabelInstr* i = static_cast<assem::LabelInstr*>(node->NodeInfo());
	label_map_->Enter(i->label_, node);
    }
    return ;
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  temp::TempList* ans = new temp::TempList();
  return ans;//the LabelInstr doesn't def any temp.
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if(dst_ == nullptr) return new temp::TempList();
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  //the dst temps are regarded as the defined temps.
  if(dst_ == nullptr) return new temp::TempList();
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  temp::TempList* ans = new temp::TempList();
  return ans;//the LabelInstr doesn't use any temp.
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  //the src temps are regarded as the used temps.
  if(src_ == nullptr) return new temp::TempList();
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if(src_ == nullptr) return new temp::TempList();
  return src_;
}
} // namespace assem
