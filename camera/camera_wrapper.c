/*
 * The stock camera HAL (camera.hi3650.so) reports a 40ms minimum
 * frame duration for every IMPLEMENTATION_DEFINED and YCbCr_420_888 output
 * of the front camera, although the sensor streams the outputs up to
 * 1440x1080 at 30fps. API1 drops outputs slower than 30fps, so it cannot
 * open the front camera at all (face unlock fails to enroll). Load the HAL
 * unchanged and report the 30fps duration it uses for the rear camera.
 */

#define LOG_TAG "camera_wrapper"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#include <hardware/camera_common.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <system/camera_metadata.h>
#include <system/graphics.h>

#define STOCK_HAL "/vendor/lib64/hw/camera.hi3650.so"
#define MAX_CAMERAS 8

// Minimum frame duration the HAL reports for its 30fps outputs.
#define FRAME_DURATION_30FPS 33331760LL
// Largest front camera output measured streaming at 30fps.
#define MAX_30FPS_AREA (1440 * 1080)

static const camera_module_t* stock;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static bool checked[MAX_CAMERAS];
static camera_metadata_t* patched[MAX_CAMERAS];

static camera_metadata_t* patch_front_durations(const camera_metadata_t* src) {
    camera_metadata_ro_entry_t facing;
    if (find_camera_metadata_ro_entry(src, ANDROID_LENS_FACING, &facing) != 0 ||
            facing.count != 1 || facing.data.u8[0] != ANDROID_LENS_FACING_FRONT) {
        return NULL;
    }

    camera_metadata_t* dst = clone_camera_metadata(src);
    if (!dst) {
        ALOGE("failed to clone front camera characteristics");
        return NULL;
    }

    camera_metadata_entry_t durations;
    size_t changed = 0;
    if (find_camera_metadata_entry(dst, ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS,
            &durations) == 0) {
        // Entries are (format, width, height, duration) tuples.
        for (size_t i = 0; i + 3 < durations.count; i += 4) {
            int64_t* d = &durations.data.i64[i];
            if ((d[0] == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED ||
                    d[0] == HAL_PIXEL_FORMAT_YCbCr_420_888) &&
                    d[1] * d[2] <= MAX_30FPS_AREA && d[3] > FRAME_DURATION_30FPS) {
                d[3] = FRAME_DURATION_30FPS;
                changed++;
            }
        }
    }

    if (changed == 0) {
        free_camera_metadata(dst);
        return NULL;
    }
    ALOGI("reporting 30fps for %zu front camera outputs", changed);
    return dst;
}

static int wrapper_get_camera_info(int camera_id, struct camera_info* info) {
    int ret = stock->get_camera_info(camera_id, info);
    if (ret != 0 || camera_id < 0 || camera_id >= MAX_CAMERAS) {
        return ret;
    }

    pthread_mutex_lock(&lock);
    if (!checked[camera_id]) {
        patched[camera_id] = patch_front_durations(info->static_camera_characteristics);
        checked[camera_id] = true;
    }
    if (patched[camera_id]) {
        info->static_camera_characteristics = patched[camera_id];
    }
    pthread_mutex_unlock(&lock);
    return ret;
}

static int wrapper_open(const hw_module_t* module, const char* id, hw_device_t** device) {
    (void)module;
    return stock->common.methods->open(&stock->common, id, device);
}

static struct hw_module_methods_t wrapper_methods = {
    .open = wrapper_open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "Huawei hi3650 camera wrapper",
        .author = "crDroid",
        .methods = &wrapper_methods,
    },
};

// The stock module fills its HMI from a constructor, so its versions and
// entry points are only known after loading it.
__attribute__((constructor)) static void load_stock(void) {
    void* handle = dlopen(STOCK_HAL, RTLD_NOW);
    if (!handle) {
        ALOGE("dlopen %s: %s", STOCK_HAL, dlerror());
        return;
    }
    stock = dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!stock) {
        ALOGE("dlsym %s: %s", HAL_MODULE_INFO_SYM_AS_STR, dlerror());
        dlclose(handle);
        return;
    }

    hw_module_t common = HAL_MODULE_INFO_SYM.common;
    common.module_api_version = stock->common.module_api_version;
    common.hal_api_version = stock->common.hal_api_version;
    HAL_MODULE_INFO_SYM = *stock;
    HAL_MODULE_INFO_SYM.common = common;
    HAL_MODULE_INFO_SYM.get_camera_info = wrapper_get_camera_info;
}
