#include <algorithm>
#include <array>
#include <map>
#include "cache.h"

int prefetch_distance=0;
int prefetch_degree=0;
struct lookahead_entry {
  uint64_t address = 0;
  int degree = 0; // degree remaining
};

int dirn=0;
uint64_t prev_addr=0;
std::map<CACHE*, lookahead_entry> lookahead;
void CACHE::prefetcher_initialize() { std::cout << NAME << " stream prefetcher" << std::endl; }

uint32_t CACHE::prefetcher_cache_operate(uint64_t addr, uint64_t ip, uint8_t cache_hit, uint8_t type, uint32_t metadata_in) { 
  uint64_t cl_addr = addr >> LOG2_BLOCK_SIZE;
  if(dirn>0 && cl_addr-prev_addr>0){
    dirn++;
    if(dirn>=2){
      lookahead[this]={cl_addr+prefetch_distance-1,prefetch_degree};
    }
  }else if(dirn<0 && cl_addr-prev_addr<0){
    dirn--;
    if(dirn>=2){
      lookahead[this]={cl_addr-prefetch_distance+1,prefetch_degree};
    }
  }else{
    dirn=0;
  }
  prev_addr=cl_addr;
  return metadata_in; 
}

uint32_t CACHE::prefetcher_cache_fill(uint64_t addr, uint32_t set, uint32_t way, uint8_t prefetch, uint64_t evicted_addr, uint32_t metadata_in)
{
  return metadata_in;
}

void CACHE::prefetcher_cycle_operate() {
    // If a lookahead is active
  if (auto [old_pf_address, degree] = lookahead[this]; degree > 0) {
    auto pf_address;
    if(dirn>=2){ 
      pf_address=lookahead[this]+1<<LOG2_BLOCK_SIZE;
    }else if(dirn<=-2){
      pf_address=lookahead[this]-1<<LOG2_BLOCK_SIZE;
    }else{
      return;
    }
    // If the next step would exceed the degree or run off the page, stop
    if (virtual_prefetch || (pf_address >> LOG2_PAGE_SIZE) == (old_pf_address >> LOG2_PAGE_SIZE)) {
      // check the MSHR occupancy to decide if we're going to prefetch to this
      // level or not
      bool success = prefetch_line(0, 0, pf_address, (get_occupancy(0, pf_address) < get_size(0, pf_address) / 2), 0);
      if (success)
        lookahead[this] = {pf_address, degree - 1};
      // If we fail, try again next cycle
    } else {
      lookahead[this] = {};
    }
  }
}

void CACHE::prefetcher_final_stats() {}
