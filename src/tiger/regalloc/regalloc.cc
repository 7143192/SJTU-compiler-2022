#include "tiger/regalloc/regalloc.h"
#include "tiger/output/logger.h"
#include "tiger/runtime/gc/roots/roots.h"

extern frame::RegManager *reg_manager;

extern gc::Roots* roots;

namespace ra {
/* TODO: Put your lab6 code here */
    std::unique_ptr<ra::Result> RegAllocator::TransferResult() {
        return std::move(result_);
    }

    void RegAllocator::RegAlloc() {
	col::Result* res = nullptr;
	std::set<temp::Temp*> newTemps;
	//col::Color* colorG = new col::Color(instr_list_, frame_);
	//try to store every temp that contains a pointer into the frame first.
	//LAB7:
	/*
	live::LiveGraph graph = LivenessAnalysis(instr_list_);//get the interf_graph.
	live::INodeListPtr all_nodes = (graph.interf_graph)->Nodes();
	live::INodeListPtr pointer_list = new live::INodeList();
	for(auto it : all_nodes->GetList()) {
	    temp::Temp* t = it->NodeInfo();
	    if((roots->temp_map).count(t) != 0) {
		//printf("a pointer-containing temp in regalloc:t%d\n", t->Int());
	        pointer_list->Append(it);//get the pointer-containing temps-list.
	    }
	}
	newTemps = RewriteProgram(pointer_list);//rewrite the code according the pointer-containing temps.
	*/
	while(true) { 
	    //live::LiveGraph graph = LivenessAnalysis(instr_list_);
	    //col::Color* colorG = new col::Color(graph, newTemps);
	    //res = colorG->MainFunc();
	    col::Color* colorG = new col::Color(instr_list_, newTemps, frame_);
	    res = colorG->MainFunc();//do the main function of the reg alloc.
	    if(res->spills == nullptr || res->spills->CheckEmpty() == true) break;//no spill nodes, stop the main loop.
	    else {
		newTemps = RewriteProgram(res->spills);//if there are spill nodes, rewrite the assem codes.
	    }
	}

	assem::InstrList* new_instr_list = new assem::InstrList();
	for(auto it : instr_list_->GetList()) {
	    if(typeid(*it) == typeid(assem::MoveInstr)) {
	        //check whether the move instr has the same src and the dst.
		assem::MoveInstr* mov = static_cast<assem::MoveInstr*>(it);
		temp::TempList* src = mov->src_;
		temp::TempList* dst = mov->dst_;
		std::list<temp::Temp*> srcList = src->GetList();
		std::list<temp::Temp*> dstList = dst->GetList();
		temp::Temp* src0;
		for(auto it = srcList.begin(); it != srcList.end(); ++it) {
		    src0 = *it;
		    break;
		}//get the first src.
		temp::Temp* dst0;//the instruction looks like "movq src, dst" always has one src and dst register.
		for(auto it = dstList.begin(); it != dstList.end(); ++it) {
		    dst0 = *it;
		    break;
		}//get the first dst.
		std::string* src_name = res->coloring->Look(src0);
		std::string* dst_name = res->coloring->Look(dst0);
		if((src_name != nullptr && dst_name != nullptr && (*src_name) == (*dst_name)) || (src_name == dst_name)) {//
		    continue;//skip the moveInstr that has the same src and dst.
		}
	    }
	    new_instr_list->Append(it);
	}
	result_->coloring_ = res->coloring;
	result_->il_ = new_instr_list;//store the result into the attr ra::result_.
 	//LAB7:
	//Gen the new pointer maps here.
	//roots->GenPointerMap(frame_);
	//(roots->temp_map).clear();
    }
    
    live::LiveGraph RegAllocator::LivenessAnalysis(assem::InstrList* list) {
        fg::FlowGraphFactory fgf(list);
	fgf.AssemFlowGraph();//get the control floe graph.
	live::LiveGraphFactory lgf(fgf.GetFlowGraph());
	lgf.Liveness();//do the liveness analysis and create the interf_graph.
	return lgf.GetLiveGraph();//get the interf_graph.
    }

    std::set<temp::Temp*> RegAllocator::RewriteProgram(live::INodeListPtr spilledNodes) {
        std::list<live::INodePtr> list = spilledNodes->GetList();
	std::set<temp::Temp*> newTemps;
	//TODO:how to handle the newTemps got from the RewriteProgram function?
	assem::InstrList* prev = new assem::InstrList();//used tp iter.
	for(auto it : instr_list_->GetList()) {
	    prev->Append(it);
	}//init the prev to the old instr_list_.
	int i = 1;
	for(auto spilled : list) {
	    std::list<assem::Instr*> prevList = prev->GetList();
	    //frame_->offset -= 8;//allocate space for the local spilled node in the stackFrame.
	    //LAB7:Allocate a new local var for the spilled nodes
	    frame::Access* in_frame = frame_->AllocLocal(true);
	    //LAB7:
	    if(spilled->NodeInfo()->is_pointer == true) {
	        in_frame->is_pointer = true;
	    }
	    assem::InstrList* newList = new assem::InstrList();//the list used to store the new instructions set.
	    //LAB7:
	    /*
	    if((roots->temp_map).count(spilled->NodeInfo()) != 0) {
		//it contains a pointer before.
		int pointer_type = (roots->temp_map)[spilled->NodeInfo()];
		std::string name = frame_->GetLabel();
		(roots->func_pointers)[name].push_back(0 - frame_->offset);//record the spilled node's offset in this func's stack frame.
		//printf("get a new pointer offset : %d\n", -frame_->offset);
		(roots->pointers_type)[name].push_back(pointer_type);//record the pointer type of the spilled node.
		(roots->temp_map).erase(spilled->NodeInfo());//erase the corresponding temp from the temp_map
	    }
	    */

	    for(auto instr = prevList.begin(); instr != prevList.end(); ++instr) {
	        temp::TempList* used = (*instr)->Use();//the used temp set of this instr.
		temp::TempList* defined = (*instr)->Def();//the defined temp set of this instr.
		temp::Temp* spilled_t = spilled->NodeInfo();//get the detailed temp of the spilled node.
		/*
		//LAB7:
		if((roots->temp_map).count(spilled_t) != 0) {
		    //it contains a pointer before.
		    int pointer_type = (roots->temp_map)[spilled_t];
		    std::string name = frame_->GetLabel();
		    (roots->func_pointers)[name].push_back(0 - frame_->offset);//record the spilled node's offset in this func's stack frame.
		    printf("get a new pointer offset : %d\n", -frame_->offset);
		    (roots->pointers_type)[name].push_back(pointer_type);//record the pointer type of the spilled node.
		    //(roots->temp_map).erase(spilled_t);//erase the corresponding temp from the temp_map
		}
		*/
		if(used->Contain(spilled_t) == false && defined->Contain(spilled_t) == false) {
		    //if the temp is not used and defined by this instr, just put this instr into the newList directly.
		    newList->Append(*instr);
		    /*
		    //LAB7:
		    if((roots->temp_map).count(spilled_t) != 0) {
		        //it contains a pointer before.
			int pointer_type = (roots->temp_map)[spilled_t];
			std::string name = frame_->GetLabel();
			(roots->func_pointers)[name].push_back(0 - frame_->offset);//record the spilled node's offset in this func's stack frame.
			(roots->pointers_type)[name].push_back(pointer_type);//record the pointer type of the spilled node.
		    }
		    */
		}
		else {
		    if(used->Contain(spilled_t) == true) {
		        //this temp is used, so add a new load instr into the newList.
			temp::Temp* newTemp = temp::TempFactory::NewTemp();

			//LAB7:GC
			if(spilled->NodeInfo()->is_pointer) {
			    newTemp->is_pointer = true;
			}

			newTemps.insert(newTemp);//add the newly created temp into the newTemps list.
			//load instr: movq offset(%rsp), dstTemp
			std::string new_load = "movq (";
			std::string label = frame_->GetLabel();//get the labelName of the current frame.
			new_load = new_load + label + "_framesize-";
			new_load += std::to_string(0 - frame_->offset);
			//new_load = new_load + "-" + std::to_string(8 * i);
			new_load += ")(`s0), `d0";
			assem::Instr* new_load_instr = new assem::OperInstr(new_load, new temp::TempList({newTemp}), new temp::TempList({reg_manager->StackPointer()}), nullptr);//create a new instr.
			newList->Append(new_load_instr);//add the newly created instr into the list.
			newList->Append(*instr);//if there is a use for the spilled node, insert the load instr BEFORE the origin instr.
			used->Replace(spilled_t, newTemp);//replace the old spilled node with the newly created one.
			//LAB7:
			/*
			if((roots->temp_map).count(spilled_t) != 0) {
			    //(roots->temp_map)[newTemp] = (roots->temp_map)[spilled_t];
			    (roots->temp_map).erase(spilled_t);
			}
			*/
		    }
		    else {
		        if(defined->Contain(spilled_t) == true) {
			    //this temp is defined, so add a new store instr into the newList.
			    //store instr: movq srcTemp, offset(%rsp)
			    temp::Temp* newTemp = temp::TempFactory::NewTemp();

			    //LAB7:GC
			    if(spilled->NodeInfo()->is_pointer == true) {
			        newTemp->is_pointer = true;
			    }

			    newTemps.insert(newTemp);
			    std::string label = frame_->GetLabel();
			    std::string new_store = "movq `s0, (";
			    new_store = new_store + label + "_framesize-";
			    new_store += std::to_string(-frame_->offset);
			    new_store += ")(`d0)";
			    assem::Instr* new_store_instr = new assem::OperInstr(new_store, new temp::TempList({reg_manager->StackPointer()}), new temp::TempList({newTemp}), nullptr);
			    newList->Append(*instr);
			    newList->Append(new_store_instr);//if there is a define of a spilled node, insert a store instr AFTER the origin instr.
			    defined->Replace(spilled_t, newTemp);//replace the old spilled temp with the newly created one.
			    //LAB7:
			    /*
			    if((roots->temp_map).count(spilled_t) != 0) {
			        //(roots->temp_map)[newTemp] = (roots->temp_map)[spilled_t];
			        (roots->temp_map).erase(spilled_t);
			    }
			    */
			}
		    }
		}
	    }
	    i++;
	    //prev = newList;
	    prev = new assem::InstrList();
	    for(auto it : newList->GetList()) prev->Append(it);
	}
	//prev = newList;
	//spilledNodes->Clear();//clear the spilled nodes.
	instr_list_ = new assem::InstrList();//clear the old instrList
	for(auto it : prev->GetList()) {
	    instr_list_->Append(it);
	}//update the instr_list_.
	return newTemps;
    }
} // namespace ra
