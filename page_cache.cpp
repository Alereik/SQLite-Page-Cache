#include "page_cache.hpp"

#include <cstdint>
#include <cstdlib>

Page::Page(int pageSize, int extraSize) : sqlite3_pcache_page() {
  pBufInner_ = calloc(pageSize + 7, 1);
  pBuf = (void *)(((uintptr_t)pBufInner_ + 7) & ~(uintptr_t)0x3);
  pExtra = calloc(extraSize, 1);
}

Page::~Page() {
  free(pBufInner_);
  free(pExtra);
}

PageCache::PageCache(int pageSize, int extraSize)
    : pageSize_(pageSize), extraSize_(extraSize), maxNumPages_(0),
      numFetches_(0), numHits_(0) {}

unsigned long long PageCache::getNumFetches() const { return numFetches_; }

unsigned long long PageCache::getNumHits() const { return numHits_; }
