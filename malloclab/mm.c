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

#define NEXT_PAGE(pg) (((page *)pg)->next)
#define PREV_PAGE(pg) (((page *)pg)->prev)
#define PAGE_SIZE(pg) (((page *)pg)->size)
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

  first_block_payload = (char*) first_pg + PG_SIZE + BLOCK_HEADER;
  current_block = first_block_payload;

  GET_SIZE(HDRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC(HDRP(first_block_payload)) = 1; //Set allocated.
  GET_SIZE(FTRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC(FTRP(first_block_payload)) = 1; //Set allocated.

  long gap = OVERHEAD+PG_SIZE+BLOCK_HEADER;
  GET_SIZE(HDRP(NEXT_BLKP(first_block_payload))) = current_avail_size - gap;
  GET_ALLOC(HDRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.
  GET_SIZE(FTRP(NEXT_BLKP(first_block_payload))) = current_avail_size - gap;
  GET_ALLOC(FTRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.

  // Terminators
  GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))) = 1; //Set allocated.

//  printf("%s\n","initialize sucess");
//  printf("%s %ld\n", "Teminator size ", (long) GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(first_block_payload)))));
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
  int pgsz_mult;

  if (extend_count >> 3 < 1){ //divide by 8
    pgsz_mult = 1;
  }else{
    pgsz_mult = extend_count >> 3;
  }
  extend_count++;


  //int pgsz_mult = 8 * (extend_count/8) < 1 ? 1 : (extend_count/8);
  //extend_count += 1;
  //printf("ec:%d\n",extend_count);

  int clampedSize = new_size > (pgsz_mult * mem_pagesize()) ? new_size : pgsz_mult * mem_pagesize();
  size_t chunk_size = PAGE_ALIGN(clampedSize * 8); //PAGE_ALIGN(clampedSize * 4);
  void *new_page = mem_map(chunk_size);

  //Find pageList end.
  void *pg = first_pg;
  while(NEXT_PAGE(pg) != NULL)
  {
   pg = NEXT_PAGE(pg);
  }

  //Hookup new page into pageList.
  NEXT_PAGE(pg) = new_page;
  NEXT_PAGE(NEXT_PAGE(pg)) = NULL;
  PREV_PAGE(NEXT_PAGE(pg)) = pg;
  // PAGE_SIZE(NEXT_PAGE(pg)) = chunk_size;

  // last_pg = NEXT_PAGE(pg);

  void *pp = new_page + PG_SIZE + BLOCK_HEADER;

  // current_block = pp;

  //create prologue for new page
  GET_SIZE(HDRP(pp)) = OVERHEAD;
  GET_ALLOC(HDRP(pp)) = 1;
  GET_SIZE(FTRP(pp)) = OVERHEAD;
  GET_ALLOC(FTRP(pp)) = 1;

  pp = NEXT_BLKP(pp);
  //create unalocated block
  GET_SIZE(HDRP(pp)) = chunk_size - PG_SIZE - OVERHEAD - BLOCK_HEADER;
  GET_SIZE(FTRP(pp)) = chunk_size - PG_SIZE - OVERHEAD - BLOCK_HEADER;
  GET_ALLOC(HDRP(pp)) = 0;
  GET_ALLOC(FTRP(pp)) = 0;

  pp = NEXT_BLKP(pp);
  //create terminator block
  GET_SIZE(HDRP(pp)) = 0;
  GET_ALLOC(HDRP(pp)) = 1;

  return (new_page + PG_SIZE + OVERHEAD + BLOCK_HEADER);

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

int new_size = ALIGN(size + OVERHEAD);
void *pg = first_pg;
void *pp;

// pg = current_avail == NULL ? first_pg : current_avail;
pp = pg + PG_SIZE + OVERHEAD + BLOCK_HEADER;


// while (GET_SIZE(HDRP(pp)) != 0)
//  {
//    if (!GET_ALLOC(HDRP(pp)) && (GET_SIZE(HDRP(pp)) >= new_size))
//      {
//  set_allocated(pp, new_size);
//  return pp;
//      }
//    pp = NEXT_BLKP(pp);
//  }

pg = first_pg;
while(pg != NULL)
{
 //if(pg != last_page_inserted)
  {
   pp = pg + PG_SIZE + OVERHEAD + BLOCK_HEADER;
   while (GET_SIZE(HDRP(pp)) != 0)
   {
    if (!GET_ALLOC(HDRP(pp)) && (GET_SIZE(HDRP(pp)) >= new_size))
    {
      set_allocated(pp, new_size);
      current_avail = pg;
      return pp;
    }
    pp = NEXT_BLKP(pp);
   }
  }
  pg = NEXT_PAGE(pg);
}

pp = extend(new_size);
set_allocated(pp, new_size);
return pp;
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
//    printf("%s\n", "mm_free reached" );
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

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
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
