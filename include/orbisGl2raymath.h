/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev/liborbis
 */
//based on raylib raytmath
/**********************************************************************************************
*
*   raymath v1.2 - Math functions to work with Vector3, Matrix and Quaternions
*
*   CONFIGURATION:
*
*   #define RAYMATH_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define RAYMATH_HEADER_ONLY
*       Define static inline functions code, so #include header suffices for use.
*       This may use up lots of memory.
*
*   #define RAYMATH_STANDALONE
*       Avoid raylib.h header inclusion in this file.
*       Vector3 and Matrix data types are defined internally in raymath module.
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2015-2020 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/
#ifndef _ORBISGLMATH_H_
#define _ORBISGLMATH_H_

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#ifndef PI
	#define PI 3.14159265358979323846
#endif

#ifndef DEG2RAD
	#define DEG2RAD (PI/180.0f)
#endif

#ifndef RAD2DEG
	#define RAD2DEG (180.0f/PI)
#endif

// Return float vector for Matrix
#ifndef MatrixToFloat
	#define MatrixToFloat(mat) (MatrixToFloatV(mat).v)
#endif

// Return float vector for Vector3
#ifndef Vector3ToFloat
	#define Vector3ToFloat(vec) (Vector3ToFloatV(vec).v)
#endif

#define RMDEF 
//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------


// NOTE: Helper types to be used instead of array return types for *ToFloat functions
typedef struct float3 { float v[3]; } float3;
typedef struct float16 { float v[16]; } float16;

#include <math.h>       // Required for: sinf(), cosf(), sqrtf(), tan(), fabs()

//----------------------------------------------------------------------------------
// Module Functions Definition - Utils math
//----------------------------------------------------------------------------------

// Clamp float value
RMDEF float Clamp(float value, float min, float max);

// Calculate linear interpolation between two floats
RMDEF float Lerp(float start, float end, float amount);

//----------------------------------------------------------------------------------
// Module Functions Definition - Vector2 math
//----------------------------------------------------------------------------------

// Vector with components value 0.0f
RMDEF Vector2 Vector2Zero(void);

// Vector with components value 1.0f
RMDEF Vector2 Vector2One(void);

// Add two vectors (v1 + v2)
RMDEF Vector2 Vector2Add(Vector2 v1, Vector2 v2);

// Subtract two vectors (v1 - v2)
RMDEF Vector2 Vector2Subtract(Vector2 v1, Vector2 v2);

// Calculate vector length
RMDEF float Vector2Length(Vector2 v);

// Calculate two vectors dot product
RMDEF float Vector2DotProduct(Vector2 v1, Vector2 v2);

// Calculate distance between two vectors
RMDEF float Vector2Distance(Vector2 v1, Vector2 v2);

// Calculate angle from two vectors in X-axis
RMDEF float Vector2Angle(Vector2 v1, Vector2 v2);

// Scale vector (multiply by value)
RMDEF Vector2 Vector2Scale(Vector2 v, float scale);

// Multiply vector by vector
RMDEF Vector2 Vector2MultiplyV(Vector2 v1, Vector2 v2);

// Negate vector
RMDEF Vector2 Vector2Negate(Vector2 v);

// Divide vector by a float value
RMDEF Vector2 Vector2Divide(Vector2 v, float div);

// Divide vector by vector
RMDEF Vector2 Vector2DivideV(Vector2 v1, Vector2 v2);

// Normalize provided vector
RMDEF Vector2 Vector2Normalize(Vector2 v);

// Calculate linear interpolation between two vectors
RMDEF Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount);

//----------------------------------------------------------------------------------
// Module Functions Definition - Vector3 math
//----------------------------------------------------------------------------------

// Vector with components value 0.0f
RMDEF Vector3 Vector3Zero(void);

// Vector with components value 1.0f
RMDEF Vector3 Vector3One(void);

// Add two vectors
RMDEF Vector3 Vector3Add(Vector3 v1, Vector3 v2);

// Subtract two vectors
RMDEF Vector3 Vector3Subtract(Vector3 v1, Vector3 v2);

// Multiply vector by scalar
RMDEF Vector3 Vector3Scale(Vector3 v, float scalar);

// Multiply vector by vector
RMDEF Vector3 Vector3Multiply(Vector3 v1, Vector3 v2);

// Calculate two vectors cross product
RMDEF Vector3 Vector3CrossProduct(Vector3 v1, Vector3 v2);

// Calculate one vector perpendicular vector
RMDEF Vector3 Vector3Perpendicular(Vector3 v);

// Calculate vector length
RMDEF float Vector3Length(const Vector3 v);

// Calculate two vectors dot product
RMDEF float Vector3DotProduct(Vector3 v1, Vector3 v2);

// Calculate distance between two vectors
RMDEF float Vector3Distance(Vector3 v1, Vector3 v2);

// Negate provided vector (invert direction)
RMDEF Vector3 Vector3Negate(Vector3 v);

// Divide vector by a float value
RMDEF Vector3 Vector3Divide(Vector3 v, float div);

// Divide vector by vector
RMDEF Vector3 Vector3DivideV(Vector3 v1, Vector3 v2);

// Normalize provided vector
RMDEF Vector3 Vector3Normalize(Vector3 v);


// Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
RMDEF void Vector3OrthoNormalize(Vector3 *v1, Vector3 *v2);

// Transforms a Vector3 by a given Matrix
RMDEF Vector3 Vector3Transform(Vector3 v, Matrix mat);

// Transform a vector by quaternion rotation
RMDEF Vector3 Vector3RotateByQuaternion(Vector3 v, Quaternion q);

// Calculate linear interpolation between two vectors
RMDEF Vector3 Vector3Lerp(Vector3 v1, Vector3 v2, float amount);

// Calculate reflected vector to normal
RMDEF Vector3 Vector3Reflect(Vector3 v, Vector3 normal);

// Return min value for each pair of components
RMDEF Vector3 Vector3Min(Vector3 v1, Vector3 v2);

// Return max value for each pair of components
RMDEF Vector3 Vector3Max(Vector3 v1, Vector3 v2);

// Compute barycenter coordinates (u, v, w) for point p with respect to triangle (a, b, c)
// NOTE: Assumes P is on the plane of the triangle
RMDEF Vector3 Vector3Barycenter(Vector3 p, Vector3 a, Vector3 b, Vector3 c);

// Returns Vector3 as float array
RMDEF float3 Vector3ToFloatV(Vector3 v);

//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix math
//----------------------------------------------------------------------------------

// Compute matrix determinant
RMDEF float MatrixDeterminant(Matrix mat);

// Returns the trace of the matrix (sum of the values along the diagonal)
RMDEF float MatrixTrace(Matrix mat);

// Transposes provided matrix
RMDEF Matrix MatrixTranspose(Matrix mat);

// Invert provided matrix
RMDEF Matrix MatrixInvert(Matrix mat);

// Normalize provided matrix
RMDEF Matrix MatrixNormalize(Matrix mat);

// Returns identity matrix
RMDEF Matrix MatrixIdentity(void);

// Add two matrices
RMDEF Matrix MatrixAdd(Matrix left, Matrix right);

// Subtract two matrices (left - right)
RMDEF Matrix MatrixSubtract(Matrix left, Matrix right);

// Returns translation matrix
RMDEF Matrix MatrixTranslate(float x, float y, float z);

// Create rotation matrix from axis and angle
// NOTE: Angle should be provided in radians
RMDEF Matrix MatrixRotate(Vector3 axis, float angle);

// Returns xyz-rotation matrix (angles in radians)
RMDEF Matrix MatrixRotateXYZ(Vector3 ang);

// Returns x-rotation matrix (angle in radians)
RMDEF Matrix MatrixRotateX(float angle);

// Returns y-rotation matrix (angle in radians)
RMDEF Matrix MatrixRotateY(float angle);

// Returns z-rotation matrix (angle in radians)
RMDEF Matrix MatrixRotateZ(float angle);

// Returns scaling matrix
RMDEF Matrix MatrixScale(float x, float y, float z);

// Returns two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
RMDEF Matrix MatrixMultiply(Matrix left, Matrix right);

// Returns perspective projection matrix
RMDEF Matrix MatrixFrustum(double left, double right, double bottom, double top, double near, double far);

// Returns perspective projection matrix
// NOTE: Angle should be provided in radians
RMDEF Matrix MatrixPerspective(double fovy, double aspect, double near, double far);

// Returns orthographic projection matrix
RMDEF Matrix MatrixOrtho(double left, double right, double bottom, double top, double near, double far);

// Returns camera look-at matrix (view matrix)
RMDEF Matrix MatrixLookAt(Vector3 eye, Vector3 target, Vector3 up);

// Returns float array of matrix data
RMDEF float16 MatrixToFloatV(Matrix mat);

//----------------------------------------------------------------------------------
// Module Functions Definition - Quaternion math
//----------------------------------------------------------------------------------

// Returns identity quaternion
RMDEF Quaternion QuaternionIdentity(void);

// Computes the length of a quaternion
RMDEF float QuaternionLength(Quaternion q);

// Normalize provided quaternion
RMDEF Quaternion QuaternionNormalize(Quaternion q);

// Invert provided quaternion
RMDEF Quaternion QuaternionInvert(Quaternion q);

// Calculate two quaternion multiplication
RMDEF Quaternion QuaternionMultiply(Quaternion q1, Quaternion q2);

// Calculate linear interpolation between two quaternions
RMDEF Quaternion QuaternionLerp(Quaternion q1, Quaternion q2, float amount);

// Calculate slerp-optimized interpolation between two quaternions
RMDEF Quaternion QuaternionNlerp(Quaternion q1, Quaternion q2, float amount);

// Calculates spherical linear interpolation between two quaternions
RMDEF Quaternion QuaternionSlerp(Quaternion q1, Quaternion q2, float amount);

// Calculate quaternion based on the rotation from one vector to another
RMDEF Quaternion QuaternionFromVector3ToVector3(Vector3 from, Vector3 to);

// Returns a quaternion for a given rotation matrix
RMDEF Quaternion QuaternionFromMatrix(Matrix mat);

// Returns a matrix for a given quaternion
RMDEF Matrix QuaternionToMatrix(Quaternion q);

// Returns rotation quaternion for an angle and axis
// NOTE: angle must be provided in radians
RMDEF Quaternion QuaternionFromAxisAngle(Vector3 axis, float angle);

// Returns the rotation angle and axis for a given quaternion
RMDEF void QuaternionToAxisAngle(Quaternion q, Vector3 *outAxis, float *outAngle);

// Returns he quaternion equivalent to Euler angles
RMDEF Quaternion QuaternionFromEuler(float roll, float pitch, float yaw);

// Return the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a Vector3 struct in degrees
RMDEF Vector3 QuaternionToEuler(Quaternion q);


// Transform a quaternion given a transformation matrix
RMDEF Quaternion QuaternionTransform(Quaternion q, Matrix mat);

#endif  // RAYMATH_H




