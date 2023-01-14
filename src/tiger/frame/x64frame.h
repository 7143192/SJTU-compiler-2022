//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager():RegManager() {
      for(int i = 0; i < 16; ++i) {
          temp::Temp* t = temp::TempFactory::NewTemp();
	  regs_.push_back(t);
      }//add the abstract reg to the regs_ list.  
      temp_map_->Enter(regs_[0], new std::string("%rax"));
      temp_map_->Enter(regs_[1], new std::string("%rdi"));
      temp_map_->Enter(regs_[2], new std::string("%rsi"));
      temp_map_->Enter(regs_[3], new std::string("%rdx"));
      temp_map_->Enter(regs_[4], new std::string("%rcx"));
      temp_map_->Enter(regs_[5], new std::string("%r8"));
      temp_map_->Enter(regs_[6], new std::string("%r9"));
      temp_map_->Enter(regs_[7], new std::string("%r10"));
      temp_map_->Enter(regs_[8], new std::string("%r11"));
      temp_map_->Enter(regs_[9], new std::string("%rbx"));
      temp_map_->Enter(regs_[10], new std::string("%rbp"));
      temp_map_->Enter(regs_[11], new std::string("%r12"));
      temp_map_->Enter(regs_[12], new std::string("%r13"));
      temp_map_->Enter(regs_[13], new std::string("%r14"));
      temp_map_->Enter(regs_[14], new std::string("%r15"));
      temp_map_->Enter(regs_[15], new std::string("%rsp"));
      return ;
  }
  [[nodiscard]] temp::TempList *Registers() override {
      //return nullptr;
      temp::TempList* ans = new temp::TempList();
      for(int i = 0; i < 15; ++i) {
	  //if(i == 3) continue; //skip the %rsi
          temp::Temp* t = GetRegister(i);
	  ans->Append(t);
      }
      return ans; 
  }
  [[nodiscard]] temp::TempList *ArgRegs() override {
      temp::TempList* ans = new temp::TempList();
      ans->Append(GetRegister(1));
      ans->Append(GetRegister(2));
      ans->Append(GetRegister(3));
      ans->Append(GetRegister(4));
      ans->Append(GetRegister(5));
      ans->Append(GetRegister(6));
      //ans->Append(GetRegister(7));
      //return the argregs rdi,rsi,rdx,rcx,r8,r9
      return ans;
  }
  [[nodiscard]] temp::TempList *CallerSaves() override {
      temp::TempList* list = new temp::TempList();
      list->Append(GetRegister(0));//rax
      list->Append(GetRegister(1));//rdi
      list->Append(GetRegister(2));//rsi
      list->Append(GetRegister(3));//rdx
      list->Append(GetRegister(4));//rcx
      list->Append(GetRegister(5));//r8
      list->Append(GetRegister(6));//r9
      list->Append(GetRegister(7));//r10
      list->Append(GetRegister(8));//r11
      return list;
  }
  [[nodiscard]] temp::TempList *CalleeSaves() override {
      temp::TempList* list = new temp::TempList();
      list->Append(GetRegister(9));//rbx
      list->Append(GetRegister(10));//rbp
      list->Append(GetRegister(11));//r12
      list->Append(GetRegister(12));//r13
      list->Append(GetRegister(13));//r14
      list->Append(GetRegister(14));//r15
      return list;
  }
  [[nodiscard]] temp::TempList *ReturnSink() override {
      //SINK regs are thr regs that are alive at the exit point of the function.
      //callee-saved + rax + sp
      
      temp::TempList* temp_list = CalleeSaves();
      temp_list->Append(StackPointer());
      temp_list->Append(ReturnValue());
      return temp_list;
      /*
      temp::TempList* temp_list = new temp::TempList();
      temp_list->Append(ReturnValue());
      return temp_list;
      */
  }
  [[nodiscard]] int WordSize() override {
      return 8;
  }
  [[nodiscard]] temp::Temp *FramePointer() override {
      return GetRegister(10);//return %rbp
  }
  [[nodiscard]] temp::Temp *StackPointer() override {
      return GetRegister(15);//return %rsp
  }
  [[nodiscard]] temp::Temp *ReturnValue() override {
      return GetRegister(0);//return %rax
  }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
