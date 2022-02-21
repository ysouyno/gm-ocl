/*
% Copyright (C) 2003 - 2020 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             L       OOO    GGGG                             %
%                             L      O   O  G                                 %
%                             L      O   O  G GG                              %
%                             L      O   O  G   G                             %
%                             LLLLL   OOO    GGG                              %
%                                                                             %
%                                                                             %
%                          Log GraphicsMagick Events                          %
%                                                                             %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                September 2002                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#if defined(MSWINDOWS) || defined(__CYGWIN__)
# include "magick/nt_feature.h"
#endif
#include "magick/blob.h"
#include "magick/log.h"
#include "magick/semaphore.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
  Define declarations.
*/
#define MagickLogFilename  "log.mgk"

/*
  Typedef declarations.
*/
typedef enum
{
  DisabledOutput = 0x0000,
  UndefinedOutput = 0x0000,
  StdoutOutput = 0x0001,
  StderrOutput = 0x0002,
  XMLFileOutput = 0x0004,
  TXTFileOutput = 0x0008,
  Win32DebugOutput = 0x0010,
  Win32EventlogOutput = 0x0020,
  MethodOutput = 0x0040
} LogOutputType;

typedef struct _LogInfo
{
  SemaphoreInfo
    *log_semaphore;

  FILE
    *file;

  TimerInfo
    timer;

  unsigned long
    generations,
    limit,
    generation,
    count;

  LogEventType
    events;

  LogOutputType
    output_type;

  LogMethod
    method;  /* Logger callback function */

  MagickBool
    log_configured;

  char
    path[256],
    filename[256],
    format[200];

} LogInfo;

/*
  This table maps between masks and the various event id's that can occur
  This following id's are not represented in this table yet, since each of
  them would require a bit in the bitmask and none of these are actually
  used in the code at this point.

     DelegateBase
     MissingDelegateBase
     CorruptImageBase
     FileOpenBase
     StreamBase
     ModuleBase
     ImageBase
     MonitorBase
     RegistryBase

 */
static const struct
{
  const char name[14];
  unsigned int name_len;
  LogEventType mask;
  int start_type;
  int end_type;
} eventmask_map[] =
  {
#define STATIC_EVMASK(name,mask,start_type,end_type) {name,sizeof(name)-1,mask,start_type,end_type}
    STATIC_EVMASK( "none", NoEventsMask, 0, 0 ),
    STATIC_EVMASK( "information", InformationEventMask, EventException, EventException+99 ),
    STATIC_EVMASK( "warning", WarningEventMask, WarningException, WarningException+99 ),
    STATIC_EVMASK( "error", ErrorEventMask, ErrorException, ErrorException+99 ),
    STATIC_EVMASK( "fatalerror", FatalErrorEventMask, FatalErrorException, FatalErrorException+99 ),
    STATIC_EVMASK( "configure", ConfigureEventMask, ConfigureBase, ConfigureBase ),
    STATIC_EVMASK( "annotate", AnnotateEventMask, AnnotateBase, AnnotateBase ),
    STATIC_EVMASK( "render", RenderEventMask, RenderBase, RenderBase ),
    STATIC_EVMASK( "transform", TransformEventMask, TransformBase, TransformBase ),
    STATIC_EVMASK( "locale", LocaleEventMask, LocaleBase, LocaleBase ),
    STATIC_EVMASK( "coder", CoderEventMask, CoderBase, CoderBase ),
    STATIC_EVMASK( "x11", X11EventMask, X11Base, UserBase ),
    STATIC_EVMASK( "cache", CacheEventMask, CacheBase, CacheBase ),
    STATIC_EVMASK( "blob", BlobEventMask, BlobBase, BlobBase ),
    STATIC_EVMASK( "deprecate", DeprecateEventMask, DeprecateBase, DeprecateBase ),
    STATIC_EVMASK( "user", UserEventMask, UserBase, UserBase ),
    STATIC_EVMASK( "resource", ResourceEventMask, ResourceBase, ResourceBase ),
    STATIC_EVMASK( "temporaryfile", TemporaryFileEventMask, TemporaryFileBase, TemporaryFileBase ),
    /* this one is actually not used anymore */
    STATIC_EVMASK( "exception", ExceptionEventMask, ExceptionBase, ExceptionBase ),
    STATIC_EVMASK( "option", OptionEventMask, OptionBase, OptionBase ),
    STATIC_EVMASK( "all", AllEventsMask, 0, 0 )
  };

static const struct
{
  const char name[14];
  unsigned int name_len;
  LogOutputType mask;
} output_map[] =
  {
#define STATIC_OMAP(name,mask) {name,sizeof(name)-1,mask}
    STATIC_OMAP( "none", UndefinedOutput ),
    STATIC_OMAP( "disabled", DisabledOutput ),
    STATIC_OMAP( "stdout", StdoutOutput ),
    STATIC_OMAP( "stderr", StderrOutput ),
    STATIC_OMAP( "xmlfile", XMLFileOutput ),
    STATIC_OMAP( "txtfile", TXTFileOutput ),
    STATIC_OMAP( "win32debug", Win32DebugOutput ),
    STATIC_OMAP( "win32eventlog", Win32EventlogOutput )
  };

/*
  Static declarations.
*/
static LogInfo
*log_info = (LogInfo *) NULL;

/*
  Forward declarations.
*/
static MagickPassFail
  ReadLogConfigureFile(const char *,const unsigned int,ExceptionInfo *);

static LogEventType
ParseEvents(const char *event_string) MAGICK_FUNC_PURE;

/*
  Parse an event specification string and return the equivalent bits.
*/
static LogEventType ParseEvents(const char *event_string)
{
  const char
    *p;

  unsigned int
    i;

  LogEventType
    events=NoEventsMask;

  for (p=event_string; p != 0; p=strchr(p,','))
    {
      while ((*p != 0) && (isspace((int)(*p)) || *p == ','))
        p++;

      for (i=0; i < ArraySize(eventmask_map); i++)
        {
          if (LocaleNCompare(p,eventmask_map[i].name,
                             eventmask_map[i].name_len) == 0)
            {
              events = (LogEventType) ((unsigned int) events |
                                       (unsigned int) eventmask_map[i].mask);
              break;
            }
        }
    }

  return events;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y L o g I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method DestroyLogInfo deallocates memory associated with the log list.
%
%  The format of the DestroyLogInfo method is:
%
%      DestroyLogInfo(void)
%
%
*/
MagickExport void DestroyLogInfo(void)
{
  if (log_info->file != (FILE *) NULL)
    if ((log_info->file != stdout) && (log_info->file != stderr))
      {
        if (log_info->output_type == XMLFileOutput)
          (void) fprintf(log_info->file,"</log>\n");
        (void) fclose(log_info->file);
        log_info->file=(FILE *) NULL;
      }
  log_info->log_configured=False;
  DestroySemaphoreInfo(&log_info->log_semaphore);
  MagickFreeMemory(log_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e L o g I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method InitializeLogInfo initializes the logging facility.  This function
%  is invoked by InitializeMagick(), which must be invoked before using any
%  other APIs.  A memory-allocation failure in this function results in a
%  fatal error for the whole program.  All of the allocations performed by
%  this function are released by DestroyLogInfo().
%
%  The format of the InitializeLogInfo method is:
%
%      MagickPassFail InitializeLogInfo(void)
%
%
*/
MagickPassFail
InitializeLogInfo(void)
{
  const char
    *p;

  assert(log_info == (LogInfo *) NULL);

  /*
    Initialize LogInfo
  */
  log_info=MagickAllocateMemory(LogInfo *,sizeof(LogInfo));
  if (log_info == (LogInfo *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToAllocateLogInfo);

  /*
    Initialize logging semaphore
  */
  log_info->log_semaphore=AllocateSemaphoreInfo();

  /*
    Lock for access (to make Coverity happy)
  */
#if defined(__COVERITY__)
  LockSemaphoreInfo(log_info->log_semaphore);
#endif /* defined(__COVERITY__) */

  log_info->file=(FILE *) NULL;
  GetTimerInfo(&log_info->timer);
  log_info->generations=3;
  log_info->limit=2000;
  log_info->generation=0;
  log_info->count=0;
  log_info->events=NoEventsMask;
  log_info->output_type=StderrOutput;
  log_info->method=0;
  log_info->log_configured=MagickFalse;

  (void) strlcpy(log_info->path,"(default)",sizeof(log_info->path));
  (void) strlcpy(log_info->filename,"Magick-%d.log",sizeof(log_info->filename));
  (void) strlcpy(log_info->format,"%t %r %u %p %m/%f/%l/%d:\n  %e",
                 sizeof(log_info->format));

#if defined(__COVERITY__)
  UnlockSemaphoreInfo(log_info->log_semaphore);
#endif /* defined(__COVERITY__) */

  /*
    Set initial logging flags using the value of MAGICK_DEBUG if it is
    set in the environment.  We do this here so it is possible to
    debug the loading of the log configuration file.
  */
  if ((p=getenv("MAGICK_DEBUG")) != (const char *) NULL)
    (void) SetLogEventMask(p);

#if UseInstalledMagick
  /*
    Try to read the log configuration file.
  */
  {
    ExceptionInfo
      exception;

    GetExceptionInfo(&exception);
    (void) ReadLogConfigureFile(MagickLogFilename,0,&exception);
    DestroyExceptionInfo(&exception);
  }

  /*
    Set override logging flags using the value of MAGICK_DEBUG if it
    is set in the environment.
  */
  if ((p=getenv("MAGICK_DEBUG")) != (const char *) NULL)
    (void) SetLogEventMask(p);
#endif /* UseInstalledMagick */

  return MagickPass;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e L o g I n f o P o s t                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method InitializeLogInfoPost finishes initialization of the logging
%  facility after the installation path is known.  This function
%  is invoked by InitializeMagick(), which must be invoked before using any
%  other APIs.  All of the allocations performed by this function are
%  released by DestroyLogInfo().
%
%  The format of the InitializeLogInfoPost method is:
%
%      MagickPassFail InitializeLogInfoPost(void)
%
%
*/
MagickPassFail
InitializeLogInfoPost(void)
{
  const char
    *p;

  /*
    Try to read the log configuration file.
  */
  if (!log_info->log_configured)
  {
    ExceptionInfo
      exception;

    GetExceptionInfo(&exception);
    (void) ReadLogConfigureFile(MagickLogFilename,0,&exception);
    DestroyExceptionInfo(&exception);

    /*
      Set override logging flags using the value of MAGICK_DEBUG if it
      is set in the environment.
    */
    if ((p=getenv("MAGICK_DEBUG")) != (const char *) NULL)
      (void) SetLogEventMask(p);
  }

  return MagickPass;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s E v e n t L o g g i n g                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEventLogging() returns True if logging of events is enabled otherwise
%  False.
%
%  The format of the IsEventLogging method is:
%
%      unsigned int IsEventLogging(void)
%
%
*/
MagickExport MagickBool IsEventLogging(void)
{
  return (log_info->events != NoEventsMask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o g M a g i c k E v e n t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LogMagickEvent() logs an event as determined by the log configuration file.
%  If an error occurs, False is returned otherwise True.
%
%  The format of the LogMagickEvent method is:
%
%      unsigned int LogMagickEvent(const LogEventType type,const char *module,
%        const char *function,const unsigned long line,const char *format,...)
%
%  A description of each parameter follows:
%
%    o type: The event type.
%
%    o filename: The source module filename.
%
%    o function: The function name.
%
%    o line: The line number of the source module.
%
%    o format: The output format.
%
%
*/
MagickExport  unsigned int LogMagickEventList(const ExceptionType type,
  const char *module,const char *function,const unsigned long line,
  const char *format,va_list operands)
{
  char
    *domain,
    *severity,
#if defined(MSWINDOWS)
    nteventtype,
#endif
    event[MaxTextExtent],
    srcname[MaxTextExtent],
    timestamp[MaxTextExtent];

  double
    elapsed_time,
    user_time;

  register const char
    *p;

  struct tm
#if defined(HAVE_LOCALTIME_R)
    tm_buf,
#endif /* if defined(HAVE_LOCALTIME_R) */
    *time_meridian;

  time_t
    seconds;

  if (!IsEventLogging())
    return(False);

  if (log_info->events != AllEventsMask)
    {
      unsigned int
        i;

      unsigned int
        enabled;

      /* first translate the base type of the event to a mask */
      enabled=False;
      for (i=0; i < ArraySize(eventmask_map); i++)
        {
          /* if the range in the table is above 100 it represents raw
             event id's. These entry types are to look for specific
             severity codes.
          */
          if (eventmask_map[i].start_type > 99)
            {
              if (((int) type >= eventmask_map[i].start_type) &&
                  ((int) type <= eventmask_map[i].end_type))
                {
                  if (((unsigned int) log_info->events) &
                      ((unsigned int) eventmask_map[i].mask))
                    {
                      enabled=True;
                      break;
                    }
                }
            }
          else
            {
              /* these ranges are for id's with the severity stripped
                 off and represent a category instead.
              */
              if ((((int) type % 100) >= eventmask_map[i].start_type) &&
                  (((int) type % 100) <= eventmask_map[i].end_type))
                {
                  if (((unsigned int) log_info->events) &
                      ((unsigned int) eventmask_map[i].mask))
                    {
                      enabled=True;
                      break;
                    }
                }
            }
        }
      if (!enabled)
        return(True);
    }

  /* fixup module info to just include the filename - and not the
     whole path to the file. This makes the log huge for no good
     reason */
  GetPathComponent(module,TailPath,srcname);

  LockSemaphoreInfo(log_info->log_semaphore);
  switch (((unsigned int) type) % 100)
  {
    case UndefinedException: domain=(char *) "Undefined"; break;
    case ExceptionBase: domain=(char *) "Exception"; break;
    case ResourceBase: domain=(char *) "Resource"; break;
    /* case ResourceLimitBase: domain=(char *) "ResourceLimit"; break; */
    case TypeBase: domain=(char *) "Type"; break;
    /* case AnnotateBase: domain=(char *) "Annotate"; break; */
    case OptionBase: domain=(char *) "Option"; break;
    case DelegateBase: domain=(char *) "Delegate"; break;
    case MissingDelegateBase: domain=(char *) "MissingDelegate"; break;
    case CorruptImageBase: domain=(char *) "CorruptImage"; break;
    case FileOpenBase: domain=(char *) "FileOpen"; break;
    case BlobBase: domain=(char *) "Blob"; break;
    case StreamBase: domain=(char *) "Stream"; break;
    case CacheBase: domain=(char *) "Cache"; break;
    case CoderBase: domain=(char *) "Coder"; break;
    case ModuleBase: domain=(char *) "Module"; break;
    case DrawBase: domain=(char *) "Draw"; break;
    /* case RenderBase: domain=(char *) "Render"; break; */
    case ImageBase: domain=(char *) "image"; break;
    case TemporaryFileBase: domain=(char *) "TemporaryFile"; break;
    case TransformBase: domain=(char *) "Transform"; break;
    case XServerBase: domain=(char *) "XServer"; break;
    case X11Base: domain=(char *) "X11"; break;
    case UserBase: domain=(char *) "User"; break;
    case MonitorBase: domain=(char *) "Monitor"; break;
    case LocaleBase: domain=(char *) "Locale"; break;
    case DeprecateBase: domain=(char *) "Deprecate"; break;
    case RegistryBase: domain=(char *) "Registry"; break;
    case ConfigureBase: domain=(char *) "Configure"; break;
    default: domain=(char *) "UnknownEvent"; break;
  }
  switch ((((unsigned int) type) / 100) * 100)
  {
    case EventException: severity=(char *) "Event"; break;
    case WarningException: severity=(char *) "Warning"; break;
    case ErrorException: severity=(char *) "Error"; break;
    case FatalErrorException: severity=(char *) "FatalError"; break;
    default: severity=(char *) "Unknown"; break;
  }
#if defined(MSWINDOWS)
  switch ((type / 100) * 100)
  {
    case EventException: nteventtype=EVENTLOG_INFORMATION_TYPE; break;
    case WarningException: nteventtype=EVENTLOG_WARNING_TYPE; break;
    case ErrorException: nteventtype=EVENTLOG_ERROR_TYPE; break;
    case FatalErrorException: nteventtype=EVENTLOG_ERROR_TYPE; break;
    default: nteventtype=EVENTLOG_INFORMATION_TYPE; break;
  }
#endif
#if defined(HAVE_VSNPRINTF)
  (void) vsnprintf(event,MaxTextExtent,format,operands);
#else
#  if defined(HAVE_VSPRINTF)
  (void) vsprintf(event,format,operands);
#  else
#    error Neither vsnprintf or vsprintf is available.
#  endif
#endif
  seconds=time((time_t *) NULL);
#if defined(HAVE_LOCALTIME_R)
  time_meridian=localtime_r(&seconds, &tm_buf);
#else
  time_meridian=localtime(&seconds); /* Thread-unsafe version */
#endif /* if defined(HAVE_LOCALTIME_R) */
  elapsed_time=GetElapsedTime(&log_info->timer);
  user_time=GetUserTime(&log_info->timer);
  (void) ContinueTimer((TimerInfo *) &log_info->timer);
  FormatString(timestamp,"%04d%02d%02d%02d%02d%02d",time_meridian->tm_year+
    1900,time_meridian->tm_mon+1,time_meridian->tm_mday,
    time_meridian->tm_hour,time_meridian->tm_min,time_meridian->tm_sec);
  if (((unsigned int) log_info->output_type) & XMLFileOutput)
    {
      /*
        Log to a file in the XML format.
      */
      log_info->count++;
      if (log_info->count >= log_info->limit)
        {
          (void) fprintf(log_info->file,"</log>\n");
          (void) fclose(log_info->file);
          log_info->file=(FILE *) NULL;
          log_info->count=0;
        }
      if (log_info->file == (FILE *) NULL)
        {
          char
            filename[MaxTextExtent];

          (void) MagickSceneFileName(filename,log_info->filename,".%lu",
                                     MagickFalse,log_info->generation);
          log_info->file=fopen(filename,"w");
          if (log_info->file == (FILE *) NULL)
            {
              UnlockSemaphoreInfo(log_info->log_semaphore);
              return(False);
            }
          (void) fprintf(log_info->file,"<?xml version=\"1.0\"?>\n");
          (void) fprintf(log_info->file,"<log>\n");
          log_info->generation++;
          if (log_info->generation >= log_info->generations)
            log_info->generation=0;
        }
      (void) fprintf(log_info->file,"<record>\n");
      (void) fprintf(log_info->file,"  <timestamp>%.1024s</timestamp>\n",
        timestamp);
      (void) fprintf(log_info->file,
        "  <elapsed-time>%ld:%-9.6f</elapsed-time>\n",
        (long) (elapsed_time/60.0),fmod(elapsed_time,60.0));
      (void) fprintf(log_info->file,"  <user-time>%0.3f</user-time>\n",
        user_time);
      (void) fprintf(log_info->file,"  <pid>%ld</pid>\n",(long) getpid());
      (void) fprintf(log_info->file,"  <module>%.1024s</module>\n",srcname);
      (void) fprintf(log_info->file,"  <function>%.1024s</function>\n",
        function);
      (void) fprintf(log_info->file,"  <line>%lu</line>\n",line);
      (void) fprintf(log_info->file,"  <domain>%.1024s</domain>\n",domain);
      (void) fprintf(log_info->file,"  <severity>%.1024s</severity>\n",severity);
      (void) fprintf(log_info->file,"  <event>%.1024s</event>\n",event);
      (void) fprintf(log_info->file,"</record>\n");
      (void) fflush(log_info->file);
      UnlockSemaphoreInfo(log_info->log_semaphore);
      return(True);
    }
  if (((unsigned int) log_info->output_type) & TXTFileOutput)
    {
      /*
        Log to a file in the TXT format.
      */
      log_info->count++;
      if (log_info->count >= log_info->limit)
        {
          (void) fclose(log_info->file);
          log_info->file=(FILE *) NULL;
          log_info->count=0;
        }
      if (log_info->file == (FILE *) NULL)
        {
          char
            filename[MaxTextExtent];

          (void) MagickSceneFileName(filename,log_info->filename,".%lu",
                                     MagickFalse,log_info->generation);
          log_info->file=fopen(filename,"w");
          if (log_info->file == (FILE *) NULL)
            {
              UnlockSemaphoreInfo(log_info->log_semaphore);
              return(False);
            }
          log_info->generation++;
          if (log_info->generation >= log_info->generations)
            log_info->generation=0;
        }
        (void) fprintf(log_info->file,
                       "%.1024s %ld:%-9.6f %0.3f %ld %.1024s %.1024s %lu"
                       " %.1024s %.1024s %.1024s\n",
                       timestamp, (long) (elapsed_time/60.0),
                       fmod(elapsed_time,60.0),
                       user_time, (long) getpid(), srcname, function, line, domain,
                       severity, event);
      (void) fflush(log_info->file);
      UnlockSemaphoreInfo(log_info->log_semaphore);
      return(True);
    }
#if defined(MSWINDOWS)
  if (log_info->output_type & Win32DebugOutput)
    {
      char
        buffer[MaxTextExtent];

      FormatString(buffer,
                   "%.1024s %ld:%-9.6f %0.3f %ld %.1024s %.1024s %lu %.1024s"
                   " %.1024s %.1024s\n",
                   timestamp, (long) (elapsed_time/60.0),
                   fmod(elapsed_time,60.0),
                   user_time, (long) getpid(), srcname, function, line,
                   domain, severity, event);
      OutputDebugString(buffer);
    }
  if (log_info->output_type & Win32EventlogOutput)
    {
#define LOGGING_ERROR_CODE 0
      char
        buffer[MaxTextExtent];

      LPCSTR
        szList[1];

      HANDLE
        hSource;

      FormatString(buffer,
                   "%.1024s %ld:%-9.6f %0.3f %ld %.1024s %.1024s %lu %.1024s"
                   " %.1024s %.1024s\n",
                   timestamp, (long) (elapsed_time/60.0),
                   fmod(elapsed_time,60.0),
                   user_time, (long) getpid(), srcname, function, line,
                   domain, severity, event);
      hSource = RegisterEventSource(NULL, MagickPackageName);
      if (hSource != NULL)
        {
          szList[0]=buffer;
          ReportEvent(hSource,nteventtype,0,LOGGING_ERROR_CODE,NULL,1,0,szList,NULL);
          DeregisterEventSource(hSource);
        }
    }
#endif
  if ((((unsigned int) log_info->output_type) & StdoutOutput) ||
      (((unsigned int) log_info->output_type) & StderrOutput))
    {
      FILE
        *file;
      /*
        Log to stdout in a "human readable" format.
      */
      file = stdout;
      if (((unsigned int) log_info->output_type) & StderrOutput)
        file = stderr;
      for (p=log_info->format; *p != '\0'; p++)
      {
        /*
          Process formatting characters in text.
        */
        if ((*p == '\\') && (*(p+1) == 'r'))
          {
            (void) fprintf(file,"\r");
            p++;
            continue;
          }
        if ((*p == '\\') && (*(p+1) == 'n'))
          {
            (void) fprintf(file,"\n");
            p++;
            continue;
          }
        if (*p != '%')
          {
            (void) fprintf(file,"%c",*p);
            continue;
          }
        p++;
        switch (*p)
        {
          case 'd':
          {
            (void) fprintf(file,"%.1024s",domain);
            break;
          }
          case 'e':
          {
            (void) fprintf(file,"%.1024s",event);
            break;
          }
          case 'f':
          {
            (void) fprintf(file,"%.1024s",function);
            break;
          }
          case 'l':
          {
            (void) fprintf(file,"%lu",line);
            break;
          }
          case 'm':
          {
            register const char
              *lp;

            for (lp=srcname+strlen(srcname)-1; lp > srcname; lp--)
              if (*lp == *DirectorySeparator)
                {
                  lp++;
                  break;
                }
            (void) fprintf(file,"%.1024s",lp);
            break;
          }
          case 'p':
          {
            (void) fprintf(file,"%ld",(long) getpid());
            break;
          }
          case 'r':
          {
            (void) fprintf(file,"%ld:%-9.6f",(long) (elapsed_time/60.0),
              fmod(elapsed_time,60.0));
            break;
          }
          case 's':
          {
            (void) fprintf(file,"%.1024s",severity);
            break;
          }
          case 't':
          {
            (void) fprintf(file,"%02d:%02d:%02d",time_meridian->tm_hour,
              time_meridian->tm_min,time_meridian->tm_sec);
            break;
          }
          case 'u':
          {
            (void) fprintf(file,"%0.3fu",user_time);
            break;
          }
          default:
          {
            (void) fprintf(file,"%%");
            (void) fprintf(file,"%c",*p);
            break;
          }
        }
      }
      (void) fprintf(file,"\n");
      (void) fflush(file);
    }
  if ((log_info->output_type & MethodOutput) &&
      (log_info->method != (LogMethod) NULL))
    {
      char
        buffer[MaxTextExtent];

      FormatString(buffer,
                   "%.1024s %ld:%-9.6f %0.3f %ld %.1024s %.1024s %lu %.1024s"
                   " %.1024s %.1024s\n",
                   timestamp, (long) (elapsed_time/60.0),
                   fmod(elapsed_time,60.0),
                   user_time, (long) getpid(), srcname, function, line,
                   domain, severity, event);
      log_info->method(type,buffer);
    }
  UnlockSemaphoreInfo(log_info->log_semaphore);
  return(True);
}
MagickExport unsigned int LogMagickEvent(const ExceptionType type,
  const char *module,const char *function,const unsigned long line,
  const char *format,...)
{
  unsigned int
    count;

  va_list
    operands;

  va_start(operands,format);
  count=LogMagickEventList(type, module, function, line, format, operands);
  va_end(operands);
  return (count);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d C o n f i g u r e F i l e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadLogConfigureFile() reads the log configuration file.
%
%  The format of the ReadLogConfigureFile method is:
%
%      unsigned int ReadLogConfigureFile(const char *basename,
%        const unsigned int depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o status: ReadLogConfigureFile() returns True if at least one log entry
%      is defined otherwise False.
%
%    o basename:  The log configuration filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickPassFail ReadLogConfigureFile(const char *basename,
  const unsigned int depth,ExceptionInfo *exception)
{
  char
    keyword[MaxTextExtent],
    path[MaxTextExtent],
    *q,
    *token,
    *xml;

  size_t
    length=0,
    token_max_length;

  MagickPassFail
    status=MagickPass;

  /*
    Read the log configure file.
  */
  (void) strlcpy(path,basename,sizeof(path));
  if (depth == 0)
    {
      /*
        Load top configuration file based on configure search path.
      */
      xml=(char *) GetConfigureBlob(basename,path,&length,exception);
    }
  else
    {
      /*
        Load subordinate configuration file based on path specified
        by parent configuration file.
      */
      xml=(char *) FileToBlob(basename,&length,exception);
    }
  if (xml == (char *) NULL)
    return MagickFail;
  token=AllocateString(xml);
  token_max_length=strlen(token);
  for (q=xml; *q != '\0'; )
  {
    /*
      Interpret Coder.
    */
    MagickGetToken(q,&q,token,token_max_length);
    if (*token == '\0')
      break;
    (void) strlcpy(keyword,token,MaxTextExtent);
    if (LocaleNCompare(keyword,"<!--",4) == 0)
      {
        /*
          Comment element.
        */
        while ((LocaleNCompare(q,"->",2) != 0) && (*q != '\0'))
          MagickGetToken(q,&q,token,token_max_length);
        continue;
      }
    if (LocaleCompare(keyword,"<include") == 0)
      {
        /*
          Include element.
        */
        while ((*token != '>') && (*q != '\0'))
        {
          (void) strlcpy(keyword,token,MaxTextExtent);
          MagickGetToken(q,&q,token,token_max_length);
          if (*token != '=')
            continue;
          MagickGetToken(q,&q,token,token_max_length);
          if (LocaleCompare(keyword,"file") == 0)
            {
              if (depth > 200)
                (void) fprintf(stderr,"%.1024s: <include /> nested too deeply",
                  path);
              else
                {
                  char
                    filename[MaxTextExtent];

                  GetPathComponent(path,HeadPath,filename);
                  if (*filename != '\0')
                    (void) strlcat(filename,DirectorySeparator,MaxTextExtent);
                  (void) strlcat(filename,token,MaxTextExtent);
                  status &= ReadLogConfigureFile(filename,depth+1,exception);
                  if (status != MagickPass)
                    {
                      MagickFreeMemory(token);
                      MagickFreeMemory(xml);
                      return (status);
                    }
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<magicklog>") == 0)
      {
        (void) strlcpy(log_info->path,path,sizeof(log_info->path));
        continue;
      }
    MagickGetToken(q,(char **) NULL,token,token_max_length);
    if (*token != '=')
      continue;
    MagickGetToken(q,&q,token,token_max_length);
    MagickGetToken(q,&q,token,token_max_length);
    switch (*keyword)
    {
      case 'E':
      case 'e':
      {
        if (LocaleCompare((char *) keyword,"events") == 0)
          log_info->events = (LogEventType) ((unsigned int) log_info->events |
                                             (unsigned int) ParseEvents(token));
        break;
      }
      case 'F':
      case 'f':
      {
        if (LocaleCompare((char *) keyword,"filename") == 0)
          {
            (void) strlcpy(log_info->filename,token,sizeof(log_info->filename));
            break;
          }
        if (LocaleCompare((char *) keyword,"format") == 0)
          {
            (void) strlcpy(log_info->format,token,sizeof(log_info->format));
            break;
          }
        break;
      }
      case 'G':
      case 'g':
      {
        if (LocaleCompare((char *) keyword,"generations") == 0)
          {
            log_info->generations=MagickAtoL(token);
            break;
          }
        break;
      }
      case 'L':
      case 'l':
      {
        if (LocaleCompare((char *) keyword,"limit") == 0)
          {
            log_info->limit=MagickAtoL(token);
            break;
          }
        break;
      }
      case 'O':
      case 'o':
      {
       if (LocaleCompare((char *) keyword,"output") == 0)
          {
            unsigned int i;

            for (i=0; i < ArraySize(output_map); i++)
            {
              if (LocaleNCompare(token,output_map[i].name,
                                 output_map[i].name_len) == 0)
                {
                  /* We do not OR these flags despite the fact that
                     they are bit masks because they are still
                     mutually exclusive implementations. Asking for
                     XML and TXT format files each use the file handle
                     field and others to do their work, so they can
                     not be used together */

                  LockSemaphoreInfo(log_info->log_semaphore);

                  log_info->output_type=output_map[i].mask;

                  UnlockSemaphoreInfo(log_info->log_semaphore);

                  break;
                }
            }
            break;
          }
        break;
      }
      default:
        break;
    }
  }
  MagickFreeMemory(token);
  MagickFreeMemory(xml);

  if ((depth == 0) && (status == MagickPass))
    log_info->log_configured=True;

  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t L o g E v e n t M a s k                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogEventMask() accepts a comma-delimited list that determines which
%  events to log.  All other events are ignored.  By default, no logging is
%  enabled.  This method returns the updated log event mask.
%
%  The format of the AcquireString method is:
%
%      unsigned long SetLogEventMask(const char *events)
%
%  A description of each parameter follows:
%
%    o events: log these events.
%
%
*/
MagickExport unsigned long SetLogEventMask(const char *events)
{
  LogEventType
    event_flags=NoEventsMask;

  LockSemaphoreInfo(log_info->log_semaphore);

  if (events != NULL)
    {
      event_flags=ParseEvents(events);
      log_info->events=event_flags;
    }

  event_flags=log_info->events;

  UnlockSemaphoreInfo(log_info->log_semaphore);

  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
                        "Set log event mask: %s", events ? events : "None");

  return (event_flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t L o g F o r m a t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogFormat() sets the format for the "human readable" log record.
%
%  The format of the LogMagickFormat method is:
%
%      SetLogFormat(const char *format)
%
%  A description of each parameter follows:
%
%    o format: The log record format.
%
%
*/
MagickExport void SetLogFormat(const char *format)
{
  LockSemaphoreInfo(log_info->log_semaphore);

  (void) strlcpy(log_info->format,format,sizeof(log_info->format));

  UnlockSemaphoreInfo(log_info->log_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t L o g M e t h o d                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogMethod() sets the method that should be called when logging.
%
%  The format of the SetLogMethod method is:
%
%      SetLogFormat(LogMethod)
%
%  A description of each parameter follows:
%
%    o method: pointer to a method that will be called when LogMagickEvent is
%      being called.
%
%
*/
MagickExport void SetLogMethod(LogMethod method)
{
  LockSemaphoreInfo(log_info->log_semaphore);

  log_info->output_type=(LogOutputType) (log_info->output_type |
    MethodOutput);
  log_info->method=method;

  UnlockSemaphoreInfo(log_info->log_semaphore);
}
