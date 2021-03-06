/*
  Copyright (C) 2003-2020 GraphicsMagick Group

  This program is covered by multiple licenses, which are described in
  Copyright.txt. You should have received a copy of Copyright.txt with this
  package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.

  GraphicsMagick Memory Allocation Methods.
*/

extern MagickExport void
  *MagickMallocAlignedArray(const size_t alignment,const size_t count,const size_t size) MAGICK_FUNC_MALLOC MAGICK_FUNC_ALLOC_SIZE_2ARG(2,3),
  *MagickMallocArray(const size_t count,const size_t size) MAGICK_FUNC_MALLOC MAGICK_FUNC_ALLOC_SIZE_2ARG(1,2);

extern MagickExport size_t
  MagickArraySize(const size_t count,const size_t size) MAGICK_FUNC_CONST;

/*
  Allocate memory
*/
#define MagickAllocateMemory(type,size) \
  ((((size) != ((size_t) (size))) || (size == 0)) ? ((type) 0) : \
   ((type) MagickMalloc((size_t) (size))))

#define MagickAllocateClearedMemory(type,size) \
  ((((size) != ((size_t) (size))) || (size == 0)) ? ((type) 0) : \
   ((type) MagickMallocCleared((size_t) (size))))

#define MagickAllocateArray(type,count,size) \
  ((type) MagickMallocArray(count,size))

#define MagickAllocateClearedArray(type,count,size) \
  ((type) MagickMallocCleared(MagickArraySize(count,size)))
/*
  Free memory and set pointer to NULL
*/
#define MagickFreeMemory(memory) \
{ \
  void *_magick_mp=memory;      \
  MagickFree(_magick_mp);       \
  memory=0;                     \
}

/*
  Reallocate memory using provided pointer.  If reallocation fails
  then free memory, setting pointer to null.  If size is 0 and memory
  is not a null pointer, then free memory.  This interface behaves
  similar to realloc() except that memory is always freed (and pointer
  set to null) if a memory allocation failure occurs.
*/
#define MagickReallocMemory(type,memory,size)   \
{ \
    size_t _new_size = (size_t) (size); \
    void *_magick_mp = MagickRealloc(memory,_new_size); \
    memory=(type) _magick_mp; \
}

/*
  Allocate memory aligned to a specified alignment boundary
*/
#define MagickAllocateAlignedMemory(type,alignment,size)                \
  ((((size) != ((size_t) (size))) || (size == 0)) ? ((type) 0) :        \
   ((type) MAGICK_ASSUME_ALIGNED(MagickMallocAligned((size_t) alignment, (size_t) (size)),alignment)))

/*
  Allocate array aligned to a specified alignment boundary
*/
#define MagickAllocateAlignedArray(type,alignment,count,size)           \
  ((((size) != ((size_t) (size))) || (size == 0)) ? ((type) 0) :        \
   ((type) MAGICK_ASSUME_ALIGNED(MagickMallocAlignedArray((size_t) alignment, (size_t) (count), (size_t) (size)),alignment)))

/*
  Free aligned memory (from MagickAllocateAlignedMemory()) and set pointer to
  NULL
*/
#define MagickFreeAlignedMemory(memory)         \
{                                               \
  void *_magick_mp=memory;                      \
  MagickFreeAligned(_magick_mp);                \
  memory=0;                                     \
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
