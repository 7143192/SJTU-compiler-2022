#include "tiger/frame/x64frame.h"
#include "tiger/runtime/gc/roots/roots.h"
extern frame::RegManager *reg_manager;

extern gc::Roots* roots;

namespace frame {
/* TODO: Put your lab5 code here */
class InFrameAccess : public Access {
public:
  int offset;
  bool is_pointer;

  explicit InFrameAccess(int offset) : offset(offset), is_pointer(false) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr) const override {
      //return nullptr;
      tree::Exp* ans = new tree::MemExp(new tree::BinopExp(//use the binary op to get the pos of the access in the frame.
			      tree::PLUS_OP, framePtr, new tree::ConstExp(offset)));
      //use memExp to get the access in the address.
      return ans;
  }

  void setStorePointer() {
      is_pointer = true;
  }
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp* ToExp(tree::Exp* framePtr) const override {
      return new tree::TempExp(reg);
  }

  void setStorePointer() {
      reg->is_pointer = true;
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  Access* AllocLocal(bool escape) override {
      if(escape == true) {
          Access* access = new InFrameAccess(offset - 8);
	  offset -= 8;
	  formals.push_back(access);
	  //frame_ptr.push_back(access);
	  return access;
      }//escaped, in the stack frame
      temp::Temp* reg = temp::TempFactory::NewTemp();
      Access* access = new InRegAccess(reg);//local var in the reg.
      return access;
  }
  X64Frame(temp::Label* name, std::list<bool> escape) :Frame(name, escape) {
      //label = name;
      offset = 0;
      std::list<Access*> list;
      formals = list;
      if(escape.size() == 0) return ;
      std::list<bool>::iterator it;
      for(it = escape.begin(); it != escape.end(); ++it) {
          Access* access = AllocLocal((*it));
	  //LAB7:GC
	  //formals.push_back(access);
	  //only push the InRegAccess here, beacause the InFrameAccess has been pushed in the AllocLocal function.
	  if(typeid(*access) == typeid(frame::InRegAccess)) {
	      formals.push_back(access);
	  }
      }//allocate the local variables
      //then try to produce the view-shift instructions.
      //note that the view-shift should look at all the formals in the view of the CALLEE.
      //and view-shift should be composed of many MOVE exps.
      view_shift = nullptr;
      /*
      if(formals.size() == 0) return ;
      std::list<Access*>::iterator it1 = formals.begin();
      int i = 0;
      while(it1 != formals.end()) {
          tree::Exp* exp;
	  exp = (*it1)->ToExp(new tree::TempExp(reg_manager->FramePointer()));
	  tree::Stm* tmp = nullptr;
	  if(i >= 6) {
	      //should in the frame.
	      temp::Temp* fp = reg_manager->FramePointer();
	      //tree::Exp* op = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, new tree::TempExp(fp), new tree::ConstExp(((i - 5) * 8))));
	      tree::Exp* op = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(fp), new tree::ConstExp((i - 5) * 8)));
	      tmp = new tree::MoveStm(exp, op);
	  }
	  else {
	      //should in the ArgReg(s).
	      tmp = new tree::MoveStm(exp, new tree::TempExp(reg_manager->GetRegister(i + 1)));//put the params stored in the regs into the local reg of this frame.
	  }
	  if(view_shift == nullptr) view_shift = tmp;
	  else view_shift = new tree::SeqStm(view_shift, tmp);
	  it1++;
	  i++;
      }
      */
  }
};
/* TODO: Put your lab5 code here */

static assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
    body->Append(new assem::OperInstr("", new temp::TempList(), reg_manager->ReturnSink(), nullptr));
    return body;
}
static tree::Stm* ProcEntryExit1(Frame* f, tree::Stm* stm) {
    //used to handle the view shift
    /*
    std::list<frame::Access*> formal_list = f->formals;
    int i = 0;
    int len = formal_list.size();
    tree::Stm* view_shift = new tree::ExpStm(new tree::ConstExp(0));
    for(auto it : formal_list) {
        if(i >= 6) {
            f->offset -= 8;
	    //tree::Exp* mem = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, new tree::TempExp(reg_manager->FramePointer()), new tree::ConstExp((i - 5) * 8)));
            //tree::Exp* mem = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(reg_manager->FramePointer()), new tree::ConstExp((i - 5) * 8)));
	    tree::Exp* mem = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, new tree::TempExp(reg_manager->FramePointer()), new tree::ConstExp((len - i) * 8)));
	    tree::Stm* mov = new tree::MoveStm(it->ToExp(new tree::TempExp(reg_manager->FramePointer())), mem);
	    if(view_shift == nullptr) view_shift = mov;
	    else view_shift = new tree::SeqStm(mov, view_shift);	    
	}
	else {
	    tree::Stm* mov = new tree::MoveStm(it->ToExp(new tree::TempExp(reg_manager->FramePointer())), new tree::TempExp(reg_manager->GetRegister(i + 1)));
	    if(view_shift == nullptr) view_shift = mov;
	    else view_shift = new tree::SeqStm(view_shift, mov);
	}
	i++;
    }
    if(view_shift == nullptr) return stm;
    return new tree::SeqStm(view_shift, stm);
    */
    
    std::list<frame::Access*> formal_list = f->formals;
    if(formal_list.size() != 0) {
        std::list<frame::Access*>::iterator it1 = formal_list.begin();
        int i = 0;
        while(it1 != formal_list.end()) {
          tree::Exp* exp;
	  exp = (*it1)->ToExp(new tree::TempExp(reg_manager->FramePointer()));
	  tree::Stm* tmp = nullptr;
	  if(i >= 6) {
	      //should in the frame.
	      temp::Temp* fp = reg_manager->FramePointer();
	      //tree::Exp* op = new tree::MemExp(new tree::BinopExp(tree::MINUS_OP, new tree::TempExp(fp), new tree::ConstExp(((i - 5) * 8))));
	      tree::Exp* op = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(fp), new tree::ConstExp((i - 5) * 8)));
	      tmp = new tree::MoveStm(exp, op);
	      //LAB7:
	      
	      if(typeid(*it1) == typeid(frame::InRegAccess)) {
		  //frame::InRegAccess* reg_it = static_cast<frame::InRegAccess*>(*it1);
		  tree::TempExp* t = static_cast<tree::TempExp*>(exp);
		  if(f->pointers[i] == 1) {
		      (roots->temp_map)[t->temp_] = 1;
		  }
		  if(f->pointers[i] == 2) {
		      (roots->temp_map)[t->temp_] = 2;
		  }
	      }
	      if(typeid(*it1) == typeid(frame::InFrameAccess)) {
		  frame::InFrameAccess* frame_it = static_cast<frame::InFrameAccess*>(*it1);
		  std::string name_label = f->GetLabel();
		  if(f->pointers[i] == 1 || f->pointers[i] == 2) {
		      (roots->func_pointers)[name_label].push_back(0 - frame_it->offset);//get the stack slot offset that contains a pointer.
		      (roots->pointers_type)[name_label].push_back(f->pointers[i]);//record the corresponding type.
		  }
	      }
	      
	  }
	  else {
	      //should in the ArgReg(s).
	      tmp = new tree::MoveStm(exp, new tree::TempExp(reg_manager->GetRegister(i + 1)));//put the params stored in the regs into the local reg of this frame.
	      //LAB7:
	      
	      if(typeid(*it1) == typeid(frame::InRegAccess)) {
		  //frame::InRegAccess* reg_it = static_cast<frame::InRegAccess*>(*it1);
		  tree::TempExp* t = static_cast<tree::TempExp*>(exp);
		  if(f->pointers[i] == 1) {
		      (roots->temp_map)[t->temp_] = 1;
		  }
		  if(f->pointers[i] == 2) {
		      (roots->temp_map)[t->temp_] = 2;
		  }
	      }
	      if(typeid(*it1) == typeid(frame::InFrameAccess)) {
		  frame::InFrameAccess* frame_it = static_cast<frame::InFrameAccess*>(*it1);
		  std::string name_label = f->GetLabel();
		  if(f->pointers[i] == 1 || f->pointers[i] == 2) {
		      (roots->func_pointers)[name_label].push_back(0 - frame_it->offset);//get the stack slot offset that contains a pointer.
		      (roots->pointers_type)[name_label].push_back(f->pointers[i]);//record the corresponding pointer type.
		  }
	      }
	      
	  }
	  if(f->view_shift == nullptr) f->view_shift = tmp;
	  else f->view_shift = new tree::SeqStm(f->view_shift, tmp);
	  it1++;
	  i++;
        }
    }

    tree::Stm* ans = new tree::SeqStm(f->view_shift, stm);
    return ans;
}
} // namespace frame
