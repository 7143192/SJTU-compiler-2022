#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	root_->SemAnalyze(venv, tenv, 0, errormsg);
}

 /*the Var part just need to lookup in the tenv and the venv, beacause they won't introduce new type or vars.*/

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	env::EnvEntry *entry = venv->Look(sym_);//search for the variable in the venv
	if(entry && typeid(*entry) == typeid(env::VarEntry)) {
	    return (static_cast<env::VarEntry*> (entry))->ty_->ActualTy();
	}
	else {//not found
	    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
	}
	return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(!ty || typeid(*ty) != typeid(type::RecordTy)) {
	    errormsg->Error(pos_, "not a record type");
	    return type::IntTy::Instance();//the a.b of a is wrong, return directly.
	}
	type::RecordTy* ty1 = static_cast<type::RecordTy*>(ty);
	std::list<type::Field*> list = ty1->fields_->GetList();//get the fields list.
	std::list<type::Field*>::iterator iter;
	type::Ty* gotTy;
	for(iter = list.begin(); iter != list.end(); ++iter) {
	    if((*iter)->name_ == sym_) {
	    	gotTy = (*iter)->ty_;
		break;
	    }//try to find the coresponding type from the declared field list of a structure.
	}
	if(iter != list.end()) return gotTy;
	else {
	    errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());//don't find the symbol.
	}
	//env::EnvEntry *entry = venv->Look(sym_);
	//if(entry && typeid(*entry) == typeid(*gotTy)) return gotTy;
	return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = var_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
	if(!ty || typeid(*ty) != typeid(type::ArrayTy)) {
	    errormsg->Error(pos_, "array type required");
	    return type::IntTy::Instance();//the start of the subVar is wrong, return directly.
	}
	type::Ty *expTy = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();//parse the subscript value.
	if(typeid(*expTy) != typeid(type::IntTy)) {
	    errormsg->Error(pos_, "array variable's subscript type should be an INT");
	    return type::IntTy::Instance();
	}
	return static_cast<type::ArrayTy*>(ty)->ty_->ActualTy();//the type of the a[exp] should be the type of the element of array a.
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
	//try to parse the var exp according to it's actual type.
	/*if(typeid(*var_) == typeid(SimpleVar)) {
	    return static_cast<SimpleVar*>(var_)->SemAnalyze(venv, tenv, labelcount, errormsg);
	}
	if(typeid(*var_) == typeid(FieldVar)) {
	    return static_cast<FieldVar*>(var_)->SemAnalyze(venv, tenv, labelcount, errormsg);
	}
	return static_cast<SubscriptVar*>(var_)->SemAnalyze(venv, tenv, labelcount, errormsg);*/
}
 
 /*the NIL,INT,STRING are the basic type, return the actual type directly.*/
type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	return type::StringTy::Instance();
}

 /*it's the check of the invocation of a function, not the declaration of it, so only lookup without inserting.*/
type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	env::EnvEntry* entry = venv->Look(func_);//get the entry of the function  in the venv.
	if(!entry || typeid(*entry) != typeid(env::FunEntry)) {
	    errormsg->Error(pos_, "undefined function %s", func_->Name().data());//don't find the corresponding function name.
	    return type::IntTy::Instance();
	}
	type::Ty* result = static_cast<env::FunEntry*>(entry)->result_;
	std::list<type::Ty*> list = static_cast<env::FunEntry*>(entry)->formals_->GetList();//get the declaration of args of the function func_.
	std::list<Exp*> called = args_->GetList();//get the args when the user call the function.	
	if(list.size() > called.size()) {
	    errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
	    if(called.size() == 0) return type::IntTy::Instance();
	}//less args
	if(list.size() < called.size()) {
	    errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
	    if(list.size() == 0) return type::IntTy::Instance();
	}//more args
	if(list.size() == 0) return result->ActualTy();//don't have args.
	std::list<type::Ty*>::iterator it1 = list.begin();
	std::list<Exp*>::iterator it2 = called.begin();
	int flag = 1;
	while(it1 != list.end() && it2 != called.end()) {
	    type::Ty* got = (*it2)->SemAnalyze(venv, tenv, labelcount, errormsg);
	    if(!got->IsSameType((*it1))) {//the input function param type is different from  the declared one.
	        errormsg->Error((*it2)->pos_, "para type mismatch");
		//return type::IntTy::Instance();
		flag = 0;
	    }
	    it1++;
	    it2++;
	}
	//if(flag == 0) return type::IntTy::Instance();
	return result->ActualTy();//return the result's type directly.
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* leftTy = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
	type::Ty* rightTy = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();

	//errormsg->Error(pos_, "DEBUG: leftTy = %s, rightTy = %s");
	//NOTE:as for the AND_OP and OR_OP, in the type-checking phase, we should make sure both the left and the right part are INT
	//beacause they are equal to the IF THEN ELSE,according tiger's rules,then and else should have the same type, which are needed to be type::IntTy.
	if(oper_ == PLUS_OP || oper_ == MINUS_OP || oper_ == TIMES_OP || oper_ == DIVIDE_OP || oper_ == AND_OP || oper_ == OR_OP) {
	    if(typeid(*leftTy) != typeid(type::IntTy)) errormsg->Error(left_->pos_, "integer required");
	    if(typeid(*rightTy) != typeid(type::IntTy)) errormsg->Error(right_->pos_, "integer required");
	    return type::IntTy::Instance();
	}
	else {//as for the comparison ops, we just make sure they have the same left and right type.
	    if(!leftTy->IsSameType(rightTy)) errormsg->Error(pos_, "same type required");
	}
	return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = tenv->Look(typ_);//try to find the type of the recordExp.
	if(ty == NULL) {
	    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
	    return type::IntTy::Instance();
	}//not found
	ty = ty->ActualTy();
	if(typeid(*ty) != typeid(type::RecordTy)) {
	    errormsg->Error(pos_, "not a record type");
	    return type::IntTy::Instance();
	}
	std::list<EField*> fields = fields_->GetList();//get the fieldList from the user's program.
	std::list<type::Field*> type_fields = static_cast<type::RecordTy*>(ty)->fields_->GetList();//get the fieldList from the stored enviroment.
	if(fields.size() != type_fields.size()) {
	    errormsg->Error(pos_, "more field vals are required");
	}
	if(type_fields.size() == 0 || fields.size() == 0) return ty;
	std::list<EField*>::iterator it1 = fields.begin();
	std::list<type::Field*>::iterator it2 = type_fields.begin();
	//int flag = 1;
	while(it1 != fields.end()) {
	    type::Ty* got;
	    for(; it2 != type_fields.end(); ++it2) {
	        if((*it1)->name_ == (*it2)->name_) {//note that the symbol table will make the same string point to the same address, so we can compare the pointer directly.
		    got = (*it2)->ty_;
		    break;
		}
	    }    
	    if(it2 == type_fields.end()) {
	        errormsg->Error(pos_, "the field name doesn't exist");
		//flag = 0;
	    }
	    it2 = type_fields.begin();
	    type::Ty* expTy = (*it1)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);//in the id := exp, the id and the exp should have the same type.
	    if(!expTy->IsSameType(got)) {
	        errormsg->Error(pos_, "unmatched assign exp");
		//flag = 0;
	    }
	    it1++;
	}
	//if(flag == 0) return type::IntTy::Instance();
	return ty;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	std::list<Exp*> list = seq_->GetList();
	type::Ty* res = type::VoidTy::Instance();
	if(list.size() == 0) return res;
	std::list<Exp*>::iterator it = list.begin();
	for(; it != list.end(); ++it) {
	    if(typeid(*it) == typeid(BreakExp)) errormsg->Error((*it)->pos_, "break is not inside any loop");//if the break is detected here, it must outside a loop.
	    res = (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);//the last exp of the seq will be the final result, so the final type should be equal to the last exp.
	}
	return res;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* left = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
	type::Ty* right = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(typeid(*var_) == typeid(SimpleVar)) {
	    env::EnvEntry* entry = venv->Look(static_cast<SimpleVar*>(var_)->sym_ );
	    if(entry->readonly_ == true) {
	        errormsg->Error(pos_, "loop variable can't be assigned");
	    }//QAQ
	}
	if(!left->IsSameType(right)) errormsg->Error(pos_, "unmatched assign exp");
	//return type::VoidTy::Instance();//the assignExp doesn't have a return type.
	return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* testTy = this->test_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(typeid(*testTy) != typeid(type::IntTy)) errormsg->Error(test_->pos_, "if exp's test must produce int value");//the test exp must have the INT type.
	type::Ty* thenTy = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
	//the elsee_ have null and not-null cases.
	if(elsee_ != NULL) {
	    type::Ty* elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
	    if(!thenTy->IsSameType(elseTy)) {
		errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
		return type::VoidTy::Instance();
	    }
	    return thenTy->ActualTy();//if there's a ELSE, the IF may produce a return value
	}
	else {
	    if(typeid(*thenTy) != typeid(type::VoidTy)) {
		errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
		return type::VoidTy::Instance();
	    }
	    return type::VoidTy::Instance();//Without a else, the IF exp can't produce any value.
	}
	return type::VoidTy::Instance();
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	//int flag = 1;
	type::Ty* testTy = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(typeid(*testTy) != typeid(type::IntTy)) {
	    errormsg->Error(test_->pos_, "while exp's test must produce int value");
	    //flag = 0;
	}
	//venv->BeginScope();
	if(body_ == NULL) return type::VoidTy::Instance();//if the body is empty, return directly.
	type::Ty* bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);//label count is increased, same to the FOR exp.
	if(typeid(*bodyTy) != typeid(type::VoidTy)) {
	    errormsg->Error(body_->pos_, "while body must produce no value");
	    //flag = 0;
	}
	//venv->EndScope();
	//if(flag == 0) return type::IntTy::Instance();
	return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	//type::Ty* varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
	//int flag = 1;
	/*if(typeid(*varTy) != typeid(type::IntTy)) {
	    flag = 0;
	    errormsg->Error(pos_, "the loop variable should be an int");
	}*/
	/*
	if(typeid(*var_) != typeid(SimpleVar)) {
	    errormsg->Error(pos_, "mismatch test var type");
	    //return type::VoidTy::Instance();
	}
	*/
	type::Ty* loTy = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
	type::Ty* hiTy = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(typeid(*loTy) != typeid(type::IntTy)) {
	    //flag = 0;
	    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
	}
	if(typeid(*hiTy) != typeid(type::IntTy)) {
	    //flag = 0;
	    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
	}
	//if(body_ == NULL) return type::VoidTy::Instance();//if the body is empty, return directly.
	venv->BeginScope();
	venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));//the loop variable can't be changed during the loop.
	if(body_ == NULL) return type::VoidTy::Instance();
	type::Ty* bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);//make the labelcount increase, so that the BREAK exp in the loop won't be detected to be error.
	venv->EndScope();
	//if(flag == 0) return type::VoidTy::Instance();
	return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	if(labelcount == 0) errormsg->Error(pos_, "break is not inside any loop");//the BREAK inside a loop should have a labelcount at least 1.
	return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	//this function is the same as the one provided in the PPT.
	venv->BeginScope();
	tenv->BeginScope();
	std::list<Dec*> decs = decs_->GetList();
	auto it = decs.begin();
	for(; it != decs.end(); ++it) {
	    (*it)->SemAnalyze(venv, tenv, labelcount, errormsg);
	}
	//type::Ty* result;
	if(body_ == nullptr) {
	    tenv->EndScope();
	    venv->EndScope();
	    return type::VoidTy::Instance();  
	}
	type::Ty* result = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
	tenv->EndScope();
	venv->EndScope();
	return result;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = tenv->Look(typ_)->ActualTy();//try to search for the declared array type.
	if(ty == NULL) {
	    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
	    return ty;
	}
	ty = ty->ActualTy();
	if(typeid(*ty) != typeid(type::ArrayTy)) {
	    errormsg->Error(pos_, "not an array type");
	    return type::IntTy::Instance();
	}
	type::Ty* sizeTy = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
	if(typeid(*sizeTy) != typeid(type::IntTy)) {
	    errormsg->Error(size_->pos_, "integer required");
	}
	type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
	/*if(typeid(*initTy) != typeid(type::IntTy)) {
	    errormsg->Error(init_->pos_, "the type of the array init val must be int.");
	}*/
	type::ArrayTy* arrayTy = static_cast<type::ArrayTy*>(ty);
	type::Ty* ty1 = arrayTy->ty_;
	if(!initTy->IsSameType(ty1)) errormsg->Error(init_->pos_, "init type and array type mismatch");
	return arrayTy;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	std::list<FunDec*> fundecs = functions_->GetList();//get the function declaration list.
	std::list<FunDec*>::iterator iter = fundecs.begin();
	for(; iter != fundecs.end(); ++iter) {
	    env::EnvEntry* entry = venv->Look((*iter)->name_);
	    if(entry != NULL) {
	        errormsg->Error(pos_, "two functions have the same name");
		continue;
	    }
	    FieldList* params = (*iter)->params_;
	    type::Ty* res = type::VoidTy::Instance();
	    if((*iter)->result_ != NULL) {
	        res = tenv->Look((*iter)->result_);
		if(res == NULL) errormsg->Error(pos_, "undefined result type");
	    }
	    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
	    venv->Enter((*iter)->name_, new env::FunEntry(formals, res));
	}//put the header(func name, params TYPE, return TYPE) to the venv.
	iter = fundecs.begin();
	for(; iter != fundecs.end(); ++iter) {
	    venv->BeginScope();
	    FieldList* params = (*iter)->params_;
	    type::TyList* formals = params->MakeFormalTyList(tenv, errormsg);
	    std::list<Field*> params1 = params->GetList();
	    std::list<type::Ty*> formals1 = formals->GetList();
	    std::list<Field*>::iterator param = params1.begin();
	    std::list<type::Ty*>::iterator formal = formals1.begin();
	    while(param != params1.end()) {
	        venv->Enter((*param)->name_, new env::VarEntry(*formal));
		param++;
		formal++;
	    }
	    type::Ty* ty = (*iter)->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
	    env::EnvEntry* entry1 = venv->Look((*iter)->name_);
	    env::FunEntry* funcEntry = static_cast<env::FunEntry*>(entry1);
	    type::Ty* gotTy = funcEntry->result_;
	    if(!gotTy->IsSameType(ty)) {
	        if(!gotTy->IsSameType(type::VoidTy::Instance())) errormsg->Error((*iter)->body_->pos_, "return type mismatch");
		else errormsg->Error((*iter)->body_->pos_, "procedure returns value");
	    }
	    venv->EndScope();
	}//put the body to the corresponding place.
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	if(typ_ == NULL) {
	    type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
	    if(typeid(*initTy) == typeid(type::NilTy)) {
	        errormsg->Error(init_->pos_, "init should not be nil without type specified");
		//return ;
	    }
	    venv->Enter(var_, new env::VarEntry(initTy->ActualTy()));
	}
	if(typ_ != NULL) {
	    type::Ty* initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
	    type::Ty* type = tenv->Look(typ_);
	    if(type == NULL) {
	        errormsg->Error(pos_, "undefined type of %s", typ_->Name().data());
		return ;
	    }
	    /*
	    if(typeid(*initTy) == typeid(type::NilTy)) {
	        if(typeid(*type) != typeid(type::RecordTy)) {
		    errormsg->Error(pos_, "the nil init exp must have a record type declared!");
		    return ;
		}
		venv->Enter(var_, new env::VarEntry(type));
		//return ;
	    }
	    */
	    else {
		if(!initTy->IsSameType(type)) {
		    errormsg->Error(pos_, "type mismatch");
		    return ;
		}
		venv->Enter(var_, new env::VarEntry(tenv->Look(typ_)));
		//return ;
	    }
	    //venv->Enter(var_, new env::VarEntry(type));
	}
	return ;
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	std::list<NameAndTy*> list = types_->GetList();
	if(list.size() == 0) return ;
	std::list<NameAndTy*>::iterator it = list.begin();
	for(; it != list.end(); ++it) {
	    type::Ty* ty = tenv->Look((*it)->name_);
	    if(ty != NULL) {
	    	errormsg->Error(pos_, "two types have the same name");
		//continue;
	    } 
	    tenv->Enter((*it)->name_, new type::NameTy((*it)->name_, NULL));
	}
	it = list.begin();
	/*for(; it != list.end(); ++it) {
	    (*it)->ty_->SemAnalyze(tenv, labelcount, errormsg);  
	}*/
	/*TODO*/
	for(; it != list.end(); ++it) {
	    type::Ty* ty = tenv->Look((*it)->name_);
	    type::Ty* got = (*it)->ty_->SemAnalyze(tenv, errormsg);
	    type::NameTy* nameTy = static_cast<type::NameTy*>(ty);
	    nameTy->ty_ = got;
	    //tenv->Set((*it)->name_, new type::NameTy((*it)->name_, got));
	    //type::NameTy* new_type = static_cast<type::NameTy*>(got);
	    //tenv->Set((*it)->name_, new type::NameTy((*it)->name_, got));//set the value of the bindings in the second iteration.
	}
 	int flag = 0;//the flag is used to chekc whether the dec list has an illegal cycle.
	it = list.begin();
	for(; it != list.end(); ++it) {
	    //env::EnvEntry* entry = venv->Look((*it)->name_);
	    //type::Ty* start = static_cast<env::VarEntry*>(entry)->ty_;
	    type::Ty* start = tenv->Look((*it)->name_);
	    if(start ==NULL) return ;
	    /*
	    while(typeid(*start) == typeid(type::NameTy)) {
		if(flag == 1) break;
		type::NameTy* nameTy = static_cast<type::NameTy*>(start);
	        if((*it)->name_->Name() == nameTy->sym_->Name()) {
		   //errormsg->Error(pos_, "illegal type cycle");
		   flag = 1;
		   //break;
		}
		//env::EnvEntry* entry1 = venv->Look(static_cast<type::NameTy*>(start)->sym_);
		start = nameTy->ty_;
	    }
	    if(flag == 1) break;
	    */
	    if(typeid(*start) == typeid(type::NameTy)) {
	        type::Ty* start_ty = static_cast<type::NameTy*>(start)->ty_;
		while(typeid(*start_ty) == typeid(type::NameTy)) {
		    type::NameTy* name_ty = static_cast<type::NameTy*>(start_ty);
		    if((*it)->name_->Name() == name_ty->sym_->Name()) {
		        errormsg->Error(pos_, "illegal type cycle");
			flag = 1;
			break;
		    }
		    start_ty = name_ty->ty_;
		}
	    }
	    if(flag == 1) break;
	}
	//if(flag == 1) errormsg->Error(pos_, "illegal type cycle");
	return ;
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = tenv->Look(name_);
	if(ty == NULL) {
	    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
	    return type::IntTy::Instance();
	}
	return new type::NameTy(name_, ty);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::FieldList* fields = record_->MakeFieldList(tenv, errormsg);
	return new type::RecordTy(fields);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
	type::Ty* ty = tenv->Look(array_);
	if(ty == NULL) {
	    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
	    return type::IntTy::Instance();
	}
	return new type::ArrayTy(ty);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
