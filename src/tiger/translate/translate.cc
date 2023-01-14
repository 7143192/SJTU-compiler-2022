#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include "tiger/runtime/gc/roots/roots.h"
#include <vector>

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

extern gc::Roots* roots;//external roots declared in the main.cc
extern std::vector<std::string> ret_pointers;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access* access = (level->frame_)->AllocLocal(escape);//allocate the new local through the frame namespace.
  Access* ans = new Access(level, access);
  return ans;
}

class Cx {
public:
  //NOTE:the patchList contains a list of two-level pointers of the target label.
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {//it's a kind of exp that will produce a value.
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    temp::Label* true_ = temp::LabelFactory::NewLabel();
    temp::Label* false_ = temp::LabelFactory::NewLabel();
    tree::Exp* zero = new tree::ConstExp(0);
    tree::CjumpStm* stm = new tree::CjumpStm(tree::NE_OP, exp_, zero, true_, false_);//convert the exp to the CJump first using exp != 0
    //std::list<temp::Label**> true_list;
    //std::list<temp::Label**> false_list;
    //true_list.push_back(&(stm->true_label_));
    //false_list.push_back(&(stm->false_label_));//note the patchList is a Two-Level Pointer List.
    PatchList patch_true({&(stm->true_label_)});
    PatchList patch_false({&(stm->false_label_)});//init the true and false patchList.
    Cx ans(patch_true, patch_false, stm);
    return ans;
  }
};

class NxExp : public Exp {//a kind of the exp that won't produce the new value, a.k.a., statement
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));//assign a default 0 to the statement directly.
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    assert(false);//the statement CAN NOT convert to the CX!
  }
};

class CxExp : public Exp {//a kind of exp that are similar to the CJump Exp.
public:
  Cx cx_;
  //NOTE:the CX exp is similar to a CJUMP instr, so it can jump to the trues/falses list after it's executed directly.
  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp* r = temp::TempFactory::NewTemp();//create a new temp to store the converted exp's result.
    temp::Label* true_ = temp::LabelFactory::NewLabel();
    temp::Label* false_ = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(true_);
    cx_.falses_.DoPatch(false_);
    /*the convert logic is as follows:
     *r <- 1(a move)
     *true:(a labelExp)
     *   r(<- 1)
     *false:(a labelExp)
     *   r <- 0(a move)
     */
    tree::Stm* stm1 = new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1));
    tree::Exp* exp1 = new tree::EseqExp(
		    cx_.stm_, 
		    new tree::EseqExp(
			    new tree::LabelStm(false_),
			    new tree::EseqExp(
				    new tree::MoveStm(new tree::TempExp(r), 
					    new tree::ConstExp(0)),
				    new tree::EseqExp(new tree::LabelStm(true_),
					    new tree::TempExp(r)))));
    tree::Exp* ans = new tree::EseqExp(stm1, exp1);
    return ans;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    tree::Exp* exp = UnEx();//convert the CX Exp to the ExExp first.
    tree::Stm* ans = new tree::ExpStm(exp);
    return ans;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

Level* Level::NewLevel(Level* parent, temp::Label* name, std::list<bool> formals) {
    formals.push_front(true);//add the staticLink in the tr::Level.
    frame::Frame* f = new frame::X64Frame(name, formals);
    Level* ans = new Level(f, parent);
    return ans; 
}

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  temp::Label* label = temp::LabelFactory::NewLabel();//the label used to notify the start of the tigermain func?
  //LAB7:
  //printf("ProgTr label = %s\n", (label->Name()).c_str());
  //(roots->ret_map)["tigermain"].push_back(label);//

  //std::list<bool> formal;
  temp::Label* first = temp::LabelFactory::NamedLabel("tigermain");//use the "tigermain" as the first-level's name to notify the start of the translation. 
  frame::Frame* f = new frame::X64Frame(first, {});//the first level is abstarct, so it does not have formals.
  main_level_ = std::make_unique<Level>(f, nullptr);//store the first Level with the main_level_ attribute.
  FillBaseVEnv();
  FillBaseTEnv();
  tr::ExpAndTy* ans = absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), label, errormsg_.get());
  //frags->PushBack(new frame::ProcFrag(ans->exp_->UnNx(), main_level_->frame_));//put the tigermain (init) function frag into the global Frags list. 
  frame::ProcFrag* proc_frag = new frame::ProcFrag(ans->exp_->UnNx(), main_level_->frame_);
  std::list<frame::Frag*>list = frags->GetList();
  list.push_front(proc_frag);
  frags->SetList(list);
}

tree::Exp* StaticLink(Level* cur, Level* target) {
    tree::Exp* fp = new tree::TempExp(reg_manager->FramePointer());//get the cur abstract %rbp in the exp form.
    if(target == nullptr) return nullptr;//the outmost level - tigermain.
    /*
    while(cur != target) {
        //frame::Access* t = (cur->frame_->formals).front();//get the first param staticLink
	//add 8 to the pos of the FP, to the pos of the current level's sl starting pos, thenuse MemOp to get the detailed static link info.
        fp = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, fp, new tree::ConstExp(8)));
	//fp = t->ToExp(fp);
        cur = cur->parent_;
    } 
    return fp;
    */
    while(cur != target) {
        auto access = cur->frame_->formals.front();
	fp = access->ToExp(fp);
	cur = cur->parent_;
    }
    return fp;
    /*
    if(target == nullptr) return nullptr;
    while(cur != target) {
	tree::BinopExp* op = new tree::BinopExp(tree::PLUS_OP, fp, new tree::ConstExp(0 - (cur->frame_->offset)));
        fp = new tree::MemExp(op);
	cur = cur->parent_;
    }
    tree::BinopExp* op = new tree::BinopExp(tree::PLUS_OP, fp, new tree::ConstExp(cur->frame_->offset));
    fp = new tree::MemExp(op);
    return fp;
    */
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::VarEntry* t = static_cast<env::VarEntry*>(venv->Look(sym_));//get the entry from env first.
  tree::Exp* init = StaticLink(level, t->access_->level_);//get the first-time definition of this var through the staticLink.
  tree::Exp* defineExp = t->access_->access_->ToExp(init);
  tr::ExpAndTy* ans = new tr::ExpAndTy(new tr::ExExp(defineExp), t->ty_->ActualTy());
  return ans;
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* got = var_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(*(got->ty_)) == typeid(type::RecordTy)) {//check the type first
      std::list<type::Field*> list = ((static_cast<type::RecordTy*>(got->ty_))->fields_)->GetList();
      std::list<type::Field*>::iterator it = list.begin();
      int i = 0;
      while(it != list.end()) {
          if(sym_ == (*it)->name_) {
	      //the got->exp_ is the start pos of the record, since the record is allocated in the heap in the Tiger, go up to get the i-th field.
	      //LAB7:CHANGED HERE!
	      tr::Exp* exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, got->exp_->UnEx(), new tree::ConstExp((i) * 8))));//get the i-th field in the definition.
	      return new tr::ExpAndTy(exp, (*it)->ty_->ActualTy());
	  }
	  i++;
	  it++;
      }
  }
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance()); 
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* got = var_->Translate(venv, tenv, level, label, errormsg);
  if(typeid(*(got->ty_)) == typeid(type::ArrayTy)) {
      tr::ExpAndTy* sub = subscript_->Translate(venv, tenv, level, label, errormsg);
      if(typeid(*(sub->ty_)) != typeid(type::IntTy)) return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
      //NOTE:according to the runtime.c, the array has a long type of its elements.
      tree::Exp* binop = new tree::BinopExp(tree::PLUS_OP, sub->exp_->UnEx(), new tree::ConstExp(2));
      tree::Exp* bin = new tree::BinopExp(tree::MUL_OP, binop, new tree::ConstExp(reg_manager->WordSize()));
      tr::Exp* exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, got->exp_->UnEx(), bin)));

      //tr::Exp* exp = new tr::ExExp(new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, got->exp_->UnEx(), 
      //			      new tree::BinopExp(tree::MUL_OP, sub->exp_->UnEx(), new tree::ConstExp(8)))));//get the Exp in the pos a + 8 * sub_exp, a is the start pos of the array.
      return new tr::ExpAndTy(exp, static_cast<type::ArrayTy*>(got->ty_->ActualTy())->ty_->ActualTy());
  }
  return new tr::ExpAndTy(nullptr, type::IntTy::Instance());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp = new tr::ExExp(new tree::ConstExp(0));//use 0 to notify the NIL Exp.
  return new tr::ExpAndTy(exp, type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp* exp = new tr::ExExp(new tree::ConstExp(val_));
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //String in Tiger should be regarded as a CONST!
  temp::Label* label1 = temp::LabelFactory::NewLabel();//the new label used to mark this string const.
  tr::Exp* exp = new tr::ExExp(new tree::NameExp(label1));
  frame::Frag* frag = new frame::StringFrag(label1, str_);
  frags->PushBack(frag);//push the string const into the global frags.
  return new tr::ExpAndTy(exp, type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::FunEntry* got = static_cast<env::FunEntry*>(venv->Look(func_));//get the function info from the env.
  type::Ty* res = type::VoidTy::Instance();//used to store the return type of the function.
  if(got->result_ != nullptr) res = got->result_;
  std::list<type::Ty*> init_list = got->formals_->GetList();//the formals declared when init.
  std::list<Exp*> got_list = args_->GetList(); //the formals got when call this function.
  if(init_list.size() != got_list.size()) return new tr::ExpAndTy(nullptr, res);//the called exp and the original-declared func should have the same num of the args
  tree::ExpList* list = new tree::ExpList();
  //the callExp's static-link should point to the pos of its direct-parent's FP.
  tree::Exp* staticLink = tr::StaticLink(level, got->level_->parent_);
  if(staticLink != nullptr) {
      list->Append(staticLink);//add the staticLink into the expList to act as the first param.
      //got->level_->frame_->pointers.push_back(0);//SL is not regared as a pointer.
  }
  if(staticLink == nullptr) {
      for(auto it : got_list) {
          tr::ExpAndTy* got1 = it->Translate(venv, tenv, level, label, errormsg);
	  list->Append(got1->exp_->UnEx());
	  /*
	  type::Ty* got_ty = got1->ty_->ActualTy();
	  if(typeid(*got_ty) == typeid(type::RecordTy) || typeid(*got_ty) == typeid(type::ArrayTy)) {
	      got->level_->frame_->pointer.push_back(1);//push a 1 as this formal is a pointer.
	  }
	  else got->level_->frame_->pointer.push_back(0);
	  */
      }
      int size = (list->GetList()).size();
      if(size > level->frame_->arg_num) level->frame_->arg_num = size;
      tr::Exp* ans_exp = new tr::ExExp(frame::Frame::ExternalCall(temp::LabelFactory::LabelString(func_), list));

      //add a return_label after a callExp.
      temp::Label* ret_label = temp::LabelFactory::NewLabel();
      //tree::Exp* ret_label_exp = new tree::NameExp(ret_label);
      //tree::Exp* call_func_exp = frame::Frame::ExternalCall(temp::LabelFactory::LabelString(func_), list);

      return new tr::ExpAndTy(ans_exp, res);
  }
  //temp::Label* func_label = temp::LabelFactory::NamedLabel(func_->Name());//create a new specific label for the function.
  if(init_list.size() == 0) {
      level->frame_->arg_num = 1;
      tr::Exp* exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), list));
      return new tr::ExpAndTy(exp, res);
  } 
  std::list<type::Ty*>::iterator init_it = init_list.begin();
  std::list<Exp*>::iterator got_it = got_list.begin();
  while(init_it != init_list.end()) {//check the formals list.
      tr::ExpAndTy* got1 = (*got_it)->Translate(venv, tenv, level, label, errormsg);
      if((got1->ty_)->IsSameType(*init_it)) {
          tree::Exp* exp1 = got1->exp_->UnEx();
	  list->Append(exp1);
	  /*
	  type::Ty* got_ty = got1->ty_->ActualTy();
	  if(typeid(*got_ty) == typeid(type::RecordTy) || typeid(*got_ty) == typeid(type::ArrayTy)) {
	      got->level_->frame_->pointer.push_back(1);//push a 1 as this formal is a pointer.
	  }
	  else got->level_->frame_->pointer.push_back(0);
	  */
      }
      else return new tr::ExpAndTy(nullptr, res);//have the unmatched type!
      init_it++;
      got_it++;
  }
  int size = (list->GetList()).size();
  if(size > level->frame_->arg_num) level->frame_->arg_num = size;
  tr::Exp* ans_exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), list));
  return new tr::ExpAndTy(ans_exp, res);
}

tree::RelOp getOp(OpExp* exp) {
    if(exp->oper_ == EQ_OP) return tree::EQ_OP;
    if(exp->oper_ == LT_OP) return tree::LT_OP;
    if(exp->oper_ == LE_OP) return tree::LE_OP;
    if(exp->oper_ == GT_OP) return tree::GT_OP;
    if(exp->oper_ == GE_OP) return tree::GE_OP;
    if(exp->oper_ == NEQ_OP) return tree::NE_OP;

}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //NOTE the & and | op should allow the short cut.
  tr::ExpAndTy* l = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* r = right_->Translate(venv, tenv, level, label, errormsg);
  //temp::Label* t = temp::LabelFactory::NewLabel();
  //temp::Label* f = temp::LabelFactory::NewLabel();
  //as forthe common BinOp,(+, -, *, /) just combine left and right directly.
  if(oper_ == Oper::PLUS_OP) {
      tr::Exp* exp = new tr::ExExp(new tree::BinopExp(tree::PLUS_OP, l->exp_->UnEx(), r->exp_->UnEx()));
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::MINUS_OP) {
      tr::Exp* exp = new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, l->exp_->UnEx(), r->exp_->UnEx()));
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::TIMES_OP) {
      tr::Exp* exp = new tr::ExExp(new tree::BinopExp(tree::MUL_OP, l->exp_->UnEx(), r->exp_->UnEx()));
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::DIVIDE_OP) {
      tr::Exp* exp = new tr::ExExp(new tree::BinopExp(tree::DIV_OP, l->exp_->UnEx(), r->exp_->UnEx()));
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  //as for the single Relation OP, convert it to a CJump directly.
  //and remeber the true_label_ and the false_label_ should be NULL at this point.
  if(oper_ == Oper::EQ_OP) {
      //According to the runtime.c, the EQ_OP should also consider the situation that checking two strings' equivalence
      if(typeid(*(l->ty_)) == typeid(type::StringTy) && typeid(*(r->ty_)) == typeid(type::StringTy)) {
          tree::ExpList* args =new tree::ExpList();
	  args->Append(l->exp_->UnEx());
	  args->Append(r->exp_->UnEx());
	  tree::Exp* exp = frame::Frame::ExternalCall("string_equal", args);
	  return new tr::ExpAndTy(new tr::ExExp(exp), type::IntTy::Instance());
      }
      tree::CjumpStm* stm = new tree::CjumpStm(tree::EQ_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::NEQ_OP) {
      tree::CjumpStm* stm = new tree::CjumpStm(tree::NE_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm); 
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::LT_OP) {
      tree::CjumpStm* stm = new tree::CjumpStm(tree::LT_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::LE_OP) {
      tree::CjumpStm* stm = new tree::CjumpStm(tree::LE_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::GT_OP) {
      tree::CjumpStm* stm = new tree::CjumpStm(tree::GT_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::GE_OP) {
      tree::CjumpStm* stm = new tree::CjumpStm(tree::GE_OP, l->exp_->UnEx(), r->exp_->UnEx(), nullptr, nullptr);
      std::list<temp::Label**> true_;
      std::list<temp::Label**> false_;
      true_.push_back(&(stm->true_label_)); 
      false_.push_back(&(stm->false_label_));
      tr::PatchList true_list(true_);
      tr::PatchList false_list(false_);
      tr::Exp* exp = new tr::CxExp(true_list, false_list, stm);
      return new tr::ExpAndTy(exp, type::IntTy::Instance());
  }
  if(oper_ == Oper::AND_OP) {
      //OpExp1 & OpExp2 
      /*
      temp::Label* z = temp::LabelFactory::NewLabel();
      if(typeid(*(left_)) != typeid(OpExp) || typeid(*(right_)) != typeid(OpExp)) {
          return new tr::ExpAndTy(nullptr, type::IntTy::Instance());//the &/| 's left and right must be the OpExp.
      }
      OpExp* l_op = static_cast<OpExp*>(left_);
      OpExp* r_op = static_cast<OpExp*>(right_);
      tr::ExpAndTy* ll = l_op->left_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* lr = l_op->right_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* rl = r_op->left_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* rr = r_op->right_->Translate(venv, tenv, level, label, errormsg);
      temp::Label* t = temp::LabelFactory::NewLabel();
      temp::Label* f = temp::LabelFactory::NewLabel();
      tree::CjumpStm* s1 = new tree::CjumpStm(getOp(l_op), ll->exp_->UnEx(), lr->exp_->UnEx(), z, f);
      tree::CjumpStm* s2 = new tree::CjumpStm(getOp(r_op), rl->exp_->UnEx(), rr->exp_->UnEx(), t, f);//should use the same f label
      tree::Stm* stm1 = new tree::SeqStm(s1, new tree::SeqStm(new tree::LabelStm(z), s2));//connect the left and right part
      tr::PatchList trues = tr::PatchList({&(s2->true_label_)});
      tr::PatchList falses = tr::PatchList({&(s1->false_label_), &(s2->false_label_)});
      tr::Exp* exp1 = new tr::CxExp(trues, falses, stm1);
      return new tr::ExpAndTy(exp1, type::IntTy::Instance());
      */
      tr::Cx cx_l = l->exp_->UnCx(errormsg);
      tr::Cx cx_r = r->exp_->UnCx(errormsg);
      temp::Label* z = temp::LabelFactory::NewLabel();
      temp::Label* t = temp::LabelFactory::NewLabel();
      temp::Label* f = temp::LabelFactory::NewLabel();
      tree::Stm* stm1 = new tree::SeqStm(cx_l.stm_, new tree::SeqStm(new tree::LabelStm(z), cx_r.stm_));
      tr::PatchList trues = tr::PatchList();
      tr::PatchList falses = tr::PatchList();
      cx_l.falses_.DoPatch(f);
      //static_cast<tree::CjumpStm*>(cx_l.stm_)->false_label_ = f;
      cx_l.trues_.DoPatch(z);
      //static_cast<tree::CjumpStm*>(cx_l.stm_)->true_label_ = z;
      cx_r.trues_.DoPatch(t);
      //static_cast<tree::CjumpStm*>(cx_r.stm_)->true_label_ = t;
      cx_r.falses_.DoPatch(f);
      //static_cast<tree::CjumpStm*>(cx_r.stm_)->false_label_ = f;
      trues = cx_r.trues_;
      falses = tr::PatchList::JoinPatch(cx_l.falses_, cx_r.falses_);
      tr::Exp* ans = new tr::CxExp(trues, falses, stm1);
      return new tr::ExpAndTy(ans, type::IntTy::Instance());
  }
  if(oper_ == Oper::OR_OP) {
      //OpExp1 | OpExp2
      /*
      temp::Label* z = temp::LabelFactory::NewLabel();
      if(typeid(*(left_)) != typeid(OpExp) || typeid(*(right_)) != typeid(OpExp)) {
          return new tr::ExpAndTy(nullptr, type::IntTy::Instance());//the &/| 's left and right must be the OpExp.
      }
      OpExp* l_op = static_cast<OpExp*>(left_);
      OpExp* r_op = static_cast<OpExp*>(right_);
      tr::ExpAndTy* ll = l_op->left_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* lr = l_op->right_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* rl = r_op->left_->Translate(venv, tenv, level, label, errormsg);
      tr::ExpAndTy* rr = r_op->right_->Translate(venv, tenv, level, label, errormsg);
      temp::Label* t = temp::LabelFactory::NewLabel();
      temp::Label* f = temp::LabelFactory::NewLabel();
      tree::CjumpStm* s1 = new tree::CjumpStm(getOp(l_op), ll->exp_->UnEx(), lr->exp_->UnEx(), t, z);
      tree::CjumpStm* s2 = new tree::CjumpStm(getOp(r_op), rl->exp_->UnEx(), rr->exp_->UnEx(), t, f);//should use the same t label
      tree::Stm* stm1 = new tree::SeqStm(s1, new tree::SeqStm(new tree::LabelStm(z), s2));//connect the left and right part
      tr::PatchList trues = tr::PatchList({&(s1->true_label_), &(s2->true_label_)});
      tr::PatchList falses = tr::PatchList({&(s2->false_label_)});
      tr::Exp* exp1 = new tr::CxExp(trues, falses, stm1);
      return new tr::ExpAndTy(exp1, type::IntTy::Instance());
      */
      tr::Cx cx_l = l->exp_->UnCx(errormsg);
      tr::Cx cx_r = r->exp_->UnCx(errormsg);
      temp::Label* z = temp::LabelFactory::NewLabel();
      temp::Label* t = temp::LabelFactory::NewLabel();
      temp::Label* f = temp::LabelFactory::NewLabel();
      tree::Stm* stm1 = new tree::SeqStm(cx_l.stm_, new tree::SeqStm(new tree::LabelStm(z), cx_r.stm_));
      tr::PatchList trues = tr::PatchList();
      tr::PatchList falses = tr::PatchList();
      cx_l.falses_.DoPatch(z);
      //static_cast<tree::CjumpStm*>(cx_l.stm_)->false_label_ = z;
      cx_l.trues_.DoPatch(t);
      //static_cast<tree::CjumpStm*>(cx_l.stm_)->true_label_ = t;
      cx_r.trues_.DoPatch(t);
      //static_cast<tree::CjumpStm*>(cx_r.stm_)->true_label_ = t;
      cx_r.falses_.DoPatch(f);
      //static_cast<tree::CjumpStm*>(cx_r.stm_)->false_label_ = f;
      trues = tr::PatchList::JoinPatch(cx_l.trues_, cx_r.trues_);
      falses = cx_r.falses_;
      tr::Exp* ans = new tr::CxExp(trues, falses, stm1);
      return new tr::ExpAndTy(ans, type::IntTy::Instance());
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //env::VarEntry* got = static_cast<env::VarEntry*>(venv->Look(typ_));
  type::Ty* got_ty = tenv->Look(typ_);
  //note the record in the Tier should be a pointer.
  //But how can I allocate the mem space to it?--use the runtime function through the ExternalCall()
  //note that the record pointer is allocated in the HEAP, so it's addr should go up to get the late field.
  std::list<EField*> list = fields_->GetList();
  tree::ExpList* args = new tree::ExpList();
  //LAB7:
  tree::Exp* exp = new tree::ConstExp((reg_manager->WordSize()) * (list.size()));
  args->Append(exp);

  //create the string used to act as the descriptor of the record.
  //use a "1" to present a pointer field, use a "0" to present a non-pointer field.
  std::string arg_s = "";
  
  for(auto it = list.begin(); it != list.end(); ++it) {
      EField* e = *it;
      //env::VarEntry* entry = static_cast<env::VarEntry*>(venv->Look(e->name_)); 
      //type::Ty* type = entry->ty_;
      tr::ExpAndTy* got = (e->exp_)->Translate(venv, tenv, level, label, errormsg);
      type::Ty* type = got->ty_->ActualTy();
      if(typeid(*type) != typeid(type::IntTy) && typeid(*type) != typeid(type::StringTy)) {
          arg_s += "1";//the current field is a pointer.
      }
      else arg_s += "0";
  }
  
  std::string header_s = (typ_->Name()) + "_header";
  args->Append(new tree::NameExp(temp::LabelFactory::NamedLabel(header_s)));
  //printf("record-descriptor-str = %s\n", arg_s.c_str());
  //args->Append(new tree::ConstExp(atoi(arg_s.c_str())));
 
  tree::Exp* res = frame::Frame::ExternalCall("alloc_record", args);//call the external function to allocate the space for the record.
  //the res record the start addr of this record exp.
  int i = 0;//used to count the number of a field.
  temp::Temp* t = temp::TempFactory::NewTemp();
  tree::Stm* stm = new tree::MoveStm(new tree::TempExp(t), res);//use the temp reg t to store the start addr of the record.
  std::list<EField*>::iterator it = list.begin();
  tree::Stm* record_seq = nullptr;
  //allocate each field a mem space, and make these instrs to be a SeqStm(because it only moves, so use the Nx.)
  while(it != list.end()) {
      tr::ExpAndTy* got_it = (*it)->exp_->Translate(venv, tenv, level, label, errormsg);
      //LAB7:
      tree::Exp* mem = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(t), 
		      new tree::ConstExp((i) * (reg_manager->WordSize()))));
      if(record_seq == nullptr) record_seq = new tree::MoveStm(mem, got_it->exp_->UnEx());
      else record_seq = new tree::SeqStm(record_seq, new tree::MoveStm(mem, got_it->exp_->UnEx()));
      it++;
      i++;//increase the i to record the num of the field.
  }
  tree::Stm* ans = new tree::SeqStm(stm, record_seq);
  //ans = new tree::SeqStm(ans, new tree::TempExp(t));
  tr::ExExp* ans_exp = new tr::ExExp(new tree::EseqExp(ans, new tree::TempExp(t)));

  roots->temp_map[t] = 1;//use 1 to record the pointer-to-record temp.
  //printf("got a temp-root:t%d\n", t->Int());
  //roots->AddTempMap(t, 1);

  return new tr::ExpAndTy(ans_exp, got_ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //the val of a seq exp should be the last part of the seq_list.
  std::list<Exp*> list = seq_->GetList();
  tr::Exp* exp = new tr::ExExp(new tree::ConstExp(0));
  tr::ExpAndTy* ans = nullptr;
  if(list.size() == 0) return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  std::list<Exp*>::iterator it = list.begin();
  //use the loop (similar to the recursive) to link the SeqExp , and convert it to the EseqExp(in tree.h) at the same time.
  while(it != list.end()) {
      ans = (*it)->Translate(venv, tenv, level, label, errormsg);
      //if(exp == nullptr) exp = new tr::ExExp(new tree::EseqExp(nullptr, ans->exp_->UnEx()));
      if(exp == nullptr) exp = new tr::ExExp(ans->exp_->UnEx());//handle the first sub-exp.
      else exp = new tr::ExExp(new tree::EseqExp(exp->UnNx(), ans->exp_->UnEx()));
      it++;
  }  
  return new tr::ExpAndTy(exp, ans->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //Note that the AssignExp don't produce a val.So it's exp should be a NxExp.
  tr::ExpAndTy* var_got = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* exp_got = exp_->Translate(venv, tenv, level, label, errormsg);
  if(!var_got->ty_->IsSameType(exp_got->ty_)) return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  tr::Exp* stm = new tr::NxExp(new tree::MoveStm(var_got->exp_->UnEx(), exp_got->exp_->UnEx()));
  return new tr::ExpAndTy(stm, type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //can only translate the test and the then part first.
  tr::ExpAndTy* test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* then = then_->Translate(venv, tenv, level, label, errormsg);
  //NOTE:elsee_ has nullptr and not-null two cases.
  tr::Cx test_exp = test->exp_->UnCx(errormsg);//change the test-exp to the CX form.
  temp::Label* true_ = temp::LabelFactory::NewLabel();
  temp::Label* false_ = temp::LabelFactory::NewLabel();
  tr::PatchList trues({&true_});
  tr::PatchList falses({&false_});
  test_exp.trues_.DoPatch(true_);
  test_exp.falses_.DoPatch(false_);
  //static_cast<tree::CjumpStm*>(test_exp.stm_)->true_label_ = true_;
  //static_cast<tree::CjumpStm*>(test_exp.stm_)->false_label_ = false_;
  if(elsee_ == nullptr) {
      //start from  the easier case---elsee_ == nullptr
      //when elsee is null, there is no ret val.
      //test:
      // true:
      //   return UnExp of then
      // false:
      //   nullptr
      /*
      tr::Exp* exp = new tr::NxExp(new tree::SeqStm(
			      new tree::LabelStm(true_),//because there isno elsee_, so just consider the true to then and false to NULL cases.
			      new tree::SeqStm(then->exp_->UnNx(),
				      new tree::LabelStm(false_))));//put the false label(do nothing in this case at the end of the ans-exp.)
      */
      tr::Exp* exp = new tr::NxExp(new tree::SeqStm(test_exp.stm_,
			      new tree::SeqStm(
				      new tree::LabelStm(true_),
				      new tree::SeqStm(then->exp_->UnNx(),
					      new tree::LabelStm(false_)))));
      return new tr::ExpAndTy(exp, type::VoidTy::Instance());//if there's no else, the if should have a Void type.
  }
  //the case that elsee_ is not null.
  //there may be a ret val.
  /*convert logic is as follow:
   * r <- then_val
   * true_:
   *     r stall
   *     goto done.
   * false_:
   *     r <- else_val
   *     goto done.
   * done:
   *     (nothing)
   */
  temp::Temp* r = temp::TempFactory::NewTemp();//the temp reg used to store the ret val.
  tr::ExpAndTy* else_ = elsee_->Translate(venv, tenv, level, label, errormsg);//translate the else exp now.
  temp::Label* test_label = temp::LabelFactory::NewLabel();
  temp::Label* then_label = temp::LabelFactory::NewLabel();
  temp::Label* else_label = temp::LabelFactory::NewLabel();
  temp::Label* done_label = temp::LabelFactory::NewLabel();
  std::vector<temp::Label*>* dst = new std::vector<temp::Label*>();
  dst->push_back(done_label);
  tree::Stm* done_stm = new tree::JumpStm(new tree::NameExp(done_label), dst);
  //the ret exp should similar to the exp returned by the Cx's UnEx() function
  //so this ans should be a ExExp
  /*
  tr::Exp* exp = new tr::ExExp(new tree::EseqExp(
			  new tree::MoveStm(new tree::TempExp(r), else_->exp_->UnEx()),//move the else_exp to the res-reg r first.
		  new tree::EseqExp(
			  test_exp.stm_,//run the test-exp.
			  new tree::EseqExp(
				  new tree::LabelStm(true_),//the then-exp part begins here.
				  new tree::EseqExp(
					  new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()),//move the then-exp to the result-reg r.
					  new tree::EseqExp(
						  done_stm,//jump to the done label to finish the IF exp.
						  new tree::EseqExp(
							  new tree::LabelStm(false_),//then else-exp begins here.
							  new tree::EseqExp(
								  done_stm,//jump to the done label directly, because the r is initialized to else-exp's result.
								  new tree::EseqExp(
									  new tree::LabelStm(done_label),//put the done-label here.
									  new tree::TempExp(r))))))))));//get the result-reg r at the end of the IF exp.
  */
  tr::Exp* exp = new tr::ExExp(new tree::EseqExp(test_exp.stm_,
			  new tree::EseqExp(
				  new tree::LabelStm(true_),
				  new tree::EseqExp(
					  new tree::MoveStm(new tree::TempExp(r),then->exp_->UnEx()),
					  new tree::EseqExp(
						  done_stm,
						  new tree::EseqExp(
							  new tree::LabelStm(false_),
							  new tree::EseqExp(
								  new tree::MoveStm(new tree::TempExp(r), else_->exp_->UnEx()),
								  new tree::EseqExp(
									  done_stm,
									  new tree::EseqExp(
										  new tree::LabelStm(done_label),
										  new tree::TempExp(r))))))))));
  return new tr::ExpAndTy(exp, then->ty_->ActualTy());//if there is a else-exp, the IF exp my produce a res.
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  /*the convert logic is as follow:
   * test:
   *     if (condition == f) goto done
   *     body
   *     goto test
   * done:
   */
  tr::ExpAndTy* test = test_->Translate(venv, tenv, level, label, errormsg);
  temp::Label* done_label = temp::LabelFactory::NewLabel();
  tr::ExpAndTy* body = body_->Translate(venv, tenv, level, done_label, errormsg);//translate the test and the body first.
  //while exp should not produce a ret val.
  temp::Label* test_label = temp::LabelFactory::NewLabel();
  //temp::Label* done_label = temp::LabelFactory::NewLabel();
  temp::Label* body_label = temp::LabelFactory::NewLabel();
  tr::Cx test_exp = test->exp_->UnCx(errormsg);//change the test_exp into the CX form.
  tr::PatchList trues({&body_label});
  tr::PatchList falses({&done_label});
  test_exp.trues_.DoPatch(body_label);
  test_exp.falses_.DoPatch(done_label);
  //static_cast<tree::CjumpStm*>(test_exp.stm_)->true_label_ = body_label;
  //static_cast<tree::CjumpStm*>(test_exp.stm_)->false_label_ = done_label;
  std::vector<temp::Label*>* jump_dst = new std::vector<temp::Label*>();
  jump_dst->push_back(test_label);
  tr::Exp* exp = new tr::NxExp(new tree::SeqStm(
			  new tree::LabelStm(test_label),//go to the test first.
			  new tree::SeqStm(test_exp.stm_,//run the test-exp.
				  new tree::SeqStm(
					  new tree::LabelStm(body_label),//go to the body here.
					  new tree::SeqStm(
						  body->exp_->UnNx(),
						  new tree::SeqStm(
							  new tree::JumpStm(new tree::NameExp(test_label),jump_dst),//after one iter, go back to the test-exp.
							  new tree::LabelStm(done_label)))))));//put the done-label at the end of the exp.
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  /* the convert logic is as follow:
   * let
   *   var i := low
   *   var_limit := high//may be useless?
   * in 
   *   while i <= limit:
   *      do {
   *         body;
   *         i++
   *      }
   * end
   */
   //lab5-part1:the covertion above is equalvilent to the follow SeqExp:
   //var i = low; var limit = high; while exp.
  /*
  tr::ExpAndTy* low = lo_->Translate(venv,tenv, level, label, errormsg);
  tr::ExpAndTy* high = hi_->Translate(venv, tenv, level, label, errormsg);
  venv->BeginScope();//should give the loop_var a new scope
  tr::ExpAndTy* body = body_->Translate(venv, tenv, level, label, errormsg);
  Var* var = new SimpleVar(pos_, var_);//create the loop var.
  Exp* while_test = new OpExp(pos_, Oper::LE_OP, new VarExp(pos_, var), hi_);//test: i <= limit
  Exp* incr = new OpExp(pos_, Oper::PLUS_OP, new VarExp(pos_, var), new IntExp(pos_, 1));//increment: i := i + 1
  ExpList* seq = new ExpList();
  //note the order.
  seq->Prepend(incr);
  seq->Prepend(body_);
  Exp* while_body = new SeqExp(pos_, seq);
  WhileExp* let_body =  new WhileExp(pos_, while_test, while_body);//create the while exp as the let-in-end exp's new body.
  tr::ExpAndTy* got = let_body->Translate(venv, tenv, level, label, errormsg);//translate the compound while-exp.
  env::VarEntry* entry = new env::VarEntry(tr::Access::AllocLocal(level, false), type::IntTy::Instance(), true);//in-reg var, type, readonly
  venv->Enter(var_, entry);//put the new loop var into the venv.
  temp::Temp* reg = (static_cast<frame::InRegAccess*>(entry->access_->access_))->reg;
  tr::Exp* i = new tr::ExExp(new tree::TempExp(reg));
  tree::Stm* init_i = new tree::MoveStm(i->UnEx(), low->exp_->UnEx());
  tr::Exp* ans = new tr::NxExp(new tree::SeqStm(init_i, got->exp_->UnNx()));
  venv->EndScope();
  return new tr::ExpAndTy(ans, type::VoidTy::Instance());
  */
  //In Lab5: directly return the ans ExpAndTy, not through the while like before.
  tr::ExpAndTy* low = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* high = hi_->Translate(venv, tenv, level, label, errormsg);
  venv->BeginScope();

  env::VarEntry* entry = new env::VarEntry(tr::Access::AllocLocal(level, false), type::IntTy::Instance(), true);//make the loop var in the stack defaultly.
  venv->Enter(var_, entry);//put the loop var i into the venv.

  //tr::ExpAndTy* body = body_->Translate(venv, tenv, level, label, errormsg);
  temp::Label* test = temp::LabelFactory::NewLabel();
  temp::Label* done = temp::LabelFactory::NewLabel();//create the label for the test and the done.
  temp::Label* body_label = temp::LabelFactory::NewLabel();
  //NOTE:the body should be translated in the done_label of thi level, because at the end of the body, it should go to the 
  //done_label of it's own, not it's parent's!
  tr::ExpAndTy* body = body_->Translate(venv, tenv, level, done, errormsg);
  //env::VarEntry* entry = new env::VarEntry(tr::Access::AllocLocal(level, false), type::IntTy::Instance(), true);//make the loop var in the stack defaultly.
  //venv->Enter(var_, entry);//put the loop var i into the venv.
  //temp::Temp* reg = (static_cast<frame::InRegAccess*>(entry->access_->access_))->reg;
  //tr::Exp* i = new tr::ExExp(new tree::TempExp(reg));//create the i in the tree namespace.
  tree::Exp* i = entry->access_->access_->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  tree::Stm* init_stm = new tree::MoveStm(i, low->exp_->UnEx());//init the i with the low exp.
  tree::Stm* test_stm = new tree::CjumpStm(tree::LE_OP, i, high->exp_->UnEx(), body_label, done);//create the loop-test stm.
  tree::Exp* incr = new tree::BinopExp(tree::PLUS_OP, i, new tree::ConstExp(1));
  tree::Stm* incr_stm = new tree::MoveStm(i, incr);//create the i = i + 1 corresponding stm.(operate the i + 1, and move the res to the i.)
  std::vector<temp::Label*>* dst = new std::vector<temp::Label*>();
  dst->push_back(test);
  tree::Stm* jmp_test = new tree::JumpStm(new tree::NameExp(test), dst);//create the jump-to-test stm.
  tree::Stm* done_stm = new tree::LabelStm(done);//create the done-label corresponding labelStm.
  /*
  tr::Exp* ans = new tr::NxExp(new tree::SeqStm(init_stm,//put the loop-var-init stm into the ans first.
			  new tree::SeqStm(test_stm,//put the test-stm into the ans stm.
				  new tree::SeqStm(
					  new tree::LabelStm(body_label),//put the body label here.
					  new tree::SeqStm(body->exp_->UnNx(),//add the body stm to the ans.
						  new tree::SeqStm(incr_stm,//put the incr stm after the body part to form the entire body of the for exp.
							  new tree::SeqStm(jmp_test,//after the body, it must jump to the test to check.
								  done_stm)))))));//put the done-label at the end of the stm, as before cases.
  */
  venv->EndScope();
  tr::Exp* ans = new tr::NxExp(new tree::SeqStm(init_stm,
			  new tree::SeqStm(new tree::LabelStm(test),
				  new tree::SeqStm(test_stm,
					  new tree::SeqStm(new tree::LabelStm(body_label),
						  new tree::SeqStm(body->exp_->UnNx(),
							  new tree::SeqStm(incr_stm,
								  new tree::SeqStm(jmp_test,
									  done_stm))))))));
  return new tr::ExpAndTy(ans, type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //where the BREAK should jump to??
  //jump to the param label directly.
  std::vector<temp::Label*>* list = new std::vector<temp::Label*>();
  list->push_back(label);
  tree::Stm* stm = new tree::JumpStm(new tree::NameExp(label), list);
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  /* the convertion logic is as follows:
   * let
   *   decs
   * in
   *   body
   * end
   * <-->
   * decs;body;(as Seq Exps)
   * <-->
   * should similar to the impl of the translate of the absyn::SeqExp.
   */
  //the let exp's ret val should be the last seqExp in the body's val.
  std::list<Dec*> decs = decs_->GetList();
  std::list<Dec*>::iterator it = decs.begin();
  //the LET exp should begin the two new scope at the same time.
  venv->BeginScope();
  tenv->BeginScope();
  tr::Exp* seq = nullptr;
  //this part is similar to the EseqExp part.
  while(it != decs.end()) {
      tr::Exp* exp = (*it)->Translate(venv, tenv, level, label, errormsg);
      if(seq == nullptr) seq = exp;
      else seq = new tr::ExExp(new tree::EseqExp(seq->UnNx(), exp->UnEx()));
      it++;
  }
  tr::ExpAndTy* body = body_->Translate(venv, tenv, level, label, errormsg);
  tenv->EndScope();
  venv->EndScope();
  if(seq == nullptr) return body;
  tr::Exp* ans = new tr::ExExp(new tree::EseqExp(seq->UnNx(), body->exp_->UnEx()));
  return new tr::ExpAndTy(ans, body->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,        
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  //arrayExp in the tiger should be a pointer
  //similar to the recordExp.
  tr::ExpAndTy* size = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* init = init_->Translate(venv, tenv, level, label, errormsg);
  
  //we do not consider the case that the array has a pointer type.

  tree::ExpList* args = new tree::ExpList();
  args->Append(size->exp_->UnEx());
  args->Append(init->exp_->UnEx());//note the order of the parameters.
  tree::Exp* ans = frame::Frame::ExternalCall("init_array", args);
  
  temp::Temp* t = temp::TempFactory::NewTemp();
  roots->temp_map[t] = 2;//use the type 2 to describe the arrayTy.
  tree::Stm* mov = new tree::MoveStm(new tree::TempExp(t), ans);//move the result to a temp, which is convient for the collection of roots.
  /*
  tree::ConstExp* size_val = static_cast<tree::ConstExp*>(size->exp_);
  int len = size_val->consti_;
  tree::Stm* fill_stm = nullptr;
  for(int i = 0; i < len; ++i) {
      tree::BinopExp* binop = new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(t), new tree::ConstExp((i + 1) * 8));
      tree::Exp* mem = new tree::MemExp(binop);
      if(fill_stm == nullptr) fill_stm = new tree::MoveStm(mem, init->exp_);
      else fill_stm = new tree::SeqStm(fill_stm, new tree::MoveStm(mem, init->exp_));
  }
  tree::Stm* ans1 = new tree::SeqStm(mov, fill_stm);
  */
  tree::Exp* res = new tree::EseqExp(mov, new tree::TempExp(t));//remeber to return the result pointer!
  return new tr::ExpAndTy(new tr::ExExp(res), tenv->Look(typ_)->ActualTy());

  //return new tr::ExpAndTy(new tr::ExExp(ans), tenv->Look(typ_)->ActualTy());
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* ans = new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
  return ans;
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec*> list = functions_->GetList();
  if(list.size() == 0) return new tr::ExExp(new tree::ConstExp(0));
  std::list<FunDec*>::iterator it = list.begin();
  while(it != list.end()) {
      type::TyList* tys = (*it)->params_->MakeFormalTyList(tenv, errormsg);
      temp::Label* func_label = temp::LabelFactory::NamedLabel((*it)->name_->Name());//alloc a label to this func from its name
      //analyze the escape information.
      std::list<bool> escape;
      std::list<Field*> fields = (*it)->params_->GetList();
      //std::list<Field*>::iterator field;
      for(auto field : fields) {
          escape.push_back(field->escape_);
      }
      //create a new level for a new function
      tr::Level* new_level = tr::Level::NewLevel(level, func_label, escape);
      type::Ty* res = type::VoidTy::Instance();
      if((*it)->result_ != nullptr) res = tenv->Look((*it)->result_);
      //first enter the function with the empty function-body into the venv
      venv->Enter((*it)->name_, new env::FunEntry(new_level, func_label, tys, res));
      it++;
  } 
  it = list.begin();
  while(it != list.end()) {
      venv->BeginScope();
      tenv->BeginScope();
      env::FunEntry* entry = static_cast<env::FunEntry*>(venv->Look((*it)->name_));
      type::TyList* tys = (*it)->params_->MakeFormalTyList(tenv, errormsg);
      auto params = (*it)->params_->GetList();
      std::list<type::Ty*> ty = tys->GetList();
      auto accesses = entry->level_->GetList();
      //auto param = params.begin();
      auto ty0 = ty.begin();
      auto access = std::next(accesses.begin());//jump the SL.
      //LAB7:
      entry->level_->frame_->pointers.push_back(0);//the SL will not be a root.beacause it contains a pointer to the stack not the heap.
      for(auto param : params) {
          venv->Enter(param->name_, new env::VarEntry(*access, *ty0));
	  //access++;
	  //ty0++;
	  //LAB7:GC
	  type::Ty* actual = (*ty0)->ActualTy();
	  /*
	  if(typeid(*actual) == typeid(type::RecordTy)) {//a record pointer.
	      entry->level_->frame_->pointers.push_back(1);
	  }
	  else {
	      if(typeid(*actual) == typeid(type::ArrayTy)) {//a array pointer.
	          entry->level_->frame_->pointers.push_back(2);
	      }
	      else entry->level_->frame_->pointers.push_back(0);//not a pointer.
	  }
	  */
	  //LAB7:GC
	  if(typeid(*actual) == typeid(type::RecordTy) || typeid(*actual) == typeid(type::ArrayTy)) {
	      //this tr::Access contains a pointer
	      (*access)->access_->setStorePointer();//set the is_pointer to true.
	  }
	  access++;
	  ty0++;
      }
      //tr::ExpAndTy* trans = (*it)->body_->Translate(venv, tenv, entry->level_, entry->label_, errormsg);//translate the body in the level this function is declared.
      tr::ExpAndTy* trans = (*it)->body_->Translate(venv, tenv, entry->level_, entry->label_, errormsg);
      venv->EndScope();
      tenv->EndScope();
      //LAB7:GC
      type::Ty* result_ty = entry->result_->ActualTy();
      if(typeid(*result_ty) == typeid(type::RecordTy) || typeid(*result_ty) == typeid(type::ArrayTy)) {
          ret_pointers.push_back(entry->label_->Name());
      }

      tree::Stm* stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), trans->exp_->UnEx());
      tree::Stm* proc_body = frame::ProcEntryExit1(entry->level_->frame_, stm);
      frags->PushBack(new frame::ProcFrag(proc_body, entry->level_->frame_));//put the newly-translated function's frag into the global frags list.
      //venv->EndScope();
      it++;
  }
  return new tr::ExExp(new tree::ConstExp(0));//the FuncDec just return a "no-op" exp.
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* init = init_->Translate(venv, tenv, level, label, errormsg);
  tr::Access* access = tr::Access::AllocLocal(level, escape_);//allocate the mem to the var.
  env::VarEntry* new_var = new env::VarEntry(access, init->ty_);
  venv->Enter(var_, new_var);
  //must return a ASSIGN here to assign the init_val to the newly-declared var.
  /*
  tree::Exp* framePtr = new tree::TempExp(reg_manager->FramePointer());//get the current level's fp pointer.
  tree::Exp* var_dec = access->access_->ToExp(framePtr);
  tr::Exp* ans = new tr::NxExp(new tree::MoveStm(var_dec, init->exp_->UnEx())); 
  return ans;
  */
  
  tree::Exp* fp = StaticLink(level, access->level_);
  tr::Exp* tmp = new tr::ExExp(access->access_->ToExp(fp));
  tr::Exp* ans = new tr::NxExp(new tree::MoveStm(tmp->UnEx(), init->exp_->UnEx()));
  //LAB7:
  type::Ty* init_ty = init->ty_->ActualTy();
  if(typeid(*init_ty) == typeid(type::RecordTy) || typeid(*init_ty) == typeid(type::ArrayTy)) {
      access->access_->setStorePointer();
  }
  return ans;
  
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy*> list = types_->GetList();
  if(list.size() == 0) return nullptr;
  std::list<NameAndTy*>::iterator it = list.begin();
  while(it != list.end()) {
      tenv->Enter((*it)->name_, new type::NameTy((*it)->name_, nullptr));
      it++;
      //first just push the empty type and the type name into the env
      //to prevent the recursive dec.
  }
  it = list.begin();
  while(it != list.end()) {
      type::Ty* type = tenv->Look((*it)->name_);
      type::NameTy* ty = static_cast<type::NameTy*>(type);
      type::Ty* res = (*it)->ty_->Translate(tenv, errormsg);//translate the type declared in this dec
      //tenv->Set((*it)->name_, new type::NameTy((*it)->name_, res));//put the newly-got type bakc to the old "empty" position.
      ty->ty_ = res;
      it++;
  }
  //LAB7:GC
  it = list.begin();
  while(it != list.end()) {
      NameAndTy* nat = *it;
      type::NameTy* name_ty = static_cast<type::NameTy*>(tenv->Look(nat->name_));
      type::Ty* got_ty = name_ty->ty_->ActualTy();
      if(typeid(*got_ty) == typeid(type::RecordTy)) {
          type::RecordTy* record_ty = static_cast<type::RecordTy*>(name_ty->ty_);
	  sym::Symbol* ty_name = name_ty->sym_;
	  std::string record_name = (ty_name->Name()) + "_header";
	  std::string record_header = "";
	  std::list<type::Field*> field_list = record_ty->fields_->GetList();
	  for(auto it1 = field_list.begin(); it1 != field_list.end(); ++it1) {
	      type::Field* field = *it1;
	      type::Ty* f_ty = field->ty_->ActualTy();
	      if(typeid(*f_ty) == typeid(type::RecordTy) || typeid(*f_ty) == typeid(type::ArrayTy)) {
	          record_header += "1";
	      }
	      else record_header += "0";
	  }
	  temp::Label* record_label = temp::LabelFactory::NamedLabel(record_name);
	  frame::StringFrag* sf = new frame::StringFrag(record_label, record_header);
	  frags->PushBack(sf);
      }
      it++;
  }
  return new tr::ExExp(new tree::ConstExp(0));//typeDec just return a "no-op" exp.
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* type = tenv->Look(name_);
  return new type::NameTy(name_, type);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Field*> list = record_->GetList();
  type::FieldList* ans = new type::FieldList();
  if(list.size() == 0) return new type::RecordTy(ans);
  std::list<Field*>::iterator it = list.begin();
  while(it != list.end()) {
      type::Ty* type = tenv->Look((*it)->typ_);
      type::Field* f = new type::Field((*it)->name_, type);
      ans->Append(f);
      it++;
  } 
  return new type::RecordTy(ans);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty* type = tenv->Look(array_);
  return new type::ArrayTy(type);
}

} // namespace absyn
