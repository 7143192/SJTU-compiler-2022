#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>
#include <list>
// Used to locate the start of ptrmap, simply get the address by &GLOBAL_GC_ROOTS
extern uint64_t GLOBAL_GC_ROOTS;

#define GET_RBP(rbp) do { __asm__("movq %%rbp, %0" : "=r"(rbp)); } while(0)

#define GET_RSP(rsp) do { __asm__("movq %%rsp, %0" : "=r"(rsp)); } while(0)

// Used to get the first tiger stack, define a sp like uint64_t *sp; and invoke GET_TIGER_STACK(sp) will work
#define GET_TIGER_STACK(sp) do {\
  unsigned long rbp; \
  GET_RBP(rbp); \ 
  sp = ((uint64_t*)((*(uint64_t*)rbp) + sizeof(uint64_t))); \
} while(0) 

#define GET_CUR_RSP(sp) do {\
  unsigned long rsp; \
  GET_RSP(rsp); \
  sp = ((uint64_t*)((*(uint64_t*)rsp) + sizeof(uint64_t))); \
} while(0)

namespace gc {

constexpr long END_MARK = 0;
/*
class TigerHeap {
public:
  char* virtual_heap;
  char* heap_mark;
  std::list<std::pair<uint64_t, uint64_t>> free_list;//used to record the free blocks of the heap.the pair is composed of <startPos, size of this block>.
  uint64_t max_size;//the max size of the heap.
  uint64_t* map_start;
  char* first_sp;
  virtual char *Allocate(uint64_t size) = 0;
  virtual uint64_t Used() const = 0;
  virtual uint64_t MaxFree() const = 0;
  virtual void Initialize(uint64_t size) = 0;
  virtual void GC() = 0;
  static constexpr uint64_t WORD_SIZE = 8;
};
*/

class TigerHeap {
 public:
  //used to maintain the info of a record in the virtual-heap.
  struct recordInfo {
    char *recordBeginPtr;
    int recordSize;
    int descriptorSize;
    unsigned char *descriptor;
  };

  //used to maintain the info of a array in the virtual-heap.
  struct arrayInfo {
    char *arrayBeginPtr;
    int arraySize;
  };

  //used to record the free-block info.
  struct freeIntervalInfo {
    char *intervalStart;
    uint64_t intervalSize;
    char *intervalEnd;
  };

  struct markResult {
    std::vector<int> arraiesActiveBitMap;
    std::vector<int> recordsActiveBitMap;
  };

  static inline bool compareIntervalBySize(freeIntervalInfo interval1,
                                           freeIntervalInfo interval2) {
    return interval1.intervalSize < interval2.intervalSize;
  }
  /*
  static inline bool compareIntervalByStartAddr(freeIntervalInfo interval1,
                                                freeIntervalInfo interval2) {
    return (uint64_t)interval1.intervalStart <
           (uint64_t)interval2.intervalStart;
  }
  */

  struct PointerMapBin {
    uint64_t returnAddress;
    uint64_t nextPointerMapAddress;
    uint64_t frameSize;
    uint64_t isMain;
    std::vector<int64_t> offsets;
    PointerMapBin(uint64_t returnAddress_, uint64_t nextPointerMapAddress_,
                  uint64_t frameSize_, uint64_t isMain_,
                  std::vector<int64_t> offsets_)
        : returnAddress(returnAddress_),
          nextPointerMapAddress(nextPointerMapAddress_),
          frameSize(frameSize_),
          isMain(isMain_),
          offsets(offsets_) {}
  };

  TigerHeap() = default;
  ~TigerHeap() = default;
  //inline void SortIntervalBySize();
  //inline void SortIntervalByStartAddr();
  //void Coalesce();
  freeIntervalInfo FindFit(int size);
  char *Allocate(uint64_t size);
  char *AllocateRecord(uint64_t size, int des_size, unsigned char *des_ptr);
  char *AllocateArray(uint64_t size);
  uint64_t Used() const;
  uint64_t MaxFree() const;
  void Initialize(uint64_t size);
  void Sweep(markResult bitMaps);
  markResult Mark();
  /*
  inline void ScanARecord(recordInfo record,
                          std::vector<int> &arraiesActiveBitMap,
                          std::vector<int> &recordsActiveBitMap);
  */
  void MarkAnPointer(uint64_t address, std::vector<int> &arraiesActiveBitMap,
                     std::vector<int> &recordsActiveBitMap);
  void GC();
  static constexpr uint64_t WORD_SIZE = 8;
  void GetAllPointerMaps();
  std::vector<uint64_t> addressToMark();
  char *heap_root;
  std::vector<recordInfo> recordsInHeap;
  std::vector<arrayInfo> arraiesInHeap;
  std::vector<freeIntervalInfo> free_intervals;
  std::vector<PointerMapBin> pointerMaps;
  uint64_t *tigerStack;
};

} // namespace gc

