#include <jni.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <cxxabi.h>
#include <pthread.h>
#include <android/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <dlfcn.h>

#include <vector>
#include <cstdio>
#include <ctime>
#include <csignal>
#include <thread>
#include <memory>
#include <string>
#include <optional>
#include <sstream>
#include <fstream>

#include "BacktraceDefine.h"
#include "Backtrace.h"
#include "nativehelper/managed_jnienv.h"
#include "../../../../matrix-android-commons/src/main/cpp/libxhook/xhook_ext.h"

#define HOOK_REQUEST_GROUPID_THREAD_PTHREAD_KEY_TRACE 0x13
#define KEY_VALID_FLAG (1 << 31)

const char *clz = "com/vompom/pthread_key_detect/PthreadKeyDetect";
const char *TAG = "pthread_key_detect";

template<typename T, std::size_t sz>
static inline constexpr std::size_t NELEM(const T(&)[sz]) { return sz; }

static struct StacktraceJNI {
    jclass ClassPthreadKeyDetect;
    jmethodID MethodCallback;
} gJ;

static void detachDestructor(void *arg) {
    pthread_t thd = pthread_self();
    JavaVM *jvm = (JavaVM *) arg;
    jvm->DetachCurrentThread();
}

int try_create_pthread_key(bool needDetach) {
    std::vector<int> keys;
    for (int i = 1; i <= PTHREAD_KEYS_MAX; i++) {
        pthread_key_t key;
        int result = pthread_key_create(&key, detachDestructor);
        if (needDetach) keys.push_back(key);
        if (result != JNI_OK) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Create thread key failed, available: %d/%d.", i - 1,
                                PTHREAD_KEYS_MAX);
            if (needDetach) {
                std::vector<pthread_key_t>::iterator it;
                for (it = keys.begin(); it != keys.end(); it++) {
                    pthread_key_delete(*it);
                }
            }
            return i - 1;
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "Create thread key success, available: %d/%d.", i, PTHREAD_KEYS_MAX);
        }
    }
    return 0;
}

int pthreadKeyAvailable(JNIEnv *env, jclass thiz) {
    return try_create_pthread_key(true);
}

void pthreadKeyFullUp(JNIEnv *env, jclass clazz) {
    try_create_pthread_key(false);
}

int pthreadKeyMax(JNIEnv *env, jclass thiz) {
    return PTHREAD_KEYS_MAX;
}

void makeNativeStack(wechat_backtrace::Backtrace *backtrace, char *&stack) {
    std::string caller_so_name;
    std::stringstream full_stack_builder;
    std::stringstream brief_stack_builder;
    std::string last_so_name;
    int index = 0;
    auto _callback = [&](wechat_backtrace::FrameDetail it) {
        std::string so_name = it.map_name;

        char *demangled_name = nullptr;
        int status = 0;

        demangled_name = abi::__cxa_demangle(it.function_name, nullptr, 0, &status);

        if (strstr(it.map_name, "libpthread_key_detect.so")) {
            return;
        }

        full_stack_builder
                << "#" << std::dec << (index++)
                << " pc " << std::hex << it.rel_pc << " "
                << it.map_name
                << " ("
                << (demangled_name ? demangled_name : "null")
                << ")"
                << std::endl;
        if (last_so_name != it.map_name) {
            last_so_name = it.map_name;
            brief_stack_builder << it.map_name << ";";
        }

        brief_stack_builder << std::hex << it.rel_pc << ";";

        if (demangled_name) {
            free(demangled_name);
        }
    };

    wechat_backtrace::restore_frame_detail(backtrace->frames.get(), backtrace->frame_size,
                                           _callback);

    stack = new char[full_stack_builder.str().size() + 1];
    strcpy(stack, full_stack_builder.str().c_str());
}

static char *getNativeBacktrace() {
    wechat_backtrace::Backtrace backtrace_zero = BACKTRACE_INITIALIZER(16);
    wechat_backtrace::unwind_adapter(backtrace_zero.frames.get(), backtrace_zero.max_frames, backtrace_zero.frame_size);
    char *nativeStack;
    makeNativeStack(&backtrace_zero, nativeStack);
    return nativeStack;
}

int (*original_pthread_key_create)(pthread_key_t *key, void (*destructor)(void *));

void pthreadKeyCallback(int type, int ret, int keySeq, const char *soName, const char *backtrace) {
    JNIEnv *env = JniInvocation::getEnv();
    if (!env) return;

    jstring soNameJS = env->NewStringUTF(soName);
    jstring nativeBacktraceJS = env->NewStringUTF(backtrace);

    env->CallStaticVoidMethod(gJ.ClassPthreadKeyDetect, gJ.MethodCallback, type, ret, keySeq, soNameJS, nativeBacktraceJS);
    env->DeleteLocalRef(soNameJS);
    env->DeleteLocalRef(nativeBacktraceJS);
}

int my_pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    int ret = original_pthread_key_create(key, destructor);
    int keySeq = *key & ~KEY_VALID_FLAG;

    void *__caller_addr = __builtin_return_address(0);
    Dl_info dl_info;
    dladdr(__caller_addr, &dl_info);
    const char *soName = dl_info.dli_fname;
    char *backtrace = getNativeBacktrace();
    if (!strstr(soName, "libc.so") && !strstr(soName, "libpthread_key_detect.so")) {
        pthreadKeyCallback(0, ret, keySeq, soName, backtrace);
    }
    delete[] backtrace;
    return ret;
}


int (*original_pthread_key_delete)(pthread_key_t key);

int my_pthread_key_delete(pthread_key_t key) {
    int ret = original_pthread_key_delete(key);
    int keySeq = key & ~KEY_VALID_FLAG;
    void *__caller_addr = __builtin_return_address(0);
    Dl_info dl_info;
    dladdr(__caller_addr, &dl_info);
    const char *soName = dl_info.dli_fname;
    char *backtrace = getNativeBacktrace();
    if (!strstr(soName, "libc.so") && !strstr(soName, "libpthread_key_detect.so")) {
        pthreadKeyCallback(1, ret, keySeq, soName, backtrace);
    }
    delete[] backtrace;
    return 0;
}

void pthreadHook(JNIEnv *env, jclass clazz) {

    xhook_grouped_register(HOOK_REQUEST_GROUPID_THREAD_PTHREAD_KEY_TRACE, ".*\\.so$", "pthread_key_create",
                           (void *) my_pthread_key_create, (void **) (&original_pthread_key_create));

    xhook_grouped_register(HOOK_REQUEST_GROUPID_THREAD_PTHREAD_KEY_TRACE, ".*\\.so$", "pthread_key_delete",
                           (void *) my_pthread_key_delete, (void **) (&original_pthread_key_delete));

    xhook_export_symtable_hook("libc.so", "pthread_key_create",
                               (void *) my_pthread_key_create, (void **) (&original_pthread_key_create));
    xhook_export_symtable_hook("libc.so", "pthread_key_delete",
                               (void *) my_pthread_key_delete, (void **) (&original_pthread_key_delete));
    xhook_enable_sigsegv_protection(0);
    xhook_refresh(0);
}

static const JNINativeMethod TOUCH_EVENT_TRACE_METHODS[] = {
        {"max",       "()I", (void *) pthreadKeyMax},
        {"useUp",     "()V", (void *) pthreadKeyFullUp},
        {"available", "()I", (void *) pthreadKeyAvailable},
        {"hook",      "()V", (void *) pthreadHook},
};

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JniInvocation::init(vm);
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    jclass pthreadKeyDetectCls = env->FindClass(clz);
    gJ.ClassPthreadKeyDetect = static_cast<jclass>(env->NewGlobalRef(pthreadKeyDetectCls));
    if (!pthreadKeyDetectCls) {
        return -1;
    }


    gJ.MethodCallback = env->GetStaticMethodID(pthreadKeyDetectCls, "pthreadKeyCallback",
                                               "(IIILjava/lang/String;Ljava/lang/String;)V");

    if (env->RegisterNatives(pthreadKeyDetectCls, TOUCH_EVENT_TRACE_METHODS,
                             static_cast<jint>(NELEM(TOUCH_EVENT_TRACE_METHODS))) != 0) {
        return -1;
    }

    env->DeleteLocalRef(pthreadKeyDetectCls);
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT  void JNI_OnUnload(JavaVM *vm, void *reserved) {
}