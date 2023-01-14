#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  root_->Traverse(env, 0);
}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry* t = env->Look(sym_);
  if(*(t->escape_) == true) return ;
  if((t->depth_) < depth) *(t->escape_) = true; 
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  //the analysis of the escape of the Field is equal to the analysis of the var.
  //i.e.,the a.f var, we should analyze the escape of a.
  var_->Traverse(env, depth);
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  //var[exp]
  var_->Traverse(env, depth);//analyze the var first.
  subscript_->Traverse(env, depth);//we should also analyze the subscript part, because the sub part is also a Exp!
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env, depth);
}

 /*in the analysis of the escape of the basic types in the Tiger, DO NOTHING.*/
void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  //in the analysis of the escape of the CallExp, we only need to care about the args_.
  std::list<Exp*> exp = args_->GetList();
  if(exp.size() == 0) return ;
  std::list<Exp*>::iterator it;
  for(it = exp.begin(); it != exp.end(); ++it) {
      (*it)->Traverse(env, depth);
  }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  left_->Traverse(env, depth);
  right_->Traverse(env, depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<EField*> list = fields_->GetList();
  if(list.size() == 0) return ;
  std::list<EField*>::iterator it;
  for(it = list.begin(); it != list.end(); ++it) {
      (*it)->exp_->Traverse(env, depth);
  }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp*> list = seq_->GetList();
  if(list.size() == 0) return ;
  std::list<Exp*>::iterator it;
  for(it = list.begin(); it != list.end(); ++it) {
      (*it)->Traverse(env, depth);
  }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env, depth);
  exp_->Traverse(env, depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env, depth);
  then_->Traverse(env, depth);
  if(elsee_ == nullptr) return ;
  elsee_->Traverse(env, depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env, depth);
  if(body_ == nullptr) return ;
  body_->Traverse(env, depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  //note that the test var in the FOR needs a new scope, so we should put it into  the env first
  //the escape should be a pointer!
  env->BeginScope();
  escape_ = false;
  esc::EscapeEntry* t = new esc::EscapeEntry(depth, &(escape_));
  env->Enter(var_, t);
  lo_->Traverse(env, depth);
  hi_->Traverse(env, depth);
  if(body_ == nullptr) {
      env->EndScope();
      return ;
  }
  body_->Traverse(env, depth);
  env->EndScope();
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  //the breakExp don't need to do the escape analysis.
  
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Dec*> list = decs_->GetList();
  for(auto l : list) {
      l->Traverse(env, depth);
  }
  if(body_ == nullptr) return ;
  body_->Traverse(env, depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  size_->Traverse(env, depth);
  init_->Traverse(env, depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return ;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<FunDec*> list = functions_->GetList();
  if(list.size() == 0) return ;
  std::list<FunDec*>::iterator it;
  for(it = list.begin(); it != list.end(); ++it) {
      env->BeginScope();
      std::list<Field*> list1 = (*it)->params_->GetList();
      std::list<Field*>::iterator it1;
      for(it1 = list1.begin(); it1 != list1.end(); ++it1) {
	  (*it1)->escape_ = false;
          esc::EscapeEntry* t = new esc::EscapeEntry(depth + 1, &((*it1)->escape_));
	  env->Enter((*it1)->name_, t);
	  //if(body_ == nullptr) continue;
	  //body_->Traverse(env, depth + 1);//note that the body in the newly-declared func should have a depth of the (dep + 1).
      }
      if((*it)->body_ == nullptr) continue;
      (*it)->body_->Traverse(env, depth + 1);//note the depth should + 1.
      env->EndScope();
  }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  escape_ = false;//the newly-declared var should have a FALSE escape attr.
  esc::EscapeEntry* t = new esc::EscapeEntry(depth, &(escape_));
  env->Enter(var_, t);
  init_->Traverse(env, depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy*> list = types_->GetList();
  if(list.size() == 0) return ;
  std::list<NameAndTy*>::iterator it;
  for(it = list.begin(); it != list.end(); ++it) {
      if(!(typeid(*(*it)->ty_) == typeid(RecordTy))) continue;
      //esc::EscapeEntry* got = env->Look((*it)->name_);
      //*(got->escape_) = true;
      RecordTy* ty = static_cast<RecordTy*>((*it)->ty_);
      std::list<Field*> fields = ty->record_->GetList();
      for(auto field : fields) {
	  //the record type will be allocated in the Heap, so it will always in the MEM.
          field->escape_ = true;
      }
  } 
  return ;//return directly because we don't need to do the escape analysis of the newly-declared type.
}

} // namespace absyn
