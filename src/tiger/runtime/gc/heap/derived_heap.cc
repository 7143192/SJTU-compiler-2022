#include "heap.h"
#include <algorithm>
#include <map>
#include <vector>

namespace gc {
TigerHeap::freeIntervalInfo TigerHeap::FindFit(int size) {
  //auto iter = free_intervals.begin();
  freeIntervalInfo bestFit;
  for(auto iter = free_intervals.begin(); iter != free_intervals.end(); ++iter) {
    //the case that there is a block has the same size of the required.
    if ((*iter).intervalSize == size) {
      bestFit = (*iter);
      free_intervals.erase(iter);
      return bestFit;
    }
    //the case that bigger.
    if ((*iter).intervalSize > size) {
      bestFit.intervalStart = (*iter).intervalStart;
      bestFit.intervalSize = size;
      bestFit.intervalEnd = (*iter).intervalStart + size;
      freeIntervalInfo rest;
      rest.intervalStart = bestFit.intervalEnd;
      rest.intervalSize = (*iter).intervalSize - size;
      rest.intervalEnd = (*iter).intervalEnd;
      free_intervals.erase(iter);
      free_intervals.push_back(rest);
      //SortIntervalBySize();
      std::sort(free_intervals.begin(), free_intervals.end(), TigerHeap::compareIntervalBySize);
      return bestFit;
    }
    //iter++;
  }
}

//used to allocate a fittable space for the structure.
char *TigerHeap::Allocate(uint64_t size) {
  uint64_t max = MaxFree();
  if (size > max) {
    return nullptr;
  }
  freeIntervalInfo allocated = FindFit(size);
  return allocated.intervalStart;
}

//used to allocate a new record.
char *TigerHeap::AllocateRecord(uint64_t size, int des_size,
                                unsigned char *des_ptr) {
  //tigerStack = sp;
  GET_TIGER_STACK(tigerStack);
  char *record_begin = Allocate(size);
  if(record_begin == nullptr) {
      return nullptr;
  }
  recordInfo info;
  info.descriptor = des_ptr;
  info.descriptorSize = des_size;
  info.recordBeginPtr = record_begin;
  info.recordSize = size;
  recordsInHeap.push_back(info);
  return record_begin;
}

//used to allocate a new array.
char *TigerHeap::AllocateArray(uint64_t size) {
  //tigerStack = sp;
  GET_TIGER_STACK(tigerStack);
  char *array_begin = Allocate(size);
  if(array_begin == nullptr) {
      return nullptr;
  }
  arrayInfo info;
  info.arrayBeginPtr = array_begin;
  info.arraySize = size;
  arraiesInHeap.push_back(info);
  return array_begin;
}

uint64_t TigerHeap::Used() const {
  uint64_t used = 0;
  for(size_t i = 0; i < recordsInHeap.size(); ++i) {
      used += recordsInHeap[i].recordSize;
  }
  for(size_t i = 0; i < arraiesInHeap.size(); ++i) {
      used += arraiesInHeap[i].arraySize;
  }
  return used;
}

uint64_t TigerHeap::MaxFree() const {
  int maxFree = 0;
  for(const freeIntervalInfo interval : free_intervals) {
    if(interval.intervalSize > maxFree) {
	maxFree = interval.intervalSize;
    }
  }
  return maxFree;
}

void TigerHeap::Initialize(uint64_t size) {
  heap_root = (char *)malloc(size);
  freeIntervalInfo freeInterval;
  //the init free blocks should only contain the whole heap.
  freeInterval.intervalStart = heap_root;//store the start pos of the allocated heap.
  freeInterval.intervalSize = size;//store the init size.
  freeInterval.intervalEnd = heap_root + size;//NOTE: char* + 1 <--> move one byte <--> sizeof(char) = 1
  free_intervals.push_back(freeInterval);
  GetAllPointerMaps();//get all pointermaps generated in the compiling-time
}

void TigerHeap::Sweep(markResult bitMaps) {
  std::vector<arrayInfo> new_arraiesInHeap;
  for(int i = 0; i < bitMaps.arraiesActiveBitMap.size(); i++) {
    if(bitMaps.arraiesActiveBitMap[i]) {
      new_arraiesInHeap.push_back(arraiesInHeap[i]);
    }
    else {
      freeIntervalInfo freedInterval;
      freedInterval.intervalStart = (char *)arraiesInHeap[i].arrayBeginPtr;
      freedInterval.intervalSize = arraiesInHeap[i].arraySize;
      freedInterval.intervalEnd = (char *)arraiesInHeap[i].arrayBeginPtr + arraiesInHeap[i].arraySize;
      free_intervals.push_back(freedInterval);
    }
  }
  std::vector<recordInfo> new_recordsInHeap;
  for(int i = 0; i < bitMaps.recordsActiveBitMap.size(); i++) {
    if(bitMaps.recordsActiveBitMap[i]) {
      new_recordsInHeap.push_back(recordsInHeap[i]);
    }
    else {
      freeIntervalInfo freedInterval;
      freedInterval.intervalStart = (char *)recordsInHeap[i].recordBeginPtr;
      freedInterval.intervalSize = recordsInHeap[i].recordSize;
      freedInterval.intervalEnd = (char *)recordsInHeap[i].recordBeginPtr + recordsInHeap[i].recordSize;
      free_intervals.push_back(freedInterval);
    }
  }
  recordsInHeap = new_recordsInHeap;
  arraiesInHeap = new_arraiesInHeap;
  //Coalesce();
}

TigerHeap::markResult TigerHeap::Mark() {
  //std::vector<int> arraiesActiveBitMap(arraiesInHeap.size(), 0);
  std::vector<int> arraiesActiveBitMap;
  for(auto i = 0; i < arraiesInHeap.size(); ++i) {
      arraiesActiveBitMap.push_back(0);
  }
  //std::vector<int> recordsActiveBitMap(recordsInHeap.size(), 0);
  std::vector<int> recordsActiveBitMap;
  for(auto i = 0; i < recordsInHeap.size(); ++i) {
      recordsActiveBitMap.push_back(0);
  }
  std::vector<uint64_t> pointers = addressToMark();
  for (uint64_t pointer : pointers) {
    MarkAnPointer(pointer, arraiesActiveBitMap, recordsActiveBitMap);
  }
  markResult bitMaps;
  bitMaps.arraiesActiveBitMap = arraiesActiveBitMap;
  bitMaps.recordsActiveBitMap = recordsActiveBitMap;
  return bitMaps;
}

/*
inline void TigerHeap::ScanARecord(recordInfo record,
                                   std::vector<int> &arraiesActiveBitMap,
                                   std::vector<int> &recordsActiveBitMap) {
  long beginAddress = (long)record.recordBeginPtr;
  for (int i = 0; i < record.descriptorSize; i++) {
    if (record.descriptor[i] == '1') {
      uint64_t* targetAddress = ((uint64_t *)(beginAddress + WORD_SIZE * i));
      MarkAnAddress(*targetAddress, arraiesActiveBitMap, recordsActiveBitMap);
    }
  }
}
*/

void TigerHeap::MarkAnPointer(uint64_t address,
                              std::vector<int> &arraiesActiveBitMap,
                              std::vector<int> &recordsActiveBitMap) {
  int arraies_size = (int)arraiesActiveBitMap.size();
  int records_size = (int)recordsActiveBitMap.size();

  for (int i = 0; i < arraies_size; i++) {
    int off = address - (uint64_t)arraiesInHeap[i].arrayBeginPtr;
    if (off >= 0 && off <= (arraiesInHeap[i].arraySize - 8)) {
      arraiesActiveBitMap[i] = 1;
      return;
    }
  }
  int size = recordsInHeap.size();
  for (int i = 0; i < records_size; i++) {
    int64_t off = (int64_t)address - (int64_t)recordsInHeap[i].recordBeginPtr;
    if (off >= 0 && off <= (recordsInHeap[i].recordSize - 8)) {
      if (recordsActiveBitMap[i]) return;
      else {
        //ScanARecord(recordsInHeap[i], arraiesActiveBitMap, recordsActiveBitMap);
	long beginAddress = (long)recordsInHeap[i].recordBeginPtr;
        for(int i = 0; i < recordsInHeap[i].descriptorSize; i++) {
            if(recordsInHeap[i].descriptor[i] == '1') {
                uint64_t* target = ((uint64_t *)(beginAddress + WORD_SIZE * i));
		//mark the field pointer position recursively.
                MarkAnPointer(*target, arraiesActiveBitMap, recordsActiveBitMap);
            }
        }
        recordsActiveBitMap[i] = 1;
        return;
      }
    }
  }
}

void TigerHeap::GC() {
  GET_TIGER_STACK(tigerStack);
  markResult bitMaps = Mark();
  Sweep(bitMaps);
}

void TigerHeap::GetAllPointerMaps() {
  uint64_t *pointerMapStart = &GLOBAL_GC_ROOTS;
  uint64_t *iter = pointerMapStart;
  while (true) {
    uint64_t returnAddress = *iter;//get the ret_label_addr.
    iter++;
    uint64_t nextPointerMapAddress = *iter;//get the next pointer map label addr in the current pointer map.
    iter++;
    uint64_t frameSize = *iter;//get the frame size of the current frame size.
    iter++;
    uint64_t uppermost = *iter;//get whether current is the uppermost function's frame <--> tigermain
    //start to read the offsets stored in current pointer map.
    iter++;
    std::vector<int64_t> offsets;
    while (true) {
      int64_t offset = *iter;
      iter++;
      if (offset == -1) break;//this is the end label of all pointer maps.
      else offsets.push_back(offset);
    }
    pointerMaps.push_back(PointerMapBin(returnAddress, nextPointerMapAddress, frameSize, uppermost, offsets));
    if (nextPointerMapAddress == 0) break;
  }
}

std::vector<uint64_t> TigerHeap::addressToMark() {
  std::vector<uint64_t> pointers;
  uint64_t *sp = tigerStack;
  bool isMain = false;
  //start from the last frame in the stack,
  //and keep going upwards,
  //util reach the tigerMain.
  //in this process, get all the pointer
  //positions in the corresponding offset
  //in the corresponding frame.
  while(isMain == false) {
    uint64_t returnAddress = *(sp);
    for(const PointerMapBin &pointMap : pointerMaps) {
      if(pointMap.returnAddress == returnAddress) {
        for(int64_t offset : pointMap.offsets) {
          uint64_t *pointerAddress = (uint64_t *)(offset + (int64_t)(sp + 1) + (int64_t)pointMap.frameSize);
          pointers.push_back(*pointerAddress);
        }
        sp += (pointMap.frameSize / WORD_SIZE + 1);
        isMain = pointMap.isMain;
        break;
      }
    }
  }
  return pointers;
}

}  // namespace gc
