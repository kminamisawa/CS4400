/*
 * mm-naive.c - The least memory-efficient malloc package.
 *
 * The following implemented allocator can be classified as an implicit coalescing
 * allocator with a few key details. "Chunks" or contiguous pages are mapped in a linked list
 * that can be found at the first 32 bytes of each chunk, the doubly linked list includes a next
 * and previous reference, along with that chunks size and a filler variable to keep payloads 16 byte aligned
 * Block headers and footers both have allocation bits, this is for mm_check to ensure that headers haven't been
 * mangled harshly. There includes an extending policy that adds 8 pages to the minimum request every 8 times
 * extend is called. There also includes an optimization in mm_malloc that searches the last page that a block was
 * allocated in.
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

#define OVERHEAD (sizeof(block_header)+sizeof(block_footer))
#define BHSIZE (sizeof(block_header))
#define PGSIZE (sizeof(page))
#define MAX_BLOCK_SIZE 1 << 32

#define NEXT_PAGE(pg) (((page *)pg)->next)
#define PREV_PAGE(pg) (((page *)pg)->prev)
#define PAGE_SIZE(pg) (((page *)pg)->size)

/* Get Header from payload pointer bp */
#define HDRP(bp) ((char *)(bp) - sizeof(block_header))
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-OVERHEAD)

#define GET_SIZE(p) ((block_header *)(p))->size
#define GET_ALLOC(p) ((block_header *)(p))->allocated

/* Payload of next block head pointer */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)- OVERHEAD))



typedef struct
{
  void *next;
  void *prev;
  size_t size;
  size_t filler;
} page;

typedef struct
{
  size_t size;    //In Bytes
  char allocated;
} block_header;

typedef struct
{
  size_t size;    //In Bytes
  char allocated;
} block_footer;

page* first_page; //First chunk pointer
void* first_pp;   //First payload pointer.
page* last_page_inserted;
void* last_block_inserted;
void* free_next_block;
int extend_count;


void examinePages()
{
  void* pg = first_page;
  while(pg != NULL)
  {
    printf("[prev:%p| %p |next:%p]\n",PREV_PAGE(pg), pg, NEXT_PAGE(pg));
    pg = NEXT_PAGE(pg);
  }
}

void examinePage(void* page)
{
  void* pg = page;
  void* pp;
  while(pg != NULL)
    {
      pp = (void *)pg + PGSIZE + BHSIZE;

      while(GET_SIZE(HDRP(pp)) != 0)
        {
          printf("[%ld,%d]=[%ld]--> ", GET_SIZE(HDRP(pp))/sizeof(block_header), GET_ALLOC(HDRP(pp)), GET_SIZE(FTRP(pp))/sizeof(block_header));
          pp = NEXT_BLKP(pp);
        }
      printf("[X]\n");
    }
}

void examineMemory()
{
  void* pp = first_pp;
  void* pg = first_page;

  int pageCount = 0;

  while(pg != NULL)
    {
      pp = (void *)pg + PGSIZE + BHSIZE;
      printf("[pg : %d]\n", pageCount);

      while(GET_SIZE(HDRP(pp)) != 0)
        {
          printf("[%ld,%d]=[%ld]--> ", GET_SIZE(HDRP(pp))/sizeof(block_header), GET_ALLOC(HDRP(pp)), GET_SIZE(FTRP(pp))/sizeof(block_header));
          pp = NEXT_BLKP(pp);
        }
      printf("[X]\n");

      pg = NEXT_PAGE(pg);
      pageCount++;
    }
}
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  extend_count = 0;

  size_t firstPageSize = PAGE_ALIGN(mem_pagesize() * 8);
  first_page = mem_map(firstPageSize);
  last_page_inserted = first_page;

  //First Page Setup
  NEXT_PAGE(first_page) = NULL;
  PREV_PAGE(first_page) = NULL;
  PAGE_SIZE(first_page) = firstPageSize;

  //First payload pointer
  first_pp = (char *)first_page + PGSIZE + BHSIZE;
  last_block_inserted = first_pp;

  void* pp = first_pp;
  //Setup Coalescing Prologue.
  GET_SIZE(HDRP(pp)) = OVERHEAD;
  GET_ALLOC(HDRP(pp)) = 1;
  GET_SIZE(FTRP(pp)) = OVERHEAD;
  GET_ALLOC(FTRP(pp)) = 1;

  pp = NEXT_BLKP(pp);
  //setup unallocated block for new chunk.
  GET_SIZE(HDRP(pp)) = firstPageSize - PGSIZE -  OVERHEAD - BHSIZE;
  GET_ALLOC(HDRP(pp)) = 0;
  GET_SIZE(FTRP(pp)) = firstPageSize - PGSIZE -  OVERHEAD - BHSIZE;
  GET_ALLOC(FTRP(pp)) = 0;

  pp = NEXT_BLKP(pp);
  //setup terminator block for new chunk
  GET_SIZE(HDRP(pp)) = 0;
  GET_ALLOC(HDRP(pp)) = 1;


  // printf("\n");
  // printf("\n");
  // examineMemory();
  // printf("\n");
  return 0;
}

void* extend(size_t new_size)
{
 int pgsz_mult = 8 * (extend_count/8) < 1 ? 1 : (extend_count/8);
 extend_count += 1;
 //printf("ec:%d\n",extend_count);

 int clampedSize = new_size > (pgsz_mult * mem_pagesize()) ? new_size : pgsz_mult * mem_pagesize();
 size_t chunk_size = PAGE_ALIGN(clampedSize * 8); //PAGE_ALIGN(clampedSize * 4);
 void *new_page = mem_map(chunk_size);

 //Find pageList end.
 void *pg = first_page;
 while(NEXT_PAGE(pg) != NULL)
 {
  pg = NEXT_PAGE(pg);
 }

 //Hookup new page into pageList.
 NEXT_PAGE(pg) = new_page;
 NEXT_PAGE(NEXT_PAGE(pg)) = NULL;
 PREV_PAGE(NEXT_PAGE(pg)) = pg;
 PAGE_SIZE(NEXT_PAGE(pg)) = chunk_size;

 last_page_inserted = NEXT_PAGE(pg);

 void *pp = new_page + PGSIZE + BHSIZE;

 last_block_inserted = pp;

 //create prologue for new page
 GET_SIZE(HDRP(pp)) = OVERHEAD;
 GET_ALLOC(HDRP(pp)) = 1;
 GET_SIZE(FTRP(pp)) = OVERHEAD;
 GET_ALLOC(FTRP(pp)) = 1;

 pp = NEXT_BLKP(pp);
 //create unalocated block
 GET_SIZE(HDRP(pp)) = chunk_size - PGSIZE - OVERHEAD - BHSIZE;
 GET_SIZE(FTRP(pp)) = chunk_size - PGSIZE - OVERHEAD - BHSIZE;
 GET_ALLOC(HDRP(pp)) = 0;
 GET_ALLOC(FTRP(pp)) = 0;

 pp = NEXT_BLKP(pp);
 //create terminator block
 GET_SIZE(HDRP(pp)) = 0;
 GET_ALLOC(HDRP(pp)) = 1;

 return (new_page + PGSIZE + OVERHEAD + BHSIZE);
}

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
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  if(size == 0)
    return NULL;

 int new_size = ALIGN(size + OVERHEAD);
 void *pg = first_page;
 void *pp;

 pg = last_page_inserted == NULL ? first_page : last_page_inserted;
 pp = pg + PGSIZE + OVERHEAD + BHSIZE;


 while (GET_SIZE(HDRP(pp)) != 0)
   {
     if (!GET_ALLOC(HDRP(pp)) && (GET_SIZE(HDRP(pp)) >= new_size))
       {
	 set_allocated(pp, new_size);
	 return pp;
       }
     pp = NEXT_BLKP(pp);
   }

 pg = first_page;
 while(pg != NULL)
 {
   //if(pg != last_page_inserted)
    {
     pp = pg + PGSIZE + OVERHEAD + BHSIZE;
     while (GET_SIZE(HDRP(pp)) != 0)
     {
      if (!GET_ALLOC(HDRP(pp)) && (GET_SIZE(HDRP(pp)) >= new_size))
      {
        set_allocated(pp, new_size);
        last_page_inserted = pg;
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

void attempt_unmap(void *ptr)
{
  void *prev = HDRP(PREV_BLKP(ptr));
  void *next = HDRP(NEXT_BLKP(ptr));

  size_t unmap_size = PAGE_ALIGN(GET_SIZE(HDRP(ptr)) + OVERHEAD + BHSIZE + PGSIZE);
  void * page_start = ptr - OVERHEAD - BHSIZE - PGSIZE;
  //printf("unmap_size :%ld\n",unmap_size);

  //Terminator block is next. Prologue block is prev.
  if(GET_SIZE(next) == 0 && GET_SIZE(prev) == OVERHEAD)
  {
    printf("%s\n", "MM REACHED");
    //Un hook page from page linked list.
    void *pg = first_page;
    while(pg != page_start)
    {
      pg = NEXT_PAGE(pg);
    }

    //page to remove is page linked list head
    if(pg == first_page)
    {
      //About to remove first page.
      if(NEXT_PAGE(first_page) == NULL && PREV_PAGE(first_page) == NULL)
        return;

      PREV_PAGE(NEXT_PAGE(first_page)) = NULL;
      first_page = last_page_inserted =  NEXT_PAGE(first_page);
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

    last_page_inserted = NULL;
    last_block_inserted = NULL;

    mem_unmap(page_start,unmap_size);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0;
  GET_ALLOC(FTRP(ptr)) = 0;
  coalesce(ptr);
  attempt_unmap(ptr);
}

int ptr_is_mapped(void *p, size_t len)
{
    void *s = ADDRESS_PAGE_START(p);
    return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  int d = 0;
  void* pg = first_page;
  void* pp;
  while(pg != NULL)
    {
      //Page is mapped.
      if(!ptr_is_mapped(pg,mem_pagesize())) { if(d)printf("16\n");return 0; }
      if(!ptr_is_mapped(pg,PAGE_SIZE(pg))) { if(d)printf("1\n");return 0; }

      pp = (void *)pg + PGSIZE + BHSIZE;

      //check for prologue blocks
      if(GET_SIZE(HDRP(pp)) != OVERHEAD || GET_ALLOC(HDRP(pp)) != 1) { if(d)printf("2\n");return 0; }
      if(GET_SIZE(FTRP(pp)) != OVERHEAD) { if(d)printf("15\n");return 0; }

      //Skip past prologue to check rest of the block.
      pp = NEXT_BLKP(pp);
      while(GET_SIZE(HDRP(pp)) != 0)
	{
	  //Header is mapped
	  if(!ptr_is_mapped(pp - BHSIZE, BHSIZE)) { if(d)printf("3\n");return 0;}

	  //Payload not 16 byte aligned
	  if( ((size_t)pp & 15) != 0 ) { if(d)printf("4\n");return 0; }

	  // Bidirectional check.
	  if( !ptr_is_mapped(HDRP(pp),  GET_SIZE(HDRP(pp))) ) { if(d)printf("5\n");return 0; }
	  if( GET_SIZE(HDRP(pp)) > (size_t)MAX_BLOCK_SIZE) { if(d)printf("6\n");return 0;}
	  if( GET_SIZE(HDRP(pp)) < 3 * BHSIZE) { if(d)printf("7\n");return 0; }

	  if( ((size_t)FTRP(pp) & 15) != 0 ) { if(d)printf("8\n");return 0; }
	  if( !ptr_is_mapped(HDRP(pp) ,GET_SIZE(FTRP(pp))) ) { if(d)printf("9\n");return 0; }
	  if( GET_SIZE(FTRP(pp)) > (size_t)MAX_BLOCK_SIZE ) { if(d)printf("10\n");return 0; }
	  if( GET_SIZE(FTRP(pp)) < 3 * BHSIZE) { if(d)printf("11\n");return 0; }

	  //Allocation bit is not 0 or 1
	  if( GET_ALLOC(HDRP(pp)) != 0 && GET_ALLOC(HDRP(pp)) != 1) { if(d)printf("12\n");return 0; }

	  //Check double check method for alloc to ensure that not both alloc bits were mangled.
	  if(GET_ALLOC(HDRP(pp)) == 1 && GET_ALLOC(FTRP(pp)) == 0) { return 0; }
	  if(GET_ALLOC(HDRP(pp)) == 0 && GET_ALLOC(FTRP(pp)) == 1) { return 0; }

	  //Header size is not the same as footer size.
	  if( GET_SIZE(HDRP(pp)) != GET_SIZE(FTRP(pp)) ) { if(d)printf("13\n");return 0; }

	  //No two consectuive blocks are free
	  if( GET_ALLOC(HDRP(PREV_BLKP(pp))) == 0 && GET_ALLOC(HDRP(pp)) == 0 ) { if(d)printf("14\n");return 0; }

	  pp = NEXT_BLKP(pp);
	}
	pg = NEXT_PAGE(pg);
    }

  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  //p is 16 byte aligned
  if( ((size_t)p & 15) != 0 ) { return 0; }

  //Header is not mapped
  if(!ptr_is_mapped(HDRP(p), BHSIZE)) { return 0;}

  // Bidirectional check.
  if( !ptr_is_mapped(HDRP(p), GET_SIZE(HDRP(p))) ) { return 0; }
  if( GET_SIZE(HDRP(p)) > (size_t)MAX_BLOCK_SIZE) { return 0; }
  if( GET_SIZE(HDRP(p)) < 3 * BHSIZE) { return 0; }

  if( !ptr_is_mapped(HDRP(p), GET_SIZE(FTRP(p))) ) { return 0; }
  if( GET_SIZE(FTRP(p)) > (size_t)MAX_BLOCK_SIZE ) { return 0; }
  if( GET_SIZE(FTRP(p)) < 3 * BHSIZE) { return 0; }

  //is not prologue or epilogue
  if(GET_SIZE(HDRP(p)) == OVERHEAD && GET_ALLOC(HDRP(p)) == 1) { return 0; }
  if(GET_SIZE(FTRP(p)) == OVERHEAD && GET_ALLOC(HDRP(p)) == 1) { return 0; }

  //is not a terminator
  if(GET_SIZE(HDRP(p)) == 0 && GET_ALLOC(HDRP(p)) == 1) { return 0; }

  // ensure that block was allocated.
  if( GET_ALLOC(HDRP(p)) != 1 ) { return 0; }

  //Double Aloc bit check
  if(GET_ALLOC(HDRP(p)) == 1 && GET_ALLOC(FTRP(p)) == 0) { return 0; }
  if(GET_ALLOC(HDRP(p)) == 0 && GET_ALLOC(FTRP(p)) == 1) { return 0; }


  return 1;
}
