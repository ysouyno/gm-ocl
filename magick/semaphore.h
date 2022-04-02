/*
  Copyright (C) 2003-2010 GraphicsMagick Group
  Copyright (C) 2002 ImageMagick Studio

  This program is covered by multiple licenses, which are described in
  Copyright.txt. You should have received a copy of Copyright.txt with this
  package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.

  Methods to lock and unlock semaphores.
*/

/*
  Functions: InitializeMagickMutex(), DestroyMagickMutex() are based on
  ImageMagick/MagickCore/mutex.h
*/
#ifndef _MAGICK_SEMAPHORE_H
#define _MAGICK_SEMAPHORE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _SemaphoreInfo SemaphoreInfo;

extern MagickExport SemaphoreInfo
  *AllocateSemaphoreInfo(void);

extern MagickExport void
  ActivateSemaphoreInfo(SemaphoreInfo **),
  DestroySemaphoreInfo(SemaphoreInfo **),
  LockSemaphoreInfo(SemaphoreInfo *),
  UnlockSemaphoreInfo(SemaphoreInfo *),
  RelinquishSemaphoreInfo(SemaphoreInfo **);

/*
  These are deprecated.
*/
extern MagickExport void
  AcquireSemaphoreInfo(SemaphoreInfo **) MAGICK_FUNC_DEPRECATED,
  LiberateSemaphoreInfo(SemaphoreInfo **) MAGICK_FUNC_DEPRECATED;

/*
  These should not be MagickExport.
*/
extern MagickExport void
  DestroySemaphore(void),
  InitializeSemaphore(void);

/*
  When included in a translation unit, the following code provides the
  translation unit a means by which to synchronize multiple threads that might
  try to enter the same critical section or to access a shared resource; it can
  be included in multiple translation units, and thereby provide a separate,
  independent means of synchronization to each such translation unit.
*/

#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
static MagickBool
  translation_unit_initialized = MagickFalse;

static omp_lock_t
  translation_unit_mutex;
#elif defined(/*MAGICKCORE_THREAD_SUPPORT*/HAVE_PTHREAD)
static pthread_mutex_t
  translation_unit_mutex = PTHREAD_MUTEX_INITIALIZER;
#elif defined(/*MAGICKCORE_WINDOWS_SUPPORT*/MSWINDOWS)
static LONG
  translation_unit_mutex = 0;
#endif

static inline void DestroyMagickMutex(void)
{
#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
  omp_destroy_lock(&translation_unit_mutex);
  translation_unit_initialized=MagickFalse;
#endif
}

static inline void InitializeMagickMutex(void)
{
#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
  omp_init_lock(&translation_unit_mutex);
  translation_unit_initialized=MagickTrue;
#endif
}

static inline void LockMagickMutex(void)
{
#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
  if (translation_unit_initialized == MagickFalse)
    InitializeMagickMutex();
  omp_set_lock(&translation_unit_mutex);
#elif defined(/*MAGICKCORE_THREAD_SUPPORT*/HAVE_PTHREAD)
  {
    int
      status;

    status=pthread_mutex_lock(&translation_unit_mutex);
    if (status != 0)
      {
        errno=status;
        ThrowFatalException(ResourceLimitFatalError,"UnableToLockSemaphore");
      }
  }
#elif defined(/*MAGICKCORE_WINDOWS_SUPPORT*/MSWINDOWS)
  while (InterlockedCompareExchange(&translation_unit_mutex,1L,0L) != 0)
    Sleep(10);
#endif
}

static inline void UnlockMagickMutex(void)
{
#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
  omp_unset_lock(&translation_unit_mutex);
#elif defined(/*MAGICKCORE_THREAD_SUPPORT*/HAVE_PTHREAD)
  {
    int
      status;

    status=pthread_mutex_unlock(&translation_unit_mutex);
    if (status != 0)
      {
        errno=status;
        ThrowFatalException(ResourceLimitFatalError,"UnableToUnlockSemaphore");
      }
  }
#elif defined(/*MAGICKCORE_WINDOWS_SUPPORT*/MSWINDOWS)
  InterlockedExchange(&translation_unit_mutex,0L);
#endif
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
