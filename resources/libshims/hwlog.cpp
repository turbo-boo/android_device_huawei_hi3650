#include <log/log.h>
#include <stdio.h>
#include <string.h>

struct AndroidLogEntry {
    time_t tv_sec;
    long tv_nsec;
    int priority;
    int32_t uid;
    int32_t pid;
    int32_t tid;
    const char* tag;
    size_t tagLen;
    size_t messageLen;
    const char* message;
};

typedef enum chr_LogPriority {
    CHR_LOG_DEBUG = 0,
    CHR_LOG_INFO,
    CHR_LOG_WARN,
    CHR_LOG_ERROR,
} logPriority;

typedef enum chr_LogTag {
    CHR_LOG_TAG_PLAT = 0,
    CHR_LOG_TAG_WIFI,
    CHR_LOG_TAG_GNSS,
    CHR_LOG_TAG_BT,
    CHR_LOG_TAG_FM,
    CHR_LOG_TAG_NFC,
} logTag;

extern "C" {
// This stub is used because the original android_log_shouldPrintLine and the
// AndroidLogFormat struct it relied on have both been removed from the codebase.
int android_log_shouldPrintLine(void* p_format __unused, const char* tag __unused,
                                           int pri __unused) {
    return 1;  // Return 1 regardless the parameters, we should always print the line.
}

size_t android_log_printLogLine(void* p_format __unused, FILE* fp,
                                           AndroidLogEntry* entry) {
    if (fwrite(entry->message, 1, entry->messageLen, fp) != entry->messageLen) {
        return -1;
    }
    return entry->messageLen;
}

// This function is defined in the system library `libpowergenie_native3.so`
// and it's dynamically loaded by `libpowerlog.so` with `dlsym()`.
int __android_logPower_print(int bufID, int priority, char* tag, char* fmt, ...) {
    char message[512];
    char new_tag[128];

    // Make sure the tag isn't empty.
    if (!tag) tag = strdup("");

    // Here, the original implementation modifies both the tag and the format.
    //   - tag: It formats the tag with the priority by using `snprintf()`.
    //   - fmt: Adds a separator (|) and additional format specifier(s).
    snprintf(new_tag, sizeof(new_tag), "%d", priority);
    snprintf(message, sizeof(message), "%s", fmt);

    // Continue normally, craft the arguments list.
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // In this case, the original implementation would call the logPower_buf_write
    // function, but we can just use AOSP's __android_log_buf_write and call it a
    // day.
    return __android_log_buf_write(bufID, priority, new_tag, message);
}

int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
    va_list ap;
    char buf[512];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return __android_log_write(prio, tag, buf);
}

void __hwlog_jank_print(int p1, int id, char* comment) {
    ALOGV("%s: p1: %d, id: %d, comment: %s", __func__, p1, id, comment);
}

int __hwlog_jank_chgtm_print(int i, int id, char* fmt __unused, ...) {
    ALOGV("%s: i: %d, id: %d", __func__, i, id);
    return 0;
}

int __hwlog_write_kernel_printf(int id, int prio, const char* tag, const char* fmt __unused, ...) {
    ALOGV("%s: id: %d, prio: %d, tag: %s", __func__, id, prio, tag);
    return 0;
}

int __android_log_exception_buf_write(int id, int prio, const char* tag, const char* fmt __unused,
                                      ...) {
    ALOGV("%s: id: %d, prio: %d, tag: %s", __func__, id, prio, tag);
    return 0;
}

unsigned long __xcollie_start_ex(const char* name, int size, void (*arg3)() __unused,
                                 const char* arg4, unsigned int arg5) {
    ALOGV("%s: name: %s, size: %d, arg4: %s, arg5: 0x%x", __func__, name, size,
          arg4 ? arg4 : "NULL", arg5);
    return 0;
}

void xcollie_end(unsigned long handle) {
    ALOGV("%s: process_handle: %lu", __func__, handle);
}

void xcollie_init() {
    ALOGV("%s: called", __func__);
}

// Huawei vendor blobs log through this: (android prio, module, tag, fmt, ...).
int __hi_log_print(int prio, const char* module __unused, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = __android_log_vprint(prio, tag, fmt, ap);
    va_end(ap);
    return ret;
}

int32_t __chr_exception(uint32_t errno) {
    ALOGV("%s: errno: %u", __func__, errno);
    return 0;
}

int32_t __chr_exception_para(uint32_t errno, uint8_t* ptr, uint16_t len) {
    ALOGV("%s: errno: %u, ptr: %p, len: %u", __func__, errno, ptr, len);
    return 0;
}

int __chr_printLog(logPriority prio, logTag tag, const char* fmt, ...) {
    ALOGV("%s: prio: %d, tag: %u, fmt: %s", __func__, prio, tag, fmt);
    return 0;
}

int __chr_print_log(logPriority prio, logTag tag, const char* fmt, ...)
        __attribute__((alias("__chr_printLog")));
} // end of extern "C"
