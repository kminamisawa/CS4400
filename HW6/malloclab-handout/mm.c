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
 * I have tried to implement in explicit free list, but I had trouble solving
 * seagfault error in colleace, so it used the implicit free list.
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
#define GET_SIZE_F(p) ((block_footer *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated // 1 = allocated. 0 is not.
#define GET_ALLOC_F(p) ((block_footer *)(p))->allocated

/* Slide page 80 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))

/* Slide page 28 on More on memory allocation*/
#define PUT(p, value)  (*(int *)(p) = (value))
#define PACK(size, alloc)  ((size) | (alloc))

#define MAX(x ,y)  ((x) > (y) ? (x) : (y))

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
  struct page *prev;
  struct page *next;
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

typedef struct list_node {
 struct list_node *prev;
 struct list_node *next;
 void *bp;
} list_node;

void *current_avail;
int current_avail_size;
void* current_block;

int count_extend;
int free_count;
page* first_pg;


list_node* free_bp_head;
/* Modified from http://www.geeksforgeeks.org/delete-a-linked-list-node-at-a-given-position
 * to create a linked list.
 */
void add_to_free_list(list_node* bp){
  // list_node* new_node = NULL;
  // new_node->next = free_bp_head;
  // new_node->prev = NULL;
  // new_node->bp = bp;
  // printf("%s\n", "add free list success so far --- 2");
  bp->next = free_bp_head;
  bp->prev = NULL;

  if (free_bp_head != NULL){
    free_bp_head->prev = bp;
  }
  free_bp_head = bp;
  free_count++;
  // printf("%ld to %ld\n", (long)free_bp_head, (long)free_bp_head->next);
}

void remove_from_free_list(list_node* bp){
  int position = 0;

  while (free_bp_head->next != bp){
    position++;
  }
  list_node* temp = free_bp_head;
  if (position == 0){
    free_bp_head = temp->next;
    return;
  }
  int i;
  for (i = 0; (temp != NULL && i < position-1); i++){
    temp = temp->next;
  }

  if (temp == NULL || temp->next == NULL){
    return;
  }

  list_node* next = temp->next->next;
  temp->next = next;
  free_count--;
}

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
  void* first_block_payload = (char*) first_pg + PG_SIZE + BLOCK_HEADER;
  current_block = first_block_payload;
  add_to_free_list(current_block + GET_SIZE(HDRP(current_block)));

  // Prologue
  GET_SIZE(HDRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC(HDRP(first_block_payload)) = 1; //Set allocated.

  // payload
  GET_SIZE(HDRP(NEXT_BLKP(first_block_payload))) = current_avail_size-GAP;
  GET_ALLOC(HDRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.
  GET_SIZE_F(FTRP(NEXT_BLKP(first_block_payload))) = current_avail_size-GAP;
  GET_ALLOC_F(FTRP(NEXT_BLKP(first_block_payload))) = 0; //Set un-allocated.

  // free_bp_head->bp = NEXT_BLKP(first_block_payload);
  // free_bp_head->next = NULL;
  // free_bp_head->prev = NULL;
  // printf("%s\n", "Initializing free_bp_head success.");

  // Epilogue
  GET_SIZE_F(FTRP(first_block_payload)) = OVERHEAD;
  GET_ALLOC_F(FTRP(first_block_payload)) = 1; //Set allocated.

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
  free_bp_head = NULL;

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

void extend_helper(void* new_bp, size_t chunk_size){
  current_block = new_bp;

  //prologue
  GET_SIZE(HDRP(new_bp)) = OVERHEAD;
  GET_ALLOC(HDRP(new_bp)) = 1;

  //payload
  GET_SIZE(HDRP(NEXT_BLKP(new_bp))) = chunk_size - GAP;
  GET_SIZE_F(FTRP(NEXT_BLKP(new_bp))) = chunk_size - GAP;
  GET_ALLOC(HDRP(NEXT_BLKP(new_bp))) = 0;
  GET_ALLOC_F(FTRP(NEXT_BLKP(new_bp))) = 0;

  //epilogue
  GET_SIZE_F(FTRP(new_bp)) = OVERHEAD;
  GET_ALLOC_F(FTRP(new_bp)) = 1;

  //terminator
  GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(new_bp)))) = 0;
  GET_ALLOC(HDRP(NEXT_BLKP(NEXT_BLKP(new_bp)))) = 1;
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

  int pg_size_by_8 = 1;
  int extend_count_by_8 = extend_count >> 3; //divide by 8
  if (extend_count_by_8 > 1){
    pg_size_by_8 = extend_count_by_8;
  }
  extend_count++;

  // pg. 58. Not exaxtly sure if this is right.
  size_t need_size = MAX(new_size, pg_size_by_8 * mem_pagesize());
  size_t chunk_size = PAGE_ALIGN(need_size * 8);

  void *new_page = mem_map(chunk_size);

  page *current_pg = first_pg;
  while(current_pg->next != NULL){
   current_pg = (page*) current_pg->next;
  }

  current_pg->next = new_page;

  page* next_page = (page*) current_pg->next;
  next_page->next = NULL;
  next_page->prev = (struct page*) current_pg;
  next_page->size = chunk_size;

  current_avail = next_page;

  void *new_bp = new_page + PG_SIZE + BLOCK_HEADER;
  current_block = new_bp;
  extend_helper(new_bp, chunk_size);

  return (new_page + GAP);
}

/*
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  //////////////////////// First Fit Pg. 11/////////////////////////////////////
  // size_t need_size = MAX(size, sizeof(free_bp_head));
  // printf("Requested size: %ld\n", size);
  // printf("Size of free_bp_head: %d\n", free_count);
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

  // It the space is not found on current page, iterate through the page,
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
    page* next_page = target_pg;
    next_page = (page*) next_page->next;
    target_pg = next_page;
  }

  // Finally, grab new page if no available block was found.
  target_bp = extend(new_size);
  set_allocated(target_bp, new_size);
  return target_bp;

  ///////////////////////  BEST FIT BEGINS HERE ////////////////////////////////
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
      // add_to_free_list(bp, size);
      // printf("%ld\n", (long)free_bp_head);
   }
 else if (prev_alloc && !next_alloc)
   { /* Case 2 */
     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
     GET_SIZE(HDRP(bp)) = size;
     GET_SIZE_F(FTRP(bp)) = size;
   }
 else if (!prev_alloc && next_alloc)
   { /* Case 3 */
     //remove_from_free_list((list_node *)HDRP(PREV_BLKP(bp)));
     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
     GET_SIZE_F(FTRP(bp)) = size;
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
     // add_to_free_list((list_node*) HDRP(bp));
   }
 else
   { /* Case 4 */
     // remove_from_free_list((list_node *)HDRP(PREV_BLKP(bp)));
     // remove_from_free_list((list_node *)HDRP(NEXT_BLKP(bp)));
     size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))));
     GET_SIZE(HDRP(PREV_BLKP(bp))) = size;
     GET_SIZE_F(FTRP(NEXT_BLKP(bp))) = size;
     bp = PREV_BLKP(bp);
     // add_to_free_list((list_node*) HDRP(bp));
   }
 //add_to_free_list((list_node*) HDRP(bp));
 return bp;
}

void unmap_helper(page* freeing_page, size_t to_free_size, void* prev, void* next){
  // printf("%s\n", "up tp page_start in mm_free");
  //Terminator block is next. Prologue block is prev.
  if(GET_SIZE(prev) == OVERHEAD && GET_SIZE(next) == 0)
  {
    if (first_pg == freeing_page){
      if(first_pg->prev == NULL && first_pg->next == NULL){
        return;
      }

      page* next_page = (page*) first_pg->next;
      next_page->prev = NULL;
      current_avail = next_page;
      first_pg = current_avail;
    }else{
      page *pg_to_remove = (page*) first_pg->next;
      while(pg_to_remove != freeing_page)
      {
        pg_to_remove = (page*) pg_to_remove->next;
      }

      if(pg_to_remove->prev != NULL && pg_to_remove->next != NULL)
      {
        page* next_page = (page*) pg_to_remove->next;
        page* prev_page = (page*) pg_to_remove->prev;
        next_page->prev = pg_to_remove->prev;
        prev_page->next = pg_to_remove->next;
      }
      else
      {
        page* prev_page = (page*) pg_to_remove->prev;
        prev_page->next = NULL;
      }
    }

    current_avail = NULL;
    current_block = NULL;

    mem_unmap(freeing_page, to_free_size);
  }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // printf("%s\n", "mm_free called");
    GET_ALLOC(HDRP(ptr)) = 0;
    GET_ALLOC_F(FTRP(ptr)) = 0;
    coalesce(ptr);

    // printf("%s\n", "up tp cpalesce sucessed in mm_free");
    void *prev = HDRP(PREV_BLKP(ptr));
    void *next = HDRP(NEXT_BLKP(ptr));
    size_t to_free_size = PAGE_ALIGN(GET_SIZE(HDRP(ptr)) + GAP);
    void * freeing_page = ptr - GAP; // HDRP(ptr)

    unmap_helper(freeing_page, to_free_size, prev, next);
    // printf("%s\n", "mm_free called");
}

/*
 * Check the allignment of the blcok
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
  // Make sure block is not too small
  if (GET_SIZE(HDRP(bp)) < 3 * BLOCK_HEADER ||
      GET_SIZE_F(FTRP(bp)) < 3 * BLOCK_HEADER){
      return false;
  }

  // Make sure block is not too too big.
  if (GET_SIZE(HDRP(bp)) > (size_t) MAX_BLOCK_SIZE ||
      GET_SIZE_F(FTRP(bp)) > (size_t) MAX_BLOCK_SIZE){
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
      ptr_is_mapped(current_pg, ((page *)current_pg)->size) == false){
      // printf("%s\n", "Called 1");
      return 0;
    }

    current_bp = current_pg + PG_SIZE + BLOCK_HEADER;

    if (GET_SIZE(HDRP(current_bp)) != OVERHEAD){
      // printf("%s\n", "Called 1");
      return 0;
    }else if(GET_ALLOC(HDRP(current_bp)) != 1){
      // printf("%s\n", "Called 2");
      return 0;
    }else if (GET_SIZE_F(FTRP(current_bp)) != OVERHEAD){
      // printf("%s %ld %ld\n", "Failed 3", GET_SIZE_F(FTRP(current_bp)), OVERHEAD);
      return 0;
    }

    current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    // Iterate through the blocks now. Slide pg 33.
    // printf("%s\n", "mm_check second while loop.");
    while (GET_SIZE(HDRP(current_bp)) != 0){
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

      //Make sure colleace happened.
      if (GET_ALLOC(HDRP(PREV_BLKP(current_bp))) == 0 && GET_ALLOC(HDRP(current_bp)) == 0){
        // printf("%s\n", "Called 7");
        return 0;
      }

      // Make sure header and footer has the same size, which it should be.
      if (GET_SIZE(HDRP(current_bp)) != GET_SIZE_F(FTRP(current_bp))){
        // printf("%s\n", "Called 11");
        return 0;
      }

      current_bp = NEXT_BLKP(current_bp); //Move on to the next block
    }
    // printf("%s\n", "mm_check so far.");
    page* next_page = (page*) current_pg;
    next_page = (page*) next_page->next;
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
  if(!ptr_is_mapped(HDRP(p), BLOCK_HEADER)){
    // printf("%s\n", "mm_can_free 2.");
    return 0;
  }

  //Added for -r 1000 option
  if(!ptr_is_mapped(HDRP(p), GET_SIZE(HDRP(p)))){
    // printf("%s\n", "mm_can_free 3.");
    return 0;
  }

  // Make sure this is a currently allocated block.
  if(GET_ALLOC(HDRP(p)) != 1){
    // printf("%s\n", "mm_can_free 6.");
    return 0;
  }
  return 1;
}
