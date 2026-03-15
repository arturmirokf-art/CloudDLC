#pragma once
#include <jni.h>
#include <jvmti.h>
#include <string>
#include <vector>

struct GlobalConfig {
    static bool aimbotEnabled;
    static float aimbotSpeed;
    static float aimbotFov;
};

class JNIHelper {
public:
    static JNIEnv* env;
    static JavaVM* jvm;
    static jvmtiEnv* jvmti;
    static jobject classLoader;

    static bool Initialize();
    static JNIEnv* GetEnv();
    static jclass FindClass(const char* className);
    static jclass FindClassJVMTI(const char* className);
    static bool CacheClassLoader();
    static std::string JS2StdString(jstring jstr);

    // Cached Classes (Global Refs)
    static jclass class_MinecraftClient;
    static jclass class_Entity;
    static jclass class_ClientWorld;
    static jclass class_List;
    static jclass class_Iterator;

    // Cached Methods/Fields
    static jmethodID mid_MC_getInstance;
    static jfieldID fid_MC_player;
    static jfieldID fid_MC_world;
    static jmethodID mid_Entity_getX;
    static jmethodID mid_Entity_getY;
    static jmethodID mid_Entity_getZ;
    static jmethodID mid_Entity_getEyeHeight;
    static jmethodID mid_Entity_getPitch;
    static jmethodID mid_Entity_getYaw;
    static jmethodID mid_Entity_setPitch;
    static jmethodID mid_Entity_setYaw;
    static jmethodID mid_World_getPlayers;
    static jmethodID mid_List_size;
    static jmethodID mid_List_iterator;
    static jmethodID mid_Iterator_hasNext;
    static jmethodID mid_Iterator_next;

    static bool InitializeCache();
};
