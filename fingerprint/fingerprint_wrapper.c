/*
 * The stock FPC HAL (fingerprint.hi3650.so, EMUI 5) implements the 2.1
 * fingerprint_device_t ABI but reports device version 1.0, which the
 * fingerprint@2.1 service rejects. Load it unchanged and report 2.1.
 */

#define LOG_TAG "fingerprint_wrapper"

#include <dlfcn.h>
#include <errno.h>

#include <hardware/fingerprint.h>
#include <hardware/hardware.h>
#include <log/log.h>

#define STOCK_HAL "/vendor/lib64/hw/fingerprint.hi3650.so"

static int wrapper_open(const hw_module_t* module, const char* id, hw_device_t** device) {
    (void)module;
    void* handle = dlopen(STOCK_HAL, RTLD_NOW);
    if (!handle) {
        ALOGE("dlopen %s: %s", STOCK_HAL, dlerror());
        return -ENOENT;
    }
    const hw_module_t* stock = dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!stock) {
        ALOGE("dlsym %s: %s", HAL_MODULE_INFO_SYM_AS_STR, dlerror());
        dlclose(handle);
        return -EINVAL;
    }
    int ret = stock->methods->open(stock, id, device);
    if (ret) {
        dlclose(handle);
        return ret;
    }
    (*device)->version = FINGERPRINT_MODULE_API_VERSION_2_1;
    return 0;
}

static struct hw_module_methods_t wrapper_methods = {
    .open = wrapper_open,
};

fingerprint_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = FINGERPRINT_MODULE_API_VERSION_2_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = FINGERPRINT_HARDWARE_MODULE_ID,
        .name = "Huawei hi3650 fingerprint wrapper",
        .author = "crDroid",
        .methods = &wrapper_methods,
    },
};
