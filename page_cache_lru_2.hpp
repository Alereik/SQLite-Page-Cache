#ifndef CS564_PROJECT_PAGE_CACHE_LRU_2_HPP
#define CS564_PROJECT_PAGE_CACHE_LRU_2_HPP

#include "page_cache.hpp"

#include <unordered_map>
#include <queue>

class LRU2ReplacementPageCache : public PageCache {
public:
  LRU2ReplacementPageCache(int pageSize, int extraSize);

  ~LRU2ReplacementPageCache() override;

  void setMaxNumPages(int maxNumPages) override;

  [[nodiscard]] int getNumPages() const override;

  Page *fetchPage(unsigned pageId, bool allocate) override;

  void unpinPage(Page *page, bool discard) override;

  void changePageId(Page *page, unsigned newPageId) override;

  void discardPages(unsigned pageIdLimit) override;

private:
  struct LRU2ReplacementPage : public Page {
    LRU2ReplacementPage(int pageSize, int extraSize, unsigned pageId, bool pinned);

    unsigned pageId;
    bool pinned;
    std::queue<unsigned> sequenceNums;
  };

  std::unordered_map<unsigned, LRU2ReplacementPage*> cachedPages;
};

#endif
