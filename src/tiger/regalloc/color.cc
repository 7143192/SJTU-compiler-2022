#include "tiger/regalloc/color.h"
#include <climits>
#include <vector>
#include <algorithm>
#include "tiger/runtime/gc/roots/roots.h"

extern frame::RegManager *reg_manager;

extern gc::Roots* roots;

namespace col {
/* TODO: Put your lab6 code here */
    /* 
    Color::Color(live::LiveGraph g):live_graph_(g) {
	K = 15;//there sre 15 kinds of colors in total(beacause we do not use the %rsp.)
	result_ = new col::Result();
	initPreColored();//init the preColored list.
        return ;
    }
    */
    Color::Color(live::LiveGraph g, std::set<temp::Temp*> temps, frame::Frame* f):live_graph_(g),newTemps(temps),frame_(f) {
        K = 15;
	result_ = new col::Result();
	initPreColored();
	return ;
    }

    Color::Color(assem::InstrList* list, std::set<temp::Temp*> temps, frame::Frame* f):newTemps(temps), frame_(f) {
        K = 15;
	result_ = new col::Result();
	initPreColored();
	live::LiveGraph graph = LivenessAnalysis(list);
	live_graph_ = graph;
	return ;
    }
    /*
    Color::Color(assem::InstrList* list, frame::Frame* frame) {
        K = 15;
	result_ = new col::Result();
	frame_ = frame;
	initPreColored();
	instr_list_ = new assem::InstrList();
	for(auto it : list->GetList()) instr_list_->Append(it);
	live::LiveGraph graph = LivenessAnalysis(instr_list_);
	live_graph_ = graph;
    }
    */
    void Color::initPreColored() {
        temp::TempList* list = reg_manager->Registers();//get the registers.
	std::list<temp::Temp*> temps = list->GetList();
	for(auto t = temps.begin(); t != temps.end(); ++t) {//store the physical registers into the precolored list.
	    temp::Temp* it = *t;
	    std::string* name = reg_manager->temp_map_->Look(it);
	    //printf("NAME:%s\n", (*name).c_str());
	    precolored[it] = (*name);//insert the temp-name pair into the precolored-map.
	    //printf("STORED NAME: %s\n", precolored[it].c_str());
	}
    }
    
    bool Color::CheckPreColored(live::INodePtr node) {//used to check whether one temp node is precolored.
        auto it = precolored.find(node->NodeInfo());
	if(it == precolored.end()) return false;
	return true;
    }

    col::Result* Color::MainFunc() {
        //the main routine of the reg-allocation.
	Build();
	MakeWorkList();
	do {
	    int size1 = (simplifyWorkList->GetList()).size();
	    int size2 = (WorkListMoves->GetList()).size();
	    int size3 = (freezeWorkList->GetList()).size();
	    int size4 = (spillWorkList->GetList()).size();
    	    if(size1 != 0) simplify();//can still simplify
	    else {
	        if(size2 != 0) Coalesce();//there are nodes-pairs that can be coalesced.
		else {
		    if(size3 != 0) Freeze();//there are nodes-pair that can not be simplified beacause there move-relation between them.
		    else {
		        if(size4 != 0) SelectSpill();//the simplify stuck, choose the potential spilled nodes.
		    }
                }
	    }
	} while(CheckLoopEnd() == false);//when all the WorkLists are empty, stop the loop directly.
	AssignColor();//assign the colors for the registers.
	//result_->coloring = color_ans;//put the color res into the result_.
	result_->coloring = temp::Map::Empty();//create a new map for the result_->coloring.
	if(color_ans.size() != 0) {
	    for(auto it = color_ans.begin(); it != color_ans.end(); ++it) {
	        result_->coloring->Enter((*it).first, new std::string((*it).second));//store the color info into the result_.
		//std::string name = frame_->name_->Name();
	    }
	}
	result_->spills = new live::INodeList();
        for(auto node : spilledNodes->GetList()) {
	    result_->spills->AppendNew(node);//put the still-spilled nodes after the color into the result.
	    printf("A SPILLED NODE: t%d\n", node->NodeInfo()->Int());
	}
	return result_;//return the result to the regalloc.cc/h level.
    }

    bool Color::CheckLoopEnd() {//used to check whether the main-loop has finished.
        int size1 = (simplifyWorkList->GetList()).size();
	int size2 = (WorkListMoves->GetList()).size();
	int size3 = (freezeWorkList->GetList()).size();
	int size4 = (spillWorkList->GetList()).size();
	if(size1 == 0 && size2 == 0 && size3 == 0 && size4 == 0) return true;
	return false;
    }

    void Color::Build() {
	//this function should be used to init the reg-alloc-required lists.
	//NOTE:as all the WorkLists and the MoveLists are all ptrs, remember to init them here!
	selectStack = new live::INodeList();
	WorkListMoves = new live::MoveList();
	activeMoves = new live::MoveList();
	frozenMoves = new live::MoveList();
	constrainedMoves = new live::MoveList();
	coalescedMoves = new live::MoveList();
	coalescedNodes = new live::INodeList();
	coloredNodes = new live::INodeList();
	spilledNodes = new live::INodeList();
	for(auto it0 = precolored.begin(); it0 != precolored.end(); ++it0) {
	    color_ans[(*it0).first] = (*it0).second;
	}//add the precolored regs into the ans_colored_list.
 	WorkListMoves = (live_graph_.moves);//all the moveInstrs are regarded as the cases that can be merged at first.
	//auto list = WorkListMoves->GetList();
	for(auto list_i : (live_graph_.moves)->GetList()) {
	    //WorkListMoves->Append(list_i.first, list_i.second);
	    live::INodePtr src = list_i.first;
	    live::INodePtr dst = list_i.second;
	    if(moveList.find(src) == moveList.end()) moveList[src] = new live::MoveList();//NOTE:the map second can be null as it is a ptr!
	    moveList[src]->Append(src, dst);
	    if(moveList.find(dst) == moveList.end()) moveList[dst] = new live::MoveList();
	    moveList[dst]->Append(src, dst);
	}//init the moveList
	live::INodeListPtr node_list = (live_graph_.interf_graph)->Nodes();
	for(auto it : node_list->GetList()) {
	    degree[it] = 0;//init degree are all 0.
	    initial.push_back(it);
	    adjList[it] = new live::INodeList();//
	}
	for(auto it1 : node_list->GetList()) {
	    live::INodeListPtr adj_list = it1->Adj();
	    for(auto it2 : adj_list->GetList()) {
	        AddEdge(it1, it2);//add a new edge between a node and its every succ and prev.
	    }
	}
        return ;
    }

    void Color::MakeWorkList() {
	//the inital nodes in the PPT are just the nodes provided in the interf_graph, beacause the MakeWorkList is envoked at the start of the RegAlloc.
	simplifyWorkList = new live::INodeList();
	freezeWorkList = new live::INodeList();
	spillWorkList = new live::INodeList();
	//std::list<live::INodePtr> initial = (live_graph_.interf_graph)->Nodes()->GetList();
	std::list<live::INodePtr>::iterator it = initial.begin();
	while(it != initial.end()) {
	    if(CheckPreColored((*it)) == true) {
	        it++;
		continue;
	    }
	    int it_degree = degree[*it];
	    if(it_degree >= K) spillWorkList->AppendNew(*it);//d >= k, be potential spilled node(s).
	    else {
	        if(MoveRelated(*it) == true) freezeWorkList->AppendNew(*it);//if it still has a list of move dsts/srcs, store into the freezeWorkList.
		else {
		    simplifyWorkList->AppendNew(*it);//if d < K and not moveRelated, store into the simplifyList.
		}
	    }
	    it++;
	}
        return ;
    }

    void Color::simplify() {//simplify a node from the list and push into the stack.
	std::list<live::INodePtr> list = simplifyWorkList->GetList();
	int time = 0;
	live::INodePtr target = nullptr;
	for(auto it = list.begin(); it != list.end() && time != 1; ++it) {
	    target = *it;
	    time++;
	}//get the first node from the list to simplify.
	simplifyWorkList->DeleteNode(target);//remove the target from the work-list.
	selectStack->AppendNew(target);//push the selected node into the selectStack.
	live::INodeListPtr adj = Adjcent(target);//get the corresponding nodes of target.
	for(auto adj_node : adj->GetList()) {
	    Decrement(adj_node);//decrement the neighbors's degree.
	}
        return ;
    }

    void Color::Coalesce() {
        //this function is used to coalesce a pair of one move instr in the WorkListMoves list.
	int time = 0;
	std::list<std::pair<live::INodePtr, live::INodePtr>> got_list = (WorkListMoves->GetList());
	
	std::pair<live::INodePtr, live::INodePtr> p;
	for(auto it = got_list.begin(); it != got_list.end() && time != 1; ++it) {
	    time++;
	    p = (*it);
	}//choose the first pair in the WorkListMoves to coalesce.
	live::INodePtr x = p.first;
	live::INodePtr y = p.second;
	live::INodePtr alias_x = GetAlias(x);
	live::INodePtr alias_y = GetAlias(y);
	live::INodePtr u = nullptr;
	live::INodePtr v = nullptr;
	if(CheckPreColored(y) == true) {
	    u = alias_y;
	    v = alias_x;
	}//make sure the precolored node is at the first place, which is useful for the following OK and the conservative function.
	else {
	    u = alias_x;
	    v = alias_y;
	}//if no precolored nodes, place the order in the origin order.
	std::pair<live::INodePtr, live::INodePtr> new_p = std::make_pair(u, v);
	WorkListMoves->Delete(x, y);
	if(u == v) {
	    //the src and the dst are the same node, coalesce directly.
	    coalescedMoves->Append(x, y);
	    AddWorkList(u);//beacause node u and v will be coalesced, and u is the alias, so just need to try to add the u to the workList.
	}
	else {
	    if((adjSet.find(new_p) != adjSet.end()) || (CheckPreColored(v) == true)) {
		//if this pair is moveRelated but interfered, make it constrained
		//if the v is precolored, make it constrained too.
		//and in the above cases, the pair can NOT coalesce successfully.
	        constrainedMoves->Append(x, y);
		AddWorkList(u);
		AddWorkList(v);//beacause u and v can not be coalesced, so need to try to add both  of them into the WorkList.
	    }
	    else {
		//this pair CAN be coalesced.
	        live::INodeListPtr adj_got = Adjcent(v);
		bool check = true;
		for(auto adj_got_i : adj_got->GetList()) {
		    if(OK(adj_got_i, u) == false) check = false;
		}
		//if u is precolored<-->George, otherwise <--> Briggs
		//NOTE the order of the pair as params.
		if((CheckPreColored(u) == true && check == true) || (CheckPreColored(u) == false && Conservative(Adjcent(v)->Union(Adjcent(u))) == true)) {
		    coalescedMoves->Append(x, y);
		    Combine(u, v);
		    AddWorkList(u);
		}
		else {
		    activeMoves->Append(x, y);//fail to coalesce.
		}
	    }
	}
        return ;
    }

    void Color::Freeze() {
	std::list<live::INodePtr> list = freezeWorkList->GetList();
	int time = 0;
	live::INodePtr node = nullptr;
	for(auto it = list.begin(); it != list.end() && time != 1; ++it) {
	    time++;
	    node = *it;
	}//choose the first node directly.
	freezeWorkList->DeleteNode(node);
	simplifyWorkList->AppendNew(node);
	FreezeMoves(node);
        return ;
    }

    void Color::SelectSpill() {
	//use a heuristic algorithm to get the better node to spill.
	//we should try our best to make sure that there is a lower probability to require spill again.
	//so we should choose the node with the BIGGEST degree to spill
	//2022/11/30-Version 1
	std::list<live::INodePtr> list = spillWorkList->GetList();
	live::INodePtr node = list.front();
	std::list<live::INodePtr>::iterator it = list.begin();
	int max = degree[node];
	while(it != list.end()) {
	    /*
	    temp::Temp* info = (*it)->NodeInfo();
	    if(newTemps.Contain(info) == true) {
		it++;
		continue;//do not spill the newly added temps.
	    }
	    */
	    //the temps added at the function RewriteProgram can not be spilled
	    if(newTemps.find((*it)->NodeInfo()) != newTemps.end()) {
	        it++;
		continue;
	    }
	    if(spilledNodes->Contain(*it) || CheckPreColored(*it) == true) {
	        it++;
		continue;
	    } 
	    if(degree[*it] > max) {
	        max = degree[*it];
		node = *it;
	    }
	    it++;
	}//choose a node to simplify
	//this just means this node MAY be colored, but there are still probablity that it has to be spilled.
	//which is decided by the AssignColor function.
	spillWorkList->DeleteNode(node);
	simplifyWorkList->AppendNew(node);
	FreezeMoves(node);
        return ;
    }

    void Color::AssignColor() {
	std::list<live::INodePtr> list = selectStack->GetList();
        while(selectStack->CheckEmpty() == false) {
	    //std::list<INodePtr> list = selectStack->GetList();
	    live::INodePtr node = nullptr;
	    //this part is to make the INodeList acts like a normal Stack.
	    for(auto it = list.rbegin(); it != list.rend(); ++it) {
	        node = (*it);
		break;
	    }//every time get the last element of the list.
	    list.pop_back();//pop the node from the got_list
	    selectStack->DeleteNode(node);
	    std::vector<std::string> OKColors;
	    //std::set<std::string> colors;
	    //std::vector<int> used;
	    for(auto it : precolored) {
	        OKColors.push_back(it.second);//get the OKColors(a.k.a., the name of the physical registers.)
		//used.push_back(0);
	    }
	    live::INodeListPtr got = adjList[node];
	    for(auto it : got->GetList()) {
		//live::INodeListPtr unioned = coloredNodes->Union(precolored);
		if(coloredNodes->Contain(GetAlias(it)) || CheckPreColored(GetAlias(it))) {
		    std::string ss = color_ans[GetAlias(it)->NodeInfo()];
		    //if a coalesced node's leader has been colored, make them have the same color.
		    for(size_t i = 0; i < OKColors.size(); ++i) {
		        if(OKColors[i] == ss) {
			    //printf("erase the color %s from the OKColors!\n", ss.c_str());
			    OKColors.erase(OKColors.begin() + i);//clear the corresponding color.
			    break;
			    //OKColors.erase(i);//erase the corresponding color.
			}
		    }
		}
	    }
	    
	    if(OKColors.size() == 0) {
	        spilledNodes->AppendNew(node);//if there is no color to use, put this node into the list of spilledNodes. 
	    }
	    
	    else {
	        coloredNodes->AppendNew(node);
		//choose the first rest color in the OKColors.
		color_ans[node->NodeInfo()] = OKColors[0];
		//color_ans[node->NodeInfo()] = *colors.begin();
		//printf("STORED ASSIGN COLOR: t%d <--> %s\n", node->NodeInfo()->Int(), color_ans[node->NodeInfo()].c_str());
	    }
	}
	//put the coalesced nodes into the list.
	//beacause we only consider the aliasNodes before
	//so we also need to add the nodes coalesced into the alias into the color_ans.
	//as the "coalesced" does not delete the coalesced node, just make them into the same group.
	for(auto it : coalescedNodes->GetList()) {
	    auto root = GetAlias(it);//get the leader of the coalesced node, and they should have the same color.
	    auto got = color_ans.find(root->NodeInfo());
	    //auto p = std::make_pair(it->NodeInfo(), got->second);
	    //color_ans.insert(p);
	    color_ans[it->NodeInfo()] = got->second;//NOTE:the use of the map, if the key already exists, the insert() does NOT work, which will make the val be empty str.
	    //printf("COALESCED COLOR ASSIGN: %d <--> %s \n", it->NodeInfo()->Int(), (got->second).c_str());
            printf("STORE COALESCED ASSIGN: t%d <--> %s\n", it->NodeInfo()->Int(), color_ans[it->NodeInfo()].c_str());
	}
        return ;
    }

    //this function is used to check whether a node has some move src/dst exists.
    bool Color::MoveRelated(live::INodePtr node) {
        live::MoveList* list = NodeMoves(node);
	return ((list->GetList()).size()) != 0;
    }

    //this function is to get the move-related nodes of a given node.
    //the formula is:moveList[node](1) intersect (activeMoves union WorkListMoves)(2)
    //the (1) part get the related-move-list of the given node, which may include some frozenNodes, which can not be coalesced or simplified.
    //the (2) part get the nodes that can be coalesced or simplified.
    live::MoveList* Color::NodeMoves(live::INodePtr node) {
	std::map<live::INodePtr, live::MoveList*>::iterator it;
	it = moveList.find(node);
	if(it == moveList.end()) return new live::MoveList();
	live::MoveList* unioned = activeMoves->Union(WorkListMoves);
 	live::MoveList* ans = (it->second)->Intersect(unioned);
	return ans;	
    }

    //this function is used to get the nodes listed in its adjcent list.(lin jie biao)
    //the formula is:adjList[node](1)\(selectedStack union coalescedNodes)(2)
    //the (1) part get the nodes originally listed in the adjcent list.
    //the (2) get the nodes that are already removed from the list(or get into some coalsced subSet.)
    live::INodeListPtr Color::Adjcent(live::INodePtr node) {
	live::INodeListPtr list = adjList[node];
	live::INodeListPtr unioned = selectStack->Union(coalescedNodes);
        live::INodeListPtr ans = list->Diff(unioned);
	return ans;
    }

    //this function is used to update the degree of the given node
    //NOTE:the interf_graph should be a undirected graph.
    void Color::Decrement(live::INodePtr node) {
	if(CheckPreColored(node)) return ;//we don't need to decrement the degree for the precolored nodes.
	int d = degree[node];
	degree[node] = d - 1;//decrement the degree first.
	if(d == K) {
	    //the old-spilled node now can be a node that can be coalesced or simplified.
	    live::INodeListPtr list = Adjcent(node);
	    //list->Append(node);//append the current node into the adjcent list.
	    live::INodeListPtr new_list = new live::INodeList();
	    new_list->AppendNew(node);
	    //list->Union(new_list);
	    EnableMoves(list->Union(new_list));
	    spillWorkList->DeleteNode(node);
	    if(MoveRelated(node)) {
	        freezeWorkList->AppendNew(node);
	    }
	    else {
	        simplifyWorkList->AppendNew(node);
	    }
	}
    }

    //this function is used to make some move-pair enabled
    //the "enabled" means that some pairs used to be regarded as not-coalesced MAY be coalesced now.
    //but whether they can be coalesced depends on the res of the Coalesce() function.
    void Color::EnableMoves(live::INodeListPtr nodes) {
	//EnableMoves func is used to make some nodes used to not coalesced may coalesce now.
        std::list<live::INodePtr> list = nodes->GetList();
	for(auto node : list) {
	    live::MoveList* moves = NodeMoves(node);
	    for(auto mov : moves->GetList()) {
	        if(activeMoves->Contain(mov.first, mov.second)) {//if this pair used to be regarded as not coalesced, remove this pair.
		    activeMoves->Delete(mov.first, mov.second);
		    WorkListMoves->Append(mov.first, mov.second);
		}
	    }
	}
    }

    //get the "leader" of the given node.
    //the "leader" is the node that used to notify the group of the nodes coalesced togther.
    live::INodePtr Color::GetAlias(live::INodePtr node) {
        if(coalescedNodes->Contain(node) == true) {
	    std::map<live::INodePtr, live::INodePtr>::iterator it = alias.find(node);
	    //if(it == alias.end()) return nullptr;
	    return GetAlias(it->second);
	}
	return node;
    }

    //the George function.
    //if a node is d < K or interfed with another node, this pair can be coalesced.
    bool Color::OK(live::INodePtr n1, live::INodePtr n2) {
	//NOTE:in thisimplement, the node n2 is regarded as the analysis target.
	//and George is used at the case that the precolored reg is as the n1 node.
	//so we should always put the precolored node at the first place if any.
	std::pair<live::INodePtr, live::INodePtr> p = std::make_pair(n1, n2);
	int d = degree[n1];
	if(d < K) return true;//the n1 has a degree less than K.
	if(adjSet.count(p) == 1) return true;//n1 is a neighbor of the n2.
	std::map<temp::Temp*, std::string>::iterator it = precolored.find(n1->NodeInfo());
	if(CheckPreColored(n1)) return true;//n1 is precolored, so it is OK.
	return false;
    }

    //the Briggs function.
    //this function operates on all the neighbors of one node.
    //if the num of the nodes that has d >= K in the neighbors is < K, return true.
    bool Color::Conservative(live::INodeListPtr list) {//Briggs
        int num = 0;
	std::list<live::INodePtr> nodes = list->GetList();
	for(auto node : nodes) {
	    int d = degree[node];
	    if(d >= K || CheckPreColored(node)) num++;
	}
	if(num < K) return true;
	return false;
    }

    //this function is used to update some WorkLists.
    void Color::AddWorkList(live::INodePtr node) {
        std::map<temp::Temp*, std::string>::iterator it = precolored.find(node->NodeInfo());
	bool flag = CheckPreColored(node);
	bool flag1 = MoveRelated(node);
	bool flag2 = degree[node] < K;
	if(!flag && !flag1 && flag2) {
	    freezeWorkList->DeleteNode(node);
	    simplifyWorkList->AppendNew(node);
	}//if a newly coalesced node is not precolored, not moveRelated and its new degree is less than K,make it simplify.
    }

    //this function is used to coalesce one pair that is shown can be merged in the function Coalesce().
    void Color::Combine(live::INodePtr node1, live::INodePtr node2) {
	//combine the node node2 into the node node1.
	if(freezeWorkList->Contain(node2) == true) {
	    freezeWorkList->DeleteNode(node2);
	}//if this node used to be simplified but moveRelated, now move it out.
	//beacause now it can combine means that it is no longer moveRelated.
	else {
	    spillWorkList->DeleteNode(node2);
	}
	coalescedNodes->AppendNew(node2);
	alias[node2] = node1;//make the node1 as the leader of the node after the coalescing.
	//moveList[node1] = moveList[node1]->Union(moveList[node2]);
	live::MoveList* tmpList1 = moveList[node1]->Union(moveList[node2]);
	moveList[node1] = tmpList1;//as node1 is the new alias, so the moveList[node1] is updated to moveList[1] U moveList[2].
	//EnableMoves(new live::INodeList({node2}));
	live::INodeListPtr ans = new live::INodeList();
	ans->AppendNew(node2);
	//as node2 is coalesced into the node1, so we need to make the move-pairs related to node2 before in the activeList into the workListMoves.
	//beacause the structure of the graph is changed, so we need to reconsider the probability to coalesce some pairs.
	//just like the MLFQ learned in the ICS course, we nedd always to put the lowest level's events into the highest level to get the resource to execute.
	EnableMoves(ans);
	live::INodeListPtr list = Adjcent(node2);
	for(auto it : list->GetList()) {//interfer every neighbor of the node2 with the node1 
	    AddEdge(it, node1);
	    Decrement(it);//the AddEdge function will add a degree to the node it, so we need to decrement by 1 since the node2 is coalesced into the node1.
	}
	if(degree[node1] >= K && freezeWorkList->Contain(node1) == true) {//if the merged node change the attribute, change its inside-WorkList.
	    freezeWorkList->DeleteNode(node1);
	    spillWorkList->AppendNew(node1);
	}
        return ;
    }

    //after all the nodes in the WorkListMoves are coalesced,
    //the nodes can still not be coalesced left in the freezeWorkList.
    //this function is used to freeze the related moves of this node.
    //the "freeze" means that these pairs can not be considered to be coalesced anymore. 
    void Color::FreezeMoves(live::INodePtr node) {
	live::MoveList* list = NodeMoves(node);
	for(auto it : list->GetList()) {
	    live::INodePtr v = nullptr;
	    if(GetAlias(it.second) == GetAlias(node)) {
	        v = GetAlias(it.first);
	    }
	    else {
	        v = GetAlias(it.second);
	    }
	    activeMoves->Delete(it.first, it.second);
	    frozenMoves->Append(it.first, it.second);
	    live::MoveList* list1 = NodeMoves(v);
	    int size = (list1->GetList()).size();
	    if(size == 0 && degree[v] < K && !CheckPreColored(v)) {
	        freezeWorkList->DeleteNode(v);
		simplifyWorkList->AppendNew(v);
	    }
	}
        return ;
    }

    void Color::AddEdge(live::INodePtr from, live::INodePtr to) {
	std::pair<live::INodePtr, live::INodePtr> p = std::make_pair(from, to);
	if(adjSet.find(p) == adjSet.end() && from != to) {//if this edge does not exist before.
	    std::pair<live::INodePtr, live::INodePtr> new_edge_1 = std::make_pair(from, to);
	    std::pair<live::INodePtr, live::INodePtr> new_edge_2 = std::make_pair(to, from);
	    adjSet.insert(new_edge_1);
	    adjSet.insert(new_edge_2);
	    if(CheckPreColored(from) == false) {
		if(adjList.find(from) == adjList.end()) adjList[from] = new live::INodeList();//remember to initialize when used fo the first time.
	        adjList[from]->AppendNew(to);
		degree[from]++;
	    }
	    if(CheckPreColored(to) == false) {
		if(adjList.find(to) == adjList.end()) adjList[to] = new live::INodeList();
	        adjList[to]->AppendNew(from);
		degree[to]++;
	    }
	}	
    }
    
    live::LiveGraph Color::LivenessAnalysis(assem::InstrList* list) {
        fg::FlowGraphFactory fgf(list);
	fgf.AssemFlowGraph();//get the control floe graph.
	live::LiveGraphFactory lgf(fgf.GetFlowGraph());
	lgf.Liveness();//do the liveness analysis and create the interf_graph.
	return lgf.GetLiveGraph();//get the interf_graph.
    }
    /*
    void Color::RewriteProgram(live::INodeListPtr spilledNodes) {
        std::list<live::INodePtr> list = spilledNodes->GetList();
	//std::list<temp::Temp*> newTemps;
	//TODO:how to handle the newTemps got from the RewriteProgram function?
	assem::InstrList* prev = new assem::InstrList();//used tp iter.
	for(auto it : instr_list_->GetList()) {
	    prev->Append(it);
	}
	int i = 1;
	for(auto spilled : list) {
	    std::list<assem::Instr*> prevList = prev->GetList();
	    frame_->offset -= 8;//allocate space for the local spilled node in the stackFrame.
	    assem::InstrList* newList = new assem::InstrList();//the list used to store the new instructions set.
	    for(auto instr = prevList.begin(); instr != prevList.end(); ++instr) {
	        temp::TempList* used = (*instr)->Use();//the used temp set of this instr.
		temp::TempList* defined = (*instr)->Def();//the defined temp set of this instr.
		temp::Temp* spilled_t = spilled->NodeInfo();//get the detailed temp of the spilled node.
		if(used->Contain(spilled_t) == false && defined->Contain(spilled_t) == false) {
		    //if the temp is not used and defined by this instr, just put this instr into the newList directly.
		    newList->Append(*instr);
		}
		else {
		    if(used->Contain(spilled_t) == true) {
		        //this temp is used, so add a new load instr into the newList.
			temp::Temp* newTemp = temp::TempFactory::NewTemp();
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
		    }
		    else {
		        if(defined->Contain(spilled_t) == true) {
			    //this temp is defined, so add a new store instr into the newList.
			    //store instr: movq srcTemp, offset(%rsp)
  			    temp::Temp* newTemp = temp::TempFactory::NewTemp();
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
	//return newTemps;
    }
    */
} // namespace col
