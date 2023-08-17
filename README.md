# SQLite-Page-Cache
Stores frequently accessed database pages in memory, evicting pages according to either the LRU or the LRU-2 page replacement policy.
<br>
<br>
<br>
```cpp
void setMaxNumPages(int maxNumPages)
```

Discard unpinned pages until either the number of pages in the cache is less than or equal to `maxNumPages` or all the pages in the cache are pinned. If there are still too many pages after discarding all unpinned pages, pages will continue to be discarded after being unpinned in the `unpinPage` function.

```cpp
Page *fetchPage(unsigned pageId, bool allocate)
```

- If the page is already in the cache, return a pointer to the page.
- If the page is not already in the cache, use the `allocate` parameter to determine how to proceed.
	- If `allocate` is false, return a null pointer.
	- If `allocate` is true, examine the number of pages in the cache.
		- If the number of pages in the cache is less than the maximum, allocate and return a pointer to a new page.
		- If the number of pages in the cache is greater than or equal to the maximum, try to replace a page.
			- If there is at least one unpinned page, return a pointer to an existing unpinned page as determined by the replacement policy.
			- If all pages are pinned, return a null pointer.

Increment `numFetches_`, and if the fetch was a hit, increment `numHits_`. If the fetch was a hit, this function executes in $O(1)$ time.

```cpp
void unpinPage(Page *page, bool discard)
```

The page is unpinned regardless of the number of prior fetches, meaning it can be safely discarded.

- If `discard` is true, then discard the page.
- If `discard` is false, examine the number of pages in the cache.
	- If the number of pages in the cache is greater than the maximum, discard the page.
	- If the number of pages in the cache is less than or equal to the maximum, do not discard the page.

This function executes in $O(1)$ time.

```cpp
void changePageId(Page *page, unsigned newPageId)
```

If a page with page ID `newPageId` is already in the cache, it is assumed that the page is unpinned, and the page is discarded.

```cpp
void discardPages(unsigned pageIdLimit)
```

If any of these pages are pinned, then they are implicitly unpinned, meaning they can be safely discarded.
