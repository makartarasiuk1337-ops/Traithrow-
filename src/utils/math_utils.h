#pragma once

#include <mathlib/vector.h>
#include <mathlib/vmatrix.h>
#include <math.h>

#define RAD2DEG(x) ((x) * 57.29577951f)

namespace Math {

    // Clamp view angles to valid Source Engine range
    inline void ClampAngles(QAngle& angles) {
        if (angles.x > 89.0f)  angles.x = 89.0f;
        if (angles.x < -89.0f) angles.x = -89.0f;
        while (angles.y > 180.0f)  angles.y -= 360.0f;
        while (angles.y < -180.0f) angles.y += 360.0f;
        angles.z = 0.0f;
    }

    // ---------------------------------------------------------------
    // CalcAngle — compute angle to look FROM src TO dst
    // In Source Engine: pitch positive = look DOWN (hence the minus)
    // Always use atan2f for correct quadrant handling.
    // ---------------------------------------------------------------
    inline void CalcAngle(const Vector& src, const Vector& dst, QAngle& angles) {
        Vector delta = dst - src;           // direction vector src→dst
        float dist2D = delta.Length2D();    // horizontal distance

        angles.x = -RAD2DEG(atan2f(delta.z, dist2D)); // pitch (inverted in Source)
        angles.y =  RAD2DEG(atan2f(delta.y, delta.x)); // yaw
        angles.z = 0.0f;
    }

    inline float VectorDistance(const Vector& v1, const Vector& v2) {
        return (v1 - v2).Length();
    }

    inline void VectorTransform(const Vector& in1, const matrix3x4_t& in2, Vector& out) {
        out.x = in1.Dot(Vector(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3];
        out.y = in1.Dot(Vector(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3];
        out.z = in1.Dot(Vector(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3];
    }
}
