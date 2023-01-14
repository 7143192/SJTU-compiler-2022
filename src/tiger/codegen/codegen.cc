#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

//#include "tiger/runtime/gc/roots/roots.h"

#include "tiger/frame/x64frame.cc"
extern frame::RegManager *reg_manager;
//extern gc::Roots* roots;

extern std::vector<std::string> ret_pointers;
std::vector<int> pointer_frame;
std::vector<int> pointer_arg;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {
//LAB7:GC
bool CheckFramePointer(int offset) {
    bool ans = std::find(pointer_frame.begin(), pointer_frame.end(), offset) != pointer_frame.end();
    return ans;
}

bool CheckReturnPointer(temp::Label* label) {
    std::string name = label->Name();
    if(name == "alloc_record" || name == "init_array") return true;//only the externalcall init_array and alloc_record can be regarded.
    bool ans = std::find(ret_pointers.begin(), ret_pointers.end(), name) != ret_pointers.end();
    return ans;
}

void CollectRoots(frame::Frame* f) {
    std::list<frame::Access*> formal_list = f->formals;
    int i = 0;
    for(auto it = formal_list.begin(); it != formal_list.end(); ++it) {
        i++;//
	auto access = *it;
	if(typeid(*access) == typeid(frame::InFrameAccess)) {
	    frame::InFrameAccess* frame_access = static_cast<frame::InFrameAccess*>(access);
	    if(frame_access->is_pointer == true) {
	        pointer_frame.push_back(frame_access->offset);
	    }
	}
	if(typeid(*access) == typeid(frame::InRegAccess)) {
	    frame::InRegAccess* reg_access = static_cast<frame::InRegAccess*>(access);
	    if(reg_access->reg->is_pointer == true) {
	        pointer_arg.push_back(i);
	    }
	}
    }
}

bool CheckArgPointer(int pos) {
    return std::find(pointer_arg.begin(), pointer_arg.end(), pos) != pointer_arg.end();
}

//this function is used to pass the pointer downward through the op instr and the mov instr.
void PassPointer(assem::Instr* instr) {
    if(typeid(*instr) == typeid(assem::OperInstr)) {
        assem::OperInstr *add_instr = static_cast<assem::OperInstr *>(instr);
	//addq, subq 
        if((add_instr->assem_.find("addq") != std::string::npos ||
         add_instr->assem_.find("subq") != std::string::npos) && add_instr->dst_ != nullptr 
		&& add_instr->src_ != nullptr && add_instr->src_->GetList().front()->is_pointer == true) {
            add_instr->dst_->GetList().front()->is_pointer = true;
	}
	//imulq, idivq
	assem::OperInstr *mul_instr = static_cast<assem::OperInstr *>(instr);
        if((mul_instr->assem_.find("mul") != std::string::npos ||
         mul_instr->assem_.find("div") != std::string::npos) && mul_instr->dst_ != nullptr 
		&& mul_instr->src_ != nullptr && mul_instr->src_->GetList().front()->is_pointer == true) {
            //mul_instr->dst_->GetList().front()->is_pointer = true;
	    std::list<temp::Temp*> list = mul_instr->dst_->GetList();
	    for(auto it = list.begin(); it != list.end(); ++it) {
	        (*it)->is_pointer = true;
	    }
	}
    }

    if(typeid(*instr) == typeid(assem::MoveInstr)) {
	//movq src, dst
        assem::MoveInstr *mov_instr = static_cast<assem::MoveInstr *>(instr);
        if(mov_instr->dst_ && mov_instr->src_)
            mov_instr->dst_->GetList().front()->is_pointer = mov_instr->src_->GetList().front()->is_pointer;
    }
}

void CodeGen::Push(temp::Temp* toPush, int i) {
    return ;
}

void CodeGen::Pop(temp::Temp* toPop, int i) {
    return ;
}

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  auto* list = new assem::InstrList();
  fs_ = frame_->name_->Name();
   
  //LAB7:GC
  cg::CollectRoots(frame_);

  temp::Temp* rbx_1 = temp::TempFactory::NewTemp();
  temp::Temp* rbp_1 = temp::TempFactory::NewTemp();
  temp::Temp* r12_1 = temp::TempFactory::NewTemp();
  temp::Temp* r13_1 = temp::TempFactory::NewTemp();
  temp::Temp* r14_1 = temp::TempFactory::NewTemp();
  temp::Temp* r15_1 = temp::TempFactory::NewTemp();
  //save the callee-saved regs before the function.
  assem::MoveInstr* mov1 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({rbx_1}), new temp::TempList({reg_manager->GetRegister(9)}));
  assem::MoveInstr* mov2 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({rbp_1}), new temp::TempList({reg_manager->GetRegister(10)}));
  assem::MoveInstr* mov3 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r12_1}), new temp::TempList({reg_manager->GetRegister(11)}));
  assem::MoveInstr* mov4 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r13_1}), new temp::TempList({reg_manager->GetRegister(12)}));
  assem::MoveInstr* mov5 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r14_1}), new temp::TempList({reg_manager->GetRegister(13)}));
  assem::MoveInstr* mov6 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({r15_1}), new temp::TempList({reg_manager->GetRegister(14)}));
  cg::PassPointer(mov1);
  list->Append(mov1);
  cg::PassPointer(mov2);
  list->Append(mov2);
  cg::PassPointer(mov3);
  list->Append(mov3);
  cg::PassPointer(mov4);
  list->Append(mov4);
  cg::PassPointer(mov5);
  list->Append(mov5);
  cg::PassPointer(mov6);
  list->Append(mov6);
  
  /*
  int i = 1;
  while(i <= 6) {
      frame_->offset -= 8;
      std::string assem1 = "movq `s0, (" + frame_->GetLabel() + "_framesize - " + std::to_string(8 * i) + ")(`s1)";
      assem::OperInstr* op = new assem::OperInstr(assem1, new temp::TempList({reg_manager->GetRegister(i + 8), reg_manager->StackPointer()}), nullptr, nullptr);
      list->Append(op);
      i++;
  }
  */
  for(auto stm : traces_->GetStmList()->GetList()) {
      stm->Munch(*list, fs_);
  }
  
  //restore the callee-saved regs after the function.
  assem::MoveInstr* restore1 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(9)}), new temp::TempList({rbx_1}));
  assem::MoveInstr* restore2 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(10)}), new temp::TempList({rbp_1}));
  assem::MoveInstr* restore3 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(11)}), new temp::TempList({r12_1}));
  assem::MoveInstr* restore4 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(12)}), new temp::TempList({r13_1}));
  assem::MoveInstr* restore5 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(13)}), new temp::TempList({r14_1}));
  assem::MoveInstr* restore6 = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(14)}), new temp::TempList({r15_1}));
  cg::PassPointer(restore6);
  list->Append(restore6);
  cg::PassPointer(restore5);
  list->Append(restore5);
  cg::PassPointer(restore4);
  list->Append(restore4);
  cg::PassPointer(restore3);
  list->Append(restore3);
  cg::PassPointer(restore2);
  list->Append(restore2);
  cg::PassPointer(restore1);
  list->Append(restore1);//restore reversely.
  /*
  i = 1;
  while(i <= 6) {
      //frame_->offset -= 8;
      std::string assem1 = "movq (" + frame_->GetLabel() + "_framesize - " + std::to_string(8 * i) + ")(`s0), `s1";
      assem::OperInstr* op = new assem::OperInstr(assem1, new temp::TempList({reg_manager->StackPointer(), reg_manager->GetRegister(i + 8)}), nullptr, nullptr);
      list->Append(op);
      i++;
  }
  */
  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
    assem::Instr* instr = new assem::LabelInstr(temp::LabelFactory::LabelString(label_), label_);//add the label to the instr-list directly.
    cg::PassPointer(instr);
    instr_list.Append(instr);
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  assem::Targets* target = new assem::Targets(jumps_);
  //JMP instr: jmp dst
  assem::OperInstr* jmpInstr = new assem::OperInstr("jmp `j0", nullptr, nullptr, target);
  jmpInstr->JumpDirectly = 1;
  cg::PassPointer(jmpInstr);
  instr_list.Append(jmpInstr);  
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //the format of the CJUMP instr:
  //cmpq t1, t2
  //cjmp dst
  //(or fall through)
  temp::Temp* t1 = left_->Munch(instr_list, fs);
  temp::Temp* t2 = right_->Munch(instr_list, fs);
  std::string cjmpName = "";//used to remeber the name of the cjmp type.
  if(op_ == tree::EQ_OP) cjmpName = "je";
  if(op_ == tree::NE_OP) cjmpName = "jne";
  if(op_ == tree::GT_OP) cjmpName = "jg";
  if(op_ == tree::GE_OP) cjmpName = "jge";
  if(op_ == tree::LT_OP) cjmpName = "jl";
  if(op_ == tree::LE_OP) cjmpName = "jle";
  cjmpName += " `j0";
  temp::TempList* list = new temp::TempList({t2, t1});
  std::vector<temp::Label*>* label = new std::vector<temp::Label*>();
  label->push_back(true_label_);
  label->push_back(false_label_);
  assem::OperInstr* cmp = new assem::OperInstr("cmpq `s0, `s1", nullptr, list, nullptr);//cmpq t1, t2
  assem::OperInstr* cjmp = new assem::OperInstr(cjmpName, nullptr, nullptr, new assem::Targets(label));//cjmp-type dst
  cg::PassPointer(cmp);
  instr_list.Append(cmp);
  cg::PassPointer(cjmp);
  instr_list.Append(cjmp);
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //temp::Temp* t1 = src_->Munch(instr_list, fs);
  //temp::Temp* t2 = dst_->Munch(instr_list, fs);
  if(typeid(*dst_) == typeid(tree::TempExp)) {
      temp::Temp* mov_dst = static_cast<tree::TempExp*>(dst_)->temp_;
      //if the dst has a type of register, the src should have a type of imm, mem, reg.
      /*
      if(typeid(*src_) == typeid(tree::TempExp)) {
	  //movq t1, t2
	  temp::Temp* mov_src = static_cast<tree::TempExp*>(src_)->Munch(instr_list, fs);
	  //temp::Temp* p = static_cast<tree::TempExp*>(src_);
	  //if(p == reg_manager->FramePointer()) return ;
          assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({mov_dst}), new temp::TempList({mov_src}));
	  instr_list.Append(mov);
      }
      if(typeid(*src_) == typeid(tree::ConstExp)) {
	  //movq imm, t2
          temp::Temp* mov_src = static_cast<tree::ConstExp*>(src_)->Munch(instr_list, fs);
	  assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({mov_dst}), new temp::TempList({mov_src})); 
	  instr_list.Append(mov);
      }
      if(typeid(*src_) == typeid(tree::MemExp)) {
	  //movq imm(mem), t2 
          temp::Temp* mov_src = static_cast<tree::MemExp*>(src_)->Munch(instr_list, fs);
	  assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({mov_dst}), new temp::TempList({mov_src}));
	  instr_list.Append(mov);
      }
      */
      temp::Temp* src_reg = src_->Munch(instr_list, fs);
      assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({mov_dst}), new temp::TempList({src_reg}));
      cg::PassPointer(mov);
      instr_list.Append(mov);
  }
  else {
      if(typeid(*dst_) == typeid(tree::MemExp)) {
          //movq t1, imm(mem)
	  //LAB7:GC	  
	  temp::Temp* mov_dst = static_cast<tree::MemExp*>(dst_)->exp_->Munch(instr_list, fs);
          temp::Temp* src_reg = src_->Munch(instr_list, fs);
	  /* 
	  tree::MemExp *memexp = static_cast<tree::MemExp *>(dst_);
          if (typeid(*(memexp->exp_)) == typeid(tree::BinopExp)) {
               tree::BinopExp *binopexp = static_cast<tree::BinopExp *>(memexp->exp_);
               if (typeid(*(binopexp->right_)) == typeid(tree::ConstExp) &&
                   typeid(*(binopexp->left_)) == typeid(tree::TempExp) &&
                   static_cast<tree::TempExp *>(binopexp->left_)->temp_ ==
                   reg_manager->FramePointer()) {
		   //TRY CHANGE HERE:
                   std::string in_frame_dst = "(" + std::string(fs) + "_framesize";
                   int offset = static_cast<tree::ConstExp *>(binopexp->right_)->consti_;
                   if (offset < 0) in_frame_dst += std::to_string(offset);
                   else in_frame_dst += ("+" + std::to_string(offset));
                   in_frame_dst += ")(%rsp)";
                   std::string ass = "movq `s0, " + in_frame_dst;
                   assem::Instr *instr = new assem::OperInstr(ass, nullptr, new temp::TempList({src_reg}), nullptr);
		   cg::PassPointer(instr);
		   instr_list.Append(instr);
		   return;
               }
               if (typeid(*(binopexp->left_)) == typeid(tree::ConstExp) &&
                   typeid(*(binopexp->right_)) == typeid(tree::TempExp) &&
                   static_cast<tree::TempExp *>(binopexp->right_)->temp_ ==
                   reg_manager->FramePointer()) {
                   std::string in_frame_dst = "(" + std::string(fs) + "_framesize";
                   int offset = static_cast<tree::ConstExp *>(binopexp->left_)->consti_;
                   if (offset < 0) in_frame_dst += std::to_string(offset);
                   else in_frame_dst += ("+" + std::to_string(offset));
                   in_frame_dst += ")(%rsp)";
                   std::string ass = "movq `s0, " + in_frame_dst;
                   assem::Instr *instr = new assem::OperInstr(ass, nullptr, new temp::TempList({src_reg}), nullptr);
                   //cg::emit(instr, instr_list);
		   cg::PassPointer(instr);
		   instr_list.Append(instr);
                   return;
               }
          }
	  */
          assem::OperInstr* mov = new assem::OperInstr("movq `s0, (`s1)", nullptr, new temp::TempList({src_reg, mov_dst}), nullptr);
	  cg::PassPointer(mov);
          instr_list.Append(mov);
      }
      else assert(0);
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //stm_->Munch(instr_list, fs);
  exp_->Munch(instr_list, fs);//only the last exp_ may produce a result.
  return ;
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //In x86_64, the addq and the subq has the format: addq t1, t2, and the res will store in the t2
  temp::Temp* left = left_->Munch(instr_list, fs);
  temp::Temp* right = right_->Munch(instr_list, fs);
  if(op_ == tree::PLUS_OP) {
      temp::Temp* tmp = temp::TempFactory::NewTemp();
      assem::MoveInstr* mov;
      //if(left != reg_manager->FramePointer()) mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({left}));
      mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({left}));
      assem::OperInstr* add_op = new assem::OperInstr("addq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({right}), nullptr);
      //if(left != reg_manager->FramePointer()) instr_list.Append(mov);
      cg::PassPointer(mov);
      instr_list.Append(mov);
      cg::PassPointer(add_op);
      instr_list.Append(add_op);
      return tmp;
  }//addq t1, t2
  if(op_ == tree::MINUS_OP) {
      temp::Temp* tmp = temp::TempFactory::NewTemp();
      assem::MoveInstr* mov;
      //if(left != reg_manager->FramePointer()) mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({left}));
      mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({left}));
      assem::OperInstr* sub_op = new assem::OperInstr("subq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({right}), nullptr);
      //if(left != reg_manager->FramePointer()) instr_list.Append(mov);
      cg::PassPointer(mov);
      instr_list.Append(mov);
      cg::PassPointer(sub_op);
      instr_list.Append(sub_op);
      return tmp;
  }//subq t1, t2
  if(op_ == tree::MUL_OP) {
      temp::Temp* tmp = temp::TempFactory::NewTemp();
      assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(0)}), new temp::TempList({left}));//get %rax here.
      assem::OperInstr* mul_op = new assem::OperInstr("imulq `s0", new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({right, reg_manager->GetRegister(0)}), nullptr);
      assem::MoveInstr* mov_ans = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({reg_manager->GetRegister(0)}));
      cg::PassPointer(mov);
      instr_list.Append(mov);
      cg::PassPointer(mul_op);
      instr_list.Append(mul_op);
      cg::PassPointer(mov_ans);
      instr_list.Append(mov_ans);
      return tmp;
  }
  if(op_ == tree::DIV_OP) {
      temp::Temp* tmp = temp::TempFactory::NewTemp();
      assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({reg_manager->GetRegister(0)}), new temp::TempList({left}));//get %rax here.
      assem::OperInstr* cqto = new assem::OperInstr("cqto", new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({reg_manager->GetRegister(0)}), nullptr);
      assem::OperInstr* div_op = new assem::OperInstr("idivq `s0", new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({right, reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), nullptr);
      assem::MoveInstr* mov_ans = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({tmp}), new temp::TempList({reg_manager->GetRegister(0)}));
      cg::PassPointer(mov);
      instr_list.Append(mov);
      cg::PassPointer(cqto);
      instr_list.Append(cqto);
      cg::PassPointer(div_op);
      instr_list.Append(div_op);
      cg::PassPointer(mov_ans);
      instr_list.Append(mov_ans);
      return tmp;
  }
  return nullptr;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* tmp = temp::TempFactory::NewTemp();
  //LAB7:GC
  
  if (typeid(*(exp_)) == typeid(tree::BinopExp)) {
    tree::BinopExp *binopexp = static_cast<tree::BinopExp *>(exp_);
    if (typeid(*(binopexp->right_)) == typeid(tree::ConstExp) &&
        typeid(*(binopexp->left_)) == typeid(tree::TempExp) &&
        static_cast<tree::TempExp *>(binopexp->left_)->temp_ == reg_manager->FramePointer()) {
      std::string in_frame_dst = "(" + std::string(fs) + "_framesize";
      int offset = static_cast<tree::ConstExp *>(binopexp->right_)->consti_;
      tmp->is_pointer = cg::CheckFramePointer(offset);
      //if (offset < 0)
      in_frame_dst += std::to_string(offset);
      //else
      //  in_frame_dst += ("+" + std::to_string(offset));
      in_frame_dst += ")(%rsp)";
      assem::Instr *mem_ins = new assem::OperInstr("movq " + in_frame_dst + ", `d0",
                               new temp::TempList({tmp}), nullptr, nullptr);
      cg::PassPointer(mem_ins);
      instr_list.Append(mem_ins);
      return tmp;
    }
    if (typeid(*(binopexp->left_)) == typeid(tree::ConstExp) &&
        typeid(*(binopexp->right_)) == typeid(tree::TempExp) &&
        static_cast<tree::TempExp *>(binopexp->right_)->temp_ == reg_manager->FramePointer()) {
      std::string in_frame_dst = "(" + std::string(fs) + "_framesize";
      int offset = static_cast<tree::ConstExp *>(binopexp->left_)->consti_;
      tmp->is_pointer = cg::CheckFramePointer(offset);
      //if(offset < 0)
      in_frame_dst += std::to_string(offset);
      //else
      //  in_frame_dst += ("+" + std::to_string(offset));
      in_frame_dst += ")(%rsp)";
      assem::Instr *mem_instr = new assem::OperInstr("movq " + in_frame_dst + ", `d0",
                               new temp::TempList({tmp}), nullptr, nullptr);
      cg::PassPointer(mem_instr);
      instr_list.Append(mem_instr);
      return tmp;
    }
  }
  
  temp::Temp* ans = exp_->Munch(instr_list, fs);//the reg ans store the position of the mem.
  //note:the MemExp should get the val stored in the pos of exp_, not the pos of exp_ shown itself.
  assem::OperInstr* mov = new assem::OperInstr("movq (`s0), `d0", new temp::TempList({tmp}), new temp::TempList({ans}), nullptr);
  cg::PassPointer(mov);
  instr_list.Append(mov);
  return tmp;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //return temp_;//
  if(temp_ == reg_manager->FramePointer()) {
      temp::Temp* ans = temp::TempFactory::NewTemp();
      std::string ss = std::string(fs);
      std::string instr = "leaq " + ss + "_framesize(`s0), `d0";
      assem::OperInstr* op = new assem::OperInstr(instr, new temp::TempList({ans}), new temp::TempList({reg_manager->StackPointer()}), nullptr);
      cg::PassPointer(op);
      instr_list.Append(op);
      return ans;
  }
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  temp::Temp* ans = exp_->Munch(instr_list, fs);
  return ans;
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //in the assem code of the x86_64, a label notify an address of the mem.
  std::string s = temp::LabelFactory::LabelString(name_);
  std::string instr = "leaq " + s + "(%rip), `d0";//the %rip is used to mark the start of the memory space.
  temp::Temp* tmp = temp::TempFactory::NewTemp();
  /*
  if(s == "init_array" || s == "alloc_record" || s == "string_equal" || s == "print" || s == "printi" 
		  || s == "flush" || s == "ord" || s == "chr" || s == "size" || s == "substring" 
		  || s == "concat" || s == "not" || s == "__wrap_getchar") {
      return tmp;
  }
  */
  assem::OperInstr* label_op = new assem::OperInstr(instr, new temp::TempList({tmp}), nullptr, nullptr);
  cg::PassPointer(label_op);
  instr_list.Append(label_op);
  return tmp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp* tmp = temp::TempFactory::NewTemp();
  std::string str = "movq $" + std::to_string(consti_) + ", " + "`d0";
  assem::OperInstr* const_op = new assem::OperInstr(str, new temp::TempList({tmp}), nullptr, nullptr);
  cg::PassPointer(const_op);
  instr_list.Append(const_op);
  return tmp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  //the sol similar to the PPT here.
  //because we don't know about the frameSize, so we can't allocate a detailed size to this new frame.
  temp::Temp* ans = temp::TempFactory::NewTemp();
  //temp::Temp* t1 = fun_->Munch(instr_list, fs);
  //temp::TempList* list = args_->MunchArgs(instr_list, fs);
  //list->Prepend(t1);//get the list of the func_info and the args_info
  temp::TempList* list = new temp::TempList();
  //list->Append(t1);
  temp::TempList* args_t = args_->MunchArgs(instr_list, fs);
  for(auto it : args_t->GetList()) list->Append(it);
  temp::TempList* calldefs = reg_manager->CallerSaves();
  temp::Label* l = static_cast<tree::NameExp*>(fun_)->name_;

  std::string callq = "callq " + temp::LabelFactory::LabelString(l);
  assem::OperInstr* call_instr = new assem::OperInstr(callq, calldefs, list, nullptr);
  cg::PassPointer(call_instr);
  instr_list.Append(call_instr);

  //LAB7:GC,add ret label after every callq instruction.
  temp::Label* ret_label = temp::LabelFactory::NewLabel();
  std::string got = temp::LabelFactory::LabelString(ret_label);
  assem::Instr* new_ret = new assem::LabelInstr(got, ret_label);
  cg::PassPointer(new_ret);
  instr_list.Append(new_ret);

  //LAB7:GC
  reg_manager->ReturnValue()->is_pointer = cg::CheckReturnPointer(l);

  assem::MoveInstr* mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({ans}), new temp::TempList({reg_manager->GetRegister(0)}));
  cg::PassPointer(mov);
  instr_list.Append(mov);
  /*
  std::string s = l->Name();
  
  if(s == "init_array" || s == "alloc_record" || s == "string_equal" || s == "print" || s == "printi" 
		  || s == "flush" || s == "ord" || s == "chr" || s == "size" || s == "substring" 
		  || s == "concat" || s == "not" || s == "__wrap_getchar" || s == "Alloc" || s == "GC" || s == "Used" || s == "MaxFree") {
      return ans;
  }
  
  temp::Label* ret_label = temp::LabelFactory::NewLabel();
  (roots->ret_map)[s].push_back(ret_label);//store the retLabel in the roots's map.

  tree::Stm* stm = new tree::LabelStm(ret_label);
  stm->Munch(instr_list, fs);//add a ret_label after each callq instr.
  //mov the %rax to the ans register to store the function ret-val.
  */
  return ans; 
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList* ans = new temp::TempList();
  temp::Temp* link = temp::TempFactory::NewTemp();
  /*
  tree::Exp* staticLink = exp_list_.front();
  if(staticLink == nullptr) {
      exp_list_.pop_front();
  }
  else {
      temp::Temp* link_t = staticLink->Munch(instr_list, fs);
      assem::MoveInstr* link_mov = new assem::MoveInstr("movq `s0, `d0", new temp::TempList({link}), new temp::TempList({link_t}));
      instr_list.Append(link_mov);//store the static-link.
      exp_list_.pop_front();//remove the static-link from the argList.
  }
  */
  std::list<tree::Exp*>::iterator it = exp_list_.begin();
  int num = 1;
  int len = exp_list_.size();
  while(it != exp_list_.end()) {
      temp::Temp* t = (*it)->Munch(instr_list, fs);
      ans->Append(t);
      it++;
  }
  for(auto t : (ans->GetList())) {
      if(num > 6) {
          //std::string instr = "movq `s0, " + std::to_string((5 - num) * 8) + "(`s1)";
	  std::string instr = "";
	  if(num != 7) instr = "movq `s0, " + std::to_string(8 * (num - 7)) + "(`s1)";
	  else instr = "movq `s0, (`s1)";
	  //std::string instr = "movq `s0, " + std::to_string((num - 2 - len) * 8) + "(`s1)";
	  assem::OperInstr* mov = new assem::OperInstr(instr, nullptr, new temp::TempList({t, reg_manager->StackPointer()}), nullptr);
	  cg::PassPointer(mov);
	  instr_list.Append(mov);
      }
      else {
          std::string instr = "movq `s0, `d0";

	  //LAB7:GC
	  reg_manager->GetRegister(num)->is_pointer = cg::CheckArgPointer(num - 1);//

	  assem::MoveInstr* mov = new assem::MoveInstr(instr, new temp::TempList({reg_manager->GetRegister(num)}), new temp::TempList({t}));
	  cg::PassPointer(mov);
	  instr_list.Append(mov);
      }
      num++;
  }
  return ans;
}

} // namespace tree
