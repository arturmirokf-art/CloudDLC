#pragma once
#include <vector>

class Aimbot {
public:
    static void OnUpdate();
    
private:
    struct Vector3 { double x, y, z; };
    struct Vector2 { float pitch, yaw; };
    
    static Vector2 GetRotations(Vector3 myPos, Vector3 targetPos);
    static float NormalizeAngle(float angle);
    static float GetDistance(Vector3 p1, Vector3 p2);
};
