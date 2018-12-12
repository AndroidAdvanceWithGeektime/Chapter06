# Chapter06

该项目展示了使用 PLTHook 技术来获取 Atrace 的日志
---------

运行环境
=====
AndroidStudio3.2
NDK16~19
支持 `x86` `armeabi-v7a`


说明
======

运行项目后点击`开启 Atrace 日志`，然后就可以在Logcat 日志中查看到捕获的日志，类似如下：

```
 ========= B|5667|Lock contention on task queue lock (owner tid: 5667)|5672
 ========= B|5667|JIT compiling int android.view.MotionEvent.getAction()|5672
 ========= B|5667|Compiling|5672
 ========= B|5667|ScopedCodeCacheWrite|5672
 ========= B|5667|mprotect all|5672
 ========= B|5667|mprotect code|5672
 ========= B|5667|ScopedCodeCacheWrite|5672
 ========= B|5667|mprotect all|5672
 ========= B|5667|mprotect code|5672
 ========= B|5667|TrimMaps|5672
 ========= B|5667|void art::ArenaPool::TrimMaps()|5672
```


Thanks
======

[Profilo](https://github.com/facebookincubator/profilo)