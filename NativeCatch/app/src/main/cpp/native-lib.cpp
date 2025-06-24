#include <jni.h>
#include <signal.h>
#include <unistd.h>
#include <android/log.h>
#include <stdio.h>
#include <execinfo.h>
#include <stdlib.h>
#include <unwind.h>    // libunwind头文件
#include <dlfcn.h>
#include <cxxabi.h>    // demangle函数（可选）
#include <string.h>

#define TAG "NativeCrash"
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// 全局变量
JavaVM* g_jvm = nullptr;
char g_crash_file_path[256] = {0};  // 保存文件路径

// 写入崩溃信息到文件
void write_crash_info(const char* error_msg) {
    FILE* file = fopen(g_crash_file_path, "w");
    if (file != nullptr) {
        fprintf(file, "%s\n", error_msg);
        fclose(file);
    } else {
        LOG_ERROR("Failed to open crash file: %s", g_crash_file_path);
    }
}

struct BacktraceState {
    void** current;
    void** end;
};

static _Unwind_Reason_Code unwind_callback(struct _Unwind_Context* context, void* arg) {
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = (void*)pc;
        }
    }
    return _URC_NO_REASON;
}

size_t capture_backtrace(void** buffer, size_t max) {
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwind_callback, &state);
    return state.current - buffer;
}

void write_stack_trace_to_string(char* buffer, size_t buffer_size) {
    const int max_frames = 32;
    void* stack_buffer[max_frames];
    size_t frame_count = capture_backtrace(stack_buffer, max_frames);

    size_t offset = 0;
    for (size_t i = 0; i < frame_count && offset < buffer_size; ++i) {
        Dl_info info;
        if (dladdr(stack_buffer[i], &info) && info.dli_sname) {
            int status = -1;
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            const char* function_name = (status == 0 && demangled) ? demangled : info.dli_sname;
            offset += snprintf(buffer + offset, buffer_size - offset,
                               "#%zu pc %p %s +0x%lx\n",
                               i, stack_buffer[i], function_name,
                               (uintptr_t)stack_buffer[i] - (uintptr_t)info.dli_saddr);
            free(demangled);
        } else {
            offset += snprintf(buffer + offset, buffer_size - offset,
                               "#%zu pc %p\n", i, stack_buffer[i]);
        }
    }
}

// ========== 信号处理器和链传递 ==========
static struct sigaction old_sigsegv_action;
static struct sigaction old_sigbus_action;
static struct sigaction old_sigill_action;
static struct sigaction old_sigfpe_action;
static struct sigaction old_sigtrap_action;
static struct sigaction old_sigabrt_action;


// 信号处理函数
void signal_handler(int signal_, siginfo_t* info, void* context) {
    // 获取错误信息
    const char* signal_name = "Unknown";
    switch (signal_) {
        case SIGSEGV: signal_name = "SIGSEGV"; break;
        case SIGTRAP: signal_name = "SIGTRAP"; break;
        case SIGABRT: signal_name = "SIGABRT"; break;
    }

    char error_msg[2048];
    snprintf(error_msg, sizeof(error_msg), "Signal: %s, Fault Address: %p", signal_name, info->si_addr);
    LOG_ERROR("Native Crash Detected: %s", error_msg);
    // 获取并写入堆栈信息
    // 拼接堆栈信息到 error_msg
    write_stack_trace_to_string(error_msg + strlen(error_msg), sizeof(error_msg) - strlen(error_msg));
    // 将崩溃信息写入文件
    write_crash_info(error_msg);

    struct sigaction* old_action = nullptr;
    switch (signal_) {
        case SIGSEGV: old_action = &old_sigsegv_action; break;
        case SIGBUS:  old_action = &old_sigbus_action; break;
        case SIGILL:  old_action = &old_sigill_action; break;
        case SIGFPE:  old_action = &old_sigfpe_action; break;
        case SIGABRT:  old_action = &old_sigabrt_action; break;
        case SIGTRAP:  old_action = &old_sigtrap_action; break;
    }

    if (old_action && old_action->sa_sigaction) {
        // 调用上层信号处理器，继续崩溃处理流程
        old_action->sa_sigaction(signal_, info, context);
    } else {
        // 没有上层处理器，恢复默认行为，并重新发送信号终止程序
        signal(signal_, SIG_DFL);
        raise(signal_);
    }
}

// 初始化信号捕获（通过 JNI 传递文件路径）
extern "C" JNIEXPORT void JNICALL
Java_com_smwl_smsdk_app_MyNativeCrashHandler_initNativeCrashHandler(JNIEnv* env, jclass clazz, jstring file_path) {
    // 保存 JavaVM
    env->GetJavaVM(&g_jvm);

    // 将 Java 传递的文件路径转换为 C 字符串
    const char* crash_file_path = env->GetStringUTFChars(file_path, nullptr);
    snprintf(g_crash_file_path, sizeof(g_crash_file_path), "%s", crash_file_path);
    env->ReleaseStringUTFChars(file_path, crash_file_path);

    // 注册信号处理函数
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, &old_sigsegv_action);
    sigaction(SIGTRAP, &sa, &old_sigtrap_action);
    sigaction(SIGABRT, &sa, &old_sigabrt_action);
    sigaction(SIGBUS, &sa, &old_sigbus_action);
    sigaction(SIGILL, &sa, &old_sigill_action);
    sigaction(SIGFPE, &sa, &old_sigfpe_action);
}

// 测试用 Native 方法（故意触发崩溃）
extern "C" JNIEXPORT void JNICALL
Java_com_smwl_smsdk_app_MyNativeCrashHandler_triggerCrash(JNIEnv* env, jclass thiz) {
    int* ptr = nullptr;
    *ptr = 1;  // 触发 SIGSEGV
}