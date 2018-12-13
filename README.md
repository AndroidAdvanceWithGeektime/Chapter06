# Chapter06

该项目展示了使用 PLTHook 技术来获取 Atrace 的日志，可以学习到systrace的一些底层机制

运行环境
=====
AndroidStudio3.2
NDK16~19
支持 `x86` `armeabi-v7a`


说明
======

运行项目后点击`开启 Atrace 日志`，然后就可以在Logcat 日志中查看到捕获的日志，类似如下：

```
 ========= B|5667|Lock contention on task queue lock (owner tid: 5667)
 ========= B|5667|JIT compiling int android.view.MotionEvent.getAction()
 ========= E
 ========= B|5667|Compiling
 ========= B|5667|ScopedCodeCacheWrite
 ========= B|5667|mprotect all
 ========= E
 ========= E
 ========= B|5667|mprotect code
 ========= B|5667|ScopedCodeCacheWrite
 ========= B|5667|mprotect all
 ========= B|5667|mprotect code
 ========= B|5667|TrimMaps
 ========= B|5667|void art::ArenaPool::TrimMaps()
```

可以通过B|事件和E|事件是成对出现的，这样就可以获得每个事件使用的时间。

关键实现
====
### hookLoadedLibs

使用plt hook libc的 write 相关方法，其中第一个参数意思是准备Hook的library名字。


这里我们用的是profilo中提供的plt hook库。

### write_hook
这里为了提升性能，只hook atrace_marker 文件对应的fd

```
ssize_t write_hook(int fd, const void *buf, size_t count) {
    if (should_log_systrace(fd, count)) {
        log_systrace(buf, count);
        return count;
    }
    return CALL_PREV(write_hook, fd, buf, count);
}
```

这里我们可能要理解一下ftrace和atrace的机制，可以参考以下参考文章：

1. [使用 ftrace](https://source.android.com/devices/tech/debug/ftrace)
2. [ftrace Documentation](https://www.kernel.org/doc/Documentation/trace/ftrace.txt)
3. [ftrace 简介](https://www.ibm.com/developerworks/cn/linux/l-cn-ftrace/index.html)
4. [atrace.cpp](https://android.googlesource.com/platform/frameworks/native/+/master/cmds/atrace/atrace.cpp)

### installSystraceSnooper
通过dlsym调用libcutils.so的方法，直接打开atrace。 这里需要适配Android版本小于18的情况

有人可能好奇类似"_ZN7android6Tracer12sEnabledTagsE"这样的名字是怎么样找出来的，我们可以参考[这篇文章](http://bramante.github.io/blog/2015/08/20/demangle-c-plus-plus-symbols/)。

```
c++filt -n _ZN7android6Tracer12sEnabledTagsE

android::Tracer::sEnabledTags
```

也可以直接objdump对应的so得到。

Thanks
======

[Profilo](https://github.com/facebookincubator/profilo)