#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {//delete a move pair from the move_list_
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {//get the ans of the "U" op between list and the move_list_
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {//get the ans of the intersection.
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  /*
   * use the rules to get the liveness ans:
   * in[s] = use[s] union (out[s] - def[s])
   * out[s] = (n in succ[s]) union (in[n])
   * since we traverse the list from the bottom to top, 
   * we need to update the out_temp first and the in_temp second.
   */
  std::list<fg::FNodePtr> nodes = flowgraph_->Nodes()->GetList();
  for(fg::FNodePtr node : nodes) {
      assem::Instr* instr = node->NodeInfo();
      temp::TempList* in_list = new temp::TempList();
      temp::TempList* out_list = new temp::TempList();
      in_->Enter(node, in_list);
      out_->Enter(node, out_list);//alloc a empty list to one instr at the in_pos and the out_pos of this instr
  }
  int end = 0;//used to notify whether the analysis has finished
  //we do the analysis form the bottom to the top.
  while(end == 0) {
      end = 1;
      nodes.reverse();//reverse the list.
      for(fg::FNodePtr node : nodes) {
      //for(auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
	  //fg::FNodePtr node = *it;
          assem::Instr* instr = node->NodeInfo();
	  temp::TempList* in_old = in_->Look(node);
	  temp::TempList* out_old = out_->Look(node);//get the old lists before the new cycle of the analysis.
	  std::list<fg::FNodePtr> succ = node->Succ()->GetList();
	  //get the new out of the current instruction.
	  for(fg::FNodePtr succ_i : succ) {
	      temp::TempList* in_succ_i = in_->Look(succ_i);//get the in info of the succ nodes.
	      temp::TempList* out_node = out_->Look(node);
	      temp::TempList* unioned = out_node->Union(in_succ_i);
	      out_->Set(node, unioned);//union all the succ in.
	  }
	  //get the new in of the current instruction.
	  temp::TempList* out_node_new = out_->Look(node);
	  temp::TempList* diffed = out_node_new->Diff(instr->Def());
	  temp::TempList* in_node_new = instr->Use()->Union(diffed);
	  in_->Set(node, in_node_new);//put the new list into the table. 
	  temp::TempList* new_got_in = in_->Look(node);
	  temp::TempList* new_got_out = out_->Look(node);
	  //check whether the analysis ans has changed for this instr.
	  //if all unchanged, this instr is safe.
 	  if(new_got_in->Check(in_old) == false || new_got_out->Check(out_old) == false) end = 0;//the lopp can not finish now!
      } 
      nodes.reverse();//reverse back
  }
}

//remember that the interf_graph is a undirected graph.
void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  //first put the precolored regs into the interf_graph_.
  live_graph_.interf_graph = new IGraph();
  live_graph_.moves = new MoveList();
  temp::TempList* precolored = reg_manager->Registers();//get precolored registers.
  const std::list<temp::Temp*>& preList = precolored->GetList();
  for(temp::Temp* t : preList) {
      INodePtr node = (live_graph_.interf_graph)->NewNode(t);//add the new temp nodes.
      temp_node_map_->Enter(t, node);//ampping the nodes and the reg.
  }
  //there is an edge between every two precolored regs.
  
  std::list<INodePtr>::iterator it1;
  std::list<INodePtr>::iterator it2;
  std::list<INodePtr> list = (live_graph_.interf_graph)->Nodes()->GetList();//in this timePoint, there are only the nodes created for the precolored registers.
  it1 = list.begin();
  while(it1 != list.end()) {
      it2 = std::next(it1);
      for(; it2 != list.end(); ++it2) {
	  //undirected graph!
          (live_graph_.interf_graph)->AddEdge((*it1), (*it2));//1 --> 2
	  (live_graph_.interf_graph)->AddEdge((*it2), (*it1));//2 --> 1
      }
      it1++;
  }
  std::list<fg::FNodePtr> instrList = flowgraph_->Nodes()->GetList();
  for(fg::FNodePtr instr : instrList) {
      temp::TempList* out_live = out_->Look(instr);
      temp::TempList* used = instr->NodeInfo()->Use();
      temp::TempList* defined = instr->NodeInfo()->Def();
      std::list<temp::Temp*> used_t = used->GetList();
      std::list<temp::Temp*> defined_t = defined->GetList();
      std::list<temp::Temp*> out_live_t = out_live->GetList();
      //add the used temp of the instr into the graph.
      /*
      if(used_t.size() != 0) {
          std::list<temp::Temp*>::iterator it1 = used_t.begin();
	  for(; it1 != used_t.end(); ++it1) {
	      if(*it1 == reg_manager->StackPointer()) continue;
	      if(temp_node_map_->Look(*it1) == nullptr) {
		  INodePtr new_node = (live_graph_.interf_graph)->NewNode(*it1);
		  temp_node_map_->Enter(*it1, new_node);
	      }
	  }
      }
      */
      //put the out-lived temp-nodes into the graph.
      if(out_live_t.size() != 0) {
          std::list<temp::Temp*>::iterator it0 = out_live_t.begin();
	  for(; it0 != out_live_t.end(); ++it0) {
	      if(*it0 == reg_manager->StackPointer()) continue;
	      if(temp_node_map_->Look(*it0) == nullptr) {
	          INodePtr new_node = (live_graph_.interf_graph)->NewNode(*it0);
		  temp_node_map_->Enter(*it0, new_node);
	      }
	  }
      }
      if(defined_t.size() != 0) {
          std::list<temp::Temp*>::iterator it2 = defined_t.begin();
	  for(; it2 != defined_t.end(); ++it2) {
	      if(*it2 == reg_manager->StackPointer()) continue;
	      if(temp_node_map_->Look(*it2) == nullptr) {
		  INodePtr new_node = (live_graph_.interf_graph)->NewNode(*it2);
		  temp_node_map_->Enter(*it2, new_node);
	      }
	  }
      }
  }
  //then try to add the edges between nodes.
  for(fg::FNodePtr instr : instrList) {
       temp::TempList* defined = instr->NodeInfo()->Def();
       temp::TempList* out = out_->Look(instr);
       //std::string assem = instr->NodeInfo()->assem_;//get the assem of the instr.
       if(typeid(*(instr->NodeInfo())) == typeid(assem::MoveInstr)) {
           //it is a movq instruction.
	   assem::MoveInstr* move_instr = static_cast<assem::MoveInstr*>(instr->NodeInfo());
	   temp::TempList* mov_src = instr->NodeInfo()->Use();//get the src temps.
	   //std::list<temp::Temp*>::iterator it = (mov_src->GetList()).begin();//the src of the MoveInstr should have only one.
	   
	   INodePtr src_node = nullptr;
	   for(auto mov_src_i : mov_src->GetList()) {
	       src_node = temp_node_map_->Look(mov_src_i);
	   }
	   for(auto it1 : defined->GetList()) {
	       if(it1 == reg_manager->StackPointer()) continue;
	       INodePtr node1 = temp_node_map_->Look(it1);//this node should be dst node of this move instruction.
	       for(auto it2 : out->GetList()) {
		   if(it2 == reg_manager->StackPointer()) continue;
		   INodePtr node2 = temp_node_map_->Look(it2);
		   if((node2->NodeInfo()) != (src_node->NodeInfo())) {
		       (live_graph_.interf_graph)->AddEdge(node1, node2);
		       (live_graph_.interf_graph)->AddEdge(node2, node1);
		   }
		   else {
		       //if this is a moveInstr, we need to eliminate the same node 
		       if((live_graph_.moves)->Contain(src_node, node1) == true) continue;
		       (live_graph_.moves)->Append(src_node, node1); 
		   }
	       }
	   }
	   
	   /*
	   temp::TempList* add_edge_temps = out->Diff(mov_src);//add edges of the move instr exclude the src_ temps.
	   //const std::list<temp::Temp*>& add_edge_list = add_edge_temps->GetList();
	   for(auto it1 : defined->GetList()) {
	       if(it1 == reg_manager->StackPointer()) continue;
	       INodePtr node1 = temp_node_map_->Look(it1);
	       for(auto it2 : add_edge_temps->GetList()) {
	           if(it2 == reg_manager->StackPointer()) continue;
		   INodePtr node2 = temp_node_map_->Look(it2);
		   (live_graph_.interf_graph)->AddEdge(node1, node2);
		   (live_graph_.interf_graph)->AddEdge(node2, node1);
	       }
	   }
	   for(auto it1 : defined->GetList()) {
	       if(it1 == reg_manager->StackPointer()) continue;
	       INodePtr node1 = temp_node_map_->Look(it1);
	       for(auto it2 : mov_src->GetList()) {
	           if(it2 == reg_manager->StackPointer()) continue;
		   INodePtr node2 = temp_node_map_->Look(it2);
		   if((live_graph_.moves)->Contain(node2, node1) == true) continue;
		   (live_graph_.moves)->Append(node2, node1);
	       }
	   }
	   */
       }
       else {//not a moveInstr.
           for(auto it1 : defined->GetList()) {
	       if(it1 == reg_manager->StackPointer()) continue;
	       INodePtr node1 = temp_node_map_->Look(it1);
	       for(auto it2 : out->GetList()) {
		   if(it2 == reg_manager->StackPointer()) continue;
		   INodePtr node2 = temp_node_map_->Look(it2);
	           (live_graph_.interf_graph)->AddEdge(node1, node2);
		   (live_graph_.interf_graph)->AddEdge(node2, node1);
	       }
	   }
       }
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();//do the liveness analysis first.
  InterfGraph();//use the result of the liveness analysis, draw the Interferce Graph.
}

} // namespace live
