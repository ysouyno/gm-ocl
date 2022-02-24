# `gm-ocl`开发记录

## <2022-02-24 Thu> 增加`--enable-opencl`参数

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