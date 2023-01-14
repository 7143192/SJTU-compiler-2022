#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <memory>
#include <string>
#include <vector>
#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"
//#include "tiger/runtime/gc/roots/roots.h"

//extern gc::Roots* roots;

namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;

  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access {
public:
  /* TODO: Put your lab5 code here */
  bool is_pointer;
  virtual ~Access() = default;
  virtual tree::Exp* ToExp(tree::Exp *framePtr) const = 0;
  virtual void setStorePointer() = 0; 
};

class Frame {
  /* TODO: Put your lab5 code here */
public:
  std::list<frame::Access*> formals;//used to record the formals
  tree::Stm* view_shift;//used to record the instrs used to do the view shift
  temp::Label* name_;//the label used to mark the begin of the frame(a function)
  int offset;//used to get the val in the pos of offset of the FP.
  int arg_num;
  std::list<frame::Access*> frame_ptr;//used to record the ptr in the frame.

  std::vector<int> pointers;//used to record the which param contains a pointer. 

  virtual frame::Access* AllocLocal(bool escape) {
      return nullptr;
  }
  Frame(temp::Label* name, int off, tree::Stm* instrs) {
      name_ = name;
      view_shift = instrs;
      offset = off;
      arg_num = 0;
  }
  Frame(temp::Label* name, std::list<bool> escapes) {
      name_ = name;
      arg_num = 0;
  }
  static tree::Exp* ExternalCall(std::string s, tree::ExpList* args) {
      return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)), args);
  }

  std::string GetLabel() {
      return temp::LabelFactory::LabelString(name_);//return the label of this frame. 
  }
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;//current function's body(the val return from ProcExit1 function)
  Frame *frame_;//current function's frame

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }
  void SetList(std::list<Frag*> list) {frags_ = list;};
private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */
static assem::Proc* ProcEntryExit3(frame::Frame* frame_, assem::InstrList* body) {
    //mainly used to produce the prologue and the endlogue of the assembly code of a function.
    std::string prologue;
    std::string epilogue;
    std::string buf = "";
    int len = (frame_->formals).size();
    //compute the stack size the arg required.
    int arg_size = (frame_->arg_num > 6) ? (frame_->arg_num - 6) * 8 : 0;
    int sub_size = arg_size - (frame_->offset);
    //prologue part
    buf += ".set ";
    buf += frame_->name_->Name();
    buf += "_framesize, ";
    buf += std::to_string(0 - (frame_->offset));//
    buf += "\n";
    prologue = buf;
    buf = "";
    buf = frame_->name_->Name() + ":\n";
    buf += "subq $";
    buf += std::to_string(sub_size);
    //buf += std::to_string(-(frame_->offset));
    buf += ", %rsp\n";
    prologue += buf;
    //epilogue part
    buf = "addq $";
    buf += std::to_string(sub_size);
    //buf += std::to_string(-(frame_->offset));
    buf += ", %rsp\n";
    epilogue = buf;

    //LAB7:
    /*
    if(frame_->GetLabel() == "tigermain") {
	temp::Label* l = temp::LabelFactory::NewLabel();
        buf = l->Name() + ":\n";
	epilogue.append(buf);
	//(roots->ret_map)["tigermain"].push_back(l);
    }
    */


    buf = "retq\n";
    epilogue.append(buf);
    buf = "";
    assem::Proc* ans = new assem::Proc(prologue, body, epilogue);
    return ans;
}
} // namespace frame

#endif
