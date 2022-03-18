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

# <2022-03-15 Tue>

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

### 关于`IM`中的`number_channels`成员

在`IM`中`number_channels`成员出现频率有点高，经调试发现`IM`中图片对象初始化时通过调用`OpenPixelCache()`然后在`InitializePixelChannelMap()`中设置`number_channels`的值。这个函数的内部大量使用了`GM`中没有类型`PixelChannel`和`PixelTrait`，不太好把它给搬到`GM`中。

查看`PixelChannel`的定义发现了它的一个特点是：虽然它是`enum`类型，但每个成员都被指派了具体的值，且发现有多个成员共用一个值的情况。以此参照仍然没有在`GM`中找到类似定义，`PixelChannel`的定义：

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
