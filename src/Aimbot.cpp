#include "Aimbot.h"
#include "JNIHelper.h"
#include <cmath>
#include <random>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float GetRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

static float LerpAngle(float start, float end, float factor) {
    float diff = end - start;
    while (diff <= -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;
    return start + diff * factor;
}

// Global target point offset (for smoothing between points)
static float currentTargetHeightOffset = 0.5f;
static float desiredTargetHeightOffset = 0.5f;

void Aimbot::OnUpdate() {
    if (!GlobalConfig::aimbotEnabled) return;

    JNIEnv* env = JNIHelper::GetEnv();
    if (!env || !JNIHelper::class_MinecraftClient) return;

    if (env->PushLocalFrame(64) != 0) return;

    jobject mcInstance = env->CallStaticObjectMethod(JNIHelper::class_MinecraftClient, JNIHelper::mid_MC_getInstance);
    if (!mcInstance) { env->PopLocalFrame(nullptr); return; }

    jobject localPlayer = env->GetObjectField(mcInstance, JNIHelper::fid_MC_player);
    if (!localPlayer) { env->PopLocalFrame(nullptr); return; }

    jobject world = env->GetObjectField(mcInstance, JNIHelper::fid_MC_world);
    if (!world) { env->PopLocalFrame(nullptr); return; }

    Vector3 myPos;
    myPos.x = env->CallDoubleMethod(localPlayer, JNIHelper::mid_Entity_getX);
    myPos.y = env->CallDoubleMethod(localPlayer, JNIHelper::mid_Entity_getY) + env->CallFloatMethod(localPlayer, JNIHelper::mid_Entity_getEyeHeight);
    myPos.z = env->CallDoubleMethod(localPlayer, JNIHelper::mid_Entity_getZ);

    jobject entityList = env->CallObjectMethod(world, JNIHelper::mid_World_getPlayers);
    if (!entityList) { env->PopLocalFrame(nullptr); return; }

    int listSize = env->CallIntMethod(entityList, JNIHelper::mid_List_size);
    if (listSize <= 1) { env->PopLocalFrame(nullptr); return; }

    jobject iterator = env->CallObjectMethod(entityList, JNIHelper::mid_List_iterator);
    if (!iterator) { env->PopLocalFrame(nullptr); return; }

    jobject closestEntity = nullptr;
    float closestDist = 4.0f; 

    while (env->CallBooleanMethod(iterator, JNIHelper::mid_Iterator_hasNext)) {
        jobject entity = env->CallObjectMethod(iterator, JNIHelper::mid_Iterator_next);
        if (!entity || env->IsSameObject(entity, localPlayer)) {
            if (entity) env->DeleteLocalRef(entity);
            continue;
        }

        Vector3 ePos;
        ePos.x = env->CallDoubleMethod(entity, JNIHelper::mid_Entity_getX);
        ePos.y = env->CallDoubleMethod(entity, JNIHelper::mid_Entity_getY);
        ePos.z = env->CallDoubleMethod(entity, JNIHelper::mid_Entity_getZ);

        float dist = GetDistance(myPos, ePos);
        if (dist < closestDist) {
            closestDist = dist;
            if (closestEntity) env->DeleteLocalRef(closestEntity);
            closestEntity = entity; 
        } else {
            env->DeleteLocalRef(entity);
        }
    }

    if (closestEntity) {
        float eyeHeight = env->CallFloatMethod(closestEntity, JNIHelper::mid_Entity_getEyeHeight);
        
        // --- SMOOTH MULTI-POINT ---
        // Change desired point occasionally
        if (GetRandomFloat(0, 100) < 0.5f) { // Very rarely change target bone
            desiredTargetHeightOffset = GetRandomFloat(0.1f, 0.9f);
        }
        // Smoothly slide current point to desired
        currentTargetHeightOffset += (desiredTargetHeightOffset - currentTargetHeightOffset) * 0.01f;

        Vector3 targetPos;
        targetPos.x = env->CallDoubleMethod(closestEntity, JNIHelper::mid_Entity_getX);
        targetPos.y = env->CallDoubleMethod(closestEntity, JNIHelper::mid_Entity_getY) + (eyeHeight * currentTargetHeightOffset);
        targetPos.z = env->CallDoubleMethod(closestEntity, JNIHelper::mid_Entity_getZ);

        Vector2 targetRots = GetRotations(myPos, targetPos);

        float currentPitch = env->CallFloatMethod(localPlayer, JNIHelper::mid_Entity_getPitch);
        float currentYaw = env->CallFloatMethod(localPlayer, JNIHelper::mid_Entity_getYaw);

        // --- ULTRA SMOOTH SPEED ---
        // We divide by a much larger factor. 100 in slider = fast, 1 = super slow.
        float speed = (GlobalConfig::aimbotSpeed / 600.0f); 
        if (speed > 1.0f) speed = 1.0f;

        float newYaw = LerpAngle(currentYaw, targetRots.yaw, speed);
        float newPitch = LerpAngle(currentPitch, targetRots.pitch, speed);

        // Very subtle micro-shaking
        newYaw += GetRandomFloat(-0.005f, 0.005f);
        newPitch += GetRandomFloat(-0.005f, 0.005f);

        env->CallVoidMethod(localPlayer, JNIHelper::mid_Entity_setPitch, newPitch);
        env->CallVoidMethod(localPlayer, JNIHelper::mid_Entity_setYaw, newYaw);
    }

    env->PopLocalFrame(nullptr);
}

Aimbot::Vector2 Aimbot::GetRotations(Vector3 myPos, Vector3 targetPos) {
    double dx = targetPos.x - myPos.x;
    double dy = targetPos.y - myPos.y;
    double dz = targetPos.z - myPos.z;
    double diffXZ = std::sqrt(dx * dx + dz * dz);
    float yaw = (float)(std::atan2(dz, dx) * 180.0 / M_PI) - 90.0f;
    float pitch = (float)-(std::atan2(dy, diffXZ) * 180.0 / M_PI);
    return { pitch, yaw };
}

float Aimbot::NormalizeAngle(float angle) {
    while (angle <= -180.0f) angle += 360.0f;
    while (angle > 180.0f) angle -= 360.0f;
    return angle;
}

float Aimbot::GetDistance(Vector3 p1, Vector3 p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    double dz = p1.z - p2.z;
    return (float)std::sqrt(dx*dx + dy*dy + dz*dz);
}
