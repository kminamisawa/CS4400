/*
 * mm-naive.c - The least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Use First-fit implementation. I tried the best-fit implementation,
 * but, it seems that it reduces the performance in our assignemnt.
 *
 * Mostly, I followed the strategy from the slide. It uses both hader and footer.
 *
 *
 *
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

/* rounds down to the nearest multiple of mem_pagesize() */
#define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

/* Slide page 77 */
#define BLOCK_HEADER (sizeof(block_header))
#define OVERHEAD (BLOCK_HEADER + sizeof(block_footer))
#define PG_SIZE (sizeof(page))
#define GAP (OVERHEAD+PG_SIZE+BLOCK_HEADER)

/* Slide page 28 from "More on memory Allocation". */
#define MAX_BLOCK_SIZE 1 << 32

/* Slide page 78 */
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-OVERHEAD)

/* Slide page 79 */
#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated // 1 = allocated. 0 is not.

/* Slide page 80 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))

/* Slide page 28 on More on memory allocation*/
#define PUT(p, value)  (*(int *)(p) = (value))
#define PACK(size, alloc)  ((size) | (alloc))

#define NEXT_PAGE(pg) (((page *)pg)->next)
#define PREV_PAGE(pg) (((page *)pg)->prev)
#define PAGE_SIZE(pg) (((page *)pg)->size)

typedef int bool;
#define true 1
#define false 0

int extend_count = 0;

/*From course website
  Make sure that the address range is on mapped pages.*/
int ptr_is_mapped(void *p, size_t len) {
   void *s = ADDRESS_PAGE_START(p);
   return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

typedef struct
{
  void *next;
  void *prev;
  size_t size;
  size_t filler;
} page;

typedef struct
{
  size_t size;
  char allocated;
} block_header;

typedef struct
{
  size_t size;
  char allocated;
} block_footer;

void *current_avail;
int current_avail_size;
void* current_block;

int count_extend;
page* first_pg;

void* first_block_payload;

/*From slide with a small modification considering the footer.*/
void set_allocated(void *bp, size_t size)
{
 size_t extra_size = GET_SIZE(HDRP(bp)) - size;

 if (extra_size > ALIGN(1 + OVERHEAD))
 {
   GET_SIZE(HDRP(bp)) = size;
   GET_SIZE(FTRP(bp)) = size;

   GET_SIZE(HDRP(NEXT_BLKP(bp))) = extra_size;
   GET_SIZE(FTRP(NEXT_BLKP(bp))) = extra_size;

   GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0;
   GET_ALLOC(FTRP(NEXT_BLKP(bp))) = 0;
 }

 GET_ALLOC(HDRP(bp)) = 1;
 GET_ALLOC(FTRP(bp)) = 1;
}

void mm_init_helper(){
  first_block_payload = (char*) first_pg + PG_SIZE + BLOCK_HEADER;
  current_block = first_block_payload;

  GET_SIZE(HDRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC(HDRP(first_block_payload)) = 1; //Set allocated.
  GET_SIZE(FTRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC(FTRP(first_block_payload)) = 1; //Set allocated.

  // long gap = OVERHEAD+PG_SIZE+BLOCK_HEADER;
  GET_SIZE(HDRP(NEXT_BLKP(first_block_payload))) = current_avail_size-GAP;
  GET_ALLOC(HDRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.
  GET_SIZE(FTRP(NEXT_BLKP(first_block_payload))) = current_avail_size-GAP;
  GET_ALLOC(FTRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.

  // Terminators
  GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))) = 1; //Set allocated.
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  first_pg = NULL;
  current_avail = NULL;
  current_avail_size = 0;
  current_block = NULL;
  extend_count = 0;

  // current_avail = NULL;
  // current_avail_size = 0;

  current_avail_size = PAGE_ALIGN(mem_pagesize() * 8); //4096*8

  first_pg = mem_map(current_avail_size);
  current_avail = first_pg;

  if(ptr_is_mapped(first_pg, current_avail_size) == 0)
  {
    return -1; //The return value should be -1 if there was a problem in performing the initialization
  }

  first_pg->next = NULL;
  first_pg->prev = NULL;
  first_pg->size = current_avail_size;

  // printf("%s\n","Initialize attempt begins.");
  mm_init_helper();
  // printf("%s\n","Initialize sucess!");
  return 0;
}

/*
 *  Adding pages. Page 85.
 */
void* extend (size_t new_size){
  // size_t chunk_size = CHUNK_ALLIGN(new_size);
  // void *bp = sbrk(chunk_size);
  //
  // GET_SIZE(HDRP(bp)) = chunk_size;
  // GET_ALLOC(HDRP(bp)) = 0;
  //
  // GET_SIZE(HDRP((NEXT_BLKP(bp)))) = 0;
  // GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 1;

  int pgsz_mult;

  if (extend_count >> 3 < 1){ //divide by 8
    pgsz_mult = 1;
  }else{
    pgsz_mult = extend_count >> 3;
  }
  printf("extend count: %d\nmem_heap() %ld\n", extend_count, mem_pagesize());
  extend_count++;

  int clampedSize = new_size > (pgsz_mult * mem_pagesize()) ? new_size : pgsz_mult * mem_pagesize();
  // size_t calculate_new_size = new_size + GAP;
  size_t chunk_size = PAGE_ALIGN(clampedSize * 8);
  void *new_page = mem_map(chunk_size);

  //Find pageList end.
  page *current_pg = first_pg;
  while(current_pg->next != NULL)
  {
   current_pg = current_pg->next;
  }

  current_pg->next = new_page;
  page* next_page = current_pg->next;
  next_page->next = NULL;
  next_page->prev = current_pg;
  next_page->size = chunk_size;
  current_avail = next_page;
  // NEXT_PAGE(NEXT_PAGE(current_pg)) = NULL;
  // PREV_PAGE(NEXT_PAGE(current_pg)) = current_pg;
  // PAGE_SIZE(NEXT_PAGE(current_pg)) = chunk_size;

  // last_current_pg = NEXT_PAGE(current_pg);

  void *new_bp = new_page + PG_SIZE + BLOCK_HEADER;

   current_block = new_bp;

  //create prologue for new page
  GET_SIZE(HDRP(new_bp)) = OVERHEAD;
  GET_ALLOC(HDRP(new_bp)) = 1;
  GET_SIZE(FTRP(new_bp)) = OVERHEAD;
  GET_ALLOC(FTRP(new_bp)) = 1;

  //new_bp = NEXT_BLKP(new_bp);
  //create unalocated block
  GET_SIZE(HDRP(NEXT_BLKP(new_bp))) = chunk_size - GAP;
  GET_SIZE(FTRP(NEXT_BLKP(new_bp))) = chunk_size - GAP;
  GET_ALLOC(HDRP(NEXT_BLKP(new_bp))) = 0;
  GET_ALLOC(FTRP(NEXT_BLKP(new_bp))) = 0;

  //new_bp = NEXT_BLKP(new_bp);
  //create terminator block
  GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(new_bp)))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(new_bp)))) = 1;

  return (new_page + GAP);

}

/*
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // First Fit Page 11
  size_t new_size = ALIGN(size + OVERHEAD);

  void *target_pg;
  if (current_avail != NULL){
    target_pg = current_avail;
  }else{
    target_pg = first_pg;
  }

  void *target_bp = target_pg + GAP;
  if (target_pg == current_avail){
    while (GET_SIZE(HDRP(target_bp)) != 0)
    {
      if (!GET_ALLOC(HDRP(target_bp)) && (GET_SIZE(HDRP(target_bp)) >= new_size))
       {
         set_allocated(target_bp, new_size);
         return target_bp;
       }
      target_bp = NEXT_BLKP(target_bp);
    }
    // Change the page if no available block is found on current page.
    target_pg = first_pg;
  }

  // It the spacde is not found on current page, iterate through the page,
  // and find an available block.
  while(target_pg != NULL)
  {
    target_bp = target_pg + GAP;
    while (GET_SIZE(HDRP(target_bp)) != 0)
    {
     if (!GET_ALLOC(HDRP(target_bp)) && (GET_SIZE(HDRP(target_bp)) >= new_size))
     {
       set_allocated(target_bp, new_size);
       current_avail = target_pg;
       return target_bp;
     }
     target_bp = NEXT_BLKP(target_bp);
    }
    target_pg = NEXT_PAGE(target_pg);
  }

  // Finally, grab new page if no available block was found.
  target_bp = extend(new_size);
  set_allocated(target_bp, new_size);
  return target_bp;

  /* Best-fit from page 12 on slides. Slow performance, around 40. */
  // size_t new_size = ALIGN(size + OVERHEAD);
  //
  // void *target_pg;
  // if (current_avail != NULL){
  //   target_pg = current_avail;
  // }else{
  //   target_pg = first_pg;
  // }
  //
  // void *target_bp = target_pg + GAP;
  // void *best_bp = NULL;
  //
  // while (GET_SIZE(HDRP(target_bp)) != 0)
  // {
  //   if (!GET_ALLOC(HDRP(target_bp)) && (GET_SIZE(HDRP(target_bp)) >= new_size))
  //    {
  //      if(!best_bp || (GET_SIZE(HDRP(target_bp)) < GET_SIZE(HDRP(best_bp)))){
  //        best_bp = target_bp;
  //      }
  //      // set_allocated(target_bp, new_size);
  //      // return target_bp;
  //    }
  //   target_bp = NEXT_BLKP(target_bp);
  // }
  //
  // if (best_bp){
  //   set_allocated(best_bp, new_size);
  //   return best_bp;
  // }
  //
  // target_pg = first_pg;
  // while(target_pg != NULL)
  // {
  //   target_bp = target_pg + GAP;
  //   while (GET_SIZE(HDRP(target_bp)) != 0)
  //   {
  //    if (!GET_ALLOC(HDRP(target_bp)) && (GET_SIZE(HDRP(target_bp)) >= new_size))
  //    {
  //      if(!best_bp || (GET_SIZE(HDRP(target_bp)) < GET_SIZE(HDRP(best_bp)))){
  //        best_bp = target_bp;
  //      }
  //      // set_allocated(target_bp, new_size);
  //      // current_avail = target_pg;
  //      // return target_bp;
  //    }
  //    target_bp = NEXT_BLKP(target_bp);
  //   }
  //   target_pg = NEXT_PAGE(target_pg);
  // }
  //
  // if (best_bp){
  //   set_allocated(best_bp, new_size);
  //   current_avail = target_pg;
  //   return best_bp;
  // }
  //
  //
  // target_bp = extend(new_size);
  // set_allocated(target_bp, new_size);
  // return target_bp;
}

/*
From Slide
TODO: USE explicit free list
*/
void *coalesce(void *bp)
{
 size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
 size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
 size_t size = GET_SIZE(HDRP(bp));

 if (prev_alloc && next_alloc)
   { /* Case 1 */
     /* nothing to do */

   }
 else if (prev_alloc && !next_alloc)
   { /* Case 2 */
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     GET_SIZE(HDRP(bp)) = size;
     GET_SIZE(FTRP(bp)) = size;
   }
 else if (!prev_alloc && next_alloc)
   { /* Case 3 */
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     GET_SIZE(FTRP(bp)) = size;
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
   }
 else
   { /* Case 4 */
     size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     GET_SIZE(FTRP(NEXT_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
   }

 return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // printf("%s\n", "mm_free called");
    GET_ALLOC(HDRP(ptr)) = 0;
    GET_ALLOC(FTRP(ptr)) = 0;
    coalesce(ptr);

    // printf("%s\n", "up tp cpalesce sucessed in mm_free");
    void *prev = HDRP(PREV_BLKP(ptr));
    void *next = HDRP(NEXT_BLKP(ptr));

    size_t unmap_size = PAGE_ALIGN(GET_SIZE(HDRP(ptr)) + OVERHEAD + BLOCK_HEADER + PG_SIZE);
    void * page_start = ptr - OVERHEAD - BLOCK_HEADER - PG_SIZE;
    // printf("%s\n", "up tp page_start in mm_free");
    //Terminator block is next. Prologue block is prev.
    if(GET_SIZE(next) == 0 && GET_SIZE(prev) == OVERHEAD)
    {
      //Un hook page from page linked list.
      void *pg = first_pg;
      // printf("%s\n", "up tp pg=first page in mm_free");
      while(pg != page_start)
      {
        pg = NEXT_PAGE(pg);
      }
      //page to remove is page linked list head
      if(pg == first_pg)
      {
        //About to remove first page.
        // printf("%s\n", "up tp pg == first_pg in mm_free");
        if(NEXT_PAGE(first_pg) == NULL && PREV_PAGE(first_pg) == NULL)
          return;

        PREV_PAGE(NEXT_PAGE(first_pg)) = NULL;
        first_pg = current_avail =  NEXT_PAGE(first_pg);
      }
      else if(NEXT_PAGE(pg) != NULL && PREV_PAGE(pg) != NULL)
      {
        PREV_PAGE(NEXT_PAGE(pg)) = PREV_PAGE(pg);
        NEXT_PAGE(PREV_PAGE(pg)) = NEXT_PAGE(pg);
      }
      else
      {
        NEXT_PAGE(PREV_PAGE(pg)) = NULL;
      }

      current_avail = NULL;
      current_block = NULL;

      mem_unmap(page_start,unmap_size);
    }
        // printf("%s\n", "mm_free called");
}

/* Check the allignment of the blcok
 * Return true if the allignment is proper. False otherwise.
*/
bool check_allignment (void* bp){
  // printf("BP is %ld\n", (unsigned long) bp);
  if (((unsigned long) bp & (ALIGNMENT - 1)) != 0){
    return false;
  }
  return true;
}

/*
 *  Check whther the size of the blcok is too samll nor too big
 *  Return true if the proper size is allocated. Otherwise, return false.
*/
bool check_block_size (void* bp){
  // Make sure block is not too small nor too big.
  if (GET_SIZE(HDRP(bp)) < 3 * BLOCK_HEADER ||
      GET_SIZE(HDRP(bp)) > (size_t) MAX_BLOCK_SIZE){
      return false;
  }

  // Make sure block is not too small nor too big.
  if (GET_SIZE(FTRP(bp)) < 3 * BLOCK_HEADER ||
      GET_SIZE(FTRP(bp)) > (size_t) MAX_BLOCK_SIZE){
      return false;
  }
  return true;
}

/*
 *  Check whther the allocation is properly set.
 *  Return true if the proper size is allocated. Otherwise, return false.
*/
bool check_allocation (void* bp){
  // Allocation must be either 0 or 1. (header)
  if (GET_ALLOC(HDRP(bp)) != 0 && GET_ALLOC(HDRP(bp)) != 1){
    return false;
  }

  // Allocation must be either 0 or 1. (footer)
  if (GET_ALLOC(FTRP(bp)) != 0 && GET_ALLOC(FTRP(bp)) != 1){
    return false;
  }

  // Allocation must match for footer and header
  if (GET_ALLOC(HDRP(bp)) == 0 && GET_ALLOC(FTRP(bp)) != 0){
    return false;
  }

  // Allocation must match for footer and header
  if (GET_ALLOC(HDRP(bp)) == 1 && GET_ALLOC(FTRP(bp)) != 1){
    return false;
  }

  return true;
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  // printf("%s\n", "mm_check called.");
  void* current_pg = first_pg;
  void *current_bp;
  while (current_pg != NULL){
    if(ptr_is_mapped(current_pg, mem_pagesize()) == false ||
      ptr_is_mapped(current_pg, PAGE_SIZE(current_pg)) == false){
      // printf("%s\n", "Called 1");
      return 0;
    }

    current_bp = current_pg + PG_SIZE + BLOCK_HEADER;

    bool check_bp_size = true;
    if (GET_SIZE(HDRP(current_bp)) != OVERHEAD){
      check_bp_size = false;
    }else if(GET_ALLOC(HDRP(current_bp)) != 1){
      check_bp_size = false;
    }else if (GET_SIZE(FTRP(current_bp)) != OVERHEAD){
      check_bp_size = false;
    }

    if(!check_bp_size){
      // printf("%s\n", "Called 2");
      return 0;
    }

    current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    // Iterate through the blocks now. Slide pg 33.
    // printf("%s\n", "mm_check second while loop.");
    while (GET_SIZE(HDRP(current_bp)) != 0){
      // Make sure header of the next block is properly allocated.
      if (!ptr_is_mapped(current_bp - BLOCK_HEADER, BLOCK_HEADER)){
        // printf("%s\n", "Called 3");
        return 0;
      }

      // Check allignment of the block.
      if (!check_allignment(current_bp)){
        // printf("%s\n", "Called 4");
        return 0;
      }

      // Check proper amount of space is allocated for header.
      if (!ptr_is_mapped(HDRP(current_bp), GET_SIZE(HDRP(current_bp)))){
        // printf("%s\n", "Called 8");
        return 0;
      }

      // Make sure block is not too small nor too big.
      if (!check_block_size(current_bp)){
        // printf("%s\n", "Called 10");
        return 0;
      }

      // Check the allignment of the block at footer.
      if (!check_allignment(FTRP(current_bp))){
        // printf("%s\n", "Called 5");
        return 0;
      }

      // Size of the header and footer must match in this implementation.
      if (GET_SIZE(HDRP(current_bp)) != GET_SIZE(FTRP(current_bp))){
        // printf("%s\n", "Called 6");
        return 0;
      }
      //printf("%s\n", "mm_check sucess so far.");
      //Make sure colleace happened.
      if (GET_ALLOC(HDRP(PREV_BLKP(current_bp))) == 0 && GET_ALLOC(HDRP(current_bp)) == 0){
        // printf("%s\n", "Called 7");
        return 0;
      }

      // Check proper amount of space is allocated for footer.
      if (!ptr_is_mapped(HDRP(current_bp), GET_SIZE(FTRP(current_bp)))){
        // printf("%s\n", "Called 9");
        return 0;
      }

      // Make sure header and footer has the same size, which it should be.
      if (GET_SIZE(HDRP(current_bp)) != GET_SIZE(FTRP(current_bp))){
        // printf("%s\n", "Called 11");
        return 0;
      }

      // Makse sure allocation is properly set.
      if (!check_allocation(current_bp)){
        // printf("%s\n", "Called 12");
        return false;
      }

      current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    }
    // printf("%s\n", "mm_check so far.");
    page* next_page = (page*) current_pg;
    next_page = next_page->next;
    current_pg = next_page;
  }
    // printf("%s\n", "mm_check sucess.");
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  // printf("%s\n", "mm_can_free called.");
  if(!check_allignment(p)){
    // printf("%s\n", "mm_can_free 1.");
    return 0;
  }

  if(!ptr_is_mapped(HDRP(p), BLOCK_HEADER)){
    // printf("%s\n", "mm_can_free 2.");
    return 0;
  }

  if(!ptr_is_mapped(HDRP(p), GET_SIZE(HDRP(p)))){
    // printf("%s\n", "mm_can_free 3.");
    return 0;
  }

  if(!ptr_is_mapped(HDRP(p), GET_SIZE(FTRP(p)))){
    // printf("%s\n", "mm_can_free 4.");
    return 0;
  }

  if(!check_block_size(p)){
    // printf("%s\n", "mm_can_free 5.");
    return 0;
  }

  // Make sure this is a currently allocated block.
  if(GET_ALLOC(HDRP(p)) != 1){
    // printf("%s\n", "mm_can_free 6.");
    return 0;
  }

  // Make sure the block is not prologue
  if(GET_SIZE(HDRP(p)) == OVERHEAD && GET_ALLOC(HDRP(p)) == 1){
    // printf("%s\n", "mm_can_free 7.");
    return 0;
  }

  // Make sure the block is not epilogue
  if(GET_SIZE(FTRP(p)) == OVERHEAD && GET_ALLOC(FTRP(p)) == 1){
    // printf("%s\n", "mm_can_free 8.");
    return 0;
  }

  // Make sure the block is not a terminator
  if(GET_SIZE(HDRP(p)) == 0 && GET_ALLOC(HDRP(p)) == 1){
    // printf("%s\n", "mm_can_free 9.");
    return 0;
  }

  // Allocation must match for footer and header
  if (GET_ALLOC(HDRP(p)) == 0 && GET_ALLOC(FTRP(p)) != 0){
    // printf("%s\n", "mm_can_free 10.");
    return false;
  }

  // Allocation must match for footer and header
  if (GET_ALLOC(HDRP(p)) == 1 && GET_ALLOC(FTRP(p)) != 1){
    // printf("%s\n", "mm_can_free 11.");
    return false;
  }

  // printf("%s\n", "mm_can_free sucess.");
  return 1;
}
