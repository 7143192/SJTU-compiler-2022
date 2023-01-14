#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include <map>
#include <set>
#include <stack>
namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */
  //to fit the spills provided bu the col::Result, we put the main routine of the reg-alloc in the color.cc/h.
  public:
      live::LiveGraph live_graph_;//used to store a live analysis graph.
      //temp::TempList newTemps;
      col::Result* result_;
      std::set<temp::Temp*> newTemps;
      frame::Frame* frame_;
      /*
      assem::InstrList* instr_list_;
      std::set<temp::Temp*> newTemps;
      frame::Frame* frame_;
      */
      //required WorkLists.
      live::INodeListPtr simplifyWorkList;//the list of the nodes that will be simplified.
      live::INodeListPtr freezeWorkList;//the list of the nodes that can be simplified but are move-related.
      live::INodeListPtr spillWorkList;//the list of the nodes that are marked to be spilled.
      //required stack
      live::INodeListPtr selectStack;//the stack used to store the nodes simplified from the graph.
      //required moveList
      live::MoveList* WorkListMoves;//the moveList of the nodes that can be colasece.
      live::MoveList* activeMoves;//the moveList of the nodes that are not ready to do the colasece.
      live::MoveList* frozenMoves;//the moveList of the nodes that won't be coalesced.
      live::MoveList* constrainedMoves;//the moveList of the nodes that are interferd but are also move-related between src_ and the dst_
      live::MoveList* coalescedMoves;//the moveList of the nodes that are already coalesced.
      //required nodes set.
      live::INodeListPtr coalescedNodes;
      live::INodeListPtr coloredNodes;
      live::INodeListPtr spilledNodes;
      //required other data structures.
      std::map<live::INodePtr, live::MoveList*> moveList;//the map between the node and its related moves.
      std::map<live::INodePtr, live::INodePtr> alias;//the alias(the chosen leader) of the coalesced pair of the nodes.
      std::map<temp::Temp*, std::string> precolored;
      std::map<live::INodePtr, int> degree;//the mapping between the node and its degree(note that the interf_graph is a undirected graph.)
      std::set<std::pair<live::INodePtr, live::INodePtr>> adjSet;//the set recording the pair of the interfering edges.
      std::map<live::INodePtr, live::INodeListPtr> adjList;//the mapping between a node and its corresponding edges.
      std::map<temp::Temp*, std::string> color_ans;//the map used to record the color and its coloring node pair.
      std::list<live::INodePtr> initial;
      //the num of the color K(= 15)
      int K;
      //Color() {};
      //Color(live::LiveGraph g);
      Color(live::LiveGraph g, std::set<temp::Temp*> temps, frame::Frame* f);
      Color(assem::InstrList* list, std::set<temp::Temp*> temps, frame::Frame* f);
      //Color(assem::InstrList* list, frame::Frame* frame);
      //Color(live::LiveGraph g, std::list<temp::Temp*> newTemps);
      ~Color() = default;
      //required functions
      col::Result* MainFunc();//the main function entry of the reg-alloc in the level of the color level.
      void simplify();
      void MakeWorkList();
      void Build();
      void Coalesce();
      void Freeze();
      void SelectSpill();
      void AssignColor();
      live::INodeListPtr Adjcent(live::INodePtr node);
      live::MoveList* NodeMoves(live::INodePtr node);
      bool MoveRelated(live::INodePtr node);
      void Decrement(live::INodePtr node);
      void EnableMoves(live::INodeListPtr list);
      bool OK(live::INodePtr n1, live::INodePtr n2);//George
      bool Conservative(live::INodeListPtr list);//briggs
      live::INodePtr GetAlias(live::INodePtr node);
      void Combine(live::INodePtr node1, live::INodePtr node2);
      //void Freeze();
      void FreezeMoves(live::INodePtr node);
      void AddWorkList(live::INodePtr node);
      bool CheckLoopEnd();
      void AddEdge(live::INodePtr from, live::INodePtr to);//add a new edge in the graph.
      void initPreColored();
      bool CheckPreColored(live::INodePtr node);

      //void RewriteProgram(live::INodeListPtr spilledNodes);//rewrite the codes after the reg-alloc with the param spilledNodes.
      live::LiveGraph LivenessAnalysis(assem::InstrList* instrList);//do the liveness analysis.

};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
