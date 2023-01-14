#include "./roots.h"
extern frame::RegManager* reg_manager;
namespace gc {
      Roots::Roots() {}
      /*
      Roots::Roots(assem::InstrList* il, frame::Frame* f, 
		      fg::FGraphPtr graph, std::vector<int> e):instr_list(il), frame(f), flowgraph(graph), escapes(e) 
      {
          return ;
      }
      */
      //this function is used to generate all pointer maps for a frame.
      std::vector<gc::PointerMap> Roots::GenPointerMaps() {
	  std::list<fg::FNodePtr> nodes = flowgraph->Nodes()->GetList();
	  for(fg::FNodePtr node : nodes) {
	      address_in[node] = std::vector<int>();
	      address_out[node] = std::vector<int>();
	  }
          GenAliveAddr();
          //GenAliveTemp();
	  live::LiveGraphFactory* ptr = new live::LiveGraphFactory(flowgraph);
          ptr->LiveMap();
          graph::Table<assem::Instr, temp::TempList> *in_ = ptr->in_.get();
          //std::list<fg::FNodePtr> nodes = flowgraph->Nodes()->GetList();
          for(fg::FNodePtr node : nodes) {
              temp_in[node] = in_->Look(node);
          }
	  //the above part is used to analyse the livenss before one instr, 
	  //which is used to prepare for the pointer-map info collection.
          AliveAfterCall();
          Rewrite();
          std::vector<PointerMap> PointerMaps;
          bool uppermost = false;
	  //if this function is the tigermain,which is the uppermost function in our system,
	  //we should NOT go upwards in the stack when meet this frame.
	  if(frame->name_->Name() == "tigermain") {
	      uppermost = true;
	  }
	  //if the addr_map is empty, there will be no pointermap for the current frame.
	  //a.k.a., the frame do not call functions in its frame.
	  if(addr_map.size() == 0) return PointerMaps;
          for(auto it = addr_map.begin(); it != addr_map.end(); ++it) {
	      auto pair = (*it);
              PointerMap tmp;
              tmp.frame_size = frame->name_->Name() + "_framesize";
	      assem::LabelInstr* got_label = static_cast<assem::LabelInstr*>(pair.first);
              tmp.ret_label = got_label->label_->Name();
              tmp.label = "L" + tmp.ret_label;//L + ret_label
              tmp.next_map = "0";
              if(uppermost) tmp.uppermost = "1";
              tmp.offsets = std::vector<std::string>();
              for(int offset : pair.second) {
                  tmp.offsets.push_back(std::to_string(offset));
	      }
              PointerMaps.push_back(tmp);
          }
	  int size = PointerMaps.size();
	  if(size == 0) return PointerMaps;
	  //the size_t type is a UNSIGNED type!!!
	  //so we should handle the empty case specially.
          for(size_t i = 0; i < PointerMaps.size() - 1; i++) {
              PointerMaps[i].next_map = PointerMaps[i + 1].label;
	  }
	  return PointerMaps;
      }

      //this function is used to get the offset in the src_ part of one move instr.
      std::vector<int> Roots::GetUseOff(assem::Instr* instr) {
          if(typeid(*instr) == typeid(assem::OperInstr)) {
              std::string ass = static_cast<assem::OperInstr *>(instr)->assem_;
              if (ass == "") return escapes;
              if(ass.find("movq") != std::string::npos && ass.find("framesize") != std::string::npos) {
                  int start = ass.find_first_of(' ') + 1;
                  int end = ass.find_first_of(',');
                  std::string src = ass.substr(start, end - start);
                  if(src[0] == '(' && src.find(')') != std::string::npos) {
                      bool negative = false;
		      if(src.find('-') != std::string::npos) negative = true;
                      int OffsetStart;
		      /*
                      if(negative == false) {
			  OffsetStart = src.find_first_of('+') + 1;
		      }
		      */
                      if(negative == true) {
			  OffsetStart = src.find_first_of('-') + 1;
		      }
                      int OffsetEnd = src.find_first_of(')');
                      std::string offset = src.substr(OffsetStart, OffsetEnd - OffsetStart);
                      if(negative == true) {
			  std::vector<int> ans;
			  int o = 0 - std::atoi(offset.c_str());
			  ans.push_back(o);
			  return ans;
		      }
		      /*
                      else {
		          std::vector<int> ans;
			  int o = std::atoi(offset.c_str());
			  ans.push_back(o);
			  return ans;
		      }
		      */
                  }
              }
          }
	  std::vector<int> res;
          return res;
      }

      //this function is used to get the offset in the dst_ part of a mov instr associated with the current frame.
      std::vector<int> Roots::GetDefOff(assem::Instr* instr) {
          if(typeid(*instr) == typeid(assem::OperInstr)) {
              std::string ass = static_cast<assem::OperInstr *>(instr)->assem_;
	      //make sure this instr is the mov that visit the frame.
              if(ass.find("movq") != std::string::npos && ass.find("framesize") != std::string::npos) {
                  int start = ass.find_first_of(',') + 2;
                  std::string dst = ass.substr(start);
                  if(dst[0] == '(' && dst.find(')') != std::string::npos) {
                      bool negative = dst.find('-') != dst.npos;
                      int OffsetStart;
                      if(negative == true) {
			  OffsetStart = dst.find_first_of('-') + 1;
		      }
                      //else OffsetStart = dst.find_first_of('+') + 1;
                      int OffsetEnd = dst.find_first_of(')');
                      std::string offset = dst.substr(OffsetStart, OffsetEnd - OffsetStart);
		      /*
                      if(negative == false) {//positive
		          std::vector<int> ans;
			  int o = std::atoi(offset.c_str());
			  ans.push_back(o);
			  return ans;  
		      }
		      */
                      if(negative == true) {//negative
		          std::vector<int> ans;
			  int o = 0 - std::atoi(offset.c_str());
			  ans.push_back(o);
			  return ans;
		      }
                  }
             }
         }
	 std::vector<int> res;
         return res;
      }

      //this function is used to analyze the mov-liveness in one instr_list.
      //the mov-liveness shows the liveness of every offset in the mov instr, 
      //used to check whether a stack slot is alive when GC.
      void Roots::GenAliveAddr() {
	  std::list<fg::FNodePtr> nodes = flowgraph->Nodes()->GetList();
          std::map<fg::FNodePtr, std::vector<int>> in_old;
	  std::map<fg::FNodePtr, std::vector<int>> out_old;
          int flag = 0;
          do {
              in_old = address_in;
              out_old = address_out;
              flag = 0;
              for(fg::FNodePtr node_ : nodes) {
                  assem::Instr *node_info = node_->NodeInfo();
                  std::vector<int> use_list = GetUseOff(node_info);
                  std::vector<int> def_list = GetDefOff(node_info);
                  address_out[node_] = std::vector<int>();
                  std::list<fg::FNodePtr> succ = node_->Succ()->GetList();
                  for(fg::FNodePtr succ_node : succ) {
		      address_out[node_] = Union(address_out[node_], address_in[succ_node]);
		  }
                  address_in[node_] = Union(Diff(address_out[node_], def_list), use_list);
                  if(address_out[node_] != out_old[node_] || address_in[node_] != in_old[node_]){
		      flag = 1;
		  }
             }
         } while(flag == 1);
      }

      /*
      //temp liveness analysis.
      void Roots::GenAliveTemp() {
          live::LiveGraphFactory* ptr = new live::LiveGraphFactory(flowgraph);
          ptr->LiveMap();
          graph::Table<assem::Instr, temp::TempList> *in_ = ptr->in_.get();
          std::list<fg::FNodePtr> nodes = flowgraph->Nodes()->GetList();
          for(fg::FNodePtr node : nodes) {
              temp_in[node] = in_->Look(node);
          }
      }
      */

      //this function is used to get all the alive pointer-containing offsets and temps 
      //which are used to gen the pointer map.
      void Roots::AliveAfterCall() {
	  std::list<temp::Temp*> callees = reg_manager->CalleeSaves()->GetList();
	  std::vector<std::string> calleeSaved;
	  for(auto it = callees.begin(); it != callees.end(); ++it) {
	      std::string* got = reg_manager->temp_map_->Look(*it);
	      calleeSaved.push_back(*got);
	  }//get all callee saved regs string.
          std::list<fg::FNodePtr> nodes = flowgraph->Nodes()->GetList();
          int flag = 0;
	  /*
          for(fg::FNodePtr node_ : nodes) {
              assem::Instr *ins = node_->NodeInfo();
              if(typeid(*ins) == typeid(assem::OperInstr)) {
                  std::string ass = static_cast<assem::OperInstr *>(ins)->assem_;
		  //this IF is used to find a call OperInstr, and move to the next instr after this callq instr.
		  //the next instr of one callq should be a ret_label.so the next part is to get the required info for the pointer map.
                  if(ass.find("callq") != std::string::npos) {
                      flag = 1;
                      continue;
                  }
              }
              if(flag == 1) {
                  flag = 0;
                  addr_map[ins] = address_in[node_];
                  temp_map_[ins] = std::vector<std::string>();
                  for(auto temp : temp_in[node_]->GetList()) {
                      if(temp->is_pointer == true) {
                          std::string name = *(coloring->Look(temp));
			  if(std::find(temp_map_[ins].begin(), temp_map_[ins].end(), name) != temp_map_[ins].end()) {
			      continue;
			  }
                          if(std::find(calleeSaved.begin(), calleeSaved.end(), name) !=calleeSaved.end()) {
                              temp_map_[ins].push_back(name);//do not put the same node into the list repeatedly.
			  }
                      }
		  }
              }
          }
	  */
	  for(auto it = nodes.begin(); it != nodes.end(); ++it) {
	      assem::Instr* instr = (*it)->NodeInfo();
	      if(typeid(*instr) == typeid(assem::OperInstr)) {
		  std::string ass = static_cast<assem::OperInstr*>(instr)->assem_;
		  if(ass.find("callq") != std::string::npos) {
		      it++;
		      //if the current instr is the callq instr, that means the next instr is the ret_label
		      //,which is important for the pointer map.so we need to analyse the instr in frame that are alive in this instr.
		      fg::FNodePtr node_ = *it;
		      assem::Instr* ins = node_->NodeInfo();
		      addr_map[ins] = address_in[node_];
		      temp_map_[ins] = std::vector<std::string>();
		      for(auto temp : temp_in[node_]->GetList()) {
		          if(temp->is_pointer == true) {
			      std::string name = *(coloring->Look(temp));
			      if(std::find(temp_map_[ins].begin(), temp_map_[ins].end(), name) != temp_map_[ins].end()) {
			          continue;
			      }
			      if(std::find(calleeSaved.begin(), calleeSaved.end(), name) != calleeSaved.end()) {
			          temp_map_[ins].push_back(name);
			      }
			  }
		      }
		  }
	      }
	  }
      }

      //this function is used to rewrite the instr after a callq.
      //we need to put the pointer-containing callee saved regs into the current frame.
      void Roots::Rewrite() {
          std::list<assem::Instr*> ins_list = instr_list->GetList();
          auto iter = ins_list.begin();
          while(iter != ins_list.end()) {
              if(typeid(*(*iter)) == typeid(assem::OperInstr)) {
                  std::string ass = static_cast<assem::OperInstr *>(*iter)->assem_;
                  if(ass.find("callq") == ass.npos) {
                      iter++;
                      continue;
                  }
                  auto label_instr = iter;
                  label_instr++;
                  for(std::string reg : temp_map_[(*label_instr)]) {
                      frame::Access *access = frame->AllocLocal(true);
                      access->setStorePointer();
                      addr_map[(*label_instr)].push_back(frame->offset);
		      std::string ass = "movq ";
		      ass += reg;
		      ass += ", (";
                      ass += frame->name_->Name() + "_framesize" + std::to_string(frame->offset) + ")(%rsp)";
                      assem::Instr* save_pointer = new assem::OperInstr(ass, nullptr, nullptr, nullptr);
                      iter = ins_list.insert(iter, save_pointer);
                      iter++;
                  }
             }
             iter++;
         }
         instr_list = new assem::InstrList();
         for(auto it = ins_list.begin(); it != ins_list.end(); ++it) {
	     instr_list->Append(*it);
	 }
      }

      //this function is used to get the union of vectors v1 and v2.
      std::vector<int> Roots::Union(std::vector<int> v1, std::vector<int> v2) {
          std::vector<int> ans;
	  if(v1.size() == 0) return v2;
	  if(v2.size() == 0) return v1;
	  //v1 U v2
	  for(size_t i = 0; i < v1.size(); ++i) ans.push_back(v1[i]);
	  for(auto it : v2) {
	      if(std::find(ans.begin(), ans.end(), it) == ans.end()) {
	          ans.push_back(it);
	      }
	  }
	  return ans;
      }

      //this function is used to get the difference of the vectors v1 and v2.
      std::vector<int> Roots::Diff(std::vector<int> v1, std::vector<int> v2) {
	  std::vector<int> ans;
	  if(v1.size() == 0) return ans;
	  //v1 - v2
	  for(auto it : v1) {
	      if(std::find(v2.begin(), v2.end(), it) == v2.end()) {
	          ans.push_back(it);
	      }
	  }
	  return ans;
      }

    void Roots::GenPointerMap(frame::Frame* f) {
	/*
	std::string name = f->GetLabel();
	std::vector<int> offsets = func_pointers[name];//the offset at the stack.
	std::vector<int> types = pointers_type[name];
	std::vector<temp::Label*> rets = ret_map[name];
	for(size_t i = 0; i < rets.size(); ++i) {
	    temp::Label* ret = rets[i];
	    bool first = false;
	    if(pre_list.size() == 0) first = true;
	    temp::Label* pointer_label = temp::LabelFactory::NewLabel();
	    //pre_list.push_back(pointer_label);
	    std::string s = "";
	    if(first == true) s = s + GC_ROOTS + ":\n";
	    s = s + (pointer_label->Name()) + ":\n";
	    if(first == false) {
	        temp::Label* label = pre_list.back();
		s = s + ".long " + label->Name() + "\n";
	    }
	    pre_list.push_back(pointer_label);
	    s = s + ".long " + ret->Name() + "\n";
	    s = s + ".long " + std::to_string(0 - f->offset) + "\n";
	    for(size_t j = 0; j < offsets.size(); ++j) {
	        s = s + ".long " + std::to_string(offsets[j]) + "\n";//the offset of a pointer-containing slot.
		//s = s + ".long " + std::to_string(types[j]) + "\n";
	    }
	    s = s + ".long -1\n";//end of one pointer map.
	    pointer_maps.push_back(s);
	}
	*/
	return ;
    }
}
