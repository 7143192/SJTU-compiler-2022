#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

#include "tiger/frame/x64frame.cc"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

class PatchList {
public:
  void DoPatch(temp::Label *label) {
    for(auto &patch : patch_list_) *patch = label;
  }

  static PatchList JoinPatch(const PatchList &first, const PatchList &second) {
    PatchList ret(first.GetList());
    for(auto &patch : second.patch_list_) {
      ret.patch_list_.push_back(patch);
    }
    return ret;
  }

  explicit PatchList(std::list<temp::Label **> patch_list) : patch_list_(patch_list) {}
  PatchList() = default;

  [[nodiscard]] const std::list<temp::Label **> &GetList() const {
    return patch_list_;
  }

private:
  std::list<temp::Label **> patch_list_;
};

class Access {
public:
  Level *level_;
  frame::Access *access_;

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}
  static Access *AllocLocal(Level *level, bool escape);
};

class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  /* TODO: Put your lab5 code here */
  Level(frame::Frame* f, Level* p):frame_(f),parent_(p) {}
  /*parent:parent's level, name: the callee function's name, formals: the escape attrs of the callee function's formals*/
  static Level* NewLevel(Level* parent, temp::Label* name, std::list<bool> formals);
  std::list<tr::Access*> GetList() {
      std::list<frame::Access*> access = frame_->formals;
      std::list<tr::Access*> ans;
      for(frame::Access* a : access) {
          tr::Access* acc = new tr::Access(this, a);
	  ans.push_back(acc);
      }
      return ans;
  }
};

class ProgTr {//ProgTr should be the entry of the translation in the semantic analysis.
public:
  // TODO: Put your lab5 code here

  ProgTr(std::unique_ptr<absyn::AbsynTree> tree, std::unique_ptr<err::ErrorMsg> error) {
	      absyn_tree_ = std::move(tree);
	      errormsg_ = std::move(error);
	      tenv_ = std::make_unique<env::TEnv>();
	      venv_ = std::make_unique<env::VEnv>();
	      /*
	      std::list<bool> formal;
	      temp::Label* first = temp::LabelFactory::NamedLabel("tigermain");//use the "tigermain" as the first-level's name to notify the start of the translation. 
	      frame::Frame* f = new frame::X64Frame(first, formal);//the first level is abstarct, so it does not have formals.
	      main_level_ = std::make_unique<Level>(f, nullptr);//store the first Level with the main_level_ attribute.
	      */
	      //FillBaseVEnv();
	      //FillBaseTEnv();//init the envs.
  }//a constructor for the ProgTr Class.
  /**
   * Translate IR tree
   */
  void Translate();

  /**
   * Transfer the ownership of errormsg to outer scope
   * @return unique pointer to errormsg
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }

private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;//the level used to nest the main function.
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  // Fill base symbol for var env and type env
  // these two functions are implemented in the file tiger/env/env.c
  void FillBaseVEnv();
  void FillBaseTEnv();
};

} // namespace tr

#endif
