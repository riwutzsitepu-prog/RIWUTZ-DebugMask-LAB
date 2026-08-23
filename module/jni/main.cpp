#include <sys/types.h>
#include "zygisk.hpp"
#include "target_package.h"

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define LOG_TAG "RiwutzPerAppDebugMask"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)

// Original JNI method pointers. Zygisk writes the original pointer back to fnPtr.
static jstring  (*orig_get_s)(JNIEnv*, jclass, jstring) = nullptr;
static jstring  (*orig_get_ss)(JNIEnv*, jclass, jstring, jstring) = nullptr;
static jint     (*orig_get_int)(JNIEnv*, jclass, jstring, jint) = nullptr;
static jlong    (*orig_get_long)(JNIEnv*, jclass, jstring, jlong) = nullptr;
static jboolean (*orig_get_bool)(JNIEnv*, jclass, jstring, jboolean) = nullptr;

static bool is_target_process(const char* process) {
    if (!process) return false;
    const size_t n = strlen(TARGET_PACKAGE);
    if (strncmp(process, TARGET_PACKAGE, n) != 0) return false;
    return process[n] == '\0' || process[n] == ':';
}

// Values below are returned only inside the configured lab process through JNI hooks.
// No setprop/resetprop/stop-adbd calls are made, so the system-wide state is unchanged.
static const char* masked_property(const char* key) {
    if (!key) return nullptr;

    if (strcmp(key, "sys.usb.config") == 0)          return "mtp";
    if (strcmp(key, "persist.sys.usb.config") == 0)  return "mtp";
    if (strcmp(key, "sys.usb.state") == 0)           return "mtp";
    if (strcmp(key, "init.svc.adbd") == 0)           return "stopped";
    if (strcmp(key, "service.adb.tcp.port") == 0)    return "-1";

    return nullptr;
}

static void log_mask(const char* key, const char* value) {
    if (key && value) LOGD("masked property: %s -> %s", key, value);
}

static jstring hook_get_s(JNIEnv* env, jclass clazz, jstring jkey) {
    if (!jkey) {
        return orig_get_s ? orig_get_s(env, clazz, jkey) : env->NewStringUTF("");
    }

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) {
        return orig_get_s ? orig_get_s(env, clazz, jkey) : env->NewStringUTF("");
    }

    const char* fake = masked_property(key);
    if (fake) log_mask(key, fake);
    env->ReleaseStringUTFChars(jkey, key);

    if (fake) return env->NewStringUTF(fake);
    return orig_get_s ? orig_get_s(env, clazz, jkey) : env->NewStringUTF("");
}

static jstring hook_get_ss(JNIEnv* env, jclass clazz, jstring jkey, jstring jdef) {
    if (!jkey) {
        return orig_get_ss ? orig_get_ss(env, clazz, jkey, jdef) : jdef;
    }

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) {
        return orig_get_ss ? orig_get_ss(env, clazz, jkey, jdef) : jdef;
    }

    const char* fake = masked_property(key);
    if (fake) log_mask(key, fake);
    env->ReleaseStringUTFChars(jkey, key);

    if (fake) return env->NewStringUTF(fake);
    return orig_get_ss ? orig_get_ss(env, clazz, jkey, jdef) : jdef;
}

static jint hook_get_int(JNIEnv* env, jclass clazz, jstring jkey, jint def) {
    if (!jkey) return orig_get_int ? orig_get_int(env, clazz, jkey, def) : def;

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) return orig_get_int ? orig_get_int(env, clazz, jkey, def) : def;

    const char* fake = masked_property(key);
    if (fake) log_mask(key, fake);
    env->ReleaseStringUTFChars(jkey, key);

    if (fake) return static_cast<jint>(atoi(fake));
    return orig_get_int ? orig_get_int(env, clazz, jkey, def) : def;
}

static jlong hook_get_long(JNIEnv* env, jclass clazz, jstring jkey, jlong def) {
    if (!jkey) return orig_get_long ? orig_get_long(env, clazz, jkey, def) : def;

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) return orig_get_long ? orig_get_long(env, clazz, jkey, def) : def;

    const char* fake = masked_property(key);
    if (fake) log_mask(key, fake);
    env->ReleaseStringUTFChars(jkey, key);

    if (fake) return static_cast<jlong>(strtoll(fake, nullptr, 10));
    return orig_get_long ? orig_get_long(env, clazz, jkey, def) : def;
}

static jboolean hook_get_bool(JNIEnv* env, jclass clazz, jstring jkey, jboolean def) {
    if (!jkey) return orig_get_bool ? orig_get_bool(env, clazz, jkey, def) : def;

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if (!key) return orig_get_bool ? orig_get_bool(env, clazz, jkey, def) : def;

    const char* fake = masked_property(key);
    if (fake) log_mask(key, fake);
    env->ReleaseStringUTFChars(jkey, key);

    if (fake) {
        return (strcmp(fake, "1") == 0 || strcmp(fake, "true") == 0 ||
                strcmp(fake, "yes") == 0 || strcmp(fake, "on") == 0)
                ? JNI_TRUE : JNI_FALSE;
    }
    return orig_get_bool ? orig_get_bool(env, clazz, jkey, def) : def;
}

class RiwutzPerAppDebugMask : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override {
        api_ = api;
        env_ = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        if (!args || !args->nice_name) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char* process = env_->GetStringUTFChars(args->nice_name, nullptr);
        const bool target = is_target_process(process);
        LOGD("process=%s target=%d", process ? process : "<null>", target ? 1 : 0);
        if (process) env_->ReleaseStringUTFChars(args->nice_name, process);

        if (!target) {
            api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        // Android 10 exposes these native methods in android.os.SystemProperties.
        // Missing methods on other Android versions are returned as nullptr by Zygisk;
        // the remaining compatible hooks can still work.
        JNINativeMethod methods[] = {
            { const_cast<char*>("native_get"),
              const_cast<char*>("(Ljava/lang/String;)Ljava/lang/String;"),
              reinterpret_cast<void*>(hook_get_s) },
            { const_cast<char*>("native_get"),
              const_cast<char*>("(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
              reinterpret_cast<void*>(hook_get_ss) },
            { const_cast<char*>("native_get_int"),
              const_cast<char*>("(Ljava/lang/String;I)I"),
              reinterpret_cast<void*>(hook_get_int) },
            { const_cast<char*>("native_get_long"),
              const_cast<char*>("(Ljava/lang/String;J)J"),
              reinterpret_cast<void*>(hook_get_long) },
            { const_cast<char*>("native_get_boolean"),
              const_cast<char*>("(Ljava/lang/String;Z)Z"),
              reinterpret_cast<void*>(hook_get_bool) },
        };

        api_->hookJniNativeMethods(env_, "android/os/SystemProperties", methods,
                                   sizeof(methods) / sizeof(methods[0]));

        orig_get_s    = reinterpret_cast<decltype(orig_get_s)>(methods[0].fnPtr);
        orig_get_ss   = reinterpret_cast<decltype(orig_get_ss)>(methods[1].fnPtr);
        orig_get_int  = reinterpret_cast<decltype(orig_get_int)>(methods[2].fnPtr);
        orig_get_long = reinterpret_cast<decltype(orig_get_long)>(methods[3].fnPtr);
        orig_get_bool = reinterpret_cast<decltype(orig_get_bool)>(methods[4].fnPtr);

        int installed = 0;
        installed += orig_get_s    ? 1 : 0;
        installed += orig_get_ss   ? 1 : 0;
        installed += orig_get_int  ? 1 : 0;
        installed += orig_get_long ? 1 : 0;
        installed += orig_get_bool ? 1 : 0;

        LOGI("target=%s; SystemProperties hooks installed=%d/5", TARGET_PACKAGE, installed);
        if (installed == 0) {
            LOGW("No compatible SystemProperties JNI methods found on this Android build");
        }
    }

private:
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
};

REGISTER_ZYGISK_MODULE(RiwutzPerAppDebugMask)
