#ifndef CS564_PROJECT_PAGE_CACHE_LRU_HPP
#define CS564_PROJECT_PAGE_CACHE_LRU_HPP

#include "page_cache.hpp"

#include <unordered_map>

class LRUReplacementPageCache : public PageCache {
public:
  LRUReplacementPageCache(int pageSize, int extraSize);

  ~LRUReplacementPageCache() override;

  void setMaxNumPages(int maxNumPages) override;

  [[nodiscard]] int getNumPages() const override;

  Page *fetchPage(unsigned pageId, bool allocate) override;

  void unpinPage(Page *page, bool discard) override;

  void changePageId(Page *page, unsigned newPageId) override;

  void discardPages(unsigned pageIdLimit) override;

private:
  struct LRUReplacementPage : public Page {
    LRUReplacementPage(int pageSize, int extraSize, unsigned pageId,
                       bool pinned, int sequenceNumber);

    unsigned pageId;
    bool pinned;
    unsigned sequenceNumber;
  };

  std::unordered_map<unsigned, LRUReplacementPage *> cachedPages;
};

#endif
