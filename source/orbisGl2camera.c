/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev
 */
//based on raylib camera
/*******************************************************************************************
*
*   raylib.camera - Camera system with multiple modes support
*
*   NOTE: Memory footprint of this library is aproximately 52 bytes (global variables)
*
*   CONFIGURATION:
*
*   #define CAMERA_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define CAMERA_STANDALONE
*       If defined, the library can be used as standalone as a camera system but some
*       functions must be redefined to manage inputs accordingly.
*
*   CONTRIBUTORS:
*       Ramon Santamaria:   Supervision, review, update and maintenance
*       Marc Palau:         Initial implementation (2014)
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


/***********************************************************************************
*
*   CAMERA IMPLEMENTATION
*
************************************************************************************/

#include <orbisGl2.h>




//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
CameraData CAMERA = {        // Global CAMERA state context
    .mode = 0,
    .targetDistance = 0,
    .playerEyesPosition = 1.85f,
    .angle = { 0 },
    .moveControl = { 'W', 'S', 'D', 'A', 'E', 'Q' },
    .smoothZoomControl = 341,
    .altControl = 342,
    .panControl = 2
};

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
//by now we leave CAMERA_STANDALONE MODE
#define CAMERA_STANDALONE
#if defined(CAMERA_STANDALONE)
// NOTE: Camera controls depend on some raylib input functions
void EnableCursor() {}       // Unlock cursor
void DisableCursor() {}      // Lock cursor

bool IsKeyDown(int key) { return 0; }

bool IsMouseButtonDown(int button) { return 0;}
int GetMouseWheelMove() { return 0; }
Vector2 GetMousePosition() { return (Vector2){ 0.0f, 0.0f }; }
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Select camera mode (multiple camera modes available)
void SetCameraMode(Camera camera, int mode)
{
    Vector3 v1 = camera.position;
    Vector3 v2 = camera.target;

    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;

    CAMERA.targetDistance = sqrtf(dx*dx + dy*dy + dz*dz);

    // Camera angle calculation
    CAMERA.angle.x = atan2f(dx, dz);                   // Camera angle in plane XZ (0 aligned with Z, move positive CCW)
    CAMERA.angle.y = atan2f(dy, sqrtf(dx*dx + dz*dz)); // Camera angle in plane XY (0 aligned with X, move positive CW)

    CAMERA.playerEyesPosition = camera.position.y;

    // Lock cursor for first person and third person cameras
    if ((mode == CAMERA_FIRST_PERSON) || (mode == CAMERA_THIRD_PERSON)) DisableCursor();
    else EnableCursor();

    CAMERA.mode = mode;
}

// Update camera depending on selected mode
// NOTE: Camera controls depend on some raylib functions:
//       System: EnableCursor(), DisableCursor()
//       Mouse: IsMouseButtonDown(), GetMousePosition(), GetMouseWheelMove()
//       Keys:  IsKeyDown()
// TODO: Port to quaternion-based camera (?)
void UpdateCamera(Camera *camera)
{
    static int swingCounter = 0;    // Used for 1st person swinging movement
    static Vector2 previousMousePosition = { 0.0f, 0.0f };

    // TODO: Compute CAMERA.targetDistance and CAMERA.angle here (?)

    // Mouse movement detection
    Vector2 mousePositionDelta = { 0.0f, 0.0f };
    Vector2 mousePosition = GetMousePosition();
    int mouseWheelMove = GetMouseWheelMove();

    // Keys input detection
    bool panKey = IsMouseButtonDown(CAMERA.panControl);
    bool altKey = IsKeyDown(CAMERA.altControl);
    bool szoomKey = IsKeyDown(CAMERA.smoothZoomControl);
    bool direction[6] = { IsKeyDown(CAMERA.moveControl[MOVE_FRONT]),
                          IsKeyDown(CAMERA.moveControl[MOVE_BACK]),
                          IsKeyDown(CAMERA.moveControl[MOVE_RIGHT]),
                          IsKeyDown(CAMERA.moveControl[MOVE_LEFT]),
                          IsKeyDown(CAMERA.moveControl[MOVE_UP]),
                          IsKeyDown(CAMERA.moveControl[MOVE_DOWN]) };

    // TODO: Touch input detection (probably gestures system required)

    if (CAMERA.mode != CAMERA_CUSTOM)
    {
        mousePositionDelta.x = mousePosition.x - previousMousePosition.x;
        mousePositionDelta.y = mousePosition.y - previousMousePosition.y;

        previousMousePosition = mousePosition;
    }

    // Support for multiple automatic camera modes
    // NOTE: In case of CAMERA_CUSTOM nothing happens here, user must update it manually
    switch (CAMERA.mode)
    {
        case CAMERA_FREE:           // Camera free controls, using standard 3d-content-creation scheme
        {
            // Camera zoom
            if ((CAMERA.targetDistance < CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                CAMERA.targetDistance -= (mouseWheelMove*CAMERA_MOUSE_SCROLL_SENSITIVITY);
                if (CAMERA.targetDistance > CAMERA_FREE_DISTANCE_MAX_CLAMP) CAMERA.targetDistance = CAMERA_FREE_DISTANCE_MAX_CLAMP;
            }
            
            // Camera looking down
            // TODO: Review, weird comparison of CAMERA.targetDistance == 120.0f?
            else if ((camera->position.y > camera->target.y) && (CAMERA.targetDistance == CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
            }
            else if ((camera->position.y > camera->target.y) && (camera->target.y >= 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;

                // if (camera->target.y < 0) camera->target.y = -0.001;
            }
            else if ((camera->position.y > camera->target.y) && (camera->target.y < 0) && (mouseWheelMove > 0))
            {
                CAMERA.targetDistance -= (mouseWheelMove*CAMERA_MOUSE_SCROLL_SENSITIVITY);
                if (CAMERA.targetDistance < CAMERA_FREE_DISTANCE_MIN_CLAMP) CAMERA.targetDistance = CAMERA_FREE_DISTANCE_MIN_CLAMP;
            }
            // Camera looking up
            // TODO: Review, weird comparisson of CAMERA.targetDistance == 120.0f?
            else if ((camera->position.y < camera->target.y) && (CAMERA.targetDistance == CAMERA_FREE_DISTANCE_MAX_CLAMP) && (mouseWheelMove < 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
            }
            else if ((camera->position.y < camera->target.y) && (camera->target.y <= 0))
            {
                camera->target.x += mouseWheelMove*(camera->target.x - camera->position.x)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.y += mouseWheelMove*(camera->target.y - camera->position.y)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;
                camera->target.z += mouseWheelMove*(camera->target.z - camera->position.z)*CAMERA_MOUSE_SCROLL_SENSITIVITY/CAMERA.targetDistance;

                // if (camera->target.y > 0) camera->target.y = 0.001;
            }
            else if ((camera->position.y < camera->target.y) && (camera->target.y > 0) && (mouseWheelMove > 0))
            {
                CAMERA.targetDistance -= (mouseWheelMove*CAMERA_MOUSE_SCROLL_SENSITIVITY);
                if (CAMERA.targetDistance < CAMERA_FREE_DISTANCE_MIN_CLAMP) CAMERA.targetDistance = CAMERA_FREE_DISTANCE_MIN_CLAMP;
            }

            // Input keys checks
            if (panKey)
            {
                if (altKey)     // Alternative key behaviour
                {
                    if (szoomKey)
                    {
                        // Camera smooth zoom
                        CAMERA.targetDistance += (mousePositionDelta.y*CAMERA_FREE_SMOOTH_ZOOM_SENSITIVITY);
                    }
                    else
                    {
                        // Camera rotation
                        CAMERA.angle.x += mousePositionDelta.x*-CAMERA_FREE_MOUSE_SENSITIVITY;
                        CAMERA.angle.y += mousePositionDelta.y*-CAMERA_FREE_MOUSE_SENSITIVITY;

                        // Angle clamp
                        if (CAMERA.angle.y > CAMERA_FREE_MIN_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_FREE_MIN_CLAMP*DEG2RAD;
                        else if (CAMERA.angle.y < CAMERA_FREE_MAX_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_FREE_MAX_CLAMP*DEG2RAD;
                    }
                }
                else
                {
                    // Camera panning
                    camera->target.x += ((mousePositionDelta.x*CAMERA_FREE_MOUSE_SENSITIVITY)*cosf(CAMERA.angle.x) + (mousePositionDelta.y*CAMERA_FREE_MOUSE_SENSITIVITY)*sinf(CAMERA.angle.x)*sinf(CAMERA.angle.y))*(CAMERA.targetDistance/CAMERA_FREE_PANNING_DIVIDER);
                    camera->target.y += ((mousePositionDelta.y*CAMERA_FREE_MOUSE_SENSITIVITY)*cosf(CAMERA.angle.y))*(CAMERA.targetDistance/CAMERA_FREE_PANNING_DIVIDER);
                    camera->target.z += ((mousePositionDelta.x*-CAMERA_FREE_MOUSE_SENSITIVITY)*sinf(CAMERA.angle.x) + (mousePositionDelta.y*CAMERA_FREE_MOUSE_SENSITIVITY)*cosf(CAMERA.angle.x)*sinf(CAMERA.angle.y))*(CAMERA.targetDistance/CAMERA_FREE_PANNING_DIVIDER);
                }
            }

            // Update camera position with changes
            camera->position.x = -sinf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.x;
            camera->position.y = -sinf(CAMERA.angle.y)*CAMERA.targetDistance + camera->target.y;
            camera->position.z = -cosf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.z;

        } break;
        case CAMERA_ORBITAL:        // Camera just orbits around target, only zoom allowed
        {
            CAMERA.angle.x += CAMERA_ORBITAL_SPEED;      // Camera orbit angle
            CAMERA.targetDistance -= (mouseWheelMove*CAMERA_MOUSE_SCROLL_SENSITIVITY);   // Camera zoom

            // Camera distance clamp
            if (CAMERA.targetDistance < CAMERA_THIRD_PERSON_DISTANCE_CLAMP) CAMERA.targetDistance = CAMERA_THIRD_PERSON_DISTANCE_CLAMP;

            // Update camera position with changes
            camera->position.x = sinf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.x;
            camera->position.y = ((CAMERA.angle.y <= 0.0f)? 1 : -1)*sinf(CAMERA.angle.y)*CAMERA.targetDistance*sinf(CAMERA.angle.y) + camera->target.y;
            camera->position.z = cosf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.z;

        } break;
        case CAMERA_FIRST_PERSON:   // Camera moves as in a first-person game, controls are configurable
        {
            camera->position.x += (sinf(CAMERA.angle.x)*direction[MOVE_BACK] -
                                   sinf(CAMERA.angle.x)*direction[MOVE_FRONT] -
                                   cosf(CAMERA.angle.x)*direction[MOVE_LEFT] +
                                   cosf(CAMERA.angle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.y += (sinf(CAMERA.angle.y)*direction[MOVE_FRONT] -
                                   sinf(CAMERA.angle.y)*direction[MOVE_BACK] +
                                   1.0f*direction[MOVE_UP] - 1.0f*direction[MOVE_DOWN])/PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.z += (cosf(CAMERA.angle.x)*direction[MOVE_BACK] -
                                   cosf(CAMERA.angle.x)*direction[MOVE_FRONT] +
                                   sinf(CAMERA.angle.x)*direction[MOVE_LEFT] -
                                   sinf(CAMERA.angle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

            // Camera orientation calculation
            CAMERA.angle.x += (mousePositionDelta.x*-CAMERA_MOUSE_MOVE_SENSITIVITY);
            CAMERA.angle.y += (mousePositionDelta.y*-CAMERA_MOUSE_MOVE_SENSITIVITY);

            // Angle clamp
            if (CAMERA.angle.y > CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_FIRST_PERSON_MIN_CLAMP*DEG2RAD;
            else if (CAMERA.angle.y < CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_FIRST_PERSON_MAX_CLAMP*DEG2RAD;

            // Recalculate camera target considering translation and rotation
            Matrix translation = MatrixTranslate(0, 0, (CAMERA.targetDistance/CAMERA_FREE_PANNING_DIVIDER));
            Matrix rotation = MatrixRotateXYZ((Vector3){ PI*2 - CAMERA.angle.y, PI*2 - CAMERA.angle.x, 0 });
            Matrix transform = MatrixMultiply(translation, rotation);

            camera->target.x = camera->position.x - transform.m12;
            camera->target.y = camera->position.y - transform.m13;
            camera->target.z = camera->position.z - transform.m14;
            
            // If movement detected (some key pressed), increase swinging
            for (int i = 0; i < 6; i++) if (direction[i]) { swingCounter++; break; }

            // Camera position update
            // NOTE: On CAMERA_FIRST_PERSON player Y-movement is limited to player 'eyes position'
            camera->position.y = CAMERA.playerEyesPosition - sinf(swingCounter/CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER)/CAMERA_FIRST_PERSON_STEP_DIVIDER;

            camera->up.x = sinf(swingCounter/(CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER*2))/CAMERA_FIRST_PERSON_WAVING_DIVIDER;
            camera->up.z = -sinf(swingCounter/(CAMERA_FIRST_PERSON_STEP_TRIGONOMETRIC_DIVIDER*2))/CAMERA_FIRST_PERSON_WAVING_DIVIDER;

        } break;
        case CAMERA_THIRD_PERSON:   // Camera moves as in a third-person game, following target at a distance, controls are configurable
        {
            camera->position.x += (sinf(CAMERA.angle.x)*direction[MOVE_BACK] -
                                   sinf(CAMERA.angle.x)*direction[MOVE_FRONT] -
                                   cosf(CAMERA.angle.x)*direction[MOVE_LEFT] +
                                   cosf(CAMERA.angle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.y += (sinf(CAMERA.angle.y)*direction[MOVE_FRONT] -
                                   sinf(CAMERA.angle.y)*direction[MOVE_BACK] +
                                   1.0f*direction[MOVE_UP] - 1.0f*direction[MOVE_DOWN])/PLAYER_MOVEMENT_SENSITIVITY;

            camera->position.z += (cosf(CAMERA.angle.x)*direction[MOVE_BACK] -
                                   cosf(CAMERA.angle.x)*direction[MOVE_FRONT] +
                                   sinf(CAMERA.angle.x)*direction[MOVE_LEFT] -
                                   sinf(CAMERA.angle.x)*direction[MOVE_RIGHT])/PLAYER_MOVEMENT_SENSITIVITY;

            // Camera orientation calculation
            CAMERA.angle.x += (mousePositionDelta.x*-CAMERA_MOUSE_MOVE_SENSITIVITY);
            CAMERA.angle.y += (mousePositionDelta.y*-CAMERA_MOUSE_MOVE_SENSITIVITY);

            // Angle clamp
            if (CAMERA.angle.y > CAMERA_THIRD_PERSON_MIN_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_THIRD_PERSON_MIN_CLAMP*DEG2RAD;
            else if (CAMERA.angle.y < CAMERA_THIRD_PERSON_MAX_CLAMP*DEG2RAD) CAMERA.angle.y = CAMERA_THIRD_PERSON_MAX_CLAMP*DEG2RAD;

            // Camera zoom
            CAMERA.targetDistance -= (mouseWheelMove*CAMERA_MOUSE_SCROLL_SENSITIVITY);

            // Camera distance clamp
            if (CAMERA.targetDistance < CAMERA_THIRD_PERSON_DISTANCE_CLAMP) CAMERA.targetDistance = CAMERA_THIRD_PERSON_DISTANCE_CLAMP;

            // TODO: It seems camera->position is not correctly updated or some rounding issue makes the camera move straight to camera->target...
            camera->position.x = sinf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.x;
            if (CAMERA.angle.y <= 0.0f) camera->position.y = sinf(CAMERA.angle.y)*CAMERA.targetDistance*sinf(CAMERA.angle.y) + camera->target.y;
            else camera->position.y = -sinf(CAMERA.angle.y)*CAMERA.targetDistance*sinf(CAMERA.angle.y) + camera->target.y;
            camera->position.z = cosf(CAMERA.angle.x)*CAMERA.targetDistance*cosf(CAMERA.angle.y) + camera->target.z;

        } break;
        case CAMERA_CUSTOM: break;
        default: break;
    }
}

// Set camera pan key to combine with mouse movement (free camera)
void SetCameraPanControl(int panKey) { CAMERA.panControl = panKey; }

// Set camera alt key to combine with mouse movement (free camera)
void SetCameraAltControl(int altKey) { CAMERA.altControl = altKey; }

// Set camera smooth zoom key to combine with mouse (free camera)
void SetCameraSmoothZoomControl(int szoomKey) { CAMERA.smoothZoomControl = szoomKey; }

// Set camera move controls (1st person and 3rd person cameras)
void SetCameraMoveControls(int frontKey, int backKey, int rightKey, int leftKey, int upKey, int downKey)
{
    CAMERA.moveControl[MOVE_FRONT] = frontKey;
    CAMERA.moveControl[MOVE_BACK] = backKey;
    CAMERA.moveControl[MOVE_RIGHT] = rightKey;
    CAMERA.moveControl[MOVE_LEFT] = leftKey;
    CAMERA.moveControl[MOVE_UP] = upKey;
    CAMERA.moveControl[MOVE_DOWN] = downKey;
}