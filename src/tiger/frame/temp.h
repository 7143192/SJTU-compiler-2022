#ifndef TIGER_FRAME_TEMP_H_
#define TIGER_FRAME_TEMP_H_

#include "tiger/symbol/symbol.h"

#include <list>

namespace temp {

using Label = sym::Symbol;

class LabelFactory {
public:
  static Label *NewLabel();
  static Label *NamedLabel(std::string_view name);
  static std::string LabelString(Label *s);

private:
  int label_id_ = 0;
  static LabelFactory label_factory;
};

class Temp {
  friend class TempFactory;

public:
  bool is_pointer;//used to record whether this temp stores apointer to the heap.
  [[nodiscard]] int Int() const;

private:
  int num_;
  explicit Temp(int num) : num_(num) {}
};

class TempFactory {
public:
  static Temp *NewTemp();

private:
  int temp_id_ = 100;
  static TempFactory temp_factory;
};

class Map {
public:
  void Enter(Temp *t, std::string *s);
  std::string *Look(Temp *t);
  void DumpMap(FILE *out);

  static Map *Empty();
  static Map *Name();
  static Map *LayerMap(Map *over, Map *under);

private:
  tab::Table<Temp, std::string> *tab_;
  Map *under_;

  Map() : tab_(new tab::Table<Temp, std::string>()), under_(nullptr) {}
  Map(tab::Table<Temp, std::string> *tab, Map *under)
      : tab_(tab), under_(under) {}
};

class TempList {
public:
  explicit TempList(Temp *t) : temp_list_({t}) {}
  TempList(std::list<temp::Temp*> list):temp_list_(std::move(list)) {}
  TempList(std::initializer_list<Temp *> list) : temp_list_(list) {}
  TempList() = default;
  void Append(Temp *t) { temp_list_.push_back(t); }

  bool Check(TempList* list) {
      //const std::list<Temp*> list1 = list->GetList();
      if(temp_list_.size() != (list->GetList()).size()) return false;
      //for(auto it1 = temp_list_.begin(), it2 = list1.begin(); it1 != temp_list_.end(); it1++, it2++) {
      //    if((*it1) != (*it2)) return false;
      //}
      for(auto it1 : temp_list_) {
          if(list->Contain(it1) == false) return false;
      }
      for(auto it2 : (list->GetList())) {
          if(this->Contain(it2) == false) return false;
      }
      return true;
  }
  bool Contain(Temp* t) {
      for(auto t1 : temp_list_) {
          if(t == t1) return true;
      }
      return false;
  }
  TempList* Union(TempList* list) {
      //std::list<Temp*> list1 = temp_list_;
      //const std::list<Temp*> list1 = list->GetList();
      TempList* ans = new TempList();
      for(auto t : temp_list_) ans->Append(t);
      for(auto t : list->GetList()) {
          if(this->Contain(t) == false) ans->Append(t);
      }
      return ans;
  }
  TempList* Diff(TempList* list) {
      //std::list<Temp*> list1 = list->GetList();
      TempList* ans = new TempList();
      for(auto t : temp_list_) {
          if(list->Contain(t) == false) ans->Append(t);
      }
      return ans;
  }
  TempList* Intersect(TempList* list) {
      TempList* ans = new TempList();
      for(auto t : temp_list_) {
          if(list->Contain(t) == true) ans->Append(t);
      }
      return ans;
  }

  void Replace(Temp* old_, Temp* new_) {
      //replace the old temp in the list with the new_ temp.
      //used in the function RewriteProgram.
      std::list<temp::Temp*> ans;
      for(auto t : temp_list_) {
          if(t != old_) {
	      ans.push_back(t);
	  }
	  else ans.push_back(new_);
      }
      temp_list_ = ans;
      return ;
  }

  [[nodiscard]] Temp *NthTemp(int i) const;
  [[nodiscard]] const std::list<Temp *> &GetList() const { return temp_list_; }

private:
  std::list<Temp *> temp_list_;
};

} // namespace temp

#endif
