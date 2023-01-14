#include "straightline/slp.h"

#include <iostream>

using namespace std;

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  //printf("test!");
  //MaxArgs return the max arguments number of the exp
  int num1 = stm1->MaxArgs();
  int num2 = stm2->MaxArgs();
  return (num1 > num2) ? num1 : num2;
}

//CompoundStm may change the original table.
Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  Table* t1 = stm1->Interp(t);
  t = t1;
  Table* t2 = stm2->Interp(t);//parse the stms recursivly.
  t = t2;//remember to update the Table!!!Beacause the stm may contains new vals for some id.
  return t;//sequence??
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

//AssignStm will always change the Table.
Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *iat = exp->Interp(t);//interpret the exp to get the num and put the new syms into the tablt t.
  if(t != nullptr) t = t->Update(id, iat->i);//if table is not null, add the updated node to the top
  else t = new Table(id, iat->i, nullptr);//if the table is empty, create the new table
  return t;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  //return exps->MaxArgs();
  int args1 = exps->MaxArgs();
  int args2 = exps->NumExps();
  if(args1 > args2) return args1;
  return args2;
}

//PrintStm may contains other exps, so it may change the original table.
Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *iat = exps->Interp(t);//recursive
  return iat->t;
}

int A::IdExp::MaxArgs() const {
    return 0;
}

int A::NumExp::MaxArgs() const {
    return 0;
}

IntAndTable *A::IdExp::Interp(Table* t) const {
    //initialized?
    //t = new Table(id, 0, t);
    //IntAndTable* iat = new IntAndTable(0, t);
    return new IntAndTable(t->Lookup(id), t);//what if NOT FOUND?
}

IntAndTable *A::NumExp::Interp(Table* t) const {
    IntAndTable* iat = new IntAndTable(num, t);//add the new node to the header of the table-list
    return iat;
}

int A::PairExpList::MaxArgs() const {
    //return (exp->MaxArgs() > tail->MaxArgs()) ? exp->MaxArgs() : tail->MaxArgs();
    int leftArgs = tail->MaxArgs();//get the tail's MaxArgs first.(recursive)
    return exp->MaxArgs() + leftArgs;
}

int A::PairExpList::NumExps() const {
    int nums = 1 + tail->NumExps();//recursive,1 means the first exp.
    return nums;    
}

IntAndTable *A::PairExpList::Interp(Table* t) const {
   IntAndTable* first = exp->Interp(t);//interpret the first exp in the list
   printf("%d", first->i);
   printf(" ");//remeber to print the parsed res!QAQ
   IntAndTable* res = tail->Interp(first->t);//recursively interpret the rest of the list
   return res;  
}

int A::LastExpList::MaxArgs() const {
    return exp->MaxArgs();
}

int A::LastExpList::NumExps() const {
    return 1;
}

IntAndTable *A::LastExpList::Interp(Table* t) const {
    IntAndTable* iat = exp->Interp(t);
    printf("%d\n", iat->i);//remember the \n!!!QAQ
    return iat;
}

int A::OpExp::MaxArgs() const {
    if(left->MaxArgs() > right->MaxArgs()) return left->MaxArgs();
    return right->MaxArgs();
}

IntAndTable *A::OpExp::Interp(Table* t) const {
    IntAndTable* iat1 = left->Interp(t);//interpret left part of the exp first
    t = iat1->t;
    IntAndTable* iat2 = right->Interp(t);//interpret right part of the exp next.
    t = iat2->t;//remember to UPDATE the table!!!QAQ
    if(oper == PLUS) {
    	return new IntAndTable(iat1->i + iat2->i, t);
    }
    if(oper == MINUS) return new IntAndTable(iat1->i - iat2->i, t);
    if(oper == TIMES) return new IntAndTable(iat1->i * iat2->i, t);
    if(oper == DIV) return new IntAndTable(iat1->i / iat2->i, t);
    return nullptr; //Illegal Operator.
}

int A::EseqExp::MaxArgs() const {
    if(stm->MaxArgs() > exp->MaxArgs()) return stm->MaxArgs();
    return exp->MaxArgs();
}

IntAndTable *A::EseqExp::Interp(Table* t) const {
    Table* iat1 = stm->Interp(t);
    return exp->Interp(iat1);
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
