<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [`gm-ocl`开发记录](#gm-ocl开发记录)
    - [<2022-02-24 Thu>](#2022-02-24-thu)
        - [增加`--enable-opencl`参数](#增加--enable-opencl参数)
    - [<2022-02-25 Fri>](#2022-02-25-fri)
        - [关于`AccelerateResizeImage()`的链接问题](#关于accelerateresizeimage的链接问题)
    - [<2022-03-02 Wed>](#2022-03-02-wed)
        - [在`archlinux`上为`Intel`启用`OpenCL`](#在archlinux上为intel启用opencl)
        - [调试`RunOpenCLBenchmark()`时的崩溃问题](#调试runopenclbenchmark时的崩溃问题)
    - [<2022-03-07 Mon>](#2022-03-07-mon)
        - [调用`clCreateBuffer()`产生异常问题（一）](#调用clcreatebuffer产生异常问题一)
        - [`vscode`环境](#vscode环境)
        - [调用`clCreateBuffer()`产生异常问题（二）](#调用clcreatebuffer产生异常问题二)
    - [<2022-03-09 Wed>](#2022-03-09-wed)
        - [调用`clCreateBuffer()`产生异常问题（三）](#调用clcreatebuffer产生异常问题三)
    - [<2022-03-15 Tue>](#2022-03-15-tue)
        - [调用`clCreateBuffer()`产生异常问题（四）](#调用clcreatebuffer产生异常问题四)
        - [调用`clCreateBuffer()`产生异常问题（五）](#调用clcreatebuffer产生异常问题五)
    - [<2022-03-16 Wed>](#2022-03-16-wed)
        - [调用`clCreateBuffer()`产生异常问题（六）](#调用clcreatebuffer产生异常问题六)
    - [<2022-03-17 Thu>](#2022-03-17-thu)
        - [关于`IM`中的`number_channels`成员（一）](#关于im中的number_channels成员一)
    - [<2022-03-18 Fri>](#2022-03-18-fri)
        - [关于`gpuSupportedResizeWeighting()`的代码能否省略](#关于gpusupportedresizeweighting的代码能否省略)
    - [<2022-03-26 Sat>](#2022-03-26-sat)
        - [又一个闪退问题](#又一个闪退问题)
        - [对`IM`的`number_channels`及`PixelChannelMap`结构体中的`channel`和`offset`成员的理解](#对im的number_channels及pixelchannelmap结构体中的channel和offset成员的理解)
    - [<2022-03-28 Mon>](#2022-03-28-mon)
        - [关于`IM`中的`number_channels`成员（二）](#关于im中的number_channels成员二)
    - [<2022-03-29 Tue>](#2022-03-29-tue)
        - [对`number_channels`的处理](#对number_channels的处理)
    - [<2022-03-30 Wed>](#2022-03-30-wed)
        - [一个低级错误引发的`core dumped`](#一个低级错误引发的core-dumped)
        - [关于`error: use of type 'double' requires cl_khr_fp64 support`错误](#关于error-use-of-type-double-requires-cl_khr_fp64-support错误)
        - [关于`IM`中`resizeHorizontalFilter()`中的`scale`变量](#关于im中resizehorizontalfilter中的scale变量)
    - [<2022-03-31 Thu>](#2022-03-31-thu)
        - [在核函数中使用`GM`的计算代码](#在核函数中使用gm的计算代码)
    - [<2022-04-01 Fri>](#2022-04-01-fri)
        - [关于`AcquireCriticalMemory()`函数的异常处理（一）](#关于acquirecriticalmemory函数的异常处理一)
    - [<2022-04-02 Sat>](#2022-04-02-sat)
        - [关于`AcquireCriticalMemory()`函数的异常处理（二）](#关于acquirecriticalmemory函数的异常处理二)
        - [关于`LiberateMagickResource()`的闪退问题（一）](#关于liberatemagickresource的闪退问题一)
    - [<2022-04-03 Sun>](#2022-04-03-sun)
        - [关于`LiberateMagickResource()`的闪退问题（二）](#关于liberatemagickresource的闪退问题二)
    - [<2022-04-06 Wed>](#2022-04-06-wed)
        - [关于`LiberateMagickResource()`的闪退问题（三）](#关于liberatemagickresource的闪退问题三)
    - [<2022-04-07 Thu>](#2022-04-07-thu)
        - [`gm benchmark`比较](#gm-benchmark比较)
    - [<2022-04-10 Sun>](#2022-04-10-sun)
        - [关于`ThrowMagickException()`的替代](#关于throwmagickexception的替代)
    - [<2022-04-11 Mon>](#2022-04-11-mon)
        - [关于`*_utf8`系列函数](#关于_utf8系列函数)
        - [关于`lt_dlclose()`函数](#关于lt_dlclose函数)
    - [<2022-04-12 Tue>](#2022-04-12-tue)
        - [关于`-lltdl`链接选项（一）](#关于-lltdl链接选项一)
        - [关于`-lltdl`链接选项（二）](#关于-lltdl链接选项二)
    - [<2022-04-13 Wed>](#2022-04-13-wed)
        - [关于`-lltdl`链接选项（三）](#关于-lltdl链接选项三)
        - [支持`windows`](#支持windows)

<!-- markdown-toc end -->

# `gm-ocl`开发记录

## <2022-02-24 Thu>

### 增加`--enable-opencl`参数

拷贝`ImageMagick`中的`m4/ax_have_opencl.m4`，在`configure.ac`中添加：

``` shell
# Enable support for OpenCL
no_cl=yes
AX_HAVE_OPENCL([C])
if test "X$no_cl" != 'Xyes'; then
  MAGICK_FEATURES="OpenCL $MAGICK_FEATURES"
fi
```

在根目录下运行`autoreconf -vif`重新生成`configure`，但出现错误：

``` shellsession
$ autoreconf -vif
autoreconf: Entering directory `.'
autoreconf: configure.ac: not using Gettext
autoreconf: running: aclocal --force -I m4
autoreconf: configure.ac: tracing
autoreconf: running: libtoolize --copy --force
libtoolize: putting auxiliary files in AC_CONFIG_AUX_DIR, 'config'.
libtoolize: copying file 'config/ltmain.sh'
libtoolize: putting macros in AC_CONFIG_MACRO_DIRS, 'm4'.
libtoolize: copying file 'm4/libtool.m4'
libtoolize: copying file 'm4/ltoptions.m4'
libtoolize: copying file 'm4/ltsugar.m4'
libtoolize: copying file 'm4/ltversion.m4'
libtoolize: copying file 'm4/lt~obsolete.m4'
autoreconf: running: /usr/bin/autoconf --force
configure.ac:1057: error: possibly undefined macro: AC_MSG_RESULT
      If this token and others are legitimate, please use m4_pattern_allow.
      See the Autoconf documentation.
autoreconf: /usr/bin/autoconf failed with exit status: 1
```

试了比如安装`pkg-config`，`autoconf-archive`都不能解决，开发环境`archlinux`的`autoconf`的版本是`2.71`，换到了`ubuntu`的`2.69`的环境下也不行。

``` shellsession
$ lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.1 LTS
Release:	20.04
Codename:	focal
$ autoconf --version
autoconf (GNU Autoconf) 2.69
Copyright (C) 2012 Free Software Foundation, Inc.
License GPLv3+/Autoconf: GNU GPL version 3 or later
<http://gnu.org/licenses/gpl.html>, <http://gnu.org/licenses/exceptions.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written by David J. MacKenzie and Akim Demaille.
```

可以将`ax_have_opencl.m4`的所有`AC_MSG_RESULT`全部删除（注：注释掉`AC_MSG_RESULT`的话还会出现其它错误导致不能生成`configure`），虽然能成功生成`configure`，但是运行`./configure`时会出现如下错误：

``` shellsession
$ ./configure
...
...
checking for gcc option to support OpenMP... -fopenmp
./configure: line 7812: syntax error near unexpected token `OpenCL,'
./configure: line 7812: `        AX_CHECK_FRAMEWORK(OpenCL,'
```

临时解决的方法是将`configure`中的`AX_CHECK_FRAMEWORK(...)`的代码段删除。

`AX_CHECK_FRAMEWORK`的答案来自：

``` shellsession
$ grep -rn 'AX_CHECK_FRAMEWORK'
ImageMagick/m4/ax_check_framework.m4:6:dnl @synopsis AX_CHECK_FRAMEWORK(framework, [action-if-found], [action-if-not-found])
ImageMagick/m4/ax_check_framework.m4:15:AC_DEFUN([AX_CHECK_FRAMEWORK],[
ImageMagick/m4/ax_have_opencl.m4:94:        AX_CHECK_FRAMEWORK([OpenCL], [
```

即，将`ImageMagick`的`m4/ax_check_framework.m4`拷贝过来即可，惊奇的是`AX_CHECK_FRAMEWORK`的问题解决，之前`AC_MSG_RESULT`的问题同时也迎刃而解。

可以删除`configure`文件后再运行`autoreconf`来代替`autoreconf -vif`，这样生成的`configure`可以最大限度的与原`configure`保持一致。

## <2022-02-25 Fri>

### 关于`AccelerateResizeImage()`的链接问题

因为增加了两个新文件`accelerate-private.h`和`accelerate.c`，`AccelerateResizeImage()`就位于其中，因为`magick/Makefile.am`中没有增加这两个文件，所以在`autoreconf`时生成的`Makefile.in`就不知道新增了两个文件，因此在`configure`时生成的`Makefile`中就不会编译`AccelerateResizeImage()`函数，因此出现的链接问题。

``` shellsession
$ rm configure
$ autoreconf
configure.ac:119: warning: AM_INIT_AUTOMAKE: two- and three-arguments forms are deprecated.  For more info, see:
configure.ac:119: https://www.gnu.org/software/automake/manual/automake.html#Modernize-AM_005fINIT_005fAUTOMAKE-invocation
```

## <2022-03-02 Wed>

### 在`archlinux`上为`Intel`启用`OpenCL`

``` shellsession
% lspci | grep VGA
00:02.0 VGA compatible controller: Intel Corporation Iris Plus Graphics G1 (Ice Lake) (rev 07)
% clinfo
Number of platforms                               0
% sudo pacman -S intel-compute-runtime
```

### 调试`RunOpenCLBenchmark()`时的崩溃问题

发现直接运行`gm display`没有崩溃问题，调试时却经常发生，有时`SIGABRT`，有时`SIGSEGV`，猜测可能是同步问题：

``` c++
/*
  We need this to get a proper performance benchmark, the operations
  are executed asynchronous.
*/
if (is_cpu == MagickFalse)
  {
    CacheInfo
      *cache_info;

    MagickCLCacheInfo
      cl_info;

    cache_info=(CacheInfo *) resizedImage->cache;
    cl_info=GetCacheInfoOpenCL(cache_info);
    if (cl_info != (MagickCLCacheInfo) NULL)
      openCL_library->clWaitForEvents(cl_info->event_count,
        cl_info->events);
  }

if (i > 0)
  StopAccelerateTimer(&timer);

if (bluredImage != (Image *) NULL)
  DestroyImage(bluredImage);
if (unsharpedImage != (Image *) NULL)
  DestroyImage(unsharpedImage);
if (resizedImage != (Image *) NULL)
  DestroyImage(resizedImage);
```

经常遇到的是：`resizedImage`为`0`（`SIGSEGV`），`DestroyImage(resizedImage);`（`SIGABRT`）。

## <2022-03-07 Mon>

### 调用`clCreateBuffer()`产生异常问题（一）

在`vscode`上的堆栈输出如下：

``` text
libc.so.6!__pthread_kill_implementation (Unknown Source:0)
libc.so.6!raise (Unknown Source:0)
libc.so.6!abort (Unknown Source:0)
libigdrcl.so![Unknown/Just-In-Time compiled code] (Unknown Source:0)
libOpenCL.so!clCreateBuffer (Unknown Source:0)
AcquireMagickCLCacheInfo(MagickCLDevice device, Quantum * pixels, const magick_int64_t length) (gm-ocl/magick/opencl.c:569)
GetAuthenticOpenCLBuffer(const Image * image, MagickCLDevice device, ExceptionInfo * exception) (gm-ocl/magick/pixel_cache.c:5252)
```

因为堆栈显示问题最终是出在`__pthread_kill_implementation`上，所以我的调查方向一直在线程同步上，代码调试了一遍又一遍却始终没有找到问题。心灰意冷并已经产生从头再开始的想法了。

注意到：

``` text
libigdrcl.so![Unknown/Just-In-Time compiled code] (Unknown Source:0)
```

谷歌了下`libigdrcl.so`，难道是安装`intel`驱动的问题？目前使用的是`intel-compute-runtime`，现改为`intel-opencl-runtime`，参考自：“[GPGPU - ArchWiki](https://wiki.archlinux.org/title/GPGPU)”。

``` shellsession
$ sudo pacman -Rns intel-compute-runtime
$ yay -S intel-opencl-runtime
```

我对比了`intel-compute-runtime`和`intel-opencl-runtime`的`clinfo`输出差异发现`intel-compute-runtime`支持`GPU`，`intel-opencl-runtime`支持`CPU`。

那代码到底有没有问题？`clCreateBuffer()`调用失败的原因要不要深究？

### `vscode`环境

``` json
// tasks.json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build with opencl",
            "type": "shell",
            "command": "make",
            "problemMatcher": [],
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```

``` json
// launch.json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/utilities/gm",
            "args": ["display", "~/temp/bg1a.jpg"],
            "stopAtEntry": false,
            "cwd": "${fileDirname}",
            "environment": [
                {
                    "name": "MAGICK_OCL_DEVICE",
                    "value": "true"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description":  "Set Disassembly Flavor to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                }
            ]
        }

    ]
}
```

正在用的`dev.sh`内容如下：

``` shell
#!/bin/bash

./configure CFLAGS='-g -O0' LDFLAGS='-ldl' --enable-opencl --prefix=$HOME/usr/local
```

### 调用`clCreateBuffer()`产生异常问题（二）

使用`MAGICK_OCL_DEVICE=GPU`且在已经安装了`opencl-compute-runtime`的情况下会产生两个问题：

1. `gm`运行卡死，无法操作，`CPU`使用率居高不下，或者
2. `gm`运行崩溃，产生如下提示：

``` shellsession
$ gm display ~/temp/bg1a.jpg
Abort was called at 250 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp
Aborted (core dumped)
```

## <2022-03-09 Wed>

### 调用`clCreateBuffer()`产生异常问题（三）

我在这里找到了一些有用的信息：“[crash in `NEO::DrmAllocation::makeBOsResident` or in `checkAllocationsForOverlapping` when using more than one opencl block in gnuradio gr-clenabled](https://github.com/intel/compute-runtime/issues/409)”。

下载了`compute-runtime-22.09.22577`的源代码：

``` c++
// compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp

OsHandleStorage HostPtrManager::prepareOsStorageForAllocation(MemoryManager &memoryManager, size_t size, const void *ptr, uint32_t rootDeviceIndex) {
    std::lock_guard<decltype(allocationsMutex)> lock(allocationsMutex);
    auto requirements = HostPtrManager::getAllocationRequirements(rootDeviceIndex, ptr, size);
    UNRECOVERABLE_IF(checkAllocationsForOverlapping(memoryManager, &requirements) == RequirementsStatus::FATAL);
    auto osStorage = populateAlreadyAllocatedFragments(requirements);
    if (osStorage.fragmentCount > 0) {
        if (memoryManager.populateOsHandles(osStorage, rootDeviceIndex) != MemoryManager::AllocationStatus::Success) {
            memoryManager.cleanOsHandles(osStorage, rootDeviceIndex);
            osStorage.fragmentCount = 0;
        }
    }
    return osStorage;
}
```

`host_ptr_manager.cpp:250`就是：

``` c++
UNRECOVERABLE_IF(checkAllocationsForOverlapping(memoryManager, &requirements) == RequirementsStatus::FATAL);
```

参考链接的：

> Is flag CL_USE_HOST_PTR used to create buffers?

> Are such buffers mapped (clEnqueueMapBuffer) and returned pointers used to create other buffers? Or passed as a ptr to EnqueueReadWriteBuffer/Image() calls?

我加了`pixels`指针的输出发现：

``` shellsession
$ gm display ~/temp/bg1a.jpg
14:45:53 0:1.237436  0.740u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dc333c70
14:45:53 0:1.244275  0.800u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648db4beac0
14:45:53 0:1.334968  1.370u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dd446b00
14:45:53 0:1.336437  1.390u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648db646ae0
14:45:53 0:1.432968  2.040u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dd446b00
14:45:53 0:1.433129  2.060u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648db646ae0
14:45:53 0:1.544831  2.780u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648de446b50
14:45:53 0:1.544873  2.790u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dc762150
14:45:53 0:1.659341  3.630u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648df446be0
14:45:53 0:1.659420  3.630u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dd935030
14:45:53 0:1.778589  4.520u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648df446be0
14:45:53 0:1.778667  4.530u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dd935030
14:45:54 0:2.242840  8.140u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x7f85b9757010
14:45:54 0:2.247728  8.200u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648dc8faa00
14:45:54 0:2.256726  8.280u 69908 opencl.c/CopyMagickCLCacheInfo/1552/User:
  clEnqueueMapBuffer return pixels: 0x5648dc8faa00
14:46:12 0:20.409439 8.320u 69908 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x5648db646ae0
Abort was called at 250 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp
Aborted (core dumped)
```

发现`0x5648db646ae0`指针在第三次调用`clCreateBuffer`时崩溃。怎么再次崩溃时的输出不一样：

``` shellsession
$ gm display ~/temp/bg1a.jpg
15:09:02 0:1.357361  1.150u 71516 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55728b02e720
15:09:02 0:1.363826  1.210u 71516 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55728a1b9570
15:09:02 0:1.460255  1.810u 71516 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55728ce2e740
15:09:02 0:1.461296  1.830u 71516 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55728b341620
15:09:02 0:1.552602  2.460u 71516 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55728c1415d0
Abort was called at 250 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp
Aborted (core dumped)
```

这次没有崩溃，但是却发现`clEnqueueMapBuffer`返回的指针被`clCreateBuffer`创建缓存却没有崩溃：

``` shellsession
$ gm display ~/temp/bg1a.jpg
15:11:13 0:1.283573  1.150u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf5096990
15:11:13 0:1.289888  1.200u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf4161c00
15:11:13 0:1.385760  1.820u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf6296b20
15:11:13 0:1.392014  1.850u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf4161c00
15:11:14 0:1.479689  2.450u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf6296b20
15:11:14 0:1.485056  2.470u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf4161c00
15:11:14 0:1.581975  3.120u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf7296b60
15:11:14 0:1.582017  3.120u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf528a550
15:11:14 0:1.700207  4.010u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf8196bc0
15:11:14 0:1.700267  4.010u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf528a550
15:11:14 0:1.816203  4.880u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf8196bc0
15:11:14 0:1.816260  4.880u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf528a550
15:11:14 0:2.285724  8.520u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x7fe42d322010
15:11:14 0:2.290441  8.550u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf54eb600
15:11:14 0:2.300859  8.630u 71783 opencl.c/CopyMagickCLCacheInfo/1552/User:
  clEnqueueMapBuffer return pixels: 0x55dcf54eb600
15:11:25 0:13.121552 8.660u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf5570380
15:11:25 0:13.135136 8.670u 71783 opencl.c/CopyMagickCLCacheInfo/1552/User:
  clEnqueueMapBuffer return pixels: 0x55dcf5570380
15:11:25 0:13.291565 8.680u 71783 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer pixels: 0x55dcf54eb600
15:11:25 0:13.300287 8.690u 71783 opencl.c/CopyMagickCLCacheInfo/1552/User:
  clEnqueueMapBuffer return pixels: 0x55dcf54eb600
```

这跟上面链接的说法不符呀！难道是因为`clEnqueueMapBuffer`中没有使用`CL_USE_HOST_PTR`标记？所以不存在上面所说的这个问题？

怪事儿，我为什么没有在`cl.h`里找到`CL_USE_HOST_PTR`的定义？

在`IM`上添加了相同的输出代码，表现和上面的类似，但`IM`却能工作的很好，看来得从其它方法再入手了。

注：`IM`的日志配置文件：`usr/local/etc/ImageMagick-7/log.xml`。

## <2022-03-15 Tue>

### 调用`clCreateBuffer()`产生异常问题（四）

还是因为内存重叠的原因：我在`opencl.c`的`AcquireMagickCLCacheInfo()`函数中调用`clCreateBuffer()`之前添加了如下的输出代码：

``` c++
LogMagickEvent(UserEvent, GetMagickModule(),
  "clCreateBuffer - req: %d, pixels: %p, len: %d",
  device->requested, pixels, length);
```

当出现问题时有如下`log`：

``` shellsession
[ysouyno@arch gm-ocl]$ gm display ~/temp/bg1a.jpg
10:51:31 0:1.368105  1.030u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 1, pixels: 0x55d124b58490, len: 15728640
10:51:31 0:1.374360  1.060u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 2, pixels: 0x55d123c23730, len: 1536000
10:51:31 0:1.469657  1.670u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 2, pixels: 0x55d125c58510, len: 15728640
10:51:31 0:1.475944  1.720u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 3, pixels: 0x55d123c23730, len: 1536000
10:51:31 0:1.566470  2.310u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 3, pixels: 0x55d125c58510, len: 15728640
10:51:31 0:1.571902  2.360u 28955 opencl.c/AcquireMagickCLCacheInfo/569/User:
  clCreateBuffer - req: 4, pixels: 0x55d124b23740, len: 1536000
Abort was called at 250 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp
Aborted (core dumped)
```

注意看第一行和最后一行的输出发现，最后一行的地址加偏移已经大于第一行的地址，说明此时内存重叠，所以出现`clCreateBuffer()`的调用崩溃。这个问题有点难解了，涉及要修改整个`GM`的内存布局？

``` emacs-lisp
(> (+ #x55d124b23740 1536000) #x55d124b58490)
```

### 调用`clCreateBuffer()`产生异常问题（五）

在前一篇的基础上继续分析，因为`clCreateBuffer()`返回的地址即`GetAuthenticOpenCLBuffer()`的返回值（它在`ComputeResizeImage()`函数中被调用），当`ComputeResizeImage()`结束时，调用`clReleaseMemObject()`将会减少该内存计数，当计数为`0`时`clCreateBuffer()`创建的内存才被释放。

为了打印内存的引用计数，增加了`clGetMemObjectInfo()`函数：

``` c++
// opencl-private.h

typedef CL_API_ENTRY cl_int
  (CL_API_CALL *MAGICKpfn_clGetMemObjectInfo)(cl_mem memobj,
    cl_mem_info param_name,size_t param_value_size,void *param_value,
    size_t *param_value_size_ret)
    CL_API_SUFFIX__VERSION_1_0;

MAGICKpfn_clGetMemObjectInfo clGetMemObjectInfo;
```

比如`ReleaseOpenCLMemObject()`函数被改成了：

``` c++
MagickPrivate void ReleaseOpenCLMemObject(cl_mem memobj)
{
  cl_uint refcnt=0;
  openCL_library->clGetMemObjectInfo(memobj, CL_MEM_REFERENCE_COUNT, sizeof(cl_uint), &refcnt, NULL);
  LogMagickEvent(UserEvent, GetMagickModule(),
    "b4 ReleaseOpenCLMemObject(%p) refcnt: %d", memobj, refcnt);
  cl_int ret=openCL_library->clReleaseMemObject(memobj);
  openCL_library->clGetMemObjectInfo(memobj, CL_MEM_REFERENCE_COUNT, sizeof(cl_uint), &refcnt, NULL);
  LogMagickEvent(UserEvent, GetMagickModule(),
    "af ReleaseOpenCLMemObject(%p) refcnt: %d", memobj, refcnt);
}
```

我有输出如下：

``` shellsession
[ysouyno@arch gm-ocl]$ gm display ~/temp/bg1a.jpg
13:51:33 0:1.510679  1.480u 47963 opencl.c/AcquireMagickCLCacheInfo/576/User:
  clCreateBuffer -- req: 1, pixles: 0x5588e97712e0, len: 15728640
13:51:33 0:1.522527  1.570u 47963 opencl.c/AcquireMagickCLCacheInfo/584/User:
  clCreateBuffer return: 0x5588e774f5c0, refcnt: 1
13:51:33 0:1.522589  1.570u 47963 opencl.c/AcquireMagickCLCacheInfo/576/User:
  clCreateBuffer -- req: 2, pixles: 0x5588e88fc130, len: 1536000
13:51:33 0:1.524313  1.590u 47963 opencl.c/AcquireMagickCLCacheInfo/584/User:
  clCreateBuffer return: 0x5588e77064b0, refcnt: 1
13:51:33 0:1.528273  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e774f5c0) refcnt: 2
13:51:33 0:1.528351  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e774f5c0) refcnt: 1
13:51:33 0:1.528410  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e77064b0) refcnt: 2
13:51:33 0:1.528465  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e77064b0) refcnt: 1
13:51:33 0:1.528520  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 1
13:51:33 0:1.528582  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 1
13:51:34 0:1.740225  2.900u 47963 opencl.c/AcquireMagickCLCacheInfo/576/User:
  clCreateBuffer -- req: 2, pixles: 0x5588eb571300, len: 15728640
13:51:34 0:1.742399  2.910u 47963 opencl.c/AcquireMagickCLCacheInfo/584/User:
  clCreateBuffer return: 0x5588e87fead0, refcnt: 1
13:51:34 0:1.742615  2.910u 47963 opencl.c/AcquireMagickCLCacheInfo/576/User:
  clCreateBuffer -- req: 3, pixles: 0x5588e9a841e0, len: 1536000
13:51:34 0:1.744057  2.930u 47963 opencl.c/AcquireMagickCLCacheInfo/584/User:
  clCreateBuffer return: 0x5588e87feda0, refcnt: 1
13:51:34 0:1.745895  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 2
13:51:34 0:1.745964  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 1
13:51:34 0:1.746004  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e87feda0) refcnt: 2
13:51:34 0:1.746081  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e87feda0) refcnt: 1
13:51:34 0:1.746126  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e87756a0) refcnt: 1
13:51:34 0:1.746185  2.930u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e87756a0) refcnt: 1
13:51:34 0:1.946106  4.270u 47963 opencl.c/AcquireMagickCLCacheInfo/576/User:
  clCreateBuffer -- req: 3, pixles: 0x5588ea9841f0, len: 15728640
Abort was called at 250 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.09.22577/shared/source/memory_manager/host_ptr_manager.cpp
Aborted (core dumped)
```

测试地址重叠：

``` emacs-lisp
(> (+ #x5588ea9841f0 15728640) #x5588eb571300)
(- #x5588eb571300 #x5588ea9841f0)
```

解析下：最后一行`0x5588ea9841f0`调用`clCreateBuffer()`时崩溃，它的地址与`0x5588eb571300`重叠，而`0x5588eb571300`申请的`cl_mem`地址为：`0x5588e87fead0`，最后一次调用`ReleaseOpenCLMemObject()`后它的引用计数为`1`，这说明`0x5588eb571300`还没被释放而`0x5588ea9841f0`又开始申请造成内存重叠。

发现一处问题，上面输出中有如下：

``` shellsession
13:51:33 0:1.528520  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/509/User:
  b4 ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 1
13:51:33 0:1.528582  1.610u 47963 opencl.c/ReleaseOpenCLMemObject/513/User:
  af ReleaseOpenCLMemObject(0x5588e87fead0) refcnt: 1
```

`ReleaseOpenCLMemObject(0x5588e87fead0)`调用前后引用计数没有减少。难道`clReleaseMemObject()`调用失败了？

原来是因为当对象已经销毁后再调用`clGetMemObjectInfo()`将会返回`-38`的错误，即`CL_INVALID_MEM_OBJECT`。

## <2022-03-16 Wed>

### 调用`clCreateBuffer()`产生异常问题（六）

我可能解决了这个问题，将问题定位在了`RunOpenCLBenchmark()`的结尾`DestroyImage(resizedImage);`处，即在`DestroyCacheInfo()`中应该有清除`OpenCL`相关内存的代码。

## <2022-03-17 Thu>

### 关于`IM`中的`number_channels`成员（一）

在`IM`中`number_channels`成员出现频率有点高，经调试发现`IM`中图片对象初始化时通过调用`OpenPixelCache()`然后在`InitializePixelChannelMap()`中设置`number_channels`的值。这个函数的内部大量使用了`GM`中没有类型`PixelChannel`和`PixelTrait`，不太好把它给搬到`GM`中。

查看`PixelChannel`的定义发现了它的一个特点是：虽然它是`enum`类型，但每个成员都被指派了具体的值，且发现有多个成员共用一个值的情况。以此参照仍然没有在`GM`中找到类似定义，`PixelChannel`的定义：

<span id="pixel_channel_def"></span>

``` c++
typedef enum
{
  UndefinedPixelChannel = 0,
  RedPixelChannel = 0,
  CyanPixelChannel = 0,
  GrayPixelChannel = 0,
  LPixelChannel = 0,
  LabelPixelChannel = 0,
  YPixelChannel = 0,
  aPixelChannel = 1,
  GreenPixelChannel = 1,
  MagentaPixelChannel = 1,
  CbPixelChannel = 1,
  bPixelChannel = 2,
  BluePixelChannel = 2,
  YellowPixelChannel = 2,
  CrPixelChannel = 2,
  BlackPixelChannel = 3,
  AlphaPixelChannel = 4,
  IndexPixelChannel = 5,
  ReadMaskPixelChannel = 6,
  WriteMaskPixelChannel = 7,
  MetaPixelChannel = 8,
  CompositeMaskPixelChannel = 9,
  IntensityPixelChannel = MaxPixelChannels,  /* ???? */
  CompositePixelChannel = MaxPixelChannels,  /* ???? */
  SyncPixelChannel = MaxPixelChannels+1      /* not a real channel */
} PixelChannel;  /* must correspond to ChannelType */
```

我模仿`IM`的`InitializePixelChannelMap()`函数写了`calc_image_number_channels()`，虽然`number_channels`的值对于同一张测试图片`bg1a.jpg`来说均为`3`，但是在`IM`中值`3`显示正确，而在`GM`中`3 + 1`才能正确，所以我在`ComputeResizeImage()`中将`calc_image_number_channels()`的返回值加上了`1`：

``` c++
number_channels=(cl_uint) calc_image_number_channels(image)+1;
```

这只是临时方案，估计下面要更改抄过来的`kernel`函数。

## <2022-03-18 Fri>

### 关于`gpuSupportedResizeWeighting()`的代码能否省略

在`AccelerateResizeImage()`中有这样的一段代码被注释掉了：

``` c++
// if ((gpuSupportedResizeWeighting(GetResizeFilterWeightingType(
//        resizeFilter)) == MagickFalse) ||
//     (gpuSupportedResizeWeighting(GetResizeFilterWindowWeightingType(
//        resizeFilter)) == MagickFalse))
//   return((Image *) NULL);
```

我认为这段代码可以省略，因为它的目的就是为了检查`IM`的`ResizeFilter`类型中的`ResizeWeightingFunctionType`类型的成员值：

``` c++
struct _ResizeFilter
{
  double
    (*filter)(const double,const ResizeFilter *),
    (*window)(const double,const ResizeFilter *),
    support,        /* filter region of support - the filter support limit */
    window_support, /* window support, usally equal to support (expert only) */
    scale,          /* dimension scaling to fit window support (usally 1.0) */
    blur,           /* x-scale (blur-sharpen) */
    coefficient[7]; /* cubic coefficents for BC-cubic filters */

  ResizeWeightingFunctionType
    filterWeightingType,
    windowWeightingType;

  size_t
    signature;
};
```

是否在`supportedResizeWeighting`数组中：

``` c++
static const ResizeWeightingFunctionType supportedResizeWeighting[] =
{
  BoxWeightingFunction,
  TriangleWeightingFunction,
  HannWeightingFunction,
  HammingWeightingFunction,
  BlackmanWeightingFunction,
  CubicBCWeightingFunction,
  SincWeightingFunction,
  SincFastWeightingFunction,
  LastWeightingFunction
};
```

很明显这个数组就是`IM`支持`GPU`的窗函数集合。在`GM`中我只在`ResizeImage()`函数中找到相近的定义：

``` c++
static const FilterInfo
  filters[SincFilter+1] =
  {
    { Box, 0.0 },
    { Box, 0.0 },
    { Box, 0.5 },
    { Triangle, 1.0 },
    { Hermite, 1.0 },
    { Hanning, 1.0 },
    { Hamming, 1.0 },
    { Blackman, 1.0 },
    { Gaussian, 1.25 },
    { Quadratic, 1.5 },
    { Cubic, 2.0 },
    { Catrom, 2.0 },
    { Mitchell, 2.0 },
    { Lanczos, 3.0 },
    { BlackmanBessel, 3.2383 },
    { BlackmanSinc, 4.0 }
  };
```

明显可见，`GM`中处理相对于`IM`简单多了，所以上面的代码我仍然保持它被注释状态。

## <2022-03-26 Sat>

### 又一个闪退问题

有一个星期没碰了，今天突然发现：

``` shellsession
[ysouyno@arch ~]$ export MAGICK_OCL_DEVICE=true
[ysouyno@arch ~]$ gm display ~/temp/bg1a.jpg
Abort was called at 39 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.11.22682/shared/source/built_ins/built_ins.cpp
gm display: abort due to signal 6 (SIGABRT) "Abort"...
Aborted (core dumped)
[ysouyno@arch ~]$ clinfo
Abort was called at 39 line in file:
/build/intel-compute-runtime/src/compute-runtime-22.11.22682/shared/source/built_ins/built_ins.cpp
Aborted (core dumped)
```

连`clinfo`都运行不了了，看来肯定不是我代码的问题。发现`intel-compute-runtime`的版本也已经更新了。

### 对`IM`的`number_channels`及`PixelChannelMap`结构体中的`channel`和`offset`成员的理解

这个标题有点长，可能文章的内容也有点长，但是思路越来越清晰。先来看`PixelChannelMap`的结构体定义：

``` c++
typedef struct _PixelChannelMap
{
  PixelChannel
    channel;

  PixelTrait
    traits;

  ssize_t
    offset;
} PixelChannelMap;
```

`PixelChannelMap`在`IM`的`_Image`结构体中对应成员`channel_map`：

``` c++
PixelChannelMap
  *channel_map;
```

首先要说的是，`channel_map`在逐个计算像素的过程中非常重要，拿`IM`的`resize.c:HorizontalFilter()`函数为例：

``` c++
for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
{
  double
    alpha,
    gamma,
    pixel;

  PixelChannel
    channel;

  PixelTrait
    resize_traits,
    traits;

  ssize_t
    j;

  ssize_t
    k;

  channel=GetPixelChannelChannel(image,i);
  traits=GetPixelChannelTraits(image,channel);
  resize_traits=GetPixelChannelTraits(resize_image,channel);
  if ((traits == UndefinedPixelTrait) ||
      (resize_traits == UndefinedPixelTrait))
    continue;
  if (((resize_traits & CopyPixelTrait) != 0) ||
      (GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))
    {
      j=(ssize_t) (MagickMin(MagickMax(bisect,(double) start),(double)
        stop-1.0)+0.5);
      k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
        (contribution[j-start].pixel-contribution[0].pixel);
      SetPixelChannel(resize_image,channel,p[k*GetPixelChannels(image)+i],
        q);
      continue;
    }
  pixel=0.0;
  if ((resize_traits & BlendPixelTrait) == 0)
    {
      /*
        No alpha blending.
      */
      for (j=0; j < n; j++)
      {
        k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
          (contribution[j].pixel-contribution[0].pixel);
        alpha=contribution[j].weight;
        pixel+=alpha*p[k*GetPixelChannels(image)+i];
      }
      SetPixelChannel(resize_image,channel,ClampToQuantum(pixel),q);
      continue;
    }
  /*
    Alpha blending.
  */
  gamma=0.0;
  for (j=0; j < n; j++)
  {
    k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
      (contribution[j].pixel-contribution[0].pixel);
    alpha=contribution[j].weight*QuantumScale*
      GetPixelAlpha(image,p+k*GetPixelChannels(image));
    pixel+=alpha*p[k*GetPixelChannels(image)+i];
    gamma+=alpha;
  }
  gamma=PerceptibleReciprocal(gamma);
  SetPixelChannel(resize_image,channel,ClampToQuantum(gamma*pixel),q);
}
```

这个循环中的`GetPixelChannels()`函数就是返回`number_channels`的值：

``` c++
static inline size_t GetPixelChannels(const Image *magick_restrict image)
{
  return(image->number_channels);
}
```

即处理每个像素的所有`channel`，通过`GetPixelChannelChannel()`函数以通道的`offset`成员获得对应的`channel`：

``` c++
static inline PixelChannel GetPixelChannelChannel(
  const Image *magick_restrict image,const ssize_t offset)
{
  return(image->channel_map[offset].channel);
}
```

返回的`channel`是`PixelChannel`类型，这个定义在上面的文章中已经给出了，见：“[PixelChannel](#pixel_channel_def)”，看定义我之前还在奇怪为什么`enum`类型指定了好多相同的`0`值，`1`值，现在终于明白了。即比如`RGB`和`CMYK`两种形式`R`和`C`都是第`0`个通道，`G`和`M`都是第`1`个通道，依次类推。`CMYK`中的`K`就是`black`，在`PixelChannel`中对应的是`BlackPixelChannel`。

重点就是`SetPixelChannel()`这个函数：

``` c++
static inline void SetPixelChannel(const Image *magick_restrict image,
  const PixelChannel channel,const Quantum quantum,
  Quantum *magick_restrict pixel)
{
  if (image->channel_map[channel].traits != UndefinedPixelTrait)
    pixel[image->channel_map[channel].offset]=quantum;
}
```

这里忽略理解`traits`，最后一个参数`pixel`就是处理像素后的目标地址，代码中的`channel`是通过循环`i`获取的，`offset`是通过`channel`获取的，最终计算出了`pixel`的真正地址，然后将计算好的`quantum`赋值进去。

最后，`channel_map`中的`channel`和`offset`是在哪里初始化的？我对比了代码发现只在：

``` c++
static inline void SetPixelChannelAttributes(
  const Image *magick_restrict image,const PixelChannel channel,
  const PixelTrait traits,const ssize_t offset)
{
  if ((ssize_t) channel >= MaxPixelChannels)
    return;
  if (offset >= MaxPixelChannels)
    return;
  image->channel_map[offset].channel=channel;
  image->channel_map[channel].offset=offset;
  image->channel_map[channel].traits=traits;
}
```

而`SetPixelChannelAttributes()`只在`InitializePixelChannelMap()`函数中会被调用，`InitializePixelChannelMap()`这个函数有点熟悉，在之前了解`number_channels`的文章中做过了介绍，这个函数内部计算并初始化了`number_channels`的值。

我对`GM`和`IM`进行了比较，`GM`对`IM`进行了精简，代码：

``` c++
for (y=0; y < (long) destination->rows; y++)
  {
    double
      weight;

    DoublePixelPacket
      pixel;

    long
      j;

    register long
      i;

    pixel=zero;
    if ((destination->matte) || (destination->colorspace == CMYKColorspace))
      {
        double
          transparency_coeff,
          normalize;

        normalize=0.0;
        for (i=0; i < n; i++)
          {
            j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
              (contribution[i].pixel-contribution[0].pixel);
            weight=contribution[i].weight;
            transparency_coeff = weight * (1 - ((double) p[j].opacity/TransparentOpacity));
            pixel.red+=transparency_coeff*p[j].red;
            pixel.green+=transparency_coeff*p[j].green;
            pixel.blue+=transparency_coeff*p[j].blue;
            pixel.opacity+=weight*p[j].opacity;
            normalize += transparency_coeff;
          }
        normalize = 1.0 / (AbsoluteValue(normalize) <= MagickEpsilon ? 1.0 : normalize);
        pixel.red *= normalize;
        pixel.green *= normalize;
        pixel.blue *= normalize;
        q[y].red=RoundDoubleToQuantum(pixel.red);
        q[y].green=RoundDoubleToQuantum(pixel.green);
        q[y].blue=RoundDoubleToQuantum(pixel.blue);
        q[y].opacity=RoundDoubleToQuantum(pixel.opacity);
      }
    else
      {
        for (i=0; i < n; i++)
          {
            j=(long) (y*(contribution[n-1].pixel-contribution[0].pixel+1)+
                      (contribution[i].pixel-contribution[0].pixel));
            weight=contribution[i].weight;
            pixel.red+=weight*p[j].red;
            pixel.green+=weight*p[j].green;
            pixel.blue+=weight*p[j].blue;
          }
        q[y].red=RoundDoubleToQuantum(pixel.red);
        q[y].green=RoundDoubleToQuantum(pixel.green);
        q[y].blue=RoundDoubleToQuantum(pixel.blue);
        q[y].opacity=OpaqueOpacity;
      }

    if ((indexes != (IndexPacket *) NULL) &&
        (source_indexes != (IndexPacket *) NULL))
      {
        i=Min(Max((long) (center+0.5),start),stop-1);
        j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
          (contribution[i-start].pixel-contribution[0].pixel);
        indexes[y]=source_indexes[j];
      }
  }
```

从上面的代码可看出，在`GM`中只处理了`matte`通道或`CMYKColorsapce`，它们都是四通道的。但是看到代码中的`indexes`，不知道这个在`GM`中的意义及是否在`IM`中有相关对应。

可以在`GM`中通过搜索`indexes_valid`来找到一些有用的信息：

``` c++
/*
  Indexes are valid if the image storage class is PseudoClass or the
  colorspace is CMYK.
*/
cache_info->indexes_valid=((image->storage_class == PseudoClass) ||
                           (image->colorspace == CMYKColorspace));
```

原来是这样，这里的`PseudoClass`就是`pseudocolor`，可以在`wiki`的`Indexed color`中找到介绍，之所以叫做索引颜色，是因为为了节省内存或磁盘空间，颜色信息不是直接由图片的像素所携带，而是存放在一个单独的颜色表中或者调色板中。

从上面贴出来的`IM`和`GM`的代码对比发现，下面两段代码类似：

``` c++
// IM
if (((resize_traits & CopyPixelTrait) != 0) ||
    (GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))
  {
    j=(ssize_t) (MagickMin(MagickMax(bisect,(double) start),(double)
      stop-1.0)+0.5);
    k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
      (contribution[j-start].pixel-contribution[0].pixel);
    SetPixelChannel(resize_image,channel,p[k*GetPixelChannels(image)+i],
      q);
    continue;
  }
```

``` c++
// GM
if ((indexes != (IndexPacket *) NULL) &&
    (source_indexes != (IndexPacket *) NULL))
  {
    i=Min(Max((long) (center+0.5),start),stop-1);
    j=y*(contribution[n-1].pixel-contribution[0].pixel+1)+
      (contribution[i-start].pixel-contribution[0].pixel);
    indexes[y]=source_indexes[j];
  }
```

依然参考`InitializePixelChannelMap()`的代码，结合刚刚知道的`GM`对于`PseudoClass`和`CMYKColorspace`时`indexes`有效，在`IM`中则`CopyPixelTrait`对应`IndexPixelChannel`：

``` c++
if (image->colorspace == CMYKColorspace)
  SetPixelChannelAttributes(image,BlackPixelChannel,trait,n++);
if (image->alpha_trait != UndefinedPixelTrait)
  SetPixelChannelAttributes(image,AlphaPixelChannel,CopyPixelTrait,n++);
if (image->storage_class == PseudoClass)
  SetPixelChannelAttributes(image,IndexPixelChannel,CopyPixelTrait,n++);
if ((image->channels & ReadMaskChannel) != 0)
  SetPixelChannelAttributes(image,ReadMaskPixelChannel,CopyPixelTrait,n++);
if ((image->channels & WriteMaskChannel) != 0)
  SetPixelChannelAttributes(image,WriteMaskPixelChannel,CopyPixelTrait,n++);
if ((image->channels & CompositeMaskChannel) != 0)
  SetPixelChannelAttributes(image,CompositeMaskPixelChannel,CopyPixelTrait,
    n++);
```

不同的是`IM`中`CMYKColorspace`没有`CopyPixelTrait`特性。

小结一下：以目前的开发状态，将`IM`中的`CopyPixelTrait`与`GM`中的`indexes`对应起来。

## <2022-03-28 Mon>

### 关于`IM`中的`number_channels`成员（二）

在“[关于`IM`中的`number_channels`成员（一）](#关于im中的number_channels成员一)”的结尾提到将计算出来的`number_channels`值加`1`才能显示正确的图形，之前说它是临时方案，看来这次要将它变成永久的了。

``` c++
number_channels=(cl_uint) calc_image_number_channels(image)+1;
```

通过详细阅读`GM`和`IM`的`HorizontalFilter()`函数发现`IM`的最内层循环是通过：

``` c++
static inline size_t GetPixelChannels(const Image *magick_restrict image)
{
  return(image->number_channels);
}
```

来获得的，而`GM`中没有这么做，它比`IM`少了刚刚提到的这一层循环，改为固定设置四个通道的值。代码在前面的笔记中已给出，见：“[对`IM`的`number_channels`及`PixelChannelMap`结构体中的`channel`和`offset`成员的理解](#对im的number_channels及pixelchannelmap结构体中的channel和offset成员的理解)”。

我觉得这样的话反而简单了，我需要修改`kernel`函数和`GM`的自身处理相匹配，或者去掉`number_channels`成员，用固定值`4`代替？

`kernel`函数我相信现在修改它没什么难度，这两天翻来覆去的看`accelerate-kernels-private.h:ResizeVerticalFilter()`函数并和`HorizontalFilter()`进行对比理解，现在已经胸有成竹了。

## <2022-03-29 Tue>

### 对`number_channels`的处理

今天开始尝试对`number_channels`进行处理，我的想法是既然`number_channels`在`GM`中已经失去了它的意义，那倒不如直接把它删除掉，在它曾经出现过的地方用数字`4`代替。当然也不是所有`number_channels`的地方都要修改，要看具体情况而定。

首先，我从`IM`中重新拷贝了`accelerate-kernels-private.h`文件，因为之前那些的修改是基于错误的`number_channels`逻辑的。其次`number_channels`的传参部分不能删除，因为原`GM`中对于图片是否有透明通道或者是否是`CMYK`的格式有做判断，因此将它改为`matte_or_cmyk`来表明是否是四通道，`1`表示有，`0`表示没有，这样的话在`accelerate-kernels-private.h`的`ResizeHorizontalFilter()`函数中`alpha_index`就可以删除了。最后在`WriteAllChannels()`处的调用，必须以`4`传入，因为即使原图不是四通道，它的第四个通道也要赋值以保证图片计算的正确性。

经过这些修改之后测试图片显示正常。

## <2022-03-30 Wed>

### 一个低级错误引发的`core dumped`

当将图片不断缩小到宽高为`1x1`时会出现如下问题：

``` shellsession
gm: magick/image.c:1407: DestroyImage: Assertion `image->signature == MagickSignature' failed.
Aborted (core dumped)
```

这是因为在`ComputeResizeImage()`函数中当缩小到`1x1`时失败，`outputReady`为`0`导致`DestroyImage(filteredImage);`的调用，但是在销毁`filteredImage`后并没有将其赋`0`导致。

看了一下`GM`的源码：

``` c++
/*
  Free memory and set pointer to NULL
*/
#define MagickFreeMemory(memory) \
{ \
  void *_magick_mp=memory;      \
  MagickFree(_magick_mp);       \
  memory=0;                     \
}
```

这里明明将传入的指针赋`0`了。难道这段代码不起作用？

其实这个宏是有作用的，但要看怎么使用它。因为`DestroyImage()`是函数调用，实际上传入的指针是一个副本，将副本赋`0`并不影响原来的值，同时也要理解宏和函数调用的不同，这里有两种情况需要考虑：

有如下测试代码：

``` c++
#include <stdio.h>
#include <stdlib.h>

typedef void (*MagickFreeFunc)(void *ptr);
static MagickFreeFunc FreeFunc = free;

void MagickFree(void *memory) {
  if (memory != (void *)NULL)
    (FreeFunc)(memory);
}

#define MagickFreeMemory(memory)                \
  {                                             \
    printf("&memory: %p\n", &memory);           \
    printf(" memory: %p\n", memory);            \
    void *_magick_mp = memory;                  \
    MagickFree(_magick_mp);                     \
    memory = 0;                                 \
  }

void destroy_image(char *image) { MagickFreeMemory(image); }

int main() {
  char *image = (char *)malloc(1024);
  printf("&image : %p\n", &image);
  printf(" image : %p\n", image);
  destroy_image(image);
  printf("&image : %p\n", &image);
  printf(" image : %p\n", image);
  return 0;
}
```

这是`GM`中的代码使用方式，输出如下：

``` shellsession
% ./a.out
&image : 0x7ffc8a46dfb0
 image : 0x55711df782a0
&memory: 0x7ffc8a46df88
 memory: 0x55711df782a0
&image : 0x7ffc8a46dfb0
 image : 0x55711df782a0
```

这里指针并没有变化，因为是函数调用，如果将代码中的`destroy_image()`改为宏`MagickFreeFunc`，则是想要的效果：

``` c++
#include <stdio.h>
#include <stdlib.h>

typedef void (*MagickFreeFunc)(void *ptr);
static MagickFreeFunc FreeFunc = free;

void MagickFree(void *memory) {
  if (memory != (void *)NULL)
    (FreeFunc)(memory);
}

#define MagickFreeMemory(memory)                \
  {                                             \
    printf("&memory: %p\n", &memory);           \
    printf(" memory: %p\n", memory);            \
    void *_magick_mp = memory;                  \
    MagickFree(_magick_mp);                     \
    memory = 0;                                 \
  }

void destroy_image(char *image) { MagickFreeMemory(image); }

int main() {
  char *image = (char *)malloc(1024);
  printf("&image : %p\n", &image);
  printf(" image : %p\n", image);
  MagickFreeMemory(image);
  printf("&image : %p\n", &image);
  printf(" image : %p\n", image);
  return 0;
}
```

``` shellsession
% ./a.out
&image : 0x7ffd7247ae08
 image : 0x5572b59cf2a0
&memory: 0x7ffd7247ae08
 memory: 0x5572b59cf2a0
&image : 0x7ffd7247ae08
 image : (nil)
```

所以要想解决这个`core dumped`的问题，就老老实实地按照`GM`的代码风格，调用完`DestroyImage()`后再紧接着赋一次`0`。

### 关于`error: use of type 'double' requires cl_khr_fp64 support`错误

今天安装了最新的`intel-compute-runtime`，看来已经修复了`core dumped`问题，见：“[又一个闪退问题](#又一个闪退问题)”中提到的问题。

``` shellsession
% sudo pacman -Ss intel-compute-runtime
[sudo] password for ysouyno:
community/intel-compute-runtime 22.12.22749-1 [installed]
    Intel(R) Graphics Compute Runtime for oneAPI Level Zero and OpenCL(TM) Driver
```

试运行了一下我的最新代码，发现有`opencl`编译错误：

``` text
error: use of type 'double' requires cl_khr_fp64 support
```

先只是简单的将`double`换成`float`来解决这个问题。

### 关于`IM`中`resizeHorizontalFilter()`中的`scale`变量

分析`IM`的`accelerate.c:resizeHorizontalFilter()`的源代码发现它的`scale`变量计算后只停留在此函数内，并没有往下传递进`kernel`函数，关于`scale`的计算代码是不是多余的？从目前我理解到的`IM`的逻辑来看，我认为它是多余的。因为向下传递给`kernel`函数的是`resizeFilterScale`变量，这个变量的值不依赖`scale`变量，而是通过传参获取现有的结构体中的值，且它进入`kernel`函数`ResizeHorizontalFilter()`后通过调用`getResizeFilterWeight()`函数再以`filterType`获得计算函数来进一步计算`scale`值，进而最终返回`weight`值。

另外发现在`kernel`函数`ResizeHorizontalFilter()`的开始部分`scale`又被计算了一次，因此我觉得可以确认`accelerate.c:resizeHorizontalFilter()`中的`scale`变量是多余的。

我在`GM`中应该怎么处理呢？考虑到`GPU`并行运行的影响，`scale`的值不依赖各个`work-group`或`work-item`。因此我认为将`scale`赋值给`resizeFilterScale`传进`kernel`函数不会影响计算结果，那这样的话`kernel`函数中的`scale`计算就显得有点多余了。

备注：代码写着写着，发现个严重问题，`OpenCL`不支持函数指针，那怎么把过滤函数传进`kernel`函数呢？

## <2022-03-31 Thu>

### 在核函数中使用`GM`的计算代码

因为`OpenCL`不支持传递函数指针，所以增加了过滤函数的类型参数进行传参，涉及了一系列函数调用的参数修改。

在`resizeHorizontalFilter()`内部计算好`scale`的值，采用`GM`的计算方法，虽然它和`IM`的计算方法差不多。将`kernel`函数中的`scale`计算代码移除，同时核函数`ResizeHorizontalFilter()`的`support`也通过参数传入，它和`scale`一样，计算放在了`resizeHorizontalFilter()`中，另发现核函数`ResizeHorizontalFilter()`中的`resizeFilterBlur`变量已经不再使用。

所有修改见此次`commit`的上个`commit`，修改代码比较多，但愿没引出新的问题。

## <2022-04-01 Fri>

### 关于`AcquireCriticalMemory()`函数的异常处理（一）

不太好给`AcquireCriticalMemory()`添加异常处理，`GM`中定义好的内存分配失败的异常就那么几个，查找所有调用`AcquireCriticalMemory()`的地方，发现有给`StringInfo`，有给`ImageInfo`，还有给`MagickCLCacheInfo`等等分配内存的，在每个调用`AcquireCriticalMemory()`的地方抛出异常是可行的，可以使用`GM`中已定义好的异常类型，比如`StringInfo`可以用`UnableToAllocateString`来代替，`ImageInfo`可以用`UnableToAllocateImage`，`MagickCLCacheInfo`可能需要增加一个异常类型；或者在`AcquireCriticalMemory()`函数内部处理异常，这正是`IM`的处理方式，但是这样的话在`AcquireCriticalMemory()`内部不能明确表达出是哪种类型操作产生的异常。当然可以通过增加参数来解决，但是处理起来同样很麻烦。

目前我修改的函数是这样的，固定了它的类型为`UnableToAllocateModuleInfo`，没有把此修改放到源代码里，目前仅存在笔记里：

``` c++
MagickExport void *AcquireCriticalMemory(const size_t len)
{
  void
    *memory;

  // Fail if memory request cannot be fulfilled.
  memory=MagickMalloc(len);
  if (memory == (void *) NULL)
    MagickFatalError3(ResourceLimitFatalError,MemoryAllocationFailed,
      UnableToAllocateModuleInfo);
  return(memory);
}
```

## <2022-04-02 Sat>

### 关于`AcquireCriticalMemory()`函数的异常处理（二）

了解了一下`GM`的异常处理，可以这么来用：

``` c++
MagickExport void *AcquireCriticalMemory(const size_t len)
{
  void
    *memory;

  ExceptionInfo
    exception;

  GetExceptionInfo(&exception);

  // Fail if memory request cannot be fulfilled.
  memory=MagickMalloc(len);
  if (memory == (void *) NULL)
    ThrowException(&exception,ResourceLimitError,MemoryAllocationFailed,
      "AcquireCriticalMemory");
  return(memory);
}
```

但是发现如果`memory`为空时`ThrowException()`并不能结束掉程序，它最终调用的是`ThrowLoggedException()`函数将其记录下来。也可以这么使用：

``` c++
MagickExport void *AcquireCriticalMemory(const size_t len)
{
  void
    *memory;

  // Fail if memory request cannot be fulfilled.
  memory=MagickMalloc(len);
  if (memory == (void *) NULL)
    MagickFatalError(ResourceLimitFatalError,MemoryAllocationFailed,
      "ocl: AcquireCriticalMemory");
  return(memory);
}
```

这是我当前使用的方案，当指针为`0`时它结束掉程序，打印出如下的信息：

``` shellsession
[ysouyno@arch gm-ocl]$ gm display ~/temp/bg1a.jpg
gm display: Memory allocation failed (ocl: AcquireCriticalMemory) [Resource temporarily unavailable].
```

如果能把输出的信息弄得再详细点儿就更好了。

值得注意的是在`GM`中有`MagickFatalError()`，`MagickFatalError2()`和`MagickFatalError3()`三个功能相似的函数，`MagickFatalError()`可以使用字符串做为参数，`MagickFatalError2()`也可以使用字符串做为参数，但是它与`MagickFatalError()`的具体应用场景还不太了解，`MagickFatalError3()`只能使用预定义的异常类型。

### 关于`LiberateMagickResource()`的闪退问题（一）

我正在处理所有标注了`TODO(ocl)`的代码，在`DestroyMagickCLCacheInfoAndPixels()`函数里的代码：

``` c++
// RelinquishMagickResource(MemoryResource,info->length); // TODO(ocl)
DestroyMagickCLCacheInfo(info);
// (void) RelinquishAlignedMemory(pixels); // TODO(ocl)
```

我这样处理之后：

``` c++
// RelinquishMagickResource(MemoryResource,info->length);
LiberateMagickResource(MemoryResource,info->length);
DestroyMagickCLCacheInfo(info);
// (void) RelinquishAlignedMemory(pixels);
MagickFreeAlignedMemory(pixels);
```

程序闪退，打印的信息如下：

``` shellsession
$ gm display ~/temp/1.png
gm display: abort due to signal 11 (SIGSEGV) "Segmentation Fault"...
Aborted (core dumped)
```

确认起因是因为使用了`LiberateMagickResource(MemoryResource,info->length);`这行代码。经过调试发现在`GetAuthenticOpenCLBuffer()`函数返回`NULL`后程序闪退。具体代码是：

``` c++
if ((cache_info->type != MemoryCache)/*  || (cache_info->mapped != MagickFalse) */)
  return((cl_mem) NULL);
```

我这样修改好像不闪退了：

``` c++
if ((cache_info->type != MemoryCache) || (cache_info->type != MapCache))
  return((cl_mem) NULL);
```

目前尚未理解`LiberateMagickResource(MemoryResource,info->length);`的用意，及像上面这样修改会不会引发什么新的问题。

注：必须清除`.cache/ImageMagick`里的所有文件才能出现闪退问题，即在跑`opencl`的`benchmark`时会出现。

## <2022-04-03 Sun>

### 关于`LiberateMagickResource()`的闪退问题（二）

理解了一下`GM`的`AcquireMagickResource()`，`LiberateMagickResource()`函数，它们实际上起到监视的作用，没有分配和释放资源的功能，包括`InitializeMagickResources()`函数，初始化内存分配的上限，磁盘上限等等，可以通过比如`MAGICK_LIMIT_MEMORY`，`MAGICK_LIMIT_DISK`等环境变量来设置。

调试发现当在`DestroyMagickCLCacheInfoAndPixels()`函数中使用`LiberateMagickResource()`后：

``` c++
// LiberateMagickResource()

case SummationLimit:
  {
    /*
      Limit depends on sum of previous allocations as well as
      the currently requested size.
    */
    LockSemaphoreInfo(info->semaphore);
    info->value-=size;
    value=info->value;
    UnlockSemaphoreInfo(info->semaphore);
    break;
  }
```

中的`info->value-=size;`可能会变成负值，这样的话，再次调用`AcquireMagickResource()`时可能返回失败，即：

``` c++
// AcquireMagickResource()

case SummationLimit:
  {
    /*
      Limit depends on sum of previous allocations as well as
      the currently requested size.
    */
    LockSemaphoreInfo(info->semaphore);
    value=info->value+size;
    if ((info->maximum != ResourceInfinity) &&
        (value > (magick_uint64_t) info->maximum))
      {
        value=info->value;
        status=MagickFail;
      }
    else
      {
        info->value=value;
      }
    UnlockSemaphoreInfo(info->semaphore);
    break;
  }
```

这里的`if`分支，这样的话，需要找到为什么`LiberateMagickResource()`会将`info->value`的值搞成负数？

## <2022-04-06 Wed>

### 关于`LiberateMagickResource()`的闪退问题（三）

`info->value`的值之所以为负数，原因其实很简单，不是`AcquireMagickResource()`调少了，就是`LiberateMagickResource()`调多了。

最终还是解决了这个问题，这是搬代码过程中自己给自己挖的一个坑，原`IM`中的代码是：

``` c++
// IM's RelinquishPixelCachePixels()

#if defined(MAGICKCORE_OPENCL_SUPPORT)
      if (cache_info->opencl != (MagickCLCacheInfo) NULL)
        {
          cache_info->opencl=RelinquishMagickCLCacheInfo(cache_info->opencl,
            MagickTrue);
          cache_info->pixels=(Quantum *) NULL;
          break;
        }
#endif
```

这里的`break;`是关键。

## <2022-04-07 Thu>

### `gm benchmark`比较

仅运行一次缩放图片的话`gm-ocl`的速度远小于`gm`，而迭代`100`次的话，`gm-ocl`速度高于`gm`，见：

启用了硬件加速：

``` shellsession
[ysouyno@arch gm-ocl]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 6.35s user 4.997407s total 20.010 iter/s 15.748 iter/cpu
[ysouyno@arch gm-ocl]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 5.99s user 4.873903s total 20.517 iter/s 16.694 iter/cpu
[ysouyno@arch gm-ocl]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.35s user 0.830804s total 1.204 iter/s 2.857 iter/cpu
[ysouyno@arch gm-ocl]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.30s user 0.136360s total 7.334 iter/s 3.333 iter/cpu
[ysouyno@arch gm-ocl]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.29s user 0.814550s total 1.228 iter/s 3.448 iter/cpu
[ysouyno@arch gm-ocl]$ echo $MAGICK_OCL_DEVICE
true
[ysouyno@arch gm-ocl]$
```

没有启用硬件加速：

``` shellsession
[ysouyno@arch ~]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 40.57s user 5.829435s total 17.154 iter/s 2.465 iter/cpu
[ysouyno@arch ~]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 42.74s user 6.115149s total 16.353 iter/s 2.340 iter/cpu
[ysouyno@arch ~]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.31s user 0.057625s total 17.354 iter/s 3.226 iter/cpu
[ysouyno@arch ~]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.32s user 0.057751s total 17.316 iter/s 3.125 iter/cpu
[ysouyno@arch ~]$ gm benchmark convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1 iter 0.31s user 0.057476s total 17.399 iter/s 3.226 iter/cpu
[ysouyno@arch ~]$ echo $MAGICK_OCL_DEVICE

[ysouyno@arch ~]$
```

分析：在启用了硬件加速后，`gm-ocl`每次都将加载`~/.cache/ImageMagick/`中的镜像，读取磁盘文件属于慢操作；而`gm`则没有这种加载时间的影响。当迭代`100`次时`gm-ocl`的加载时间比重就缩小了。

如果改成`1000`次的话，似乎`gm-ocl`的优势更加明显。

启用了硬件加速：

``` shellsession
[ysouyno@arch gm-ocl]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 6.02s user 4.814306s total 20.771 iter/s 16.611 iter/cpu
[ysouyno@arch gm-ocl]$ gm benchmark -iterations 1000 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1000 iter 59.54s user 43.377261s total 23.054 iter/s 16.795 iter/cpu
[ysouyno@arch gm-ocl]$ echo $MAGICK_OCL_DEVICE
true
[ysouyno@arch gm-ocl]$
```

没有启用硬件加速：

``` shellsession
[ysouyno@arch ~]$ gm benchmark -iterations 100 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 100 iter 41.49s user 5.985783s total 16.706 iter/s 2.410 iter/cpu
[ysouyno@arch ~]$ gm benchmark -iterations 1000 convert ~/temp/bg1a.jpg -resize 960x540 ~/temp/out.jpg
Results: 8 threads 1000 iter 536.90s user 77.881720s total 12.840 iter/s 1.863 iter/cpu
[ysouyno@arch ~]$ echo $MAGICK_OCL_DEVICE

[ysouyno@arch ~]$
```

## <2022-04-10 Sun>

### 关于`ThrowMagickException()`的替代

刚刚我为`GM`增加了`AccelerateEvent`，因为我在`opencl.c:CompileOpenCLKernel()`中没有找到合适的方法替代`IM`的`ThrowMagickException()`，目前用日志代替异常（其实这个异常本身也没有结束程序的功能，况且如果这里发生错误的话也没必要结束程序）：

``` c++
status=openCL_library->clBuildProgram(device->program,1,&device->deviceID,
  options,NULL,NULL);
if (status != CL_SUCCESS)
{
  // (void) ThrowMagickException(exception,GetMagickModule(),DelegateWarning,
  //   "clBuildProgram failed.","(%d)",(int)status);
  (void) LogMagickEvent(AccelerateEvent,GetMagickModule(), // TODO(ocl)
    "clBuildProgram failed: %d",(int)status);
  LogOpenCLBuildFailure(device,kernel,exception);
  return(MagickFalse);
}
```

如果程序走进这个`if`分支中，说明使用硬件加速失败，期望的行为是走原来流程，这样的话就不会因为初始化硬件加速而耽误太长时间。而实际上程序确实花费了一定的时间且走了原有流程。原`IM`中也是如此，我觉得这是一个问题。

## <2022-04-11 Mon>

### 关于`*_utf8`系列函数

从`IM`中拷贝过来的`open_utf8()`，`fopen_utf8()`，`stat_utf8()`及`remove_utf8()`函数直接用非`_utf8`的函数代替。`IM`在`windows`下使用的是宽字符，所以有那样的处理。

### 关于`lt_dlclose()`函数

之前将`lt_dlclose()`函数改成了`dlclose()`函数，真是多此一举。因为在`windows`下`lt_dlclose()`是一个宏，它最终调用`FreeLibrary()`。

在`linux`下使用`lt_dlclose()`需要添加`-lltdl`链接选项，发现在`IM`中只要使用了`--enable-opencl`后运行`./configure`就自动添加上了`-lltdl`，我想在`GM`中也要实现它。

## <2022-04-12 Tue>

### 关于`-lltdl`链接选项（一）

从早上到现在我一直在尝试`--enable-opencl`时自动添加上`-lltdl`链接选项。我参考了`IM`中的`configure.ac`中的实现，修改后`GM`的`configure.ac`的片断：

``` shell
#
# Optionally check for libltdl if using it is still enabled
#
# Only use/depend on libtdl if we are building modules.  This is a
# change from previous releases (prior to 1.3.17) which supported
# loaded modules via libtdl if shared libraries were built.  of
# whether modules are built or not.
have_ltdl='no'
LIB_LTDL=''
if test "$build_modules" != 'no' || test "X$no_cl" != 'Xyes'
then
  AC_MSG_CHECKING([for libltdl ])
  AC_MSG_RESULT()
  failed=0
  passed=0
  AC_CHECK_HEADER([ltdl.h],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`])
  AC_CHECK_LIB([ltdl],[lt_dlinit],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`],)
  AC_MSG_CHECKING([if libltdl package is complete])
  if test $passed -gt 0
  then
    if test $failed -gt 0
    then
      AC_MSG_RESULT([no -- some components failed test])
      have_ltdl='no (failed tests)'
    else
      LIB_LTDL='-lltdl'
      LIBS="$LIB_LTDL $LIBS"
      AC_DEFINE(HasLTDL,1,[Define if using libltdl to support dynamically loadable modules])
      AC_MSG_RESULT([yes])
      have_ltdl='yes'
    fi
  else
    AC_MSG_RESULT([no])
  fi
  if test "$have_ltdl" != 'yes'
  then
    AC_MSG_FAILURE([libltdl is required by modules and OpenCL builds],[1])
  fi
fi
AM_CONDITIONAL(WITH_LTDL, test "$have_ltdl" != 'no')
```

然后在设置`MAGICK_DEP_LIBS`值的`if`和`else`分支中保证都含有`$LIB_LTDL`，同时注意`no_cl`的变量位置问题，否则上面代码段的`no_cl`值为空，导致上面代码段中的`if`分支始终能进入。

虽然经过这样的处理可以实现当使用`--enable-opencl`时自动加上`-lltdl`链接选项，但是引出了一个新的问题，当运行`gm`时：

``` shellsession
[ysouyno@arch gm-ocl]$ gm display ~/temp/bg1a.jpg
gm display: No decode delegate for this image format (/home/ysouyno/temp/bg1a.jpg).
gm display: Unable to open file (Untitled) [No such file or directory].
[ysouyno@arch gm-ocl]$
```

经过调查发现，这是由于`HasLTDL`宏被启用的缘故。

### 关于`-lltdl`链接选项（二）

这里发现另外一个问题，在`IM`中也存在这个问题。

虚拟机环境中，有存在`cl.h`头文件，但没有`libOpenCL.so`的情况，这种情况下安装各种`intel`或者`mesa`的`runtime`均不能配置成功可运行的`opencl`的环境（可以从`clinfo`的运行结果来看），这样的话，按“[关于`-lltdl`链接选项（一）](#关于-lltdl链接选项一)”的修改使用`--enable-opencl`选项编译`GM`和`IM`的话，均编译失败：

``` shellsession
undefined reference to `lt_dlclose'
```

因为如果有`cl.h`头文件的话，那么`HAVE_CL_CL_H`宏将启用，则`HAVE_OPENCL`宏也被启用，这样的话`lt_dlclose()`就可见了，但是没有`opencl`的链接环境，导致`no_cl`变量为`yes`，则`-lltdl`被忽略，从而链接失败，出现上述问题。

``` c++
#if defined(HAVE_CL_CL_H)
#  include <CL/cl.h>
#  define HAVE_OPENCL 1
#endif
#if defined(HAVE_OPENCL_CL_H)
#  include <OpenCL/cl.h>
#  define HAVE_OPENCL 1
#endif
```

阅读了一下`GM`的`configure.ac`中关于`build_modules`的代码，了解到要在原生的`GM`中启用`-lltdl`，需要使用如下命令：

``` shellsession
$ ./configure --enable-shared --with-modules
```

这样在`lib/GraphicsMagick-1.3.35/module-Q8/coders`目录中生成大量的`.la`文件。

我在想，我的要求只是简单的在`--enable-opencl`时添加一个链接选项，有必要大动干戈的修改原`GM`的`libltdl`的编译逻辑吗？我可以在`configure.ac`中额外处理`no_cl`，而不去启用`HasLTDL`宏？这样处理好不好？

## <2022-04-13 Wed>

### 关于`-lltdl`链接选项（三）

赶紧结束吧，这个链接选项不能搞两天呀！既然`IM`也有同样的问题，那么就不考虑上面所说的，存在`cl.h`头文件，但没有`libOpenCL.so`的情况，造成链接失败。比如出现如下提示：

``` shellsession
undefined reference to symbol 'dlsym@@GLIBC_2.2.5'
```

其它的测试看起来一切正常。

另因为`lt_dlclose()`也适用于`windows`平台，因此得尽快支持该平台，此平台还有好多开发，宏调整等等，得尽快完善起来。

### 支持`windows`

1. 为`configure.exe`增加“Enable OpenCL”多选框
2. 从“[VisualMagick](https://github.com/ImageMagick/VisualMagick.git)”拷贝`OpenCL/CL`头文件。
3. `vs`报错：`error C2004: expected 'defined(id)'`，因为它不支持这样的语法：

``` c++
#if defined(/*MAGICKCORE_OPENMP_SUPPORT*/HAVE_OPENMP)
```

4. 一些函数的`MagickExport`得去掉，因为重定义。
5. `MAGICKCORE_WINDOWS_SUPPORT`替换为`MSWINDOWS`。
