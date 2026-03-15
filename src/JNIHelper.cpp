#include "JNIHelper.h"
#include "Mappings_1_21_4.h"
#include <iostream>
#include <algorithm>

JNIEnv* JNIHelper::env = nullptr;
JavaVM* JNIHelper::jvm = nullptr;
jvmtiEnv* JNIHelper::jvmti = nullptr;
jobject JNIHelper::classLoader = nullptr;

jclass JNIHelper::class_MinecraftClient = nullptr;
jclass JNIHelper::class_Entity = nullptr;
jclass JNIHelper::class_ClientWorld = nullptr;
jclass JNIHelper::class_List = nullptr;
jclass JNIHelper::class_Iterator = nullptr;

jmethodID JNIHelper::mid_MC_getInstance = nullptr;
jfieldID JNIHelper::fid_MC_player = nullptr;
jfieldID JNIHelper::fid_MC_world = nullptr;
jmethodID JNIHelper::mid_Entity_getX = nullptr;
jmethodID JNIHelper::mid_Entity_getY = nullptr;
jmethodID JNIHelper::mid_Entity_getZ = nullptr;
jmethodID JNIHelper::mid_Entity_getEyeHeight = nullptr;
jmethodID JNIHelper::mid_Entity_getPitch = nullptr;
jmethodID JNIHelper::mid_Entity_getYaw = nullptr;
jmethodID JNIHelper::mid_Entity_setPitch = nullptr;
jmethodID JNIHelper::mid_Entity_setYaw = nullptr;
jmethodID JNIHelper::mid_World_getPlayers = nullptr;
jmethodID JNIHelper::mid_List_size = nullptr;
jmethodID JNIHelper::mid_List_iterator = nullptr;
jmethodID JNIHelper::mid_Iterator_hasNext = nullptr;
jmethodID JNIHelper::mid_Iterator_next = nullptr;

bool GlobalConfig::aimbotEnabled = false; 
float GlobalConfig::aimbotSpeed = 75.0f;
float GlobalConfig::aimbotFov = 360.0f;

bool JNIHelper::Initialize() {
    jsize count;
    if (JNI_GetCreatedJavaVMs(&jvm, 1, &count) != JNI_OK || count == 0) return false;
    jint res = jvm->GetEnv((void**)&env, JNI_VERSION_1_8);
    if (res == JNI_EDETACHED) {
        if (jvm->AttachCurrentThread((void**)&env, nullptr) != JNI_OK) return false;
    }
    jvm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);
    return env != nullptr;
}

JNIEnv* JNIHelper::GetEnv() {
    JNIEnv* currentEnv = nullptr;
    if (!jvm) return nullptr;
    jint res = jvm->GetEnv((void**)&currentEnv, JNI_VERSION_1_8);
    if (res == JNI_EDETACHED) {
        if (jvm->AttachCurrentThread((void**)&currentEnv, nullptr) != JNI_OK) {
            return nullptr;
        }
    }
    return currentEnv;
}

jclass JNIHelper::FindClassJVMTI(const char* className) {
    if (!jvmti) return nullptr;
    JNIEnv* e = GetEnv();
    jclass* classes;
    jint classCount;
    if (jvmti->GetLoadedClasses(&classCount, &classes) != JVMTI_ERROR_NONE) return nullptr;
    std::string targetSig = "L" + std::string(className) + ";";
    jclass foundClass = nullptr;
    for (int i = 0; i < classCount; i++) {
        char* signature = nullptr;
        if (jvmti->GetClassSignature(classes[i], &signature, nullptr) == JVMTI_ERROR_NONE && signature) {
            if (targetSig == signature) {
                foundClass = (jclass)e->NewLocalRef(classes[i]);
                if (!classLoader) {
                    jobject loader = nullptr;
                    if (jvmti->GetClassLoader(classes[i], &loader) == JVMTI_ERROR_NONE && loader) {
                        classLoader = e->NewGlobalRef(loader);
                    }
                }
                jvmti->Deallocate((unsigned char*)signature);
                break;
            }
            jvmti->Deallocate((unsigned char*)signature);
        }
    }
    jvmti->Deallocate((unsigned char*)classes);
    return foundClass;
}

bool JNIHelper::InitializeCache() {
    JNIEnv* e = GetEnv();
    if (!e) return false;
    e->ExceptionClear();

    std::cout << "[D] Initializing CloudDLC Cache v1.5.1...\n";
    CacheClassLoader(); 

    auto GetGlobalClass = [&](const char* name) -> jclass {
        e->ExceptionClear();
        jclass local = FindClass(name);
        if (!local) { std::cout << "[-] Missing class: " << name << "\n"; return nullptr; }
        jclass global = (jclass)e->NewGlobalRef(local);
        e->DeleteLocalRef(local);
        return global;
    };

    class_MinecraftClient = GetGlobalClass(Mappings::Minecraft::ClassName);
    class_Entity = GetGlobalClass(Mappings::Entity::ClassName);
    class_ClientWorld = GetGlobalClass(Mappings::ClientWorld::ClassName);
    class_List = GetGlobalClass(Mappings::Java::List::ClassName);
    class_Iterator = GetGlobalClass(Mappings::Java::Iterator::ClassName);

    if (!class_MinecraftClient || !class_Entity || !class_ClientWorld) return false;

    auto GetMID = [&](jclass c, const char* name, const char* sig, bool isStatic) -> jmethodID {
        e->ExceptionClear();
        jmethodID mid = isStatic ? e->GetStaticMethodID(c, name, sig) : e->GetMethodID(c, name, sig);
        if (e->ExceptionCheck() || !mid) {
            std::cout << "[-] Missing method: " << name << "\n";
            e->ExceptionClear();
            return nullptr;
        }
        return mid;
    };

    mid_MC_getInstance = GetMID(class_MinecraftClient, Mappings::Minecraft::GetInstance, Mappings::Minecraft::GetInstanceSig, true);
    fid_MC_player = e->GetFieldID(class_MinecraftClient, Mappings::Minecraft::PlayerField, Mappings::Minecraft::PlayerSig);
    fid_MC_world = e->GetFieldID(class_MinecraftClient, Mappings::Minecraft::WorldField, Mappings::Minecraft::WorldSig);

    mid_Entity_getX = GetMID(class_Entity, Mappings::Entity::GetX, Mappings::Entity::GetCoordsSig, false);
    mid_Entity_getY = GetMID(class_Entity, Mappings::Entity::GetY, Mappings::Entity::GetCoordsSig, false);
    mid_Entity_getZ = GetMID(class_Entity, Mappings::Entity::GetZ, Mappings::Entity::GetCoordsSig, false);
    mid_Entity_getEyeHeight = GetMID(class_Entity, Mappings::Entity::EyeHeight, Mappings::Entity::EyeHeightSig, false);
    mid_Entity_getYaw = GetMID(class_Entity, Mappings::Entity::GetYaw, Mappings::Entity::GetRotationSig, false);
    mid_Entity_getPitch = GetMID(class_Entity, Mappings::Entity::GetPitch, Mappings::Entity::GetRotationSig, false);
    mid_Entity_setYaw = GetMID(class_Entity, Mappings::Entity::SetYaw, Mappings::Entity::SetRotationSig, false);
    mid_Entity_setPitch = GetMID(class_Entity, Mappings::Entity::SetPitch, Mappings::Entity::SetRotationSig, false);

    mid_World_getPlayers = GetMID(class_ClientWorld, Mappings::ClientWorld::GetPlayers, Mappings::ClientWorld::GetPlayersSig, false);
    mid_List_size = GetMID(class_List, "size", "()I", false);
    mid_List_iterator = GetMID(class_List, Mappings::Java::List::Iterator, Mappings::Java::List::IteratorSig, false);
    mid_Iterator_hasNext = GetMID(class_Iterator, Mappings::Java::Iterator::HasNext, Mappings::Java::Iterator::HasNextSig, false);
    mid_Iterator_next = GetMID(class_Iterator, Mappings::Java::Iterator::Next, Mappings::Java::Iterator::NextSig, false);

    // THOROUGH NULL CHECK
    if (!mid_MC_getInstance || !fid_MC_player || !mid_Entity_getX || !mid_Entity_setYaw || !mid_World_getPlayers) {
        std::cout << "[-] Critical Mappings Missing!\n";
        return false;
    }

    std::cout << "[+] Cache Finalized v1.5.1.\n";
    return true;
}

bool JNIHelper::CacheClassLoader() {
    if (classLoader) return true;
    JNIEnv* e = GetEnv();
    if (!e) return false;
    jclass mc = FindClassJVMTI(Mappings::Minecraft::ClassName);
    if (mc) { e->DeleteLocalRef(mc); return true; }
    return false;
}

jclass JNIHelper::FindClass(const char* className) {
    JNIEnv* e = GetEnv();
    if (!e) return nullptr;
    e->ExceptionClear();
    jclass clazz = e->FindClass(className);
    if (!e->ExceptionCheck() && clazz) return clazz;
    e->ExceptionClear();
    if (classLoader) {
        std::string name(className);
        std::replace(name.begin(), name.end(), '/', '.'); 
        jclass loaderClass = e->GetObjectClass(classLoader);
        jmethodID loadClassMethod = e->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        jstring jname = e->NewStringUTF(name.c_str());
        jclass foundClass = (jclass)e->CallObjectMethod(classLoader, loadClassMethod, jname);
        e->DeleteLocalRef(jname); e->DeleteLocalRef(loaderClass);
        if (!e->ExceptionCheck() && foundClass) return foundClass;
        e->ExceptionClear();
    }
    return FindClassJVMTI(className);
}

std::string JNIHelper::JS2StdString(jstring jstr) {
    JNIEnv* e = GetEnv();
    if (!e || !jstr) return "";
    const char* chars = e->GetStringUTFChars(jstr, nullptr);
    std::string ret(chars);
    e->ReleaseStringUTFChars(jstr, chars);
    return ret;
}
