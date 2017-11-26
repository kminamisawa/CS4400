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
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- BLOCK_HEADER))

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

void *current_avail = NULL;
int current_avail_size = 0;
void* current_block = NULL;

int count_extend = 0;
page* first_pg = NULL;
page* last_pg = NULL;

void* first_block_payload;

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
  current_avail = NULL;
  current_avail_size = 0;

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

  printf("%s\n","Initialize attempt begins.");
  mm_init_helper();
  printf("%s\n","Initialize sucess!");
  // printf("%s %ld\n", "Teminator size ", (long) GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))));
  return 0;
}

/*
Adding pages. Page 85.
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
  // int pgsz_mult;
  //
  // if (extend_count >> 3 < 1){ //divide by 8
  //   pgsz_mult = 1;
  // }else{
  //   pgsz_mult = extend_count >> 3;
  // }
  // extend_count++;


  //int pgsz_mult = 8 * (extend_count/8) < 1 ? 1 : (extend_count/8);
  //extend_count += 1;
  //printf("ec:%d\n",extend_count);

  // int clampedSize = new_size > (pgsz_mult * mem_pagesize()) ? new_size : pgsz_mult * mem_pagesize();
  size_t calculate_new_size = new_size + GAP;
  size_t chunk_size = PAGE_ALIGN(calculate_new_size * 8); //PAGE_ALIGN(clampedSize * 4);
  void *new_page = mem_map(chunk_size);

  //Find pageList end.
  page *current_pg = first_pg;
  while(current_pg->next != NULL)
  {
   current_pg = current_pg->next;
  }

  //Hookup new page into pageList.
  current_pg->next = new_page;
  // current_pg->next->next = NULL;
  page* next_page = current_pg->next;
  next_page->next = NULL;
  next_page->prev = current_pg;
  next_page->size = chunk_size;
  last_pg = next_page;
  // NEXT_PAGE(NEXT_PAGE(current_pg)) = NULL;
  // PREV_PAGE(NEXT_PAGE(current_pg)) = current_pg;
  // PAGE_SIZE(NEXT_PAGE(current_pg)) = chunk_size;

  // last_current_pg = NEXT_PAGE(current_pg);

  void *new_bp = new_page + PG_SIZE + BLOCK_HEADER;

  // current_block = new_bp;

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
  // int newsize = ALIGN(size);
  // void *p;
  //
  // if (current_avail_size < newsize) {
  //   current_avail_size = PAGE_ALIGN(newsize);
  //   current_avail = mem_map(current_avail_size);
  //   if (current_avail == NULL)
  //     return NULL;
  // }
  //
  // p = current_avail;
  // current_avail += newsize;
  // current_avail_size -= newsize;
  //
  // printf("%s %d\n", "mm_malloc called", (int) current_avail);
//  return p;
  if(size == 0)
  return NULL;

  size_t new_size = ALIGN(size + OVERHEAD);
  void *current_pg = first_pg;
  void *new_bp;

  // pg = current_avail == NULL ? first_pg : current_avail;
  new_bp = current_pg + PG_SIZE + OVERHEAD + BLOCK_HEADER;


// while (GET_SIZE(HDRP(new_bp)) != 0)
//  {
//    if (!GET_ALLOC(HDRP(new_bp)) && (GET_SIZE(HDRP(new_bp)) >= new_size))
//      {
//  set_allocated(new_bp, new_size);
//  return new_bp;
//      }
//    new_bp = NEXT_BLKP(new_bp);
//  }

  // current_pg = first_pg;
  while(current_pg != NULL)
  {
   //if(current_pg != last_page_inserted)
    {
     new_bp = current_pg + PG_SIZE + OVERHEAD + BLOCK_HEADER;
     while (GET_SIZE(HDRP(new_bp)) != 0)
     {
      if (!GET_ALLOC(HDRP(new_bp)) && (GET_SIZE(HDRP(new_bp)) >= new_size))
      {
        set_allocated(new_bp, new_size);
        current_avail = current_pg;
        return new_bp;
      }
      new_bp = NEXT_BLKP(new_bp);
     }
    }
    current_pg = NEXT_PAGE(current_pg);
  }

  new_bp = extend(new_size);
  set_allocated(new_bp, new_size);
  return new_bp;
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
    //printf("%s\n", "mm_free reached" );
    // printf("%s\n", "mm_free called");
    GET_ALLOC(HDRP(ptr)) = 0;
    //printf("%s\n", "mm_free called");
    // GET_ALLOC(FTRP(ptr)) = 0;
    // // printf("%s\n", "mm_free called");
    // coalesce(ptr);

    // void *prev = HDRP(PREV_BLKP(ptr));
    // void *next = HDRP(NEXT_BLKP(ptr));
    //
    // size_t unmap_size = PAGE_ALIGN(GET_SIZE(HDRP(ptr)) + OVERHEAD + BLOCK_HEADER + PG_SIZE);
    // void * page_start = ptr - OVERHEAD - BLOCK_HEADER - PG_SIZE;
    // //printf("unmap_size :%ld\n",unmap_size);
    //
    // //Terminator block is next. Prologue block is prev.
    // if(GET_SIZE(next) == 0 && GET_SIZE(prev) == OVERHEAD)
    // {
    //   //Un hook page from page linked list.
    //   void *pg = first_pg;
    //   while(pg != page_start)
    //   {
    //     pg = NEXT_PAGE(pg);
    //   }
    //
    //   //page to remove is page linked list head
    //   if(pg == first_pg)
    //   {
    //     //About to remove first page.
    //     if(NEXT_PAGE(first_pg) == NULL && PREV_PAGE(first_pg) == NULL)
    //       return;
    //
    //     PREV_PAGE(NEXT_PAGE(first_pg)) = NULL;
    //     first_pg = current_avail =  NEXT_PAGE(first_pg);
    //   }
    //   else if(NEXT_PAGE(pg) != NULL && PREV_PAGE(pg) != NULL)
    //   {
    //     PREV_PAGE(NEXT_PAGE(pg)) = PREV_PAGE(pg);
    //     NEXT_PAGE(PREV_PAGE(pg)) = NEXT_PAGE(pg);
    //   }
    //   else
    //   {
    //     NEXT_PAGE(PREV_PAGE(pg)) = NULL;
    //   }
    //
    //   last_pg = NULL;
    //   current_block = NULL;
    //
    //   mem_unmap(page_start,unmap_size);
    // }
    //     printf("%s\n", "mm_free called");
}

/* Check the allignment of the blcok*/
bool check_allignment (void* bp){
  // printf("BP is %ld\n", (unsigned long) bp);
  if (((unsigned long) bp & (ALIGNMENT - 1)) != 0){
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
  void* current_pg = first_pg;
  void *current_bp;

  while (current_pg != NULL){
    if(ptr_is_mapped(current_pg, mem_pagesize()) == false ||
      ptr_is_mapped(current_pg, PAGE_SIZE(current_pg)) == false){
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
      return 0;
    }

    current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    // Iterate through the blocks now. Slide pg 33.
    while (GET_SIZE(HDRP(current_bp)) != 0){
      // Make sure header of the next block is properly allocated.
      if (!ptr_is_mapped(current_bp - BLOCK_HEADER, BLOCK_HEADER)){
        return 0;
      }

      // Check allignment of the block.
      if (!check_allignment(current_bp)){
        return 0;
      }

      // Check the allignment of the block at footer.
      if (!check_allignment(FTRP(current_bp))){
        return 0;
      }

      // Size of the header and footer must match in this implementation.
      if (GET_SIZE(HDRP(current_bp)) != GET_SIZE(current_bp)){
        return 0;
      }

      // Make sure colleace happened.
      if (GET_ALLOC(HDRP(PREV_BLKP(current_bp))) == 0 && GET_ALLOC(HDRP(current_bp)) == 0){
        return 0;
      }

      // Check proper amount of space is allocated for header.
      if (!ptr_is_mapped(HDRP(current_bp), GET_SIZE(HDRP(current_bp)))){
        return 0;
      }

      // Check proper amount of space is allocated for footer.
      if (!ptr_is_mapped(FTRP(current_bp), GET_SIZE(FTRP(current_bp)))){
        return 0;
      }

      // Make sure block is not too small nor too big.
      if (GET_SIZE(HDRP(current_bp)) < 3 * BLOCK_HEADER ||
        GET_SIZE(HDRP(current_bp)) > (size_t) MAX_BLOCK_SIZE){
        return 0;
      }

      // Make sure block is not too small nor too big.
      if (GET_SIZE(FTRP(current_bp)) < 3 * BLOCK_HEADER ||
        GET_SIZE(FTRP(current_bp)) > (size_t) MAX_BLOCK_SIZE){
        return 0;
      }

      // Make sure header and footer has the same size, which it should be.
      if (GET_SIZE(HDRP(current_bp)) != GET_SIZE(FTRP(current_bp))){
        return 0;
      }

      // Allocation must be either 0 or 1. (header)
      if (GET_ALLOC(HDRP(current_bp)) != 0 && GET_ALLOC(HDRP(current_bp)) != 0){
        return 0;
      }

      // Allocation must be either 0 or 1. (footer)
      if (GET_ALLOC(FTRP(current_bp)) != 0 && GET_ALLOC(FTRP(current_bp)) != 0){
        return 0;
      }

      // Allocation must match for footer and header
      if (GET_ALLOC(HDRP(current_bp)) == 0 && GET_ALLOC(FTRP(current_bp)) != 0){
        return 0;
      }

      // Allocation must match for footer and header
      if (GET_ALLOC(HDRP(current_bp)) == 1 && GET_ALLOC(FTRP(current_bp)) != 1){
        return 0;
      }

      current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    }
    page* next_page = (page*) current_pg;
    next_page = next_page->next;
    current_pg = next_page;
  }
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return 1;
}
