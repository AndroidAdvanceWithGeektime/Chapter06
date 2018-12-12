#include <jni.h>
#include <string>

#include <atomic>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sstream>
#include <unordered_set>
#include <android/log.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/system_properties.h>
#include <vector>
#include <syscall.h>
#include "linker.h"
#include "hooks.h"

#define  LOG_TAG    "HOOOOOOOOK"
#define  ALOG(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
static const int64_t kSecondNanos = 1000000000;

int *atrace_marker_fd = nullptr;
std::atomic<uint64_t> *atrace_enabled_tags = nullptr;
std::atomic<uint64_t> original_tags(UINT64_MAX);
std::atomic<bool> systrace_installed;
bool first_enable = true;

int32_t threadID() {
    return static_cast<int32_t>(syscall(__NR_gettid));
}

int64_t monotonicTime() {
    timespec ts{};
    syscall(__NR_clock_gettime, CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * kSecondNanos + ts.tv_nsec;
}

void log_systrace(const void *buf, size_t count) {
    const char *msg = reinterpret_cast<const char *>(buf);

    switch (msg[0]) {
        case 'B': { // begin synchronous event. format: "B|<pid>|<name>"
            ALOG("========= %s|%d", msg, threadID());
            break;
        }
        case 'E': { // end synchronous event. format: "E"
            break;
        }
        default:
            return;
    }
}

bool should_log_systrace(int fd, size_t count) {
    return systrace_installed && fd == *atrace_marker_fd && count > 0;
}

ssize_t write_hook(int fd, const void *buf, size_t count) {
    if (should_log_systrace(fd, count)) {
        log_systrace(buf, count);
        return count;
    }
    return CALL_PREV(write_hook, fd, buf, count);
}

ssize_t __write_chk_hook(int fd, const void *buf, size_t count, size_t buf_size) {
    if (should_log_systrace(fd, count)) {
        log_systrace(buf, count);
        return count;
    }
    return CALL_PREV(__write_chk_hook, fd, buf, count, buf_size);
}


void hookLoadedLibs() {
    hook_plt_method("libc.so", "write", (hook_func) &write_hook);
    hook_plt_method("libc.so", "__write_chk", (hook_func) &__write_chk_hook);

}

static int getAndroidSdk() {
    static auto android_sdk = ([] {
        char sdk_version_str[PROP_VALUE_MAX];
        __system_property_get("ro.build.version.sdk", sdk_version_str);
        return atoi(sdk_version_str);
    })();
    return android_sdk;
}

void installSystraceSnooper() {
    auto sdk = getAndroidSdk();
    {
        std::string lib_name("libcutils.so");
        std::string enabled_tags_sym("atrace_enabled_tags");
        std::string fd_sym("atrace_marker_fd");

        if (sdk < 18) {
            lib_name = "libutils.so";
            // android::Tracer::sEnabledTags
            enabled_tags_sym = "_ZN7android6Tracer12sEnabledTagsE";
            // android::Tracer::sTraceFD
            fd_sym = "_ZN7android6Tracer8sTraceFDE";
        }

        void *handle;
        if (sdk < 21) {
            handle = dlopen(lib_name.c_str(), RTLD_LOCAL);
        } else {
            handle = dlopen(nullptr, RTLD_GLOBAL);
        }

        atrace_enabled_tags =
                reinterpret_cast<std::atomic<uint64_t> *>(
                        dlsym(handle, enabled_tags_sym.c_str()));

        if (atrace_enabled_tags == nullptr) {
            throw std::runtime_error("Enabled Tags not defined");
        }

        atrace_marker_fd =
                reinterpret_cast<int *>(dlsym(handle, fd_sym.c_str()));

        if (atrace_marker_fd == nullptr) {
            throw std::runtime_error("Trace FD not defined");
        }
        if (*atrace_marker_fd == -1) {
            throw std::runtime_error("Trace FD not valid");
        }
    }

    if (linker_initialize()) {
        throw std::runtime_error("Could not initialize linker library");
    }

    hookLoadedLibs();

    systrace_installed = true;
}

void enableSystrace() {
    if (!systrace_installed) {
        return;
    }

    if (!first_enable) {
        // On every enable, except the first one, find if new libs were loaded
        // and install systrace hook for them
        try {
            hookLoadedLibs();
        } catch (...) {
            // It's ok to continue if the refresh has failed
        }
    }
    first_enable = false;

    auto prev = atrace_enabled_tags->exchange(UINT64_MAX);
    if (prev !=
        UINT64_MAX) { // if we somehow call this twice in a row, don't overwrite the real tags
        original_tags = prev;
    }
}

void restoreSystrace() {
    if (!systrace_installed) {
        return;
    }

    uint64_t tags = original_tags;
    if (tags != UINT64_MAX) { // if we somehow call this before enableSystrace, don't screw it up
        atrace_enabled_tags->store(tags);
    }
}

bool installSystraceHook() {
    try {
        ALOG("===============install systrace hoook==================");
        installSystraceSnooper();
        return true;
    } catch (const std::runtime_error &e) {
        return false;
    }
}


extern "C"
JNIEXPORT void JNICALL
Java_com_dodola_atrace_Atrace_enableSystraceNative(JNIEnv *env, jclass type) {
    enableSystrace();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_dodola_atrace_Atrace_restoreSystraceNative(JNIEnv *env, jclass type) {
    restoreSystrace();

}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_dodola_atrace_Atrace_installSystraceHook(JNIEnv *env, jclass type) {
    installSystraceHook();
    return static_cast<jboolean>(true);
}
