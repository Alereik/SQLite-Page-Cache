#ifndef CS564_PROJECT_PAGE_CACHE_HPP
#define CS564_PROJECT_PAGE_CACHE_HPP

#include "dependencies/sqlite/sqlite3.h"

class Page : sqlite3_pcache_page {
public:
  /**
   * Construct a Page.
   * @param pageSize Size in bytes of the page.
   * @param extraSize Size in bytes of the buffer to store extra information.
   */
  Page(int pageSize, int extraSize);

  Page(const Page &) = delete;
  Page &operator=(const Page &) = delete;

  ~Page();

private:
  void *pBufInner_;
};

class PageCache {
public:
  /**
   * Construct a PageCache.
   * @param pageSize Page size in bytes. Assumed to be a power of two.
   * @param extraSize Extra space in bytes. Assumed to be less than 250.
   */
  PageCache(int pageSize, int extraSize);

  /**
   * Destroy the PageCache.
   */
  virtual ~PageCache() = default;

  /**
   * Set the maximum number of pages in the cache. Discard unpinned pages until
   * either the number of pages in the cache is less than or equal to
   * `maxNumPages` or all the pages in the cache are pinned. If there are still
   * too many pages after discarding all unpinned pages, pages will continue to
   * be discarded after being unpinned in the `unpinPage` function.
   * @param maxNumPages Maximum number of pages in the cache.
   */
  virtual void setMaxNumPages(int maxNumPages) = 0;

  /**
   * Get the number of pages in the cache, both pinned and unpinned.
   * @return Number of pages in the cache.
   */
  [[nodiscard]] virtual int getNumPages() const = 0;

  /**
   * Fetch and pin a page. If the page is already in the cache, return a
   * pointer to the page. If the page is not already in the cache, use the
   * `allocate` parameter to determine how to proceed. If `allocate` is false,
   * return a null pointer. If `allocate` is true, examine the number of pages
   * in the cache. If the number of pages in the cache is less than the maximum,
   * allocate and return a pointer to a new page. If the number of pages in the
   * cache is greater than or equal to the maximum, return a pointer to an
   * existing unpinned page. If all pages are pinned, return a null pointer.
   * @param pageId Page ID.
   * @param allocate Allocate new page on miss.
   * @param hit True if the request was a hit and false otherwise.
   * @return Pointer to a page. May be null.
   */
  virtual Page *fetchPage(unsigned pageId, bool allocate) = 0;

  /**
   * Unpin a page. The page is unpinned regardless of the number of prior
   * fetches, meaning it can be safely discarded. If `discard` is true, discard
   * the page. If `discard` is false, examine the number of pages in the cache.
   * If the number of pages in the cache is greater than the maximum, discard
   * the page.
   * @param page Pointer to a page.
   * @param discard Discard the page.
   */
  virtual void unpinPage(Page *page, bool discard) = 0;

  /**
   * Change the page ID associated with a page. If a page with page ID
   * `newPageId` is already in the cache, it is assumed that the page is
   * unpinned, and the page is discarded.
   * @param page Pointer to a page.
   * @param newPageId New page ID.
   */
  virtual void changePageId(Page *page, unsigned newPageId) = 0;

  /**
   * Discard all pages with page IDs greater than or equal to `pageIdLimit`. If
   * any of these pages are pinned, then they are implicitly unpinned, meaning
   * they can be safely discarded.
   * @param pageIdLimit Page ID limit.
   */
  virtual void discardPages(unsigned pageIdLimit) = 0;

  /**
   * Get the number of fetches since creation.
   * @return Number of fetches since creation.
   */
  [[nodiscard]] unsigned long long getNumFetches() const;

  /**
   * Get the number of hits since creation.
   * @return Number of hits since creation.
   */
  [[nodiscard]] unsigned long long getNumHits() const;

protected:
  /** Maximum number of pages in the cache. */
  int maxNumPages_;

  /** Size in bytes of a page. */
  int pageSize_;

  /** Size in bytes of the buffer to store extra information. */
  int extraSize_;

  /** Number of fetches since creation. */
  unsigned long long numFetches_;

  /** Number of hits since creation. */
  unsigned long long numHits_;
};

template <typename PageCacheImplementation>
struct PageCacheMethods : sqlite3_pcache_methods2 {
  explicit PageCacheMethods() : sqlite3_pcache_methods2() {
    xInit = [](void *) { return SQLITE_OK; };

    xShutdown = nullptr;

    xCreate = [](int pageSize, int extraSize, int) {
      return (sqlite3_pcache *)new PageCacheImplementation(pageSize, extraSize);
    };

    xCachesize = [](sqlite3_pcache *pageCacheBase, int maxNumPages) {
      auto pageCache = (PageCache *)pageCacheBase;
      pageCache->setMaxNumPages(maxNumPages);
    };

    xPagecount = [](sqlite3_pcache *pageCacheBase) {
      auto pageCache = (PageCache *)pageCacheBase;
      return pageCache->getNumPages();
    };

    xFetch = [](sqlite3_pcache *pageCacheBase, unsigned pageId,
                int createFlag) {
      auto pageCache = (PageCache *)pageCacheBase;
      return (sqlite3_pcache_page *)pageCache->fetchPage(pageId, createFlag);
    };

    xUnpin = [](sqlite3_pcache *pageCacheBase, sqlite3_pcache_page *pageBase,
                int discard) {
      auto pageCache = (PageCache *)pageCacheBase;
      auto page = (Page *)pageBase;
      pageCache->unpinPage(page, discard);
    };

    xRekey = [](sqlite3_pcache *pageCacheBase, sqlite3_pcache_page *pageBase,
                unsigned, unsigned newPageId) {
      auto pageCache = (PageCache *)pageCacheBase;
      auto page = (Page *)pageBase;
      pageCache->changePageId(page, newPageId);
    };

    xTruncate = [](sqlite3_pcache *pageCacheBase, unsigned pageIdLimit) {
      auto pageCache = (PageCache *)pageCacheBase;
      pageCache->discardPages(pageIdLimit);
    };

    xDestroy = [](sqlite3_pcache *pageCacheBase) {
      auto pageCache = (PageCache *)pageCacheBase;
      delete pageCache;
    };
  }
};

#endif
