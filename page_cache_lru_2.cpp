#include "page_cache_lru_2.hpp"
#include "utilities/exception.hpp"

// Order of unpinning, incremented after each unpinning
int sequenceNumber = 0;

/**
 * Consruct an LRU-2 replacement policy Page.
 * @param argPageSize - Page size in bytes. Assumed to be a power of two.
 * @param argExtraSize - Extra space in bytes. Assumed to be less than 250.
 * @param argPageId - Page ID.
 * @param argPinned - The page's pin status.
 */
LRU2ReplacementPageCache::LRU2ReplacementPage::LRU2ReplacementPage(
    int argPageSize, int argExtraSize, unsigned argPageId, bool argPinned)
    : Page(argPageSize, argExtraSize), pageId(argPageId), pinned(argPinned),
    sequenceNums() {}

/**
 * Construct a PageCache.
 * @param pageSize - Page size in bytes. Assumed to be a power of two.
 * @param extraSize - Extra space in bytes. Assumed to be less than 250.
 */
LRU2ReplacementPageCache::LRU2ReplacementPageCache(int pageSize, int extraSize)
    : PageCache(pageSize, extraSize) {}

/**
 * Destructor of PageCache.
 */
LRU2ReplacementPageCache::~LRU2ReplacementPageCache() {
  for (auto iterator = cachedPages.begin(); iterator != cachedPages.end();
       ++iterator) {

    delete iterator->second;
  }
  cachedPages.clear();
}

/**
 * Set the maximum number of pages in the cache. Discard unpinned pages until
 * either the number of pages in the cache is less than or equal to
 * `maxNumPages` or all the pages in the cache are pinned. If there are still
 * too many pages after discarding all unpinned pages, pages will continue to
 * be discarded after being unpinned in the `unpinPage` function.
 * @param maxNumPages - Maximum number of pages in the cache.
 */
void LRU2ReplacementPageCache::setMaxNumPages(int maxNumPages) {
  maxNumPages_ = maxNumPages;
  // Discard unpinned pages until the number of pages in the cache is less than
  // or equal to `maxNumPages_` or only pinned pages remain.
  for (auto iterator = cachedPages.begin(); iterator != cachedPages.end();) {

    if (getNumPages() <= maxNumPages) {
      break;
    }

    if (!iterator->second->pinned) {
      delete iterator->second;
      iterator = cachedPages.erase(iterator);
    }
    else {
      ++iterator;
    }
  }
}

/**
 * Get the number of pages in the cache, both pinned and unpinned.
 * @return Number of pages in the cache.
 */
int LRU2ReplacementPageCache::getNumPages() const {
  return (int)cachedPages.size();
}

/**
 * Fetch and pin a page. If the page is already in the cache, return a
 * pointer to the page. If the page is not already in the cache, use the
 * `allocate` parameter to determine how to proceed. If `allocate` is false,
 * return a null pointer. If `allocate` is true, examine the number of pages
 * in the cache. If the number of pages in the cache is less than the maximum,
 * allocate and return a pointer to a new page. If the number of pages in the
 * cache is greater than or equal to the maximum, return a pointer to an
 * existing unpinned page. If all pages are pinned, return a null pointer.
 * @param pageId - Page ID.
 * @param allocate - Allocate new page on miss.
 * @return Pointer to a page. May be null.
 */
Page *LRU2ReplacementPageCache::fetchPage(unsigned pageId, bool allocate) {
  ++numFetches_;
  auto iterator = cachedPages.find(pageId);
  // Page already in cache
  if (iterator != cachedPages.end()) {
    iterator->second->pinned = true;
    ++numHits_;
    return iterator->second;
  }
  // Page not already in cache, check 'allocate' value
  else {
    // 'allocate' true
    if (allocate) {
      // Number of pages < maximum
      if (getNumPages() < maxNumPages_) {
        auto newPage = new LRU2ReplacementPage(pageSize_, extraSize_, pageId,
            true);

        cachedPages.emplace(pageId, newPage);
        return newPage;
      }
      // Number of pages >= maximum
      else {
        // Search for unpinned replacement.
        unsigned numUnpinned = 0;
        unsigned numOneAccess = 0;
        unsigned min = sequenceNumber;
        LRU2ReplacementPage *replacement;
        // Check number of unpinned pages, unpinned pages with only one access
        for (auto iterator = cachedPages.begin();
             iterator != cachedPages.end(); ++iterator) {

          if (!iterator->second->pinned) {
            ++numUnpinned;
            if (iterator->second->sequenceNums.size() < 2) {
              ++numOneAccess;
            }
          }
        }

        if (numUnpinned == 0) {
          return nullptr;
        }
        // Single unpinned page with only one access.
        if (numOneAccess == 1) {
          for (auto iterator = cachedPages.begin();
               iterator != cachedPages.end(); ++iterator) {

            if (!iterator->second->pinned &&
                iterator->second->sequenceNums.size() < 2) {

              iterator->second->pinned = true;
              replacement = iterator->second;
              cachedPages.erase(iterator);
              cachedPages.emplace(pageId, replacement);
              return replacement;
            }
          }
        }
        // Multiple unpinned pages with only one access, return LRU'd.
        else if (numOneAccess >= 1) {
          for (auto iterator = cachedPages.begin();
            iterator != cachedPages.end(); ++iterator) {

            if (!iterator->second->pinned &&
                iterator->second->sequenceNums.size() < 2 &&
                iterator->second->sequenceNums.front() <= min) {

              min = iterator->second->sequenceNums.front();
              replacement = iterator->second;
            }
          }
          replacement->pinned = true;
          auto it = cachedPages.find(replacement->pageId);
          cachedPages.erase(it);
          cachedPages.emplace(pageId, replacement);
          return replacement;
        }
        // All pages >= two accesses, return LRU-2'd.
        else {
          for (auto iterator = cachedPages.begin();
            iterator != cachedPages.end(); ++iterator) {

            if (iterator->second->sequenceNums.front() <= min) {
              min = iterator->second->sequenceNums.front();
              replacement = iterator->second;
            }
          }
          replacement->pinned = true;
          auto it = cachedPages.find(replacement->pageId);
          cachedPages.erase(it);
          cachedPages.emplace(pageId, replacement);
          return replacement;
        }
      }
    }
    // 'allocate' false
    else {
      return nullptr;
    }
  }
  throw NotImplementedException("LRU2ReplacementPageCache::fetchPage");
}

/**
 * Unpin a page. The page is unpinned regardless of the number of prior
 * fetches, meaning it can be safely discarded. If `discard` is true, discard
 * the page. If `discard` is false, examine the number of pages in the cache.
 * If the number of pages in the cache is greater than the maximum, discard
 * the page.
 * @param page - Pointer to a page.
 * @param discard - Discard the page.
 */
void LRU2ReplacementPageCache::unpinPage(Page *page, bool discard) {
  auto *thisPage = (LRU2ReplacementPage *)page;
  // Discard page if 'discard' true or number of pages grater than maximum
  if (discard || getNumPages() > maxNumPages_) {
    cachedPages.erase(thisPage->pageId);
    delete thisPage;
  }
  // Unpin and add sequence number to queue
  else {
    thisPage->pinned = false;
    thisPage->sequenceNums.push(sequenceNumber);
    ++sequenceNumber;
    // Keep queue size <= 2
    if (thisPage->sequenceNums.size() > 2) {
      thisPage->sequenceNums.pop();
    }
  }
}

/**
 * Change the page ID associated with a page. If a page with page ID
 * `newPageId` is already in the cache, it is assumed that the page is
 * unpinned, and the page is discarded.
 * @param page - Pointer to a page.
 * @param newPageId - New page ID.
 */
void LRU2ReplacementPageCache::changePageId(Page *page, unsigned newPageId) {
  auto *thisPage = (LRU2ReplacementPage *)page;
  auto searchedPage = cachedPages.find(newPageId);
  // Page found, already in cache, having 'newPageId' as its page ID. Discard.
  if (searchedPage != cachedPages.end()) {
    cachedPages.erase(newPageId);
    delete searchedPage->second;
  }
  // Change page ID
  cachedPages.erase(thisPage->pageId);
  thisPage->pageId = newPageId;
  cachedPages.emplace(newPageId, thisPage);
}

/**
 * Discard all pages with page IDs greater than or equal to `pageIdLimit`. If
 * any of these pages are pinned, then they are implicitly unpinned, meaning
 * they can be safely discarded.
 * @param pageIdLimit - Page ID limit.
 */
void LRU2ReplacementPageCache::discardPages(unsigned pageIdLimit) {
  for (auto iterator = cachedPages.begin(); iterator != cachedPages.end();) {

    if (iterator->second->pageId >= pageIdLimit) {
      delete iterator->second;
      iterator = cachedPages.erase(iterator);
    }
    else {
      ++iterator;
    }
  }
}
