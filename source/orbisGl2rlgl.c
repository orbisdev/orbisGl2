/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev
 */
//based on raylib rlgl
/**********************************************************************************************
*
*   rlgl - raylib OpenGL abstraction layer
*
*   rlgl is a wrapper for multiple OpenGL versions (1.1, 2.1, 3.3 Core, ES 2.0) to
*   pseudo-OpenGL 1.1 style functions (rlVertex, rlTranslate, rlRotate...).
*
*   When chosing an OpenGL version greater than OpenGL 1.1, rlgl stores vertex data on internal
*   VBO buffers (and VAOs if available). It requires calling 3 functions:
*       rlglInit()  - Initialize internal buffers and auxiliar resources
*       rlglDraw()  - Process internal buffers and send required draw calls
*       rlglClose() - De-initialize internal buffers data and other auxiliar resources
*
*   CONFIGURATION:
*
*   #define GRAPHICS_API_OPENGL_11
*   #define GRAPHICS_API_OPENGL_21
*   #define GRAPHICS_API_OPENGL_33
*   #define GRAPHICS_API_OPENGL_ES2
*       Use selected OpenGL graphics backend, should be supported by platform
*       Those preprocessor defines are only used on rlgl module, if OpenGL version is
*       required by any other module, use rlGetVersion() tocheck it
*
*   #define RLGL_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define RLGL_STANDALONE
*       Use rlgl as standalone library (no raylib dependency)
*
*   #define SUPPORT_VR_SIMULATOR
*       Support VR simulation functionality (stereo rendering)
*
*   DEPENDENCIES:
*       raymath     - 3D math functionality (Vector3, Matrix, Quaternion)
*       GLAD        - OpenGL extensions loading (OpenGL 3.3 Core only)
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2020 Ramon Santamaria (@raysan5)
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

#include <stdlib.h>
#include <string.h>
#include <debugnet.h>
#include <fcntl.h>
#include <orbisGl2.h>
#include <orbisNfs.h>

extern rlglData RLGL;


//----------------------------------------------------------------------------------
// Module Functions Definition - Matrix operations
//----------------------------------------------------------------------------------


// Choose the current matrix to be transformed
void rlMatrixMode(int mode)
{
	if (mode == RL_PROJECTION) RLGL.State.currentMatrix = &RLGL.State.projection;
	else if (mode == RL_MODELVIEW) RLGL.State.currentMatrix = &RLGL.State.modelview;
	//else if (mode == RL_TEXTURE) // Not supported
	RLGL.State.currentMatrixMode = mode;
}

// Push the current matrix into RLGL.State.stack
void rlPushMatrix(void)
{
	if (RLGL.State.stackCounter >= MAX_MATRIX_STACK_SIZE) debugNetPrintf(ERROR, "[ORBISGL] %s Matrix RLGL.State.stack overflow\n",__FUNCTION__);
	if (RLGL.State.currentMatrixMode == RL_MODELVIEW)
	{
		RLGL.State.doTransform = true;
		RLGL.State.currentMatrix = &RLGL.State.transform;
	}

	RLGL.State.stack[RLGL.State.stackCounter] = *RLGL.State.currentMatrix;
	RLGL.State.stackCounter++;
}

// Pop lattest inserted matrix from RLGL.State.stack
void rlPopMatrix(void)
{
	if (RLGL.State.stackCounter > 0)
	{
		Matrix mat = RLGL.State.stack[RLGL.State.stackCounter - 1];
		*RLGL.State.currentMatrix = mat;
		RLGL.State.stackCounter--;
	}
	if ((RLGL.State.stackCounter == 0) && (RLGL.State.currentMatrixMode == RL_MODELVIEW))
	{
		RLGL.State.currentMatrix = &RLGL.State.modelview;
		RLGL.State.doTransform = false;
	}
}

// Reset current matrix to identity matrix
void rlLoadIdentity(void)
{
	*RLGL.State.currentMatrix = MatrixIdentity();
}

// Multiply the current matrix by a translation matrix
void rlTranslatef(float x, float y, float z)
{
	Matrix matTranslation = MatrixTranslate(x, y, z);
	// NOTE: We transpose matrix with multiplication order
	*RLGL.State.currentMatrix = MatrixMultiply(matTranslation, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by a rotation matrix
void rlRotatef(float angleDeg, float x, float y, float z)
{
	Matrix matRotation = MatrixIdentity();
	Vector3 axis = (Vector3){ x, y, z };
	matRotation = MatrixRotate(Vector3Normalize(axis), angleDeg*DEG2RAD);
	// NOTE: We transpose matrix with multiplication order
	*RLGL.State.currentMatrix = MatrixMultiply(matRotation, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by a scaling matrix
void rlScalef(float x, float y, float z)
{
	Matrix matScale = MatrixScale(x, y, z);
	// NOTE: We transpose matrix with multiplication order
	*RLGL.State.currentMatrix = MatrixMultiply(matScale, *RLGL.State.currentMatrix);
}

// Multiply the current matrix by another matrix
void rlMultMatrixf(float *matf)
{
	// Matrix creation from array
	Matrix mat={matf[0], matf[4], matf[8], matf[12],
				matf[1], matf[5], matf[9], matf[13],
				matf[2], matf[6], matf[10], matf[14],
				matf[3], matf[7], matf[11], matf[15]};
	*RLGL.State.currentMatrix = MatrixMultiply(*RLGL.State.currentMatrix, mat);
}

// Multiply the current matrix by a perspective matrix generated by parameters
void rlFrustum(double left, double right, double bottom, double top, double znear, double zfar)
{
	Matrix matPerps = MatrixFrustum(left, right, bottom, top, znear, zfar);
	*RLGL.State.currentMatrix = MatrixMultiply(*RLGL.State.currentMatrix, matPerps);
}

// Multiply the current matrix by an orthographic matrix generated by parameters
void rlOrtho(double left, double right, double bottom, double top, double znear, double zfar)
{
	Matrix matOrtho = MatrixOrtho(left, right, bottom, top, znear, zfar);
	*RLGL.State.currentMatrix = MatrixMultiply(*RLGL.State.currentMatrix, matOrtho);
}


// Set the viewport area (transformation from normalized device coordinates to window coordinates)
// NOTE: Updates global variables: RLGL.State.framebufferWidth, RLGL.State.framebufferHeight
void rlViewport(int x, int y, int width, int height)
{
	glViewport(x, y, width, height);
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Vertex level operations
//----------------------------------------------------------------------------------


// Initialize drawing mode (how to organize vertex)
void rlBegin(int mode)
{
	// Draw mode can be RL_LINES, RL_TRIANGLES and RL_QUADS
    // NOTE: In all three cases, vertex are accumulated over default internal vertex buffer
    if (RLGL.State.draws[RLGL.State.drawsCounter - 1].mode != mode)
    {
        if (RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount > 0)
        {
            // Make sure current RLGL.State.draws[i].vertexCount is aligned a multiple of 4,
            // that way, following QUADS drawing will keep aligned with index processing
            // It implies adding some extra alignment vertex at the end of the draw,
            // those vertex are not processed but they are considered as an additional offset
            // for the next set of vertex to be drawn
            if (RLGL.State.draws[RLGL.State.drawsCounter - 1].mode == RL_LINES) RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = ((RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount < 4)? RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount : RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount%4);
            else if (RLGL.State.draws[RLGL.State.drawsCounter - 1].mode == RL_TRIANGLES) RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = ((RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount < 4)? 1 : (4 - (RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount%4)));

            else RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = 0;

            if (rlCheckBufferLimit(RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment)) rlglDraw();
            else
            {
                RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;
                RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;
                RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;

                RLGL.State.drawsCounter++;
            }
        }

        if (RLGL.State.drawsCounter >= MAX_DRAWCALL_REGISTERED) rlglDraw();

        RLGL.State.draws[RLGL.State.drawsCounter - 1].mode = mode;
        RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount = 0;
        RLGL.State.draws[RLGL.State.drawsCounter - 1].textureId = RLGL.State.defaultTextureId;
    }
}

// Finish vertex providing
void rlEnd(void)
{
	// Make sure vertexCount is the same for vertices, texcoords, colors and normals
	// NOTE: In OpenGL 1.1, one glColor call can be made for all the subsequent glVertex calls
	// Make sure colors count match vertex count
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter != RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter)
	{
		int addColors = RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter - RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter;
		for (int i = 0; i < addColors; i++)
		{
			RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter] = RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter - 4];
			RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 1] = RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter - 3];
			RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 2] = RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter - 2];
			RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 3] = RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter - 1];
			RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter++;
		}
	}
	// Make sure texcoords count match vertex count
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter != RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter)
	{
		int addTexCoords = RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter - RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter;
		for (int i = 0; i < addTexCoords; i++)
		{
			RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords[2*RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter] = 0.0f;
			RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords[2*RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter + 1] = 0.0f;
			RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter++;
		}
	}
	// TODO: Make sure normals count match vertex count... if normals support is added in a future... :P
	// NOTE: Depth increment is dependant on rlOrtho(): z-near and z-far values,
	// as well as depth buffer bit-depth (16bit or 24bit or 32bit)
	// Correct increment formula would be: depthInc = (zfar - znear)/pow(2, bits)
	RLGL.State.currentDepth += (1.0f/20000.0f);
	// Verify internal buffers limits
	// NOTE: This check is combined with usage of rlCheckBufferLimit()
	if ((RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter) >= (MAX_BATCH_ELEMENTS*4 - 4))
	{
		// WARNING: If we are between rlPushMatrix() and rlPopMatrix() and we need to force a rlglDraw(),
		// we need to call rlPopMatrix() before to recover *RLGL.State.currentMatrix (RLGL.State.modelview) for the next forced draw call!
		// If we have multiple matrix pushed, it will require "RLGL.State.stackCounter" pops before launching the draw
		for (int i = RLGL.State.stackCounter; i >= 0; i--) rlPopMatrix();
		rlglDraw();
	}
}

// Define one vertex (position)
// NOTE: Vertex position data is the basic information required for drawing
void rlVertex3f(float x, float y, float z)
{
	Vector3 vec = { x, y, z };
	// Transform provided vector if required
	if (RLGL.State.doTransform) vec = Vector3Transform(vec, RLGL.State.transform);
	// Verify that MAX_BATCH_ELEMENTS limit not reached
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter < (MAX_BATCH_ELEMENTS*4))
	{
		RLGL.State.vertexData[RLGL.State.currentBuffer].vertices[3*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter] = vec.x;
		RLGL.State.vertexData[RLGL.State.currentBuffer].vertices[3*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter + 1] = vec.y;
		RLGL.State.vertexData[RLGL.State.currentBuffer].vertices[3*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter + 2] = vec.z;
		RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter++;
		RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount++;
	}
	else debugNetPrintf(ERROR,"[ORBISGL] %s MAX_BATCH_ELEMENTS overflow\n",__FUNCTION__);
}

// Define one vertex (position)
void rlVertex2f(float x, float y)
{
	rlVertex3f(x, y, RLGL.State.currentDepth);
}

// Define one vertex (position)
void rlVertex2i(int x, int y)
{
	rlVertex3f((float)x, (float)y, RLGL.State.currentDepth);
}

// Define one vertex (texture coordinate)
// NOTE: Texture coordinates are limited to QUADS only
void rlTexCoord2f(float x, float y)
{
	RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords[2*RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter] = x;
	RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords[2*RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter + 1] = y;
	RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter++;
}

// Define one vertex (normal)
// NOTE: Normals limited to TRIANGLES only?
void rlNormal3f(float x, float y, float z)
{
	// TODO: Normals usage...
}

// Define one vertex (color)
void rlColor4ub(byte x, byte y, byte z, byte w)
{
	RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter] = x;
	RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 1] = y;
	RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 2] = z;
	RLGL.State.vertexData[RLGL.State.currentBuffer].colors[4*RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter + 3] = w;
	RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter++;
}

// Define one vertex (color)
void rlColor4f(float r, float g, float b, float a)
{
	rlColor4ub((byte)(r*255), (byte)(g*255), (byte)(b*255), (byte)(a*255));
}

// Define one vertex (color)
void rlColor3f(float x, float y, float z)
{
	rlColor4ub((byte)(x*255), (byte)(y*255), (byte)(z*255), 255);
}



//----------------------------------------------------------------------------------
// Module Functions Definition - OpenGL equivalent functions (common to 1.1, 3.3+, ES2)
//----------------------------------------------------------------------------------

// Enable texture usage
void rlEnableTexture(unsigned int id)
{
	if (RLGL.State.draws[RLGL.State.drawsCounter - 1].textureId != id)
	{
		if (RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount > 0)
		{
			// Make sure current RLGL.State.draws[i].vertexCount is aligned a multiple of 4,
			// that way, following QUADS drawing will keep aligned with index processing
			// It implies adding some extra alignment vertex at the end of the draw,
			// those vertex are not processed but they are considered as an additional offset
			// for the next set of vertex to be drawn
			if (RLGL.State.draws[RLGL.State.drawsCounter - 1].mode == RL_LINES) RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = ((RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount < 4)? RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount : RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount%4);
			else if (RLGL.State.draws[RLGL.State.drawsCounter - 1].mode == RL_TRIANGLES) RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = ((RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount < 4)? 1 : (4 - (RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount%4)));
			else RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment = 0;
			if (rlCheckBufferLimit(RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment)) rlglDraw();
			else
			{
				RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;
				RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;
				RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter += RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexAlignment;
				RLGL.State.drawsCounter++;
			}
		}
		if (RLGL.State.drawsCounter >= MAX_DRAWCALL_REGISTERED) rlglDraw();
		RLGL.State.draws[RLGL.State.drawsCounter - 1].textureId = id;
		RLGL.State.draws[RLGL.State.drawsCounter - 1].vertexCount = 0;
	}
}

// Disable texture usage
void rlDisableTexture(void)
{
	// NOTE: If quads batch limit is reached,
	// we force a draw call and next batch starts
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter >= (MAX_BATCH_ELEMENTS*4)) rlglDraw();
}

// Set texture parameters (wrap mode/filter mode)
void rlTextureParameters(unsigned int id, int param, int value)
{
	glBindTexture(GL_TEXTURE_2D, id);
	switch (param)
	{
		case RL_TEXTURE_WRAP_S:
		case RL_TEXTURE_WRAP_T:
		{
			if (value == RL_WRAP_MIRROR_CLAMP)
			{
				if (RLGL.ExtSupported.texMirrorClamp) glTexParameteri(GL_TEXTURE_2D, param, value);
				else debugNetPrintf(ERROR, "[ORBISGL] %s Clamp mirror wrap mode not supported\n",__FUNCTION__);
			}
			else glTexParameteri(GL_TEXTURE_2D, param, value);
		} break;
		case RL_TEXTURE_MAG_FILTER:
		case RL_TEXTURE_MIN_FILTER: glTexParameteri(GL_TEXTURE_2D, param, value); break;
		case RL_TEXTURE_ANISOTROPIC_FILTER:
		{
			if (value <= RLGL.ExtSupported.maxAnisotropicLevel) glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
			else if (RLGL.ExtSupported.maxAnisotropicLevel > 0.0f)
			{
				debugNetPrintf(ERROR, "[ORBISGL] %s [TEX ID %i] Maximum anisotropic filter level supported is %iX\n",__FUNCTION__,id, RLGL.ExtSupported.maxAnisotropicLevel);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)value);
			}
			else debugNetPrintf(ERROR, "[ORBISGL] %s Anisotropic filtering not supported\n",__FUNCTION__);
		} break;
		default: break;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Enable rendering to texture (fbo)
void rlEnableRenderTexture(unsigned int id)
{
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	//glDisable(GL_CULL_FACE);    // Allow double side drawing for texture flipping
	//glCullFace(GL_FRONT);
}

// Disable rendering to texture
void rlDisableRenderTexture(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
}

// Enable depth test
void rlEnableDepthTest(void) { glEnable(GL_DEPTH_TEST); }

// Disable depth test
void rlDisableDepthTest(void) { glDisable(GL_DEPTH_TEST); }

// Enable backface culling
void rlEnableBackfaceCulling(void) { glEnable(GL_CULL_FACE); }

// Disable backface culling
void rlDisableBackfaceCulling(void) { glDisable(GL_CULL_FACE); }

// Enable scissor test
void rlEnableScissorTest(void) { glEnable(GL_SCISSOR_TEST); }

// Disable scissor test
void rlDisableScissorTest(void) { glDisable(GL_SCISSOR_TEST); }

// Scissor test
void rlScissor(int x, int y, int width, int height) { glScissor(x, y, width, height); }

// Enable wire mode
void rlEnableWireMode(void)
{
	// NOTE: glPolygonMode() not available on OpenGL ES
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

// Disable wire mode
void rlDisableWireMode(void)
{
    // NOTE: glPolygonMode() not available on OpenGL ES
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Unload texture from GPU memory
void rlDeleteTextures(unsigned int id)
{
	if (id > 0) glDeleteTextures(1, &id);
}

// Unload render texture from GPU memory
void rlDeleteRenderTextures(RenderTexture2D target)
{
	if (target.texture.id > 0) glDeleteTextures(1, &target.texture.id);
	if (target.depth.id > 0)
	{
		if (target.depthTexture) glDeleteTextures(1, &target.depth.id);
		else glDeleteRenderbuffers(1, &target.depth.id);
	}
	if (target.id > 0) glDeleteFramebuffers(1, &target.id);
	debugNetPrintf(INFO, "[ORBISGL] %s [FBO ID %i] Unloaded render texture data from VRAM (GPU)\n",__FUNCTION__, target.id);
}

// Unload shader from GPU memory
void rlDeleteShader(unsigned int id)
{
	if (id != 0) glDeleteProgram(id);
}

// Unload vertex data (VAO) from GPU memory
void rlDeleteVertexArrays(unsigned int id)
{
	if (RLGL.ExtSupported.vao)
	{
		if (id != 0) glDeleteVertexArrays(1, &id);
		debugNetPrintf(INFO, "[ORBISG] %s [VAO ID %i] Unloaded model data from VRAM (GPU)\n",__FUNCTION__, id);
	}
}

// Unload vertex data (VBO) from GPU memory
void rlDeleteBuffers(unsigned int id)
{
	if (id != 0)
	{
		glDeleteBuffers(1, &id);
		if (!RLGL.ExtSupported.vao) debugNetPrintf(INFO, "[ORBISGL] %s [VBO ID %i] Unloaded model vertex data from VRAM (GPU)\n",__FUNCTION__,id);
	}
}

// Clear color buffer with color
void rlClearColor(byte r, byte g, byte b, byte a)
{
	// Color values clamp to 0.0f(0) and 1.0f(255)
	float cr = (float)r/255;
	float cg = (float)g/255;
	float cb = (float)b/255;
	float ca = (float)a/255;
	glClearColor(cr, cg, cb, ca);
}

// Clear used screen buffers (color and depth)
void rlClearScreenBuffers(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear used buffers: Color and Depth (Depth is used for 3D)
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);     // Stencil buffer not used...
}

// Update GPU buffer with new data
void rlUpdateBuffer(int bufferId, void *data, int dataSize)
{
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);
	glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, data);
}

//----------------------------------------------------------------------------------
// Module Functions Definition - rlgl Functions
//----------------------------------------------------------------------------------


// Initialize rlgl: OpenGL extensions, default buffers/shaders/textures, OpenGL states
void rlglInit(int width, int height)
{
	// Check OpenGL information and capabilities
	//------------------------------------------------------------------------------
	// Print current OpenGL and GLSL version
	debugNetPrintf(INFO, "GPU: Vendor:   %s", glGetString(GL_VENDOR));
	debugNetPrintf(INFO, "GPU: Renderer: %s", glGetString(GL_RENDERER));
	debugNetPrintf(INFO, "GPU: Version:  %s", glGetString(GL_VERSION));
	debugNetPrintf(INFO, "GPU: GLSL:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	// NOTE: We can get a bunch of extra information about GPU capabilities (glGet*)
	//int maxTexSize;
	//glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	//debugNetPrintf(LOG_INFO, "GL_MAX_TEXTURE_SIZE: %i", maxTexSize);
	//GL_MAX_TEXTURE_IMAGE_UNITS
	//GL_MAX_VIEWPORT_DIMS
	//int numAuxBuffers;
	//glGetIntegerv(GL_AUX_BUFFERS, &numAuxBuffers);
	//debugNetPrintf(LOG_INFO, "GL_AUX_BUFFERS: %i", numAuxBuffers);
	//GLint numComp = 0;
	//GLint format[32] = { 0 };
	//glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numComp);
	//glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, format);
	//for (int i = 0; i < numComp; i++) debugNetPrintf(LOG_INFO, "Supported compressed format: 0x%x", format[i]);
	// NOTE: We don't need that much data on screen... right now...
	// TODO: Automatize extensions loading using rlLoadExtensions() and GLAD
	// Actually, when rlglInit() is called in InitWindow() in core.c,
	// OpenGL required extensions have already been loaded (PLATFORM_DESKTOP)
    // Get supported extensions list
	GLint numExt = 0;
	// Allocate 512 strings pointers (2 KB)
	const char **extList = malloc(sizeof(const char *)*512);
	const char *extensions = (const char *)glGetString(GL_EXTENSIONS);  // One big const string
	// NOTE: We have to duplicate string because glGetString() returns a const string
	int len = strlen(extensions) + 1;
	char *extensionsDup = (char *)calloc(len, sizeof(char));
	strcpy(extensionsDup, extensions);
	extList[numExt] = extensionsDup;
	for (int i = 0; i < len; i++)
	{
		if (extensionsDup[i] == ' ')
		{
			extensionsDup[i] = '\0';
			numExt++;
			extList[numExt] = &extensionsDup[i + 1];
		}
	}
	// NOTE: Duplicated string (extensionsDup) must be deallocated
	debugNetPrintf(INFO, "Number of supported extensions: %i", numExt);
	// Show supported extensions
	//for (int i = 0; i < numExt; i++)  debugNetPrintf(LOG_INFO, "Supported extension: %s", extList[i]);
	// Check required extensions
	for (int i = 0; i < numExt; i++)
	{
		// Check VAO support
		// NOTE: Only check on OpenGL ES, OpenGL 3.3 has VAO support as core feature
		if (strcmp(extList[i], (const char *)"GL_OES_vertex_array_object") == 0)
		{
			// The extension is supported by our hardware and driver, try to get related functions pointers
			// NOTE: emscripten does not support VAOs natively, it uses emulation and it reduces overall performance...
			glGenVertexArrays = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
			glBindVertexArray = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
			glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
			//glIsVertexArray = (PFNGLISVERTEXARRAYOESPROC)eglGetProcAddress("glIsVertexArrayOES");     // NOTE: Fails in WebGL, omitted
			if ((glGenVertexArrays != NULL) && (glBindVertexArray != NULL) && (glDeleteVertexArrays != NULL)) RLGL.ExtSupported.vao = true;
		}
		// Check NPOT textures support
		// NOTE: Only check on OpenGL ES, OpenGL 3.3 has NPOT textures full support as core feature
		if (strcmp(extList[i], (const char *)"GL_OES_texture_npot") == 0) RLGL.ExtSupported.texNPOT = true;
		// Check texture float support
		if (strcmp(extList[i], (const char *)"GL_OES_texture_float") == 0) RLGL.ExtSupported.texFloat32 = true;
		// Check depth texture support
		if ((strcmp(extList[i], (const char *)"GL_OES_depth_texture") == 0) ||
			(strcmp(extList[i], (const char *)"GL_WEBGL_depth_texture") == 0)) RLGL.ExtSupported.texDepth = true;
		if (strcmp(extList[i], (const char *)"GL_OES_depth24") == 0) RLGL.ExtSupported.maxDepthBits = 24;
		if (strcmp(extList[i], (const char *)"GL_OES_depth32") == 0) RLGL.ExtSupported.maxDepthBits = 32;
		// DDS texture compression support
		if ((strcmp(extList[i], (const char *)"GL_EXT_texture_compression_s3tc") == 0) ||
			(strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_s3tc") == 0) ||
			(strcmp(extList[i], (const char *)"GL_WEBKIT_WEBGL_compressed_texture_s3tc") == 0)) RLGL.ExtSupported.texCompDXT = true;
		// ETC1 texture compression support
		if ((strcmp(extList[i], (const char *)"GL_OES_compressed_ETC1_RGB8_texture") == 0) ||
			(strcmp(extList[i], (const char *)"GL_WEBGL_compressed_texture_etc1") == 0)) RLGL.ExtSupported.texCompETC1 = true;
		// ETC2/EAC texture compression support
		if (strcmp(extList[i], (const char *)"GL_ARB_ES3_compatibility") == 0) RLGL.ExtSupported.texCompETC2 = true;
		// PVR texture compression support
		if (strcmp(extList[i], (const char *)"GL_IMG_texture_compression_pvrtc") == 0) RLGL.ExtSupported.texCompPVRT = true;
		// ASTC texture compression support
		if (strcmp(extList[i], (const char *)"GL_KHR_texture_compression_astc_hdr") == 0) RLGL.ExtSupported.texCompASTC = true;
		// Anisotropic texture filter support
		if (strcmp(extList[i], (const char *)"GL_EXT_texture_filter_anisotropic") == 0)
		{
			RLGL.ExtSupported.texAnisoFilter = true;
			glGetFloatv(0x84FF, &RLGL.ExtSupported.maxAnisotropicLevel);   // GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
		}
		// Clamp mirror wrap mode supported
		if (strcmp(extList[i], (const char *)"GL_EXT_texture_mirror_clamp") == 0) RLGL.ExtSupported.texMirrorClamp = true;
		// Debug marker support
		if (strcmp(extList[i], (const char *)"GL_EXT_debug_marker") == 0) RLGL.ExtSupported.debugMarker = true;
	}

	// Free extensions pointers
	free(extList);
	free(extensionsDup);    // Duplicated string must be deallocated
	if (RLGL.ExtSupported.vao) debugNetPrintf(INFO, "[EXTENSION] VAO extension detected, VAO functions initialized successfully");
	else debugNetPrintf(ERROR, "[EXTENSION] VAO extension not found, VAO usage not supported");
	if (RLGL.ExtSupported.texNPOT) debugNetPrintf(INFO, "[EXTENSION] NPOT textures extension detected, full NPOT textures supported");
	else debugNetPrintf(ERROR, "[EXTENSION] NPOT textures extension not found, limited NPOT support (no-mipmaps, no-repeat)");
	if (RLGL.ExtSupported.texCompDXT) debugNetPrintf(INFO, "[EXTENSION] DXT compressed textures supported");
	if (RLGL.ExtSupported.texCompETC1) debugNetPrintf(INFO, "[EXTENSION] ETC1 compressed textures supported");
	if (RLGL.ExtSupported.texCompETC2) debugNetPrintf(INFO, "[EXTENSION] ETC2/EAC compressed textures supported");
	if (RLGL.ExtSupported.texCompPVRT) debugNetPrintf(INFO, "[EXTENSION] PVRT compressed textures supported");
	if (RLGL.ExtSupported.texCompASTC) debugNetPrintf(INFO, "[EXTENSION] ASTC compressed textures supported");
	if (RLGL.ExtSupported.texAnisoFilter) debugNetPrintf(INFO, "[EXTENSION] Anisotropic textures filtering supported (max: %.0fX)", RLGL.ExtSupported.maxAnisotropicLevel);
	if (RLGL.ExtSupported.texMirrorClamp) debugNetPrintf(INFO, "[EXTENSION] Mirror clamp wrap texture mode supported");
	if (RLGL.ExtSupported.debugMarker) debugNetPrintf(INFO, "[EXTENSION] Debug Marker supported");
	// Initialize buffers, default shaders and default textures
	//----------------------------------------------------------
	// Init default white texture
	unsigned char pixels[4] = { 255, 255, 255, 255 };   // 1 pixel RGBA (4 bytes)
	RLGL.State.defaultTextureId = rlLoadTexture(pixels, 1, 1, UNCOMPRESSED_R8G8B8A8, 1);
	if (RLGL.State.defaultTextureId != 0) debugNetPrintf(INFO, "[TEX ID %i] Base white texture loaded successfully", RLGL.State.defaultTextureId);
	else debugNetPrintf(ERROR, "Base white texture could not be loaded");
	// Init default Shader (customized for GL 3.3 and ES2)
	RLGL.State.defaultShader = LoadShaderDefault();
	RLGL.State.currentShader = RLGL.State.defaultShader;
	// Init default vertex arrays buffers
	LoadBuffersDefault();
	// Init transformations matrix accumulator
	RLGL.State.transform = MatrixIdentity();
	// Init draw calls tracking system
	RLGL.State.draws = (DrawCall *)malloc(sizeof(DrawCall)*MAX_DRAWCALL_REGISTERED);
	for (int i = 0; i < MAX_DRAWCALL_REGISTERED; i++)
	{
		RLGL.State.draws[i].mode = RL_QUADS;
		RLGL.State.draws[i].vertexCount = 0;
		RLGL.State.draws[i].vertexAlignment = 0;
		//RLGL.State.draws[i].vaoId = 0;
		//RLGL.State.draws[i].shaderId = 0;
		RLGL.State.draws[i].textureId = RLGL.State.defaultTextureId;
		//RLGL.State.draws[i].RLGL.State.projection = MatrixIdentity();
		//RLGL.State.draws[i].RLGL.State.modelview = MatrixIdentity();
	}
	RLGL.State.drawsCounter = 1;
	// Init RLGL.State.stack matrices (emulating OpenGL 1.1)
	for (int i = 0; i < MAX_MATRIX_STACK_SIZE; i++) RLGL.State.stack[i] = MatrixIdentity();
	// Init RLGL.State.projection and RLGL.State.modelview matrices
	RLGL.State.projection = MatrixIdentity();
	RLGL.State.modelview = MatrixIdentity();
	RLGL.State.currentMatrix = &RLGL.State.modelview;
	// Initialize OpenGL default states
	//----------------------------------------------------------
	// Init state: Depth test
	glDepthFunc(GL_LEQUAL);                                 // Type of depth testing to apply
	glDisable(GL_DEPTH_TEST);                               // Disable depth testing for 2D (only used for 3D)
	// Init state: Blending mode
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);      // Color blending function (how colors are mixed)
	glEnable(GL_BLEND);                                     // Enable color blending (required to work with transparencies)
	// Init state: Culling
	// NOTE: All shapes/models triangles are drawn CCW
	glCullFace(GL_BACK);                                    // Cull the back face (default)
	glFrontFace(GL_CCW);                                    // Front face are defined counter clockwise (default)
	glEnable(GL_CULL_FACE);                                 // Enable backface culling

	// Init state: Color/Depth buffers clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);                   // Set clear color (black)
	glClearDepth(1.0f);                                     // Set clear depth value (default)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);     // Clear color and depth buffers (depth buffer required for 3D)
	// Store screen size into global variables
	RLGL.State.framebufferWidth = width;
	RLGL.State.framebufferHeight = height;
	// Init texture and rectangle used on basic shapes drawing
	RLGL.State.shapesTexture = GetTextureDefault();
	RLGL.State.shapesTextureRec = (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f };
	debugNetPrintf(INFO, "OpenGL default states initialized successfully");
}

// Vertex Buffer Object deinitialization (memory free)
void rlglClose(void)
{
	UnloadShaderDefault();              // Unload default shader
	UnloadBuffersDefault();             // Unload default buffers
	glDeleteTextures(1, &RLGL.State.defaultTextureId); // Unload default texture
	debugNetPrintf(INFO, "[ORBISGL] [TEX ID %i] Unloaded texture data (base white texture) from VRAM\n", RLGL.State.defaultTextureId);
	free(RLGL.State.draws);
}

// Update and draw internal buffers
void rlglDraw(void)
{
	// Only process data if we have data to process
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter > 0)
	{
		UpdateBuffersDefault();
		DrawBuffersDefault();       // NOTE: Stereo rendering is checked inside
	}
}

// Returns current OpenGL version
int rlGetVersion(void)
{
    return OPENGL_ES_20;
}

// Check internal buffer overflow for a given number of vertex
bool rlCheckBufferLimit(int vCount)
{
	bool overflow = false;
	if ((RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter + vCount) >= (MAX_BATCH_ELEMENTS*4)) overflow = true;
	return overflow;
}

// Set debug marker
void rlSetDebugMarker(const char *text)
{
	//if(RLGL.ExtSupported.debugMarker) glInsertEventMarkerEXT(0, text);

}

// Load OpenGL extensions
// NOTE: External loader function could be passed as a pointer
void rlLoadExtensions(void *loader)
{

}

// Get world coordinates from screen coordinates
Vector3 rlUnproject(Vector3 source, Matrix proj, Matrix view)
{
	Vector3 result = { 0.0f, 0.0f, 0.0f };
	// Calculate unproject matrix (multiply view patrix by projection matrix) and invert it
	Matrix matViewProj = MatrixMultiply(view, proj);
	matViewProj = MatrixInvert(matViewProj);
	// Create quaternion from source point
	Quaternion quat = { source.x, source.y, source.z, 1.0f };
	// Multiply quat point by unproject matrix
	quat = QuaternionTransform(quat, matViewProj);
	// Normalized world points in vectors
	result.x = quat.x/quat.w;
	result.y = quat.y/quat.w;
	result.z = quat.z/quat.w;
	return result;
}

// Convert image data to OpenGL texture (returns OpenGL valid Id)
unsigned int rlLoadTexture(void *data, int width, int height, int format, int mipmapCount)
{
	glBindTexture(GL_TEXTURE_2D, 0);    // Free any old binding
	unsigned int id = 0;
	
	if ((!RLGL.ExtSupported.texCompDXT) && ((format == COMPRESSED_DXT1_RGB) || (format == COMPRESSED_DXT1_RGBA) ||
		(format == COMPRESSED_DXT3_RGBA) || (format == COMPRESSED_DXT5_RGBA)))
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s DXT compressed texture format not supported\n",__FUNCTION__);
		return id;
	}
	if ((!RLGL.ExtSupported.texCompETC1) && (format == COMPRESSED_ETC1_RGB))
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s ETC1 compressed texture format not supported\n",__FUNCTION__);
		return id;
	}
	if ((!RLGL.ExtSupported.texCompETC2) && ((format == COMPRESSED_ETC2_RGB) || (format == COMPRESSED_ETC2_EAC_RGBA)))
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s ETC2 compressed texture format not supported\n",__FUNCTION__);
		return id;
	}
	if ((!RLGL.ExtSupported.texCompPVRT) && ((format == COMPRESSED_PVRT_RGB) || (format == COMPRESSED_PVRT_RGBA)))
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s PVRT compressed texture format not supported\n",__FUNCTION__);
		return id;
	}
	if ((!RLGL.ExtSupported.texCompASTC) && ((format == COMPRESSED_ASTC_4x4_RGBA) || (format == COMPRESSED_ASTC_8x8_RGBA)))
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s ASTC compressed texture format not supported\n",__FUNCTION__);
		return id;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &id);              // Generate texture id
	//glActiveTexture(GL_TEXTURE0);     // If not defined, using GL_TEXTURE0 by default (shader texture)
	glBindTexture(GL_TEXTURE_2D, id);
	int mipWidth = width;
	int mipHeight = height;
	int mipOffset = 0;          // Mipmap data offset
	debugNetPrintf(DEBUG,"[ORBISGL] %s Load texture from data memory address: 0x%x\n",__FUNCTION__,data);
	// Load the different mipmap levels
	for (int i = 0; i < mipmapCount; i++)
	{
		unsigned int mipSize = GetPixelDataSize(mipWidth, mipHeight, format);
		unsigned int glInternalFormat, glFormat, glType;
		rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
		debugNetPrintf(DEBUG,"[ORBISGL] %s Load mipmap level %i (%i x %i), size: %i, offset: %i\n",__FUNCTION__,i, mipWidth, mipHeight, mipSize, mipOffset);
		if (glInternalFormat != -1)
		{
			if (format < COMPRESSED_DXT1_RGB) glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, glFormat, glType, (unsigned char *)data + mipOffset);
			else glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, mipWidth, mipHeight, 0, mipSize, (unsigned char *)data + mipOffset);
		}
		mipWidth /= 2;
		mipHeight /= 2;
		mipOffset += mipSize;
		// Security check for NPOT textures
		if (mipWidth < 1) mipWidth = 1;
		if (mipHeight < 1) mipHeight = 1;
	}
	// Texture parameters configuration
	// NOTE: glTexParameteri does NOT affect texture uploading, just the way it's used
	// NOTE: OpenGL ES 2.0 with no GL_OES_texture_npot support (i.e. WebGL) has limited NPOT support, so CLAMP_TO_EDGE must be used
	if (RLGL.ExtSupported.texNPOT)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);       // Set texture to repeat on x-axis
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);       // Set texture to repeat on y-axis
	}
	else
	{
		// NOTE: If using negative texture coordinates (LoadOBJ()), it does not work!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);       // Set texture to clamp on x-axis
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);       // Set texture to clamp on y-axis
	}
	// Magnification and minification filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // Alternative: GL_LINEAR
	// At this point we have the texture loaded in GPU and texture parameters configured
	// NOTE: If mipmaps were not in data, they are not generated automatically
	// Unbind current texture
	glBindTexture(GL_TEXTURE_2D, 0);
	if (id > 0) debugNetPrintf(INFO, "[ORBISGL] %s [TEX ID %i] Texture created successfully (%ix%i - %i mipmaps)\n",__FUNCTION__,id, width, height, mipmapCount);
	else debugNetPrintf(ERROR, "[ORBISGL] %s Texture could not be created\n",__FUNCTION__);
	return id;
}

// Load depth texture/renderbuffer (to be attached to fbo)
// WARNING: OpenGL ES 2.0 requires GL_OES_depth_texture/WEBGL_depth_texture extensions
unsigned int rlLoadTextureDepth(int width, int height, int bits, bool useRenderBuffer)
{
	unsigned int id = 0;
	unsigned int glInternalFormat = GL_DEPTH_COMPONENT16;
	if ((bits != 16) && (bits != 24) && (bits != 32)) bits = 16;
	if (bits == 24)
	{
		if (RLGL.ExtSupported.maxDepthBits >= 24) glInternalFormat = GL_DEPTH_COMPONENT24_OES;
	}
	if (bits == 32)
	{
		if (RLGL.ExtSupported.maxDepthBits == 32) glInternalFormat = GL_DEPTH_COMPONENT32_OES;
	}
	if (!useRenderBuffer && RLGL.ExtSupported.texDepth)
	{
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		// Create the renderbuffer that will serve as the depth attachment for the framebuffer
		// NOTE: A renderbuffer is simpler than a texture and could offer better performance on embedded devices
		glGenRenderbuffers(1, &id);
		glBindRenderbuffer(GL_RENDERBUFFER, id);
		glRenderbufferStorage(GL_RENDERBUFFER, glInternalFormat, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	return id;
}

// Load texture cubemap
// NOTE: Cubemap data is expected to be 6 images in a single column,
// expected the following convention: +X, -X, +Y, -Y, +Z, -Z
unsigned int rlLoadTextureCubemap(void *data, int size, int format)
{
	unsigned int cubemapId = 0;
	unsigned int dataSize = GetPixelDataSize(size, size, format);
	glGenTextures(1, &cubemapId);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapId);
	unsigned int glInternalFormat, glFormat, glType;
	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
	if (glInternalFormat != -1)
	{
		// Load cubemap faces
		for (unsigned int i = 0; i < 6; i++)
		{
			if (format < COMPRESSED_DXT1_RGB) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, glFormat, glType, (unsigned char *)data + i*dataSize);
			else glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, size, size, 0, dataSize, (unsigned char *)data + i*dataSize);
		}
	}
	// Set cubemap texture sampling parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return cubemapId;
}

// Update already loaded texture in GPU with new data
// NOTE: We don't know safely if internal texture format is the expected one...
void rlUpdateTexture(unsigned int id, int width, int height, int format, const void *data)
{
	glBindTexture(GL_TEXTURE_2D, id);
	unsigned int glInternalFormat, glFormat, glType;
	rlGetGlTextureFormats(format, &glInternalFormat, &glFormat, &glType);
	if ((glInternalFormat != -1) && (format < COMPRESSED_DXT1_RGB))
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, glFormat, glType, (unsigned char *)data);
	}
	else debugNetPrintf(ERROR, "[ORBISGL] %s Texture format updating not supported\n",__FUNCTION__);
}

// Get OpenGL internal formats and data type from raylib PixelFormat
void rlGetGlTextureFormats(int format, unsigned int *glInternalFormat, unsigned int *glFormat, unsigned int *glType)
{
	*glInternalFormat = -1;
	*glFormat = -1;
	*glType = -1;
	switch (format)
	{
		// NOTE: on OpenGL ES 2.0 (WebGL), internalFormat must match format and options allowed are: GL_LUMINANCE, GL_RGB, GL_RGBA
		case UNCOMPRESSED_GRAYSCALE: *glInternalFormat = GL_LUMINANCE; *glFormat = GL_LUMINANCE; *glType = GL_UNSIGNED_BYTE; break;
		case UNCOMPRESSED_GRAY_ALPHA: *glInternalFormat = GL_LUMINANCE_ALPHA; *glFormat = GL_LUMINANCE_ALPHA; *glType = GL_UNSIGNED_BYTE; break;
		case UNCOMPRESSED_R5G6B5: *glInternalFormat = GL_RGB; *glFormat = GL_RGB; *glType = GL_UNSIGNED_SHORT_5_6_5; break;
		case UNCOMPRESSED_R8G8B8: *glInternalFormat = GL_RGB; *glFormat = GL_RGB; *glType = GL_UNSIGNED_BYTE; break;
		case UNCOMPRESSED_R5G5B5A1: *glInternalFormat = GL_RGBA; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case UNCOMPRESSED_R4G4B4A4: *glInternalFormat = GL_RGBA; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_SHORT_4_4_4_4; break;
		case UNCOMPRESSED_R8G8B8A8: *glInternalFormat = GL_RGBA; *glFormat = GL_RGBA; *glType = GL_UNSIGNED_BYTE; break;
		case UNCOMPRESSED_R32: if (RLGL.ExtSupported.texFloat32) *glInternalFormat = GL_LUMINANCE; *glFormat = GL_LUMINANCE; *glType = GL_FLOAT; break;   // NOTE: Requires extension OES_texture_float
		case UNCOMPRESSED_R32G32B32: if (RLGL.ExtSupported.texFloat32) *glInternalFormat = GL_RGB; *glFormat = GL_RGB; *glType = GL_FLOAT; break;         // NOTE: Requires extension OES_texture_float
		case UNCOMPRESSED_R32G32B32A32: if (RLGL.ExtSupported.texFloat32) *glInternalFormat = GL_RGBA; *glFormat = GL_RGBA; *glType = GL_FLOAT; break;    // NOTE: Requires extension OES_texture_float
		case COMPRESSED_DXT1_RGB: if (RLGL.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; break;
		case COMPRESSED_DXT1_RGBA: if (RLGL.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; break;
		case COMPRESSED_DXT3_RGBA: if (RLGL.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
		case COMPRESSED_DXT5_RGBA: if (RLGL.ExtSupported.texCompDXT) *glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
		case COMPRESSED_ETC1_RGB: if (RLGL.ExtSupported.texCompETC1) *glInternalFormat = GL_ETC1_RGB8_OES; break;                      // NOTE: Requires OpenGL ES 2.0 or OpenGL 4.3
		case COMPRESSED_ETC2_RGB: if (RLGL.ExtSupported.texCompETC2) *glInternalFormat = GL_COMPRESSED_RGB8_ETC2; break;               // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		case COMPRESSED_ETC2_EAC_RGBA: if (RLGL.ExtSupported.texCompETC2) *glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC; break;     // NOTE: Requires OpenGL ES 3.0 or OpenGL 4.3
		case COMPRESSED_PVRT_RGB: if (RLGL.ExtSupported.texCompPVRT) *glInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; break;    // NOTE: Requires PowerVR GPU
		case COMPRESSED_PVRT_RGBA: if (RLGL.ExtSupported.texCompPVRT) *glInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; break;  // NOTE: Requires PowerVR GPU
		case COMPRESSED_ASTC_4x4_RGBA: if (RLGL.ExtSupported.texCompASTC) *glInternalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR; break;  // NOTE: Requires OpenGL ES 3.1 or OpenGL 4.3
		case COMPRESSED_ASTC_8x8_RGBA: if (RLGL.ExtSupported.texCompASTC) *glInternalFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR; break;  // NOTE: Requires OpenGL ES 3.1 or OpenGL 4.3
		default: debugNetPrintf(ERROR, "[ORBISGL] %s Texture format not supported\n",__FUNCTION__); break;
	}
}

// Unload texture from GPU memory
void rlUnloadTexture(unsigned int id)
{
	if (id > 0) glDeleteTextures(1, &id);
}

// Load a texture to be used for rendering (fbo with default color and depth attachments)
// NOTE: If colorFormat or depthBits are no supported, no attachment is done
RenderTexture2D rlLoadRenderTexture(int width, int height, int format, int depthBits, bool useDepthTexture)
{
	RenderTexture2D target = { 0 };
	if (useDepthTexture && RLGL.ExtSupported.texDepth) target.depthTexture = true;
	// Create the framebuffer object
	glGenFramebuffers(1, &target.id);
	glBindFramebuffer(GL_FRAMEBUFFER, target.id);
	// Create fbo color texture attachment
	//-----------------------------------------------------------------------------------------------------
	if ((format != -1) && (format < COMPRESSED_DXT1_RGB))
	{
		// WARNING: Some texture formats are not supported for fbo color attachment
		target.texture.id = rlLoadTexture(NULL, width, height, format, 1);
		target.texture.width = width;
		target.texture.height = height;
		target.texture.format = format;
		target.texture.mipmaps = 1;
	}
	//-----------------------------------------------------------------------------------------------------
	// Create fbo depth renderbuffer/texture
	//-----------------------------------------------------------------------------------------------------
	if (depthBits > 0)
	{
		target.depth.id = rlLoadTextureDepth(width, height, depthBits, !useDepthTexture);
		target.depth.width = width;
		target.depth.height = height;
		target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
		target.depth.mipmaps = 1;
	}
	//-----------------------------------------------------------------------------------------------------
	// Attach color texture and depth renderbuffer to FBO
	//-----------------------------------------------------------------------------------------------------
	rlRenderTextureAttach(target, target.texture.id, 0);    // COLOR attachment
	rlRenderTextureAttach(target, target.depth.id, 1);      // DEPTH attachment
	//-----------------------------------------------------------------------------------------------------
	// Check if fbo is complete with attachments (valid)
	//-----------------------------------------------------------------------------------------------------
	if (rlRenderTextureComplete(target)) debugNetPrintf(INFO, "[ORBISGL] %s [FBO ID %i] Framebuffer object created successfully\n",__FUNCTION__,target.id);
	//-----------------------------------------------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return target;
}

// Attach color buffer texture to an fbo (unloads previous attachment)
// NOTE: Attach type: 0-Color, 1-Depth renderbuffer, 2-Depth texture
void rlRenderTextureAttach(RenderTexture2D target, unsigned int id, int attachType)
{
	glBindFramebuffer(GL_FRAMEBUFFER, target.id);
	if (attachType == 0) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);
	else if (attachType == 1)
	{
		if (target.depthTexture) glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, id, 0);
		else glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, id);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Verify render texture is complete
bool rlRenderTextureComplete(RenderTexture target)
{
	bool result = false;
	glBindFramebuffer(GL_FRAMEBUFFER, target.id);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (status)
		{
			case GL_FRAMEBUFFER_UNSUPPORTED: debugNetPrintf(ERROR, "[ORBISGL] %s Framebuffer is unsupported\n",__FUNCTION__); break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: debugNetPrintf(ERROR, "[ORBISGL] %s Framebuffer has incomplete attachment\n",__FUNCTION__); break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: debugNetPrintf(ERROR, "[ORBISGL] %s Framebuffer has incomplete dimensions\n",__FUNCTION__); break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: debugNetPrintf(ERROR, "[ORBISGL] %s Framebuffer has a missing attachment\n",__FUNCTION__); break;
			default: break;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	result = (status == GL_FRAMEBUFFER_COMPLETE);
	return result;
}

// Generate mipmap data for selected texture
void rlGenerateMipmaps(Texture2D *texture)
{
	glBindTexture(GL_TEXTURE_2D, texture->id);
	// Check if texture is power-of-two (POT)
	bool texIsPOT = false;
	if (((texture->width > 0) && ((texture->width & (texture->width - 1)) == 0)) &&
	    ((texture->height > 0) && ((texture->height & (texture->height - 1)) == 0))) texIsPOT = true;
	if ((texIsPOT) || (RLGL.ExtSupported.texNPOT))
	{
		//glHint(GL_GENERATE_MIPMAP_HINT, GL_DONT_CARE);   // Hint for mipmaps generation algorythm: GL_FASTEST, GL_NICEST, GL_DONT_CARE
		glGenerateMipmap(GL_TEXTURE_2D);    // Generate mipmaps automatically
		debugNetPrintf(INFO, "[ORBIS] %s [TEX ID %i] Mipmaps generated automatically\n",__FUNCTION__,texture->id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);   // Activate Trilinear filtering for mipmaps
		#define MIN(a,b) (((a)<(b))?(a):(b))
		#define MAX(a,b) (((a)>(b))?(a):(b))
		texture->mipmaps =  1 + (int)floor(log(MAX(texture->width, texture->height))/log(2));
	}
	else debugNetPrintf(ERROR, "[ORBISGL] %s [TEX ID %i] Mipmaps can not be generated\n",__FUNCTION__,texture->id);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Upload vertex data into a VAO (if supported) and VBO
void rlLoadMesh(Mesh *mesh, bool dynamic)
{
	if (mesh->vaoId > 0)
	{
		// Check if mesh has already been loaded in GPU
		debugNetPrintf(ERROR, "[ORBISGL] %s Trying to re-load an already loaded mesh\n",__FUNCTION__);
		return;
	}
	mesh->vaoId = 0;        // Vertex Array Object
	mesh->vboId[0] = 0;     // Vertex positions VBO
	mesh->vboId[1] = 0;     // Vertex texcoords VBO
	mesh->vboId[2] = 0;     // Vertex normals VBO
	mesh->vboId[3] = 0;     // Vertex colors VBO
	mesh->vboId[4] = 0;     // Vertex tangents VBO
	mesh->vboId[5] = 0;     // Vertex texcoords2 VBO
	mesh->vboId[6] = 0;     // Vertex indices VBO
	int drawHint = GL_STATIC_DRAW;
	if (dynamic) drawHint = GL_DYNAMIC_DRAW;
	if (RLGL.ExtSupported.vao)
	{
		// Initialize Quads VAO (Buffer A)
		glGenVertexArrays(1, &mesh->vaoId);
		glBindVertexArray(mesh->vaoId);
	}
	// NOTE: Attributes must be uploaded considering default locations points
	// Enable vertex attributes: position (shader-location = 0)
	glGenBuffers(1, &mesh->vboId[0]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->vertexCount, mesh->vertices, drawHint);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(0);
	// Enable vertex attributes: texcoords (shader-location = 1)
	glGenBuffers(1, &mesh->vboId[1]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*mesh->vertexCount, mesh->texcoords, drawHint);
	glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(1);
	// Enable vertex attributes: normals (shader-location = 2)
	if (mesh->normals != NULL)
	{
		glGenBuffers(1, &mesh->vboId[2]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh->vertexCount, mesh->normals, drawHint);
		glVertexAttribPointer(2, 3, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(2);
	}
	else
	{
		// Default color vertex attribute set to WHITE
		glVertexAttrib3f(2, 1.0f, 1.0f, 1.0f);
		glDisableVertexAttribArray(2);
	}
	// Default color vertex attribute (shader-location = 3)
	if (mesh->colors != NULL)
	{
		glGenBuffers(1, &mesh->vboId[3]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[3]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned char)*4*mesh->vertexCount, mesh->colors, drawHint);
		glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
		glEnableVertexAttribArray(3);
	}
	else
	{
		// Default color vertex attribute set to WHITE
		glVertexAttrib4f(3, 1.0f, 1.0f, 1.0f, 1.0f);
		glDisableVertexAttribArray(3);
	}
	// Default tangent vertex attribute (shader-location = 4)
	if (mesh->tangents != NULL)
	{
		glGenBuffers(1, &mesh->vboId[4]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[4]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*mesh->vertexCount, mesh->tangents, drawHint);
		glVertexAttribPointer(4, 4, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(4);
	}
	else
	{
		// Default tangents vertex attribute
		glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
		glDisableVertexAttribArray(4);
	}
	// Default texcoord2 vertex attribute (shader-location = 5)
	if (mesh->texcoords2 != NULL)
	{
		glGenBuffers(1, &mesh->vboId[5]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId[5]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*mesh->vertexCount, mesh->texcoords2, drawHint);
		glVertexAttribPointer(5, 2, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(5);
	}
	else
	{
		// Default texcoord2 vertex attribute
		glVertexAttrib2f(5, 0.0f, 0.0f);
		glDisableVertexAttribArray(5);
	}
	if (mesh->indices != NULL)
	{
		glGenBuffers(1, &mesh->vboId[6]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vboId[6]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*mesh->triangleCount*3, mesh->indices, drawHint);
	}
	if (RLGL.ExtSupported.vao)
	{
		if (mesh->vaoId > 0) debugNetPrintf(INFO, "[ORBISGL] %s [VAO ID %i] Mesh uploaded successfully to VRAM (GPU)\n",__FUNCTION__,mesh->vaoId);
		else debugNetPrintf(ERROR, "[ORBISGL] %s Mesh could not be uploaded to VRAM (GPU)\n",__FUNCTION__);
	}
	else
	{
		debugNetPrintf(INFO, "[ORBISGL] %s [VBOs] Mesh uploaded successfully to VRAM (GPU)\n",__FUNCTION__);
	}
}

// Load a new attributes buffer
unsigned int rlLoadAttribBuffer(unsigned int vaoId, int shaderLoc, void *buffer, int size, bool dynamic)
{
	unsigned int id = 0;
	int drawHint = GL_STATIC_DRAW;
	if (dynamic) drawHint = GL_DYNAMIC_DRAW;
	if (RLGL.ExtSupported.vao) glBindVertexArray(vaoId);
	glGenBuffers(1, &id);
	glBindBuffer(GL_ARRAY_BUFFER, id);
	glBufferData(GL_ARRAY_BUFFER, size, buffer, drawHint);
	glVertexAttribPointer(shaderLoc, 2, GL_FLOAT, 0, 0, 0);
	glEnableVertexAttribArray(shaderLoc);
	if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	return id;
}

// Update vertex or index data on GPU (upload new data to one buffer)
void rlUpdateMesh(Mesh mesh, int buffer, int num)
{
	rlUpdateMeshAt(mesh, buffer, num, 0);
}

// Update vertex or index data on GPU, at index
// WARNING: error checking is in place that will cause the data to not be
//          updated if offset + size exceeds what the buffer can hold
void rlUpdateMeshAt(Mesh mesh, int buffer, int num, int index)
{
	// Activate mesh VAO
	if (RLGL.ExtSupported.vao) glBindVertexArray(mesh.vaoId);
	switch (buffer)
	{
		case 0:     // Update vertices (vertex position)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[0]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*num, mesh.vertices, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*3*index, sizeof(float)*3*num, mesh.vertices);
		} break;
		case 1:     // Update texcoords (vertex texture coordinates)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[1]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*num, mesh.texcoords, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*2*index, sizeof(float)*2*num, mesh.texcoords);
		} break;
		case 2:     // Update normals (vertex normals)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[2]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*num, mesh.normals, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*3*index, sizeof(float)*3*num, mesh.normals);
		} break;
		case 3:     // Update colors (vertex colors)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[3]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*num, mesh.colors, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(unsigned char)*4*index, sizeof(unsigned char)*4*num, mesh.colors);
		} break;
		case 4:     // Update tangents (vertex tangents)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[4]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*num, mesh.tangents, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*4*index, sizeof(float)*4*num, mesh.tangents);
		} break;
		case 5:     // Update texcoords2 (vertex second texture coordinates)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[5]);
			if (index == 0 && num >= mesh.vertexCount) glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*num, mesh.texcoords2, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.vertexCount) break;
			else glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*2*index, sizeof(float)*2*num, mesh.texcoords2);
		} break;
		case 6:     // Update indices (triangle index buffer)
		{
			// the * 3 is because each triangle has 3 indices
			unsigned short *indices = mesh.indices;
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vboId[6]);
			if (index == 0 && num >= mesh.triangleCount)
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices)*num*3, indices, GL_DYNAMIC_DRAW);
			else if (index + num >= mesh.triangleCount)
				break;
			else
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*indices)*index*3, sizeof(*indices)*num*3, indices);
		} break;
		default: break;
	}
	// Unbind the current VAO
	if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	// Another option would be using buffer mapping...
	//mesh.vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	// Now we can modify vertices
	//glUnmapBuffer(GL_ARRAY_BUFFER);
}

// Draw a 3d mesh with material and transform
void rlDrawMesh(Mesh mesh, Material material, Matrix transform)
{
	// Bind shader program
	glUseProgram(material.shader.id);
	// Matrices and other values required by shader
	//-----------------------------------------------------
	// Calculate and send to shader model matrix (used by PBR shader)
	if (material.shader.locs[LOC_MATRIX_MODEL] != -1) SetShaderValueMatrix(material.shader, material.shader.locs[LOC_MATRIX_MODEL], transform);
	// Upload to shader material.colDiffuse
	if (material.shader.locs[LOC_COLOR_DIFFUSE] != -1)
		glUniform4f(material.shader.locs[LOC_COLOR_DIFFUSE],(float)material.maps[MAP_DIFFUSE].color.r/255.0f,
															(float)material.maps[MAP_DIFFUSE].color.g/255.0f,
															(float)material.maps[MAP_DIFFUSE].color.b/255.0f,
															(float)material.maps[MAP_DIFFUSE].color.a/255.0f);
	// Upload to shader material.colSpecular (if available)
	if (material.shader.locs[LOC_COLOR_SPECULAR] != -1)
		glUniform4f(material.shader.locs[LOC_COLOR_SPECULAR],(float)material.maps[MAP_SPECULAR].color.r/255.0f,
															(float)material.maps[MAP_SPECULAR].color.g/255.0f,
															(float)material.maps[MAP_SPECULAR].color.b/255.0f,
															(float)material.maps[MAP_SPECULAR].color.a/255.0f);
	if (material.shader.locs[LOC_MATRIX_VIEW] != -1) SetShaderValueMatrix(material.shader, material.shader.locs[LOC_MATRIX_VIEW], RLGL.State.modelview);
	if (material.shader.locs[LOC_MATRIX_PROJECTION] != -1) SetShaderValueMatrix(material.shader, material.shader.locs[LOC_MATRIX_PROJECTION], RLGL.State.projection);
	// At this point the modelview matrix just contains the view matrix (camera)
	// That's because BeginMode3D() sets it an no model-drawing function modifies it, all use rlPushMatrix() and rlPopMatrix()
	Matrix matView = RLGL.State.modelview;         // View matrix (camera)
	Matrix matProjection = RLGL.State.projection;  // Projection matrix (perspective)
	// TODO: Consider possible transform matrices in the RLGL.State.stack
	// Is this the right order? or should we start with the first stored matrix instead of the last one?
	//Matrix matStackTransform = MatrixIdentity();
	//for (int i = RLGL.State.stackCounter; i > 0; i--) matStackTransform = MatrixMultiply(RLGL.State.stack[i], matStackTransform);
	// Transform to camera-space coordinates
	Matrix matModelView = MatrixMultiply(transform, MatrixMultiply(RLGL.State.transform, matView));
	//-----------------------------------------------------
	// Bind active texture maps (if available)
	for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
	{
		if (material.maps[i].texture.id > 0)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			if ((i == MAP_IRRADIANCE) || (i == MAP_PREFILTER) || (i == MAP_CUBEMAP)) glBindTexture(GL_TEXTURE_CUBE_MAP, material.maps[i].texture.id);
			else glBindTexture(GL_TEXTURE_2D, material.maps[i].texture.id);
			glUniform1i(material.shader.locs[LOC_MAP_DIFFUSE + i], i);
		}
	}
	// Bind vertex array objects (or VBOs)
	if (RLGL.ExtSupported.vao) glBindVertexArray(mesh.vaoId);
	else
	{
		// Bind mesh VBO data: vertex position (shader-location = 0)
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[0]);
		glVertexAttribPointer(material.shader.locs[LOC_VERTEX_POSITION], 3, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_POSITION]);
		// Bind mesh VBO data: vertex texcoords (shader-location = 1)
		glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[1]);
		glVertexAttribPointer(material.shader.locs[LOC_VERTEX_TEXCOORD01], 2, GL_FLOAT, 0, 0, 0);
		glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_TEXCOORD01]);
		// Bind mesh VBO data: vertex normals (shader-location = 2, if available)
		if (material.shader.locs[LOC_VERTEX_NORMAL] != -1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[2]);
			glVertexAttribPointer(material.shader.locs[LOC_VERTEX_NORMAL], 3, GL_FLOAT, 0, 0, 0);
			glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_NORMAL]);
		}
		// Bind mesh VBO data: vertex colors (shader-location = 3, if available)
		if (material.shader.locs[LOC_VERTEX_COLOR] != -1)
		{
			if (mesh.vboId[3] != 0)
			{
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[3]);
				glVertexAttribPointer(material.shader.locs[LOC_VERTEX_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
				glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_COLOR]);
			}
			else
			{
				// Set default value for unused attribute
				// NOTE: Required when using default shader and no VAO support
				glVertexAttrib4f(material.shader.locs[LOC_VERTEX_COLOR], 1.0f, 1.0f, 1.0f, 1.0f);
				glDisableVertexAttribArray(material.shader.locs[LOC_VERTEX_COLOR]);
			}
		}
		// Bind mesh VBO data: vertex tangents (shader-location = 4, if available)
		if (material.shader.locs[LOC_VERTEX_TANGENT] != -1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[4]);
			glVertexAttribPointer(material.shader.locs[LOC_VERTEX_TANGENT], 4, GL_FLOAT, 0, 0, 0);
			glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_TANGENT]);
		}
		// Bind mesh VBO data: vertex texcoords2 (shader-location = 5, if available)
		if (material.shader.locs[LOC_VERTEX_TEXCOORD02] != -1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vboId[5]);
			glVertexAttribPointer(material.shader.locs[LOC_VERTEX_TEXCOORD02], 2, GL_FLOAT, 0, 0, 0);
			glEnableVertexAttribArray(material.shader.locs[LOC_VERTEX_TEXCOORD02]);
		}
		if (mesh.indices != NULL) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vboId[6]);
	}
	int eyesCount = 1;
	for (int eye = 0; eye < eyesCount; eye++)
	{
		if (eyesCount == 1) RLGL.State.modelview = matModelView;
		
		// Calculate model-view-projection matrix (MVP)
		Matrix matMVP = MatrixMultiply(RLGL.State.modelview, RLGL.State.projection);        // Transform to screen-space coordinates
		// Send combined model-view-projection matrix to shader
		glUniformMatrix4fv(material.shader.locs[LOC_MATRIX_MVP], 1, false, MatrixToFloat(matMVP));
		// Draw call!
		if (mesh.indices != NULL) glDrawElements(GL_TRIANGLES, mesh.triangleCount*3, GL_UNSIGNED_SHORT, 0); // Indexed vertices draw
		else glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
	}
	// Unbind all binded texture maps
	for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);       // Set shader active texture
		if ((i == MAP_IRRADIANCE) || (i == MAP_PREFILTER) || (i == MAP_CUBEMAP)) glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		else glBindTexture(GL_TEXTURE_2D, 0);   // Unbind current active texture
	}
	// Unind vertex array objects (or VBOs)
	if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		if (mesh.indices != NULL) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	// Unbind shader program
	glUseProgram(0);
	// Restore RLGL.State.projection/RLGL.State.modelview matrices
	// NOTE: In stereo rendering matrices are being modified to fit every eye
	RLGL.State.projection = matProjection;
	RLGL.State.modelview = matView;
}

// Unload mesh data from CPU and GPU
void rlUnloadMesh(Mesh mesh)
{
	free(mesh.vertices);
	free(mesh.texcoords);
	free(mesh.normals);
	free(mesh.colors);
	free(mesh.tangents);
	free(mesh.texcoords2);
	free(mesh.indices);
	free(mesh.animVertices);
	free(mesh.animNormals);
	free(mesh.boneWeights);
	free(mesh.boneIds);
	rlDeleteBuffers(mesh.vboId[0]);   // vertex
	rlDeleteBuffers(mesh.vboId[1]);   // texcoords
	rlDeleteBuffers(mesh.vboId[2]);   // normals
	rlDeleteBuffers(mesh.vboId[3]);   // colors
	rlDeleteBuffers(mesh.vboId[4]);   // tangents
	rlDeleteBuffers(mesh.vboId[5]);   // texcoords2
	rlDeleteBuffers(mesh.vboId[6]);   // indices
	rlDeleteVertexArrays(mesh.vaoId);
}

// Read screen pixel data (color buffer)
unsigned char *rlReadScreenPixels(int width, int height)
{
	unsigned char *screenData = (unsigned char *)calloc(width*height*4, sizeof(unsigned char));
	// NOTE 1: glReadPixels returns image flipped vertically -> (0,0) is the bottom left corner of the framebuffer
	// NOTE 2: We are getting alpha channel! Be careful, it can be transparent if not cleared properly!
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, screenData);
	// Flip image vertically!
	unsigned char *imgData = (unsigned char *)malloc(width*height*sizeof(unsigned char)*4);
	for (int y = height - 1; y >= 0; y--)
	{
		for (int x = 0; x < (width*4); x++)
		{
			imgData[((height - 1) - y)*width*4 + x] = screenData[(y*width*4) + x];  // Flip line
			// Set alpha component value to 255 (no trasparent image retrieval)
			// NOTE: Alpha value has already been applied to RGB in framebuffer, we don't need it!
			if (((x + 1)%4) == 0) imgData[((height - 1) - y)*width*4 + x] = 255;
		}
	}
	free(screenData);
	return imgData;     // NOTE: image data should be freed
}

// Read texture pixel data
void *rlReadTexturePixels(Texture2D texture)
{
	void *pixels = NULL;
	// glGetTexImage() is not available on OpenGL ES 2.0
	// Texture2D width and height are required on OpenGL ES 2.0. There is no way to get it from texture id.
	// Two possible Options:
	// 1 - Bind texture to color fbo attachment and glReadPixels()
	// 2 - Create an fbo, activate it, render quad with texture, glReadPixels()
	// We are using Option 1, just need to care for texture format on retrieval
	// NOTE: This behaviour could be conditioned by graphic driver...
	RenderTexture2D fbo = rlLoadRenderTexture(texture.width, texture.height, UNCOMPRESSED_R8G8B8A8, 16, false);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);
	glBindTexture(GL_TEXTURE_2D, 0);
	// Attach our texture to FBO
	// NOTE: Previoust attached texture is automatically detached
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.id, 0);
	// We read data as RGBA because FBO texture is configured as RGBA, despite binding another texture format
	pixels = (unsigned char *)malloc(GetPixelDataSize(texture.width, texture.height, UNCOMPRESSED_R8G8B8A8));
	glReadPixels(0, 0, texture.width, texture.height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	// Re-attach internal FBO color texture before deleting it
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo.texture.id, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Clean up temporal fbo
	rlDeleteRenderTextures(fbo);
	return pixels;
}

//----------------------------------------------------------------------------------
// Module Functions Definition - Shaders Functions
// NOTE: Those functions are exposed directly to the user in raylib.h
//----------------------------------------------------------------------------------

// Get default internal texture (white texture)
Texture2D GetTextureDefault(void)
{
	Texture2D texture = { 0 };
	texture.id = RLGL.State.defaultTextureId;
	texture.width = 1;
	texture.height = 1;
	texture.mipmaps = 1;
	texture.format = UNCOMPRESSED_R8G8B8A8;
	return texture;
}

// Get texture to draw shapes (RAII)
Texture2D GetShapesTexture(void)
{
	return RLGL.State.shapesTexture;
}

// Get texture rectangle to draw shapes
Rectangle GetShapesTextureRec(void)
{
	return RLGL.State.shapesTextureRec;
}

// Define default texture used to draw shapes
void SetShapesTexture(Texture2D texture, Rectangle source)
{
	RLGL.State.shapesTexture = texture;
	RLGL.State.shapesTextureRec = source;
}

// Get default shader
Shader GetShaderDefault(void)
{
    return RLGL.State.defaultShader;
}

// Load shader from files and bind default locations
// NOTE: If shader string is NULL, using default vertex/fragment shaders
Shader LoadShader(const char *vsFileName, const char *fsFileName)
{
	Shader shader = { 0 };
	// NOTE: Shader.locs is allocated by LoadShaderCode()
	char *vShaderStr = NULL;
	char *fShaderStr = NULL;
	if (vsFileName != NULL) vShaderStr = LoadFileText(vsFileName);
	if (fsFileName != NULL) fShaderStr = LoadFileText(fsFileName);
	debugNetPrintf(INFO,"[ORBISGL] %s 1\n",__FUNCTION__);

	shader = LoadShaderCode(vShaderStr, fShaderStr);
	debugNetPrintf(INFO,"[ORBISGL] %s 2\n",__FUNCTION__);

	if (vShaderStr != NULL) free(vShaderStr);
	if (fShaderStr != NULL) free(fShaderStr);
	return shader;
}

// Load shader from code strings
// NOTE: If shader string is NULL, using default vertex/fragment shaders
Shader LoadShaderCode(const char *vsCode, const char *fsCode)
{
	Shader shader = { 0 };
	shader.locs = (int *)calloc(MAX_SHADER_LOCATIONS, sizeof(int));
	// NOTE: All locations must be reseted to -1 (no location)
	for (int i = 0; i < MAX_SHADER_LOCATIONS; i++) shader.locs[i] = -1;
	unsigned int vertexShaderId = RLGL.State.defaultVShaderId;
	unsigned int fragmentShaderId = RLGL.State.defaultFShaderId;
	debugNetPrintf(INFO,"[ORBISGL] %s vid=%d fid=%d\n",__FUNCTION__,vertexShaderId,fragmentShaderId);

	if (vsCode != NULL) vertexShaderId = CompileShader(vsCode, GL_VERTEX_SHADER);
	debugNetPrintf(INFO,"[ORBISGL] %s vid=%d fid=%d\n",__FUNCTION__,vertexShaderId,fragmentShaderId);
	if (fsCode != NULL) fragmentShaderId = CompileShader(fsCode, GL_FRAGMENT_SHADER);
	debugNetPrintf(INFO,"[ORBISGL] %s vid=%d fid=%d\n",__FUNCTION__,vertexShaderId,fragmentShaderId);

	if ((vertexShaderId == RLGL.State.defaultVShaderId) && (fragmentShaderId == RLGL.State.defaultFShaderId)) shader = RLGL.State.defaultShader;
	else
	{
		shader.id = LoadShaderProgram(vertexShaderId, fragmentShaderId);
		if (vertexShaderId != RLGL.State.defaultVShaderId) glDeleteShader(vertexShaderId);
		if (fragmentShaderId != RLGL.State.defaultFShaderId) glDeleteShader(fragmentShaderId);
		if (shader.id == 0)
		{
			debugNetPrintf(ERROR, "[ORBISGL] %s Custom shader could not be loaded\n",__FUNCTION__);
			shader = RLGL.State.defaultShader;
		}
		// After shader loading, we TRY to set default location names
		if (shader.id > 0) SetShaderDefaultLocations(&shader);
	}
	// Get available shader uniforms
	// NOTE: This information is useful for debug...
	int uniformCount = -1;
	glGetProgramiv(shader.id, GL_ACTIVE_UNIFORMS, &uniformCount);
	for (int i = 0; i < uniformCount; i++)
	{
		int namelen = -1;
		int num = -1;
		char name[256]; // Assume no variable names longer than 256
		GLenum type = GL_ZERO;
		// Get the name of the uniforms
		glGetActiveUniform(shader.id, i,sizeof(name) - 1, &namelen, &num, &type, name);
		name[namelen] = 0;
		debugNetPrintf(DEBUG,"[ORBISGL] %s [SHDR ID %i] Active uniform [%s] set at location: %i\n",__FUNCTION__, shader.id, name, glGetUniformLocation(shader.id, name));
	}
	return shader;
}

// Unload shader from GPU memory (VRAM)
void UnloadShader(Shader shader)
{
	if (shader.id > 0)
	{
		rlDeleteShader(shader.id);
		debugNetPrintf(INFO, "[ORBISGL] %s [SHDR ID %i] Unloaded shader program data\n",__FUNCTION__,shader.id);
	}
	free(shader.locs);
}

// Begin custom shader mode
void BeginShaderMode(Shader shader)
{
	if (RLGL.State.currentShader.id != shader.id)
	{
		rlglDraw();
		RLGL.State.currentShader = shader;
	}
}

// End custom shader mode (returns to default shader)
void EndShaderMode(void)
{
	BeginShaderMode(RLGL.State.defaultShader);
}

// Get shader uniform location
int GetShaderLocation(Shader shader, const char *uniformName)
{
	int location = -1;
	location = glGetUniformLocation(shader.id, uniformName);
	if (location == -1) debugNetPrintf(ERROR, "[ORBISGL] %s [SHDR ID %i][%s] Shader uniform could not be found\n",__FUNCTION__, shader.id, uniformName);
	else debugNetPrintf(INFO, "[ORBISGL] %s [SHDR ID %i][%s] Shader uniform set at location: %i\n",__FUNCTION__,shader.id, uniformName, location);
	return location;
}

// Set shader uniform value
void SetShaderValue(Shader shader, int uniformLoc, const void *value, int uniformType)
{
	SetShaderValueV(shader, uniformLoc, value, uniformType, 1);
}

// Set shader uniform value vector
void SetShaderValueV(Shader shader, int uniformLoc, const void *value, int uniformType, int count)
{
	glUseProgram(shader.id);
	switch (uniformType)
	{
		case UNIFORM_FLOAT: glUniform1fv(uniformLoc, count, (float *)value); break;
		case UNIFORM_VEC2: glUniform2fv(uniformLoc, count, (float *)value); break;
		case UNIFORM_VEC3: glUniform3fv(uniformLoc, count, (float *)value); break;
		case UNIFORM_VEC4: glUniform4fv(uniformLoc, count, (float *)value); break;
		case UNIFORM_INT: glUniform1iv(uniformLoc, count, (int *)value); break;
		case UNIFORM_IVEC2: glUniform2iv(uniformLoc, count, (int *)value); break;
		case UNIFORM_IVEC3: glUniform3iv(uniformLoc, count, (int *)value); break;
		case UNIFORM_IVEC4: glUniform4iv(uniformLoc, count, (int *)value); break;
		case UNIFORM_SAMPLER2D: glUniform1iv(uniformLoc, count, (int *)value); break;
		default: debugNetPrintf(ERROR, "[ORBISGL] %s Shader uniform could not be set data type not recognized\n",__FUNCTION__);
	}
	//glUseProgram(0);      // Avoid reseting current shader program, in case other uniforms are set
}


// Set shader uniform value (matrix 4x4)
void SetShaderValueMatrix(Shader shader, int uniformLoc, Matrix mat)
{
	glUseProgram(shader.id);
	glUniformMatrix4fv(uniformLoc, 1, false, MatrixToFloat(mat));
	//glUseProgram(0);
}

// Set shader uniform value for texture
void SetShaderValueTexture(Shader shader, int uniformLoc, Texture2D texture)
{
	glUseProgram(shader.id);
	glUniform1i(uniformLoc, texture.id);
	//glUseProgram(0);
}

// Set a custom projection matrix (replaces internal projection matrix)
void SetMatrixProjection(Matrix projection)
{
	RLGL.State.projection = projection;
}

// Return internal projection matrix
Matrix GetMatrixProjection(void) {
	return RLGL.State.projection;
}

// Set a custom modelview matrix (replaces internal modelview matrix)
void SetMatrixModelview(Matrix view)
{
    RLGL.State.modelview = view;
}

// Return internal modelview matrix
Matrix GetMatrixModelview(void)
{
    Matrix matrix = MatrixIdentity();
    matrix = RLGL.State.modelview;
    return matrix;
}

// Generate cubemap texture from HDR texture
// TODO: OpenGL ES 2.0 does not support GL_RGB16F texture format, neither GL_DEPTH_COMPONENT24
Texture2D GenTextureCubemap(Shader shader, Texture2D map, int size)
{
	Texture2D cubemap = { 0 };
	// NOTE: SetShaderDefaultLocations() already setups locations for projection and view Matrix in shader
	// Other locations should be setup externally in shader before calling the function
	// Set up depth face culling and cubemap seamless
	glDisable(GL_CULL_FACE);
	// Setup framebuffer
	unsigned int fbo, rbo;
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size, size);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
	// Set up cubemap to render and attach to framebuffer
	// NOTE: Faces are stored as 32 bit floating point values
	glGenTextures(1, &cubemap.id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
	for (unsigned int i = 0; i < 6; i++)
	{
		if (RLGL.ExtSupported.texFloat32) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, size, size, 0, GL_RGB, GL_FLOAT, NULL);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Create projection and different views for each face
	Matrix fboProjection = MatrixPerspective(90.0*DEG2RAD, 1.0, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
	Matrix fboViews[6] = {
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }),
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }),
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
		MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f })
	};
	// Convert HDR equirectangular environment map to cubemap equivalent
	glUseProgram(shader.id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, map.id);
	SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_PROJECTION], fboProjection);
	// Note: don't forget to configure the viewport to the capture dimensions
	glViewport(0, 0, size, size);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for (int i = 0; i < 6; i++)
	{
		SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_VIEW], fboViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap.id, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GenDrawCube();
	}
	// Unbind framebuffer and textures
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Reset viewport dimensions to default
	glViewport(0, 0, RLGL.State.framebufferWidth, RLGL.State.framebufferHeight);
	//glEnable(GL_CULL_FACE);
	// NOTE: Texture2D is a GL_TEXTURE_CUBE_MAP, not a GL_TEXTURE_2D!
	cubemap.width = size;
	cubemap.height = size;
	cubemap.mipmaps = 1;
	cubemap.format = UNCOMPRESSED_R32G32B32;
	return cubemap;
}

// Generate irradiance texture using cubemap data
// TODO: OpenGL ES 2.0 does not support GL_RGB16F texture format, neither GL_DEPTH_COMPONENT24
Texture2D GenTextureIrradiance(Shader shader, Texture2D cubemap, int size)
{
    Texture2D irradiance = { 0 };

/*#if defined(GRAPHICS_API_OPENGL_33) // || defined(GRAPHICS_API_OPENGL_ES2)
    // NOTE: SetShaderDefaultLocations() already setups locations for projection and view Matrix in shader
    // Other locations should be setup externally in shader before calling the function

    // Setup framebuffer
    unsigned int fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Create an irradiance cubemap, and re-scale capture FBO to irradiance scale
    glGenTextures(1, &irradiance.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.id);
    for (unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create projection (transposed) and different views for each face
    Matrix fboProjection = MatrixPerspective(90.0*DEG2RAD, 1.0, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
    Matrix fboViews[6] = {
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f })
    };

    // Solve diffuse integral by convolution to create an irradiance cubemap
    glUseProgram(shader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
    SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_PROJECTION], fboProjection);

    // Note: don't forget to configure the viewport to the capture dimensions
    glViewport(0, 0, size, size);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    for (int i = 0; i < 6; i++)
    {
        SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_VIEW], fboViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance.id, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GenDrawCube();
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport dimensions to default
    glViewport(0, 0, RLGL.State.framebufferWidth, RLGL.State.framebufferHeight);

    irradiance.width = size;
    irradiance.height = size;
    irradiance.mipmaps = 1;
    //irradiance.format = UNCOMPRESSED_R16G16B16;
#endif*/
    return irradiance;
}

// Generate prefilter texture using cubemap data
// TODO: OpenGL ES 2.0 does not support GL_RGB16F texture format, neither GL_DEPTH_COMPONENT24
Texture2D GenTexturePrefilter(Shader shader, Texture2D cubemap, int size)
{
    Texture2D prefilter = { 0 };

/*#if defined(GRAPHICS_API_OPENGL_33) // || defined(GRAPHICS_API_OPENGL_ES2)
    // NOTE: SetShaderDefaultLocations() already setups locations for projection and view Matrix in shader
    // Other locations should be setup externally in shader before calling the function
    // TODO: Locations should be taken out of this function... too shader dependant...
    int roughnessLoc = GetShaderLocation(shader, "roughness");

    // Setup framebuffer
    unsigned int fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Create a prefiltered HDR environment map
    glGenTextures(1, &prefilter.id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter.id);
    for (unsigned int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Generate mipmaps for the prefiltered HDR texture
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Create projection (transposed) and different views for each face
    Matrix fboProjection = MatrixPerspective(90.0*DEG2RAD, 1.0, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
    Matrix fboViews[6] = {
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, 1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.0f, 0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f, 0.0f })
    };

    // Prefilter HDR and store data into mipmap levels
    glUseProgram(shader.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.id);
    SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_PROJECTION], fboProjection);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    #define MAX_MIPMAP_LEVELS   5   // Max number of prefilter texture mipmaps

    for (int mip = 0; mip < MAX_MIPMAP_LEVELS; mip++)
    {
        // Resize framebuffer according to mip-level size.
        unsigned int mipWidth  = size*(int)powf(0.5f, (float)mip);
        unsigned int mipHeight = size*(int)powf(0.5f, (float)mip);

        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip/(float)(MAX_MIPMAP_LEVELS - 1);
        glUniform1f(roughnessLoc, roughness);

        for (int i = 0; i < 6; i++)
        {
            SetShaderValueMatrix(shader, shader.locs[LOC_MATRIX_VIEW], fboViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter.id, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            GenDrawCube();
        }
    }

    // Unbind framebuffer and textures
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport dimensions to default
    glViewport(0, 0, RLGL.State.framebufferWidth, RLGL.State.framebufferHeight);

    prefilter.width = size;
    prefilter.height = size;
    //prefilter.mipmaps = 1 + (int)floor(log(size)/log(2));
    //prefilter.format = UNCOMPRESSED_R16G16B16;
#endif*/
    return prefilter;
}

// Generate BRDF texture using cubemap data
// NOTE: OpenGL ES 2.0 does not support GL_RGB16F texture format, neither GL_DEPTH_COMPONENT24
// TODO: Review implementation: https://github.com/HectorMF/BRDFGenerator
Texture2D GenTextureBRDF(Shader shader, int size)
{
	Texture2D brdf = { 0 };
	// Generate BRDF convolution texture
	glGenTextures(1, &brdf.id);
	glBindTexture(GL_TEXTURE_2D, brdf.id);
	if (RLGL.ExtSupported.texFloat32) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Render BRDF LUT into a quad using FBO
	unsigned int fbo, rbo;
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size, size);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf.id, 0);
	glViewport(0, 0, size, size);
	glUseProgram(shader.id);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GenDrawQuad();
	// Unbind framebuffer and textures
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Unload framebuffer but keep color texture
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &fbo);
	// Reset viewport dimensions to default
	glViewport(0, 0, RLGL.State.framebufferWidth, RLGL.State.framebufferHeight);
	brdf.width = size;
	brdf.height = size;
	brdf.mipmaps = 1;
	brdf.format = UNCOMPRESSED_R32G32B32;
	return brdf;
}

// Begin blending mode (alpha, additive, multiplied)
// NOTE: Only 3 blending modes supported, default blend mode is alpha
void BeginBlendMode(int mode)
{
	static int blendMode = 0;   // Track current blending mode
	if ((blendMode != mode) && (mode < 3))
	{
		rlglDraw();
		switch (mode)
		{
			case BLEND_ALPHA: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
			case BLEND_ADDITIVE: glBlendFunc(GL_SRC_ALPHA, GL_ONE); break; // Alternative: glBlendFunc(GL_ONE, GL_ONE);
			case BLEND_MULTIPLIED: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); break;
			default: break;
		}
		blendMode = mode;
	}
}

// End blending mode (reset to default: alpha blending)
void EndBlendMode(void)
{
	BeginBlendMode(BLEND_ALPHA);
}


//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

// Compile custom shader and return shader id
unsigned int CompileShader(const char *shaderStr, int type)
{
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderStr, NULL);
	GLint success = 0;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE)
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s [SHDR ID %i] Failed to compile shader...\n",__FUNCTION__,shader);
		int maxLength = 0;
		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		char log[maxLength];
		glGetShaderInfoLog(shader, maxLength, &length, log);
		debugNetPrintf(INFO, "[ORBISGL] %s %s\n",__FUNCTION__,log);
	}
	else debugNetPrintf(INFO, "[ORBISGL] %s [SHDR ID %i] Shader compiled successfully\n",__FUNCTION__,shader);
	return shader;
}

// Load custom shader strings and return program id
unsigned int LoadShaderProgram(unsigned int vShaderId, unsigned int fShaderId)
{
	unsigned int program = 0;
	GLint success = 0;
	program = glCreateProgram();
	glAttachShader(program, vShaderId);
	glAttachShader(program, fShaderId);
	// NOTE: Default attribute shader locations must be binded before linking
	glBindAttribLocation(program, 0, DEFAULT_ATTRIB_POSITION_NAME);
	glBindAttribLocation(program, 1, DEFAULT_ATTRIB_TEXCOORD_NAME);
	glBindAttribLocation(program, 2, DEFAULT_ATTRIB_NORMAL_NAME);
	glBindAttribLocation(program, 3, DEFAULT_ATTRIB_COLOR_NAME);
	glBindAttribLocation(program, 4, DEFAULT_ATTRIB_TANGENT_NAME);
	glBindAttribLocation(program, 5, DEFAULT_ATTRIB_TEXCOORD2_NAME);
	// NOTE: If some attrib name is no found on the shader, it locations becomes -1
	glLinkProgram(program);
	// NOTE: All uniform variables are intitialised to 0 when a program links
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (success == GL_FALSE)
	{
		debugNetPrintf(ERROR, "[ORBISGL] %s [SHDR ID %i] Failed to link shader program...\n",__FUNCTION__,program);
		int maxLength = 0;
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
		char log[maxLength];
		glGetProgramInfoLog(program, maxLength, &length, log);
		debugNetPrintf(INFO, "[ORBISGL] %s %s\n",__FUNCTION__, log);
		glDeleteProgram(program);
		program = 0;
	}
	else debugNetPrintf(INFO, "[ORBISGL] %s [SHDR ID %i] Shader program loaded successfully\n",__FUNCTION__,program);
	return program;
}


// Load default shader (just vertex positioning and texture coloring)
// NOTE: This shader program is used for internal buffers
Shader LoadShaderDefault(void)
{
	Shader shader = { 0 };
	shader.locs = (int *)calloc(MAX_SHADER_LOCATIONS, sizeof(int));
	// NOTE: All locations must be reseted to -1 (no location)
	for (int i = 0; i < MAX_SHADER_LOCATIONS; i++) shader.locs[i] = -1;
	// Vertex shader directly defined, no external file required
	const char *defaultVShaderStr =
	"#version 100                       \n"
	"attribute vec3 vertexPosition;     \n"
	"attribute vec2 vertexTexCoord;     \n"
	"attribute vec4 vertexColor;        \n"
	"varying vec2 fragTexCoord;         \n"
	"varying vec4 fragColor;            \n"
	"uniform mat4 mvp;                  \n"
	"void main()                        \n"
	"{                                  \n"
	"    fragTexCoord = vertexTexCoord; \n"
	"    fragColor = vertexColor;       \n"
	"    gl_Position = mvp*vec4(vertexPosition, 1.0); \n"
	"}                                  \n";
	// Fragment shader directly defined, no external file required
	const char *defaultFShaderStr =
	"#version 100                       \n"
	"precision mediump float;           \n"     // precision required for OpenGL ES2 (WebGL)
	"varying vec2 fragTexCoord;         \n"
	"varying vec4 fragColor;            \n"
	"uniform sampler2D texture0;        \n"
	"uniform vec4 colDiffuse;           \n"
	"void main()                        \n"
	"{                                  \n"
	"    vec4 texelColor = texture2D(texture0, fragTexCoord); \n" // NOTE: texture2D() is deprecated on OpenGL 3.3 and ES 3.0
	"    gl_FragColor = texelColor*colDiffuse*fragColor;      \n"
	"}                                  \n";
	// NOTE: Compiled vertex/fragment shaders are kept for re-use
	RLGL.State.defaultVShaderId = CompileShader(defaultVShaderStr, GL_VERTEX_SHADER);     // Compile default vertex shader
	RLGL.State.defaultFShaderId = CompileShader(defaultFShaderStr, GL_FRAGMENT_SHADER);   // Compile default fragment shader
	shader.id = LoadShaderProgram(RLGL.State.defaultVShaderId, RLGL.State.defaultFShaderId);
	if (shader.id > 0)
	{
		debugNetPrintf(INFO, "[ORBISGL] %s [SHDR ID %i] Default shader loaded successfully\n",__FUNCTION__,shader.id);
		// Set default shader locations: attributes locations
		shader.locs[LOC_VERTEX_POSITION] = glGetAttribLocation(shader.id, "vertexPosition");
		shader.locs[LOC_VERTEX_TEXCOORD01] = glGetAttribLocation(shader.id, "vertexTexCoord");
		shader.locs[LOC_VERTEX_COLOR] = glGetAttribLocation(shader.id, "vertexColor");
		// Set default shader locations: uniform locations
		shader.locs[LOC_MATRIX_MVP]  = glGetUniformLocation(shader.id, "mvp");
		shader.locs[LOC_COLOR_DIFFUSE] = glGetUniformLocation(shader.id, "colDiffuse");
		shader.locs[LOC_MAP_DIFFUSE] = glGetUniformLocation(shader.id, "texture0");
		// NOTE: We could also use below function but in case DEFAULT_ATTRIB_* points are
		// changed for external custom shaders, we just use direct bindings above
		//SetShaderDefaultLocations(&shader);
	}
	else debugNetPrintf(ERROR, "[ORBISGL] %s [SHDR ID %i] Default shader could not be loaded\n",__FUNCTION__,shader.id);
	//checkshader(shader,1);
	return shader;
}

// Get location handlers to for shader attributes and uniforms
// NOTE: If any location is not found, loc point becomes -1
void SetShaderDefaultLocations(Shader *shader)
{
	// NOTE: Default shader attrib locations have been fixed before linking:
	//          vertex position location    = 0
	//          vertex texcoord location    = 1
	//          vertex normal location      = 2
	//          vertex color location       = 3
	//          vertex tangent location     = 4
	//          vertex texcoord2 location   = 5
	// Get handles to GLSL input attibute locations
	shader->locs[LOC_VERTEX_POSITION] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_POSITION_NAME);
	shader->locs[LOC_VERTEX_TEXCOORD01] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_TEXCOORD_NAME);
	shader->locs[LOC_VERTEX_TEXCOORD02] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_TEXCOORD2_NAME);
	shader->locs[LOC_VERTEX_NORMAL] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_NORMAL_NAME);
	shader->locs[LOC_VERTEX_TANGENT] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_TANGENT_NAME);
	shader->locs[LOC_VERTEX_COLOR] = glGetAttribLocation(shader->id, DEFAULT_ATTRIB_COLOR_NAME);
	// Get handles to GLSL uniform locations (vertex shader)
	shader->locs[LOC_MATRIX_MVP]  = glGetUniformLocation(shader->id, "mvp");
	shader->locs[LOC_MATRIX_PROJECTION]  = glGetUniformLocation(shader->id, "projection");
	shader->locs[LOC_MATRIX_VIEW]  = glGetUniformLocation(shader->id, "view");
	// Get handles to GLSL uniform locations (fragment shader)
	shader->locs[LOC_COLOR_DIFFUSE] = glGetUniformLocation(shader->id, "colDiffuse");
	shader->locs[LOC_MAP_DIFFUSE] = glGetUniformLocation(shader->id, "texture0");
	shader->locs[LOC_MAP_SPECULAR] = glGetUniformLocation(shader->id, "texture1");
	shader->locs[LOC_MAP_NORMAL] = glGetUniformLocation(shader->id, "texture2");
}

// Unload default shader
void UnloadShaderDefault(void)
{
	glUseProgram(0);
	glDetachShader(RLGL.State.defaultShader.id, RLGL.State.defaultVShaderId);
	glDetachShader(RLGL.State.defaultShader.id, RLGL.State.defaultFShaderId);
	glDeleteShader(RLGL.State.defaultVShaderId);
	glDeleteShader(RLGL.State.defaultFShaderId);
	glDeleteProgram(RLGL.State.defaultShader.id);
}

// to check locs content is good from different c files
/*void checkshader(Shader shader,int i)
{
	int j;
	if(shader.locs!=NULL)
	{
		for(j=0;j<MAX_SHADER_LOCATIONS;j++)
		{
			debugNetPrintf(3,"[ORBISGL] shader.locs[%d]=%d\n",j,shader.locs[j]);
			sleep(1);
		}
	}
	else
	{
		debugNetPrintf(3,"[ORBISGL] shader locs not good %d \n",i);
	
	}
	debugNetPrintf(3,"[ORBISGL] end  %d \n",i);
	sleep(1);

}*/
// Load default internal buffers
void LoadBuffersDefault(void)
{
	//checkshader(RLGL.State.defaultShader,4);

	// Initialize CPU (RAM) arrays (vertex position, texcoord, color data and indexes)
	//--------------------------------------------------------------------------------------------
	for (int i = 0; i < MAX_BATCH_BUFFERING; i++)
	{
		RLGL.State.vertexData[i].vertices = (float *)malloc(sizeof(float)*3*4*MAX_BATCH_ELEMENTS);        // 3 float by vertex, 4 vertex by quad
		RLGL.State.vertexData[i].texcoords = (float *)malloc(sizeof(float)*2*4*MAX_BATCH_ELEMENTS);       // 2 float by texcoord, 4 texcoord by quad
		RLGL.State.vertexData[i].colors = (unsigned char *)malloc(sizeof(unsigned char)*4*4*MAX_BATCH_ELEMENTS);  // 4 float by color, 4 colors by quad
		RLGL.State.vertexData[i].indices = (unsigned short *)malloc(sizeof(unsigned short)*6*MAX_BATCH_ELEMENTS);  // 6 int by quad (indices)
		for (int j = 0; j < (3*4*MAX_BATCH_ELEMENTS); j++) RLGL.State.vertexData[i].vertices[j] = 0.0f;
		for (int j = 0; j < (2*4*MAX_BATCH_ELEMENTS); j++) RLGL.State.vertexData[i].texcoords[j] = 0.0f;
		for (int j = 0; j < (4*4*MAX_BATCH_ELEMENTS); j++) RLGL.State.vertexData[i].colors[j] = 0;
		int k = 0;
		// Indices can be initialized right now
		for (int j = 0; j < (6*MAX_BATCH_ELEMENTS); j += 6)
		{
			RLGL.State.vertexData[i].indices[j] = 4*k;
			RLGL.State.vertexData[i].indices[j + 1] = 4*k + 1;
			RLGL.State.vertexData[i].indices[j + 2] = 4*k + 2;
			RLGL.State.vertexData[i].indices[j + 3] = 4*k;
			RLGL.State.vertexData[i].indices[j + 4] = 4*k + 2;
			RLGL.State.vertexData[i].indices[j + 5] = 4*k + 3;
			k++;
		}
		RLGL.State.vertexData[i].vCounter = 0;
		RLGL.State.vertexData[i].tcCounter = 0;
		RLGL.State.vertexData[i].cCounter = 0;
	}
	debugNetPrintf(INFO, "[ORBISGL] %s Internal buffers initialized successfully (CPU)\n",__FUNCTION__);
	//--------------------------------------------------------------------------------------------
	// Upload to GPU (VRAM) vertex data and initialize VAOs/VBOs
	//--------------------------------------------------------------------------------------------
	for (int i = 0; i < MAX_BATCH_BUFFERING; i++)
	{
		if (RLGL.ExtSupported.vao)
		{
			// Initialize Quads VAO
			glGenVertexArrays(1, &RLGL.State.vertexData[i].vaoId);
			glBindVertexArray(RLGL.State.vertexData[i].vaoId);
		}
		// Quads - Vertex buffers binding and attributes enable
		// Vertex position buffer (shader-location = 0)
		glGenBuffers(1, &RLGL.State.vertexData[i].vboId[0]);
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[i].vboId[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[i].vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_POSITION]);
		glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_POSITION], 3, GL_FLOAT, 0, 0, 0);
		// Vertex texcoord buffer (shader-location = 1)
		glGenBuffers(1, &RLGL.State.vertexData[i].vboId[1]);
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[i].vboId[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[i].texcoords, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_TEXCOORD01]);
		glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_TEXCOORD01], 2, GL_FLOAT, 0, 0, 0);
		// Vertex color buffer (shader-location = 3)
		glGenBuffers(1, &RLGL.State.vertexData[i].vboId[2]);
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[i].vboId[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned char)*4*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[i].colors, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_COLOR]);
		glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
		// Fill index buffer
		glGenBuffers(1, &RLGL.State.vertexData[i].vboId[3]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RLGL.State.vertexData[i].vboId[3]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short)*6*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[i].indices, GL_STATIC_DRAW);
	}
	debugNetPrintf(INFO, "[ORBISGL] %s Internal buffers uploaded successfully (GPU)\n",__FUNCTION__);
	// Unbind the current VAO
	if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	//--------------------------------------------------------------------------------------------
}

// Update default internal buffers (VAOs/VBOs) with vertex array data
// NOTE: If there is not vertex data, buffers doesn't need to be updated (vertexCount > 0)
// TODO: If no data changed on the CPU arrays --> No need to re-update GPU arrays (change flag required)
void UpdateBuffersDefault(void)
{
	// Update vertex buffers data
	if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter > 0)
	{
		// Activate elements VAO
		if (RLGL.ExtSupported.vao) glBindVertexArray(RLGL.State.vertexData[RLGL.State.currentBuffer].vaoId);
		// Vertex positions buffer
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter, RLGL.State.vertexData[RLGL.State.currentBuffer].vertices);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[RLGL.State.currentBuffer].vertices, GL_DYNAMIC_DRAW);  // Update all buffer
		// Texture coordinates buffer
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[1]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*2*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter, RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*2*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[RLGL.State.currentBuffer].texcoords, GL_DYNAMIC_DRAW); // Update all buffer
		// Colors buffer
		glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[2]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned char)*4*RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter, RLGL.State.vertexData[RLGL.State.currentBuffer].colors);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*4*MAX_BATCH_ELEMENTS, RLGL.State.vertexData[RLGL.State.currentBuffer].colors, GL_DYNAMIC_DRAW);    // Update all buffer
		// NOTE: glMapBuffer() causes sync issue.
		// If GPU is working with this buffer, glMapBuffer() will wait(stall) until GPU to finish its job.
		// To avoid waiting (idle), you can call first glBufferData() with NULL pointer before glMapBuffer().
		// If you do that, the previous data in PBO will be discarded and glMapBuffer() returns a new
		// allocated pointer immediately even if GPU is still working with the previous data.
		// Another option: map the buffer object into client's memory
		// Probably this code could be moved somewhere else...
		// RLGL.State.vertexData[RLGL.State.currentBuffer].vertices = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		// if (RLGL.State.vertexData[RLGL.State.currentBuffer].vertices)
		// {
		    // Update vertex data
		// }
		// glUnmapBuffer(GL_ARRAY_BUFFER);
		// Unbind the current VAO
		if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	}
}

// Draw default internal buffers vertex data
void DrawBuffersDefault(void)
{
	Matrix matProjection = RLGL.State.projection;
	Matrix matModelView = RLGL.State.modelview;
	int eyesCount = 1;
	for (int eye = 0; eye < eyesCount; eye++)
	{
		// Draw buffers
		if (RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter > 0)
		{
			// Set current shader and upload current MVP matrix
			glUseProgram(RLGL.State.currentShader.id);
			// Create modelview-projection matrix
			Matrix matMVP = MatrixMultiply(RLGL.State.modelview, RLGL.State.projection);
			glUniformMatrix4fv(RLGL.State.currentShader.locs[LOC_MATRIX_MVP], 1, false, MatrixToFloat(matMVP));
			glUniform4f(RLGL.State.currentShader.locs[LOC_COLOR_DIFFUSE], 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform1i(RLGL.State.currentShader.locs[LOC_MAP_DIFFUSE], 0);    // Provided value refers to the texture unit (active)
			// TODO: Support additional texture units on custom shader
			//if (RLGL.State.currentShader->locs[LOC_MAP_SPECULAR] > 0) glUniform1i(RLGL.State.currentShader.locs[LOC_MAP_SPECULAR], 1);
			//if (RLGL.State.currentShader->locs[LOC_MAP_NORMAL] > 0) glUniform1i(RLGL.State.currentShader.locs[LOC_MAP_NORMAL], 2);
			// NOTE: Right now additional map textures not considered for default buffers drawing
			int vertexOffset = 0;
			if (RLGL.ExtSupported.vao) glBindVertexArray(RLGL.State.vertexData[RLGL.State.currentBuffer].vaoId);
			else
			{
				// Bind vertex attrib: position (shader-location = 0)
				glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[0]);
				glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_POSITION], 3, GL_FLOAT, 0, 0, 0);
				glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_POSITION]);
				// Bind vertex attrib: texcoord (shader-location = 1)
				glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[1]);
				glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_TEXCOORD01], 2, GL_FLOAT, 0, 0, 0);
				glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_TEXCOORD01]);
				// Bind vertex attrib: color (shader-location = 3)
				glBindBuffer(GL_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[2]);
				glVertexAttribPointer(RLGL.State.currentShader.locs[LOC_VERTEX_COLOR], 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
				glEnableVertexAttribArray(RLGL.State.currentShader.locs[LOC_VERTEX_COLOR]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RLGL.State.vertexData[RLGL.State.currentBuffer].vboId[3]);
			}
			glActiveTexture(GL_TEXTURE0);
			for (int i = 0; i < RLGL.State.drawsCounter; i++)
			{
				glBindTexture(GL_TEXTURE_2D, RLGL.State.draws[i].textureId);
				// TODO: Find some way to bind additional textures --> Use global texture IDs? Register them on draw[i]?
				//if (RLGL.State.currentShader->locs[LOC_MAP_SPECULAR] > 0) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, textureUnit1_id); }
				//if (RLGL.State.currentShader->locs[LOC_MAP_SPECULAR] > 0) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, textureUnit2_id); }
				if ((RLGL.State.draws[i].mode == RL_LINES) || (RLGL.State.draws[i].mode == RL_TRIANGLES)) glDrawArrays(RLGL.State.draws[i].mode, vertexOffset, RLGL.State.draws[i].vertexCount);
				else
				{
				    glDrawElements(GL_TRIANGLES, RLGL.State.draws[i].vertexCount/4*6, GL_UNSIGNED_SHORT, (GLvoid *)(sizeof(GLushort)*vertexOffset/4*6));
				}
				vertexOffset += (RLGL.State.draws[i].vertexCount + RLGL.State.draws[i].vertexAlignment);
			}
			if (!RLGL.ExtSupported.vao)
			{
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			glBindTexture(GL_TEXTURE_2D, 0);    // Unbind textures
		}
		if (RLGL.ExtSupported.vao) glBindVertexArray(0); // Unbind VAO
		glUseProgram(0);    // Unbind shader program
	}
	// Reset vertex counters for next frame
	RLGL.State.vertexData[RLGL.State.currentBuffer].vCounter = 0;
	RLGL.State.vertexData[RLGL.State.currentBuffer].tcCounter = 0;
	RLGL.State.vertexData[RLGL.State.currentBuffer].cCounter = 0;
	// Reset depth for next draw
	RLGL.State.currentDepth = -1.0f;
	// Restore projection/modelview matrices
	RLGL.State.projection = matProjection;
	RLGL.State.modelview = matModelView;
	// Reset RLGL.State.draws array
	for (int i = 0; i < MAX_DRAWCALL_REGISTERED; i++)
	{
		RLGL.State.draws[i].mode = RL_QUADS;
		RLGL.State.draws[i].vertexCount = 0;
		RLGL.State.draws[i].textureId = RLGL.State.defaultTextureId;
	}
	RLGL.State.drawsCounter = 1;
	// Change to next buffer in the list
	RLGL.State.currentBuffer++;
	if (RLGL.State.currentBuffer >= MAX_BATCH_BUFFERING) RLGL.State.currentBuffer = 0;
}

// Unload default internal buffers vertex data from CPU and GPU
void UnloadBuffersDefault(void)
{
	// Unbind everything
	if (RLGL.ExtSupported.vao) glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	for (int i = 0; i < MAX_BATCH_BUFFERING; i++)
	{
		// Delete VBOs from GPU (VRAM)
		glDeleteBuffers(1, &RLGL.State.vertexData[i].vboId[0]);
		glDeleteBuffers(1, &RLGL.State.vertexData[i].vboId[1]);
		glDeleteBuffers(1, &RLGL.State.vertexData[i].vboId[2]);
		glDeleteBuffers(1, &RLGL.State.vertexData[i].vboId[3]);
		// Delete VAOs from GPU (VRAM)
		if (RLGL.ExtSupported.vao) glDeleteVertexArrays(1, &RLGL.State.vertexData[i].vaoId);
		// Free vertex arrays memory from CPU (RAM)
		free(RLGL.State.vertexData[i].vertices);
		free(RLGL.State.vertexData[i].texcoords);
		free(RLGL.State.vertexData[i].colors);
		free(RLGL.State.vertexData[i].indices);
	}
}

// Renders a 1x1 XY quad in NDC
void GenDrawQuad(void)
{
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    float vertices[] = {
        // Positions        // Texture Coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    // Set up plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

    // Link vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void *)(3*sizeof(float)));

    // Draw quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &quadVAO);
}

// Renders a 1x1 3D cube in NDC
void GenDrawCube(void)
{
	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;
	float vertices[] = {
		-1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f , 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f
	};
	// Set up cube VAO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	// Fill buffer
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// Link vertex attributes
	glBindVertexArray(cubeVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(3*sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(6*sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	// Draw cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteVertexArrays(1, &cubeVAO);
}


