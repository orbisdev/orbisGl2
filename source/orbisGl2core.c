/*
 * liborbis 
 * Copyright (C) 2015,2016,2017,2018 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 * Repository https://github.com/orbisdev
 */
#include <stdlib.h>
#include <string.h>
#include <debugnet.h>
#include <stdlib.h>
#include <orbisGl2.h>
#include <orbisNfs.h>
#include <fcntl.h>


rlglData RLGL = { 0 };

OrbisGlConfig *orbisGlConf=NULL;
int orbisgl_external_conf=-1;

EGLint attribs[] = 
{
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_DEPTH_SIZE, 16,
	EGL_STENCIL_SIZE, 0,
	EGL_SAMPLE_BUFFERS, 0,
	EGL_SAMPLES, 0,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_NONE,
};

EGLint ctx_attribs[] = 
{
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE,
};

EGLint window_attribs[] = {
	EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
	EGL_NONE,
};

void orbisGlSwapBuffers()
{
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			eglSwapBuffers(orbisGlConf->device, orbisGlConf->surface);
		}
	}
}
void orbisGlDestroyProgram(GLuint program_id)
{
	int ret;
	if(orbisGlConf && program_id>0)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			glDeleteProgram(program_id);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBIS_GL] glDeleteProgram failed: 0x%08X\n", ret);
			}
		}
	}
}
void orbisGlShaderLog(GLuint shader_id)
{
	int ret;
	if(orbisGlConf && shader_id>0)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{		
			GLint log_length;
			glGetShaderiv(shader_id,GL_INFO_LOG_LENGTH,&log_length);
			if(log_length)
			{
				GLchar log_buffer[log_length];
				glGetShaderInfoLog(shader_id,log_length,NULL,log_buffer);
				ret=glGetError();
				if(ret)
				{
					debugNetPrintf(ERROR,"[ORBISGL] glGetShaderInfoLog failed: 0x%08X\n",ret);
					return;
				}
				debugNetPrintf(ERROR,"[ORBISGL] shader compilation with log:\n%s\n",log_buffer);
			}
			else
			{
				debugNetPrintf(DEBUG,"[ORBISGL] shader compilantion no log\n");
			}
		}
	}
}
void orbisGlProgramLog(GLuint program_id)
{
	int ret;
	if(orbisGlConf && program_id>0)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			GLint log_length;
			glGetProgramiv(program_id,GL_INFO_LOG_LENGTH,&log_length);
			if(log_length)
			{
				GLchar log_buffer[log_length];
				glGetProgramInfoLog(program_id,log_length,NULL,log_buffer);
				ret=glGetError();
				if(ret)
				{
					debugNetPrintf(ERROR,"[ORBISGL] glGetProgramInfoLog failed: 0x%08X\n",ret);
					return;
				}
				debugNetPrintf(DEBUG,"[ORBISGL] program link with log:\n%s\n",log_buffer);
			}
			else
			{
				debugNetPrintf(DEBUG,"[ORBISGL] program link no log\n");
			}
		}
	}
}
GLuint orbisGlCompileShader(const GLenum type,const GLchar* source) 
{
	GLuint shader_id;
	GLint compile_status;
	int ret;
	
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			shader_id=glCreateShader(type);
			
			if(!shader_id)
			{
				ret=glGetError();
				debugNetPrintf(ERROR,"[ORBISGL] glCreateShader failed: 0x%08X\n",ret);
				return 0;
			}
			glShaderSource(shader_id,1,(const GLchar **)&source,NULL);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glShaderSource failed: 0x%08X\n",ret);
				//avoid this by now some weird behaviour
				//return 0;
			}
			glCompileShader(shader_id);
			ret=glGetError();
			if(ret)
			{	
				debugNetPrintf(ERROR,"[ORBISGL] glCompileShader failed: 0x%08X\n",ret);
				return 0;
			}
			glGetShaderiv(shader_id,GL_COMPILE_STATUS,&compile_status);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glGetShaderiv failed: 0x%08X\n",ret);
				return 0;
			}
			if (!compile_status)
			{
					
					debugNetPrintf(ERROR,"[ORBISGL] shader compilation failed\n");
					orbisGlShaderLog(shader_id);
					return 0;
			}
			debugNetPrintf(DEBUG,"[ORBISGL] shader compilation shader_id=%d done\n",shader_id);
			orbisGlShaderLog(shader_id);
			return shader_id;
		}
	}
	debugNetPrintf(ERROR,"[ORBISGL] orbisGl is not initialized\n");
	
	return 0;
}
GLuint orbisGlLinkProgram(const GLuint vertex_shader,const GLuint fragment_shader)
{
	GLuint program_id;
	int ret;
	GLint link_status;
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			program_id=glCreateProgram();
			if(!program_id)
			{
				ret=glGetError();
				debugNetPrintf(ERROR,"[ORBIS_GL] glCreateProgram failed: 0x%08X\n", ret);
				return 0;
			}
			glAttachShader(program_id,vertex_shader);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glAttachShader(vertex_shader) failed: 0x%08X\n",ret);
				return 0;
			}
			glAttachShader(program_id,fragment_shader);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glAttachShader(fragment_shader) failed: 0x%08X\n",ret);
				return 0;
			}
			glLinkProgram(program_id);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glLinkProgram() failed: 0x%08X\n",ret);
				return 0;
			}
			glGetProgramiv(program_id, GL_LINK_STATUS,&link_status);
			if (!link_status)
			{
				debugNetPrintf(ERROR,"[ORBISGL] program link failed\n");
				orbisGlProgramLog(program_id);
				return 0;
			}
			debugNetPrintf(INFO,"[ORBISGL] link program_id=%d done\n",program_id);
			orbisGlProgramLog(program_id);
			return program_id;
		}
	}
	debugNetPrintf(ERROR,"[ORBISGL] orbisGl is not initialized\n");
	
	return 0;
}


GLuint orbisGlCreateProgram(const char* vertexShaderFilename, const char* fragmentShaderFilename)
{
    debugNetPrintf(DEBUG,"[ORBISGL] orbisGlLoadShaders %s %s\n",vertexShaderFilename,fragmentShaderFilename);
	char * vsSource=(char *)orbisNfsGetFileContent(vertexShaderFilename);
	if(vsSource==NULL)
	{
	    debugNetPrintf(DEBUG,"[ORBISGL] can't open vertex shader at %s\n",vertexShaderFilename);
		return 0;
	}
	GLuint vs=orbisGlCompileShader(GL_VERTEX_SHADER,vsSource);
	if(vs==0)
	{
	    debugNetPrintf(DEBUG,"[ORBISGL] can't compile vertex shader at %s\n",vertexShaderFilename);
		return 0;
	}
	free(vsSource);
	char * fsSource=(char *)orbisNfsGetFileContent(fragmentShaderFilename);
	if(fsSource==NULL)
	{
	    debugNetPrintf(DEBUG,"[ORBISGL] can't open fragment shader at %s\n",vertexShaderFilename);
		return 0;
	}
	GLuint fs=orbisGlCompileShader(GL_FRAGMENT_SHADER,fsSource);
	if(fs==0)
	{
	    debugNetPrintf(DEBUG,"[ORBISGL] can't compile fragment shader at %s\n",vertexShaderFilename);
	}
	free(fsSource);
	GLuint po=orbisGlLinkProgram(vs,fs);
	if(po==0)
	{
	    debugNetPrintf(DEBUG,"[ORBISGL] can't link program with vertex shader %d and fragment shader %d\n",vs,fs);
		return 0;
	}
	glDeleteShader(vs);
	glDeleteShader(fs);
	
	return po;
}

GLuint orbisGlCreateTexture(const GLsizei width,const GLsizei height,const GLenum type,const GLvoid* pixels)
{
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	if(texture_id>0)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type, GL_UNSIGNED_BYTE, pixels);
		//glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	return texture_id;
}
void orbisGlDestroyTexture(GLuint texture_id)
{
	int ret;
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
		
			glDeleteTextures(1,&texture_id);
			ret=glGetError();
			if(ret)
			{
				debugNetPrintf(ERROR,"[ORBISGL] glDeleteTextures failed: 0x%08X\n",ret);
			}
		}
	}
}
void orbisGlDestroyDisplay()
{
	int ret;
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			ret=eglTerminate(orbisGlConf->device);
			if(!ret)
			{
				ret=eglGetError();
				debugNetPrintf(ERROR,"[ORBISGL] eglTerminate failed: 0x%08X\n",ret);
			}
			orbisGlConf->device=EGL_NO_DISPLAY;
		}
	}
}
void orbisGlDestroySurface()
{
	int ret;
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{
			ret=eglDestroySurface(orbisGlConf->device,orbisGlConf->surface);
			if (!ret)
			{
				ret=eglGetError();
				debugNetPrintf(ERROR,"[ORBISGL] eglDestroySurface failed: 0x%08X\n",ret);
			}
			orbisGlConf->surface=EGL_NO_SURFACE;
		}
	}
}
void orbisGlDestroyContext()
{
	int ret;
	if(orbisGlConf)
	{
		if(orbisGlConf->orbisgl_initialized==1)
		{		
			ret=eglDestroyContext(orbisGlConf->device,orbisGlConf->context);
			if (!ret)
			{
				ret=eglGetError();
				debugNetPrintf(ERROR,"[ORBISGL] eglDestroyContext failed: 0x%08X\n",ret);
			}
			orbisGlConf->context=EGL_NO_CONTEXT;
		}
	}
}
void orbisGlFinish()
{
	if(orbisgl_external_conf!=1)
	{
		rlglClose();

		if(orbisGlConf->orbisgl_initialized==1)
		{		
			orbisGlDestroyContext();
			orbisGlDestroySurface();
			orbisGlDestroyDisplay();			
		}
		orbisGlConf->orbisgl_initialized=-1;
		debugNetPrintf(DEBUG,"[ORBISGL] liborbisGl finished\n");
	}
}

OrbisGlConfig *orbisGlGetConf()
{
	if(orbisGlConf)
	{
		return orbisGlConf;
	}
	
	return NULL; 
}
int orbisGlSetConf(OrbisGlConfig *conf)
{
	if(conf)
	{
		orbisGlConf=conf;
		orbisgl_external_conf=1;
		return orbisGlConf->orbisgl_initialized;
	}
	return 0; 
}
int orbisGlInitWithConf(OrbisGlConfig *conf)
{
	int ret;
	ret=orbisGlSetConf(conf);
	if(ret)
	{
		debugNetPrintf(DEBUG,"[ORBISGL] liborbisGl already initialized using configuration external\n");
		debugNetPrintf(DEBUG,"[ORBISGL] orbisgl_initialized=%d\n",orbisGlConf->orbisgl_initialized);
		debugNetPrintf(DEBUG,"[ORBISGL] ready to have a lot of fun...\n");
		return orbisGlConf->orbisgl_initialized;
	}
	else
	{
		return 0;
	}
}

int GetScreenWidth()
{
	if(orbisGlConf)
	{
		return orbisGlConf->screen.width;
	}
	return 0;
}
int GetScreenHeight()
{
	if(orbisGlConf)
	{
		return orbisGlConf->screen.height;
	}
	return 0;	
}



// Set background color (framebuffer clear color)
void ClearBackground(Color color)
{
	rlClearColor(color.r, color.g, color.b, color.a);   // Set clear color
	rlClearScreenBuffers();                             // Clear current framebuffers
}
// Setup canvas (framebuffer) to start drawing
void BeginDrawing(void)
{
	if(orbisGlConf)
	{
		orbisGlConf->current = GetTime();// Number of elapsed seconds since InitTimer()
		orbisGlConf->update = orbisGlConf->current - orbisGlConf->previous;
		orbisGlConf->previous = orbisGlConf->current;

		rlLoadIdentity();// Reset current matrix (MODELVIEW)
		rlMultMatrixf(MatrixToFloat(orbisGlConf->screenScale));// Apply screen scaling

		//rlTranslatef(0.375, 0.375, 0);// HACK to have 2D pixel-perfect drawing on OpenGL 1.1
		// NOTE: Not required with OpenGL 3.3+
	}
}

// End canvas drawing and swap buffers (double buffering)
void EndDrawing(void)
{
	if(orbisGlConf)
	{
		rlglDraw();// Draw Buffers (Only OpenGL 3+ and ES2)
		SwapBuffers();// Copy back buffer to front buffer
		//PollInputEvents();
		// Frame time control system
		orbisGlConf->current = GetTime();
		orbisGlConf->draw = orbisGlConf->current - orbisGlConf->previous;
		orbisGlConf->previous = orbisGlConf->current;
		orbisGlConf->frame = orbisGlConf->update + orbisGlConf->draw;
		// Wait for some milliseconds...
		if (orbisGlConf->frame < orbisGlConf->target)
		{
		    Wait((float)(orbisGlConf->target - orbisGlConf->frame)*1000.0f);
		    orbisGlConf->current = GetTime();
		    double waitTime = orbisGlConf->current - orbisGlConf->previous;
		    orbisGlConf->previous = orbisGlConf->current;
		    orbisGlConf->frame += waitTime;// Total frame time: update + draw + wait
		}
	}
}

// Initialize 2D mode with custom camera (2D)
void BeginMode2D(Camera2D camera)
{
	if(orbisGlConf)
	{
		rlglDraw();// Draw Buffers (Only OpenGL 3+ and ES2)
		rlLoadIdentity();// Reset current matrix (MODELVIEW)
		// Apply 2d camera transformation to modelview
		rlMultMatrixf(MatrixToFloat(GetCameraMatrix2D(camera)));
		// Apply screen scaling if required
		rlMultMatrixf(MatrixToFloat(orbisGlConf->screenScale));
	}
}

// Ends 2D mode with custom camera
void EndMode2D(void)
{
	if(orbisGlConf)
	{
		rlglDraw();                         // Draw Buffers (Only OpenGL 3+ and ES2)
		rlLoadIdentity();                   // Reset current matrix (MODELVIEW)
		rlMultMatrixf(MatrixToFloat(orbisGlConf->screenScale)); // Apply screen scaling if required
	}
}

// Initializes 3D mode with custom camera (3D)
void BeginMode3D(Camera3D camera)
{
	if(orbisGlConf)
	{
		rlglDraw();                         // Draw Buffers (Only OpenGL 3+ and ES2)
		rlMatrixMode(RL_PROJECTION);        // Switch to projection matrix
		rlPushMatrix();                     // Save previous matrix, which contains the settings for the 2d ortho projection
		rlLoadIdentity();                   // Reset current matrix (PROJECTION)
		float aspect = (float)orbisGlConf->currentFbo.width/(float)orbisGlConf->currentFbo.height;
		if (camera.type == CAMERA_PERSPECTIVE)
		{
		    // Setup perspective projection
		    double top = 0.01*tan(camera.fovy*0.5*DEG2RAD);
		    double right = top*aspect;
		    rlFrustum(-right, right, -top, top, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
		}
		else if (camera.type == CAMERA_ORTHOGRAPHIC)
		{
		    // Setup orthographic projection
		    double top = camera.fovy/2.0;
		    double right = top*aspect;
		    rlOrtho(-right, right, -top,top, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
		}
		// NOTE: zNear and zFar values are important when computing depth buffer values
		rlMatrixMode(RL_MODELVIEW);// Switch back to modelview matrix
		rlLoadIdentity();// Reset current matrix (MODELVIEW)
		// Setup Camera view
		Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
		rlMultMatrixf(MatrixToFloat(matView));// Multiply MODELVIEW matrix by view matrix (camera)
		rlEnableDepthTest();// Enable DEPTH_TEST for 3D
	}
}

// Ends 3D mode and returns to default 2D orthographic mode
void EndMode3D(void)
{
	if(orbisGlConf)
	{
		rlglDraw();							// Process internal buffers (update + draw)
		rlMatrixMode(RL_PROJECTION);		// Switch to projection matrix
		rlPopMatrix();						// Restore previous matrix (PROJECTION) from matrix stack
		rlMatrixMode(RL_MODELVIEW);		// Get back to modelview matrix
		rlLoadIdentity();					// Reset current matrix (MODELVIEW)
		rlMultMatrixf(MatrixToFloat(orbisGlConf->screenScale)); // Apply screen scaling if required
		rlDisableDepthTest();				// Disable DEPTH_TEST for 2D
	}
}

// Initializes render texture for drawing
void BeginTextureMode(RenderTexture2D target)
{
	if(orbisGlConf)
	{
		rlglDraw();							// Draw Buffers (Only OpenGL 3+ and ES2)
		rlEnableRenderTexture(target.id);	// Enable render target
		// Set viewport to framebuffer size
		rlViewport(0, 0, target.texture.width, target.texture.height);
		rlMatrixMode(RL_PROJECTION);		// Switch to PROJECTION matrix
		rlLoadIdentity();					// Reset current matrix (PROJECTION)
		// Set orthographic projection to current framebuffer size
		// NOTE: Configured top-left corner as (0, 0)
		rlOrtho(0, target.texture.width, target.texture.height, 0, 0.0f, 1.0f);
		rlMatrixMode(RL_MODELVIEW);		// Switch back to MODELVIEW matrix
		rlLoadIdentity();					// Reset current matrix (MODELVIEW)
		//rlScalef(0.0f, -1.0f, 0.0f);		// Flip Y-drawing (?)
		// Setup current width/height for proper aspect ratio
		// calculation when using BeginMode3D()
		orbisGlConf->currentFbo.width = target.texture.width;
		orbisGlConf->currentFbo.height = target.texture.height;
	}
}

// Ends drawing to render texture
void EndTextureMode(void)
{
	if(orbisGlConf)
	{
		rlglDraw();							// Draw Buffers (Only OpenGL 3+ and ES2)
		rlDisableRenderTexture();			// Disable render target
		// Set viewport to default framebuffer size
		SetupViewport(orbisGlConf->render.width, orbisGlConf->render.height);
		// Reset current screen size
		orbisGlConf->currentFbo.width = GetScreenWidth();
		orbisGlConf->currentFbo.height = GetScreenHeight();
	}
}

// Begin scissor mode (define screen area for following drawing)
// NOTE: Scissor rec refers to bottom-left corner, we change it to upper-left
void BeginScissorMode(int x, int y, int width, int height)
{
	if(orbisGlConf)
	{
		rlglDraw(); // Force drawing elements
		rlEnableScissorTest();
		rlScissor(x, GetScreenHeight() - (y + height), width, height);
	}
}

// End scissor mode
void EndScissorMode(void)
{
	if(orbisGlConf)
	{	
		rlglDraw(); // Force drawing elements
		rlDisableScissorTest();
	}
}


// Get transform matrix for camera
Matrix GetCameraMatrix(Camera camera)
{
	return MatrixLookAt(camera.position, camera.target, camera.up);
}

// Returns camera 2d transform matrix
Matrix GetCameraMatrix2D(Camera2D camera)
{
	Matrix matTransform = { 0 };
	// The camera in world-space is set by
	//   1. Move it to target
	//   2. Rotate by -rotation and scale by (1/zoom)
	//      When setting higher scale, it's more intuitive for the world to become bigger (= camera become smaller),
	//      not for the camera getting bigger, hence the invert. Same deal with rotation.
	//   3. Move it by (-offset);
	//      Offset defines target transform relative to screen, but since we're effectively "moving" screen (camera)
	//      we need to do it into opposite direction (inverse transform)
	// Having camera transform in world-space, inverse of it gives the modelview transform.
	// Since (A*B*C)' = C'*B'*A', the modelview is
	//   1. Move to offset
	//   2. Rotate and Scale
	//   3. Move by -target
	Matrix matOrigin = MatrixTranslate(-camera.target.x, -camera.target.y, 0.0f);
	Matrix matRotation = MatrixRotate((Vector3){ 0.0f, 0.0f, 1.0f }, camera.rotation*DEG2RAD);
	Matrix matScale = MatrixScale(camera.zoom, camera.zoom, 1.0f);
	Matrix matTranslation = MatrixTranslate(camera.offset.x, camera.offset.y, 0.0f);
	matTransform = MatrixMultiply(MatrixMultiply(matOrigin, MatrixMultiply(matScale, matRotation)), matTranslation);
	return matTransform;
}



// Returns size position for a 3d world space position (useful for texture drawing)
Vector2 GetWorldToScreenEx(Vector3 position, Camera3D camera, int width, int height)
{
	// Calculate projection matrix (from perspective instead of frustum
	Matrix matProj = MatrixIdentity();
	if (camera.type == CAMERA_PERSPECTIVE)
	{
		// Calculate projection matrix from perspective
		matProj = MatrixPerspective(camera.fovy * DEG2RAD, ((double)width/(double)height), DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
	}
	else if (camera.type == CAMERA_ORTHOGRAPHIC)
	{
		float aspect = (float)orbisGlConf->screen.width/(float)orbisGlConf->screen.height;
		double top = camera.fovy/2.0;
		double right = top*aspect;
		// Calculate projection matrix from orthographic
		matProj = MatrixOrtho(-right, right, -top, top, DEFAULT_NEAR_CULL_DISTANCE, DEFAULT_FAR_CULL_DISTANCE);
	}
	// Calculate view matrix from camera look at (and transpose it)
	Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
	// Convert world position vector to quaternion
	Quaternion worldPos = { position.x, position.y, position.z, 1.0f };
	// Transform world position to view
	worldPos = QuaternionTransform(worldPos, matView);
	// Transform result to projection (clip space position)
	worldPos = QuaternionTransform(worldPos, matProj);
	// Calculate normalized device coordinates (inverted y)
	Vector3 ndcPos = { worldPos.x/worldPos.w, -worldPos.y/worldPos.w, worldPos.z/worldPos.w };
	// Calculate 2d screen position vector
	Vector2 screenPosition = { (ndcPos.x + 1.0f)/2.0f*(float)width, (ndcPos.y + 1.0f)/2.0f*(float)height };
	return screenPosition;
}

// Returns the screen space position from a 3d world space position
Vector2 GetWorldToScreen(Vector3 position, Camera3D camera)
{
	Vector2 screenPosition = GetWorldToScreenEx(position, camera, GetScreenWidth(), GetScreenHeight());
	return screenPosition;
}
// Returns the screen space position for a 2d camera world space position
Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera)
{
	Matrix matCamera = GetCameraMatrix2D(camera);
	Vector3 transform = Vector3Transform((Vector3){ position.x, position.y, 0 }, matCamera);
	return (Vector2){ transform.x, transform.y };
}

// Returns the world space position for a 2d camera screen space position
Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera)
{
	Matrix invMatCamera = MatrixInvert(GetCameraMatrix2D(camera));
	Vector3 transform = Vector3Transform((Vector3){ position.x, position.y, 0 }, invMatCamera);
	return (Vector2){ transform.x, transform.y };
}

// Set target FPS (maximum)
void SetTargetFPS(int fps)
{
	if (fps < 1) orbisGlConf->target = 0.0;
	else orbisGlConf->target = 1.0/(double)fps;
	debugNetPrintf(INFO, "[ORBISGL] %s Target time per frame: %02.03f milliseconds\n",__FUNCTION__,(float)orbisGlConf->target*1000);
}

// Returns time in seconds for last frame drawn
float GetFrameTime(void)
{
	return (float)orbisGlConf->frame;
}

// Returns current FPS
int GetFPS(void)
{
	#define FPS_CAPTURE_FRAMES_COUNT    30      // 30 captures
	#define FPS_AVERAGE_TIME_SECONDS   0.5f     // 500 millisecondes
	#define FPS_STEP (FPS_AVERAGE_TIME_SECONDS/FPS_CAPTURE_FRAMES_COUNT)

	static int index = 0;
	static float history[FPS_CAPTURE_FRAMES_COUNT] = { 0 };
	static float average = 0, last = 0;
	float fpsFrame = GetFrameTime();

	if (fpsFrame == 0) return 0;

	if ((GetTime() - last) > FPS_STEP)
	{
		last = GetTime();
		index = (index + 1)%FPS_CAPTURE_FRAMES_COUNT;
		average -= history[index];
		history[index] = fpsFrame/FPS_CAPTURE_FRAMES_COUNT;
		average += history[index];
	}

	return (int)roundf(1.0f/average);
}

// Get elapsed time measure in seconds since InitTimer()
// NOTE: On PLATFORM_DESKTOP InitTimer() is called on InitWindow()
// NOTE: On PLATFORM_DESKTOP, timer is initialized on glfwInit()
double GetTime(void)
{

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long long int time = (unsigned long long int)ts.tv_sec*1000000000LLU + (unsigned long long int)ts.tv_nsec;

    return (double)(time - orbisGlConf->base)*1e-9;  // Elapsed time since InitTimer()

}

// Returns hexadecimal value for a Color
int ColorToInt(Color color)
{
	return (((int)color.r << 24) | ((int)color.g << 16) | ((int)color.b << 8) | (int)color.a);
}

// Returns color normalized as float [0..1]
Vector4 ColorNormalize(Color color)
{
	Vector4 result;
	result.x = (float)color.r/255.0f;
	result.y = (float)color.g/255.0f;
	result.z = (float)color.b/255.0f;
	result.w = (float)color.a/255.0f;
	return result;
}

// Returns color from normalized values [0..1]
Color ColorFromNormalized(Vector4 normalized)
{
	Color result;
	result.r = normalized.x*255.0f;
	result.g = normalized.y*255.0f;
	result.b = normalized.z*255.0f;
	result.a = normalized.w*255.0f;
	return result;
}

// Returns HSV values for a Color
// NOTE: Hue is returned as degrees [0..360]
Vector3 ColorToHSV(Color color)
{
	Vector3 rgb = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
	Vector3 hsv = { 0.0f, 0.0f, 0.0f };
	float min, max, delta;
	min = rgb.x < rgb.y? rgb.x : rgb.y;
	min = min  < rgb.z? min  : rgb.z;
	max = rgb.x > rgb.y? rgb.x : rgb.y;
	max = max  > rgb.z? max  : rgb.z;
	hsv.z = max;            // Value
	delta = max - min;
	if (delta < 0.00001f)
	{
		hsv.y = 0.0f;
		hsv.x = 0.0f;       // Undefined, maybe NAN?
		return hsv;
	}
	if (max > 0.0f)
	{
		// NOTE: If max is 0, this divide would cause a crash
		hsv.y = (delta/max);    // Saturation
	}
	else
	{
		// NOTE: If max is 0, then r = g = b = 0, s = 0, h is undefined
		hsv.y = 0.0f;
		hsv.x = NAN;        // Undefined
		return hsv;
	}
	// NOTE: Comparing float values could not work properly
	if (rgb.x >= max) hsv.x = (rgb.y - rgb.z)/delta;    // Between yellow & magenta
	else
	{
		if (rgb.y >= max) hsv.x = 2.0f + (rgb.z - rgb.x)/delta;  // Between cyan & yellow
		else hsv.x = 4.0f + (rgb.x - rgb.y)/delta;      // Between magenta & cyan
	}
	hsv.x *= 60.0f;     // Convert to degrees
	if (hsv.x < 0.0f) hsv.x += 360.0f;
	return hsv;
}

// Returns a Color from HSV values
// Implementation reference: https://en.wikipedia.org/wiki/HSL_and_HSV#Alternative_HSV_conversion
// NOTE: Color->HSV->Color conversion will not yield exactly the same color due to rounding errors
Color ColorFromHSV(Vector3 hsv)
{
	Color color = { 0, 0, 0, 255 };
	float h = hsv.x, s = hsv.y, v = hsv.z;
	// Red channel
	float k = fmod((5.0f + h/60.0f), 6);
	float t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.r = (v - v*s*k)*255;
	// Green channel
	k = fmod((3.0f + h/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.g = (v - v*s*k)*255;
	// Blue channel
	k = fmod((1.0f + h/60.0f), 6);
	t = 4.0f - k;
	k = (t < k)? t : k;
	k = (k < 1)? k : 1;
	k = (k > 0)? k : 0;
	color.b = (v - v*s*k)*255;
	return color;
}

// Returns a Color struct from hexadecimal value
Color GetColor(int hexValue)
{
	Color color;
	color.r = (unsigned char)(hexValue >> 24) & 0xFF;
	color.g = (unsigned char)(hexValue >> 16) & 0xFF;
	color.b = (unsigned char)(hexValue >> 8) & 0xFF;
	color.a = (unsigned char)hexValue & 0xFF;
	return color;
}

// Returns a random value between min and max (both included)
int GetRandomValue(int min, int max)
{
	if (min > max)
	{
	    int tmp = max;
	    max = min;
	    min = tmp;
	}
	return (rand()%(abs(max - min) + 1) + min);
}

// Color fade-in or fade-out, alpha goes from 0.0f to 1.0f
Color Fade(Color color, float alpha)
{
	if (alpha < 0.0f) alpha = 0.0f;
	else if (alpha > 1.0f) alpha = 1.0f;
	return (Color){color.r, color.g, color.b, (unsigned char)(255.0f*alpha)};
}

// Get pointer to extension for a filename string
const char *GetExtension(const char *fileName)
{
    const char *dot = strrchr(fileName, '.');

    if (!dot || dot == fileName) return NULL;

    return (dot + 1);
}

// Check file extension
// NOTE: Extensions checking is not case-sensitive
bool IsFileExtension(const char *fileName, const char *ext)
{
    bool result = false;
    const char *fileExt = GetExtension(fileName);

    if (fileExt != NULL)
    {
        int extCount = 0;
        const char **checkExts = TextSplit(ext, ';', &extCount);

        char fileExtLower[16] = { 0 };
        strcpy(fileExtLower, TextToLower(fileExt));

        for (int i = 0; i < extCount; i++)
        {
            if (TextIsEqual(fileExtLower, TextToLower(checkExts[i] + 1)))
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

// Set viewport for a provided width and height
void SetupViewport(int width, int height)
{
	orbisGlConf->render.width = width;
	orbisGlConf->render.height = height;
	// Set viewport width and height
	// NOTE: We consider render size and offset in case black bars are required and
	// render area does not match full display area (this situation is only applicable on fullscreen mode)
	rlViewport(orbisGlConf->renderOffset.x/2, orbisGlConf->renderOffset.y/2, orbisGlConf->render.width - orbisGlConf->renderOffset.x, orbisGlConf->render.height - orbisGlConf->renderOffset.y);
	rlMatrixMode(RL_PROJECTION);		// Switch to PROJECTION matrix
	rlLoadIdentity();					// Reset current matrix (PROJECTION)
	// Set orthographic projection to current framebuffer size
	// NOTE: Configured top-left corner as (0, 0)
	rlOrtho(0, orbisGlConf->render.width, orbisGlConf->render.height, 0, 0.0f, 1.0f);
	rlMatrixMode(RL_MODELVIEW);		// Switch back to MODELVIEW matrix
	rlLoadIdentity();					// Reset current matrix (MODELVIEW)
}

// Compute framebuffer size relative to screen size and display size
// NOTE: Global variables CORE.Window.render.width/CORE.Window.render.height and CORE.Window.renderOffset.x/CORE.Window.renderOffset.y can be modified
void SetupFramebuffer(int width, int height)
{
	// Calculate CORE.Window.render.width and CORE.Window.render.height, we have the display size (input params) and the desired screen size (global var)
	if ((orbisGlConf->screen.width > orbisGlConf->display.width) || (orbisGlConf->screen.height > orbisGlConf->display.height))
	{
		debugNetPrintf(INFO, "[ORBISGL] %s DOWNSCALING: Required screen size (%ix%i) is bigger than display size (%ix%i)\n",__FUNCTION__, orbisGlConf->screen.width, orbisGlConf->screen.height, orbisGlConf->display.width, orbisGlConf->display.height);
		// Downscaling to fit display with border-bars
		float widthRatio = (float)orbisGlConf->display.width/(float)orbisGlConf->screen.width;
		float heightRatio = (float)orbisGlConf->display.height/(float)orbisGlConf->screen.height;
		if (widthRatio <= heightRatio)
		{
			orbisGlConf->render.width = orbisGlConf->display.width;
			orbisGlConf->render.height = (int)round((float)orbisGlConf->screen.height*widthRatio);
			orbisGlConf->renderOffset.x = 0;
			orbisGlConf->renderOffset.y = (orbisGlConf->display.height - orbisGlConf->render.height);
		}
		else
		{
			orbisGlConf->render.width = (int)round((float)orbisGlConf->screen.width*heightRatio);
			orbisGlConf->render.height = orbisGlConf->display.height;
			orbisGlConf->renderOffset.x = (orbisGlConf->display.width - orbisGlConf->render.width);
			orbisGlConf->renderOffset.y = 0;
		}
		// Screen scaling required
		float scaleRatio = (float)orbisGlConf->render.width/(float)orbisGlConf->screen.width;
		orbisGlConf->screenScale = MatrixScale(scaleRatio, scaleRatio, 1.0f);
		// NOTE: We render to full display resolution!
		// We just need to calculate above parameters for downscale matrix and offsets
		orbisGlConf->render.width = orbisGlConf->display.width;
		orbisGlConf->render.height = orbisGlConf->display.height;
		debugNetPrintf(INFO, "[ORBISGL] %s Downscale matrix generated, content will be rendered at: %i x %i\n",__FUNCTION__,orbisGlConf->render.width, orbisGlConf->render.height);
	}
	else if ((orbisGlConf->screen.width < orbisGlConf->display.width) || (orbisGlConf->screen.height < orbisGlConf->display.height))
	{
		// Required screen size is smaller than display size
		debugNetPrintf(INFO, "[ORBISGL] %s UPSCALING: Required screen size: %i x %i -> Display size: %i x %i\n",__FUNCTION__,orbisGlConf->screen.width, orbisGlConf->screen.height, orbisGlConf->display.width, orbisGlConf->display.height);
		// Upscaling to fit display with border-bars
		float displayRatio = (float)orbisGlConf->display.width/(float)orbisGlConf->display.height;
		float screenRatio = (float)orbisGlConf->screen.width/(float)orbisGlConf->screen.height;
		if (displayRatio <= screenRatio)
		{
		    orbisGlConf->render.width = orbisGlConf->screen.width;
		    orbisGlConf->render.height = (int)round((float)orbisGlConf->screen.width/displayRatio);
		    orbisGlConf->renderOffset.x = 0;
		    orbisGlConf->renderOffset.y = (orbisGlConf->render.height - orbisGlConf->screen.height);
		}
		else
		{
		    orbisGlConf->render.width = (int)round((float)orbisGlConf->screen.height*displayRatio);
		    orbisGlConf->render.height = orbisGlConf->screen.height;
		    orbisGlConf->renderOffset.x = (orbisGlConf->render.width - orbisGlConf->screen.width);
		    orbisGlConf->renderOffset.y = 0;
		}
	}
	else
	{
		orbisGlConf->render.width = orbisGlConf->screen.width;
		orbisGlConf->render.height = orbisGlConf->screen.height;
		orbisGlConf->renderOffset.x = 0;
		orbisGlConf->renderOffset.y = 0;
	}
}

// Initialize hi-resolution timer
void InitTimer(void)
{
    srand((unsigned int)time(NULL));              // Initialize random seed



    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)  // Success
    {
        orbisGlConf->base = (unsigned long long int)now.tv_sec*1000000000LLU + (unsigned long long int)now.tv_nsec;
    }
    else debugNetPrintf(INFO, "[DEBUG] %s No hi-resolution timer available\n",__FUNCTION__);

    orbisGlConf->previous = GetTime();       // Get time as double
}

// Wait for some milliseconds (stop program execution)
// NOTE: Sleep() granularity could be around 10 ms, it means, Sleep() could
// take longer than expected... for that reason we use the busy wait loop
// Ref: http://stackoverflow.com/questions/43057578/c-programming-win32-games-sleep-taking-longer-than-expected
// Ref: http://www.geisswerks.com/ryan/FAQS/timing.html --> All about timming on Win32!
void Wait(float ms)
{
    double prevTime = GetTime();
    double nextTime = 0.0;

    // Busy wait loop
    while ((nextTime - prevTime) < ms/1000.0f) nextTime = GetTime();
}

void SwapBuffers()
{
	orbisGlSwapBuffers();
}

int orbisGlCreateConf(unsigned int width,unsigned int height)
{
	if(!orbisGlConf)
	{
		orbisGlConf=(OrbisGlConfig *)malloc(sizeof(OrbisGlConfig));
		
		memset(&orbisGlConf->pgl_config, 0, sizeof(orbisGlConf->pgl_config));
		{
			orbisGlConf->pgl_config.size=sizeof(orbisGlConf->pgl_config);
			orbisGlConf->pgl_config.flags=SCE_PGL_FLAGS_USE_COMPOSITE_EXT | SCE_PGL_FLAGS_USE_FLEXIBLE_MEMORY | 0x60;
			orbisGlConf->pgl_config.processOrder=1;
			orbisGlConf->pgl_config.systemSharedMemorySize=0x200000;
			orbisGlConf->pgl_config.videoSharedMemorySize=0x2400000;
			orbisGlConf->pgl_config.maxMappedFlexibleMemory=0xAA00000;
			orbisGlConf->pgl_config.drawCommandBufferSize=0xC0000;
			orbisGlConf->pgl_config.lcueResourceBufferSize=0x10000;
			orbisGlConf->pgl_config.dbgPosCmd_0x40=ATTR_ORBISGL_WIDTH;
			orbisGlConf->pgl_config.dbgPosCmd_0x44=ATTR_ORBISGL_HEIGHT;
			orbisGlConf->pgl_config.dbgPosCmd_0x48=0;
			orbisGlConf->pgl_config.dbgPosCmd_0x4C=0;
			orbisGlConf->pgl_config.unk_0x5C=2;
		}
		orbisGlConf->device=EGL_NO_DISPLAY;
		orbisGlConf->surface=EGL_NO_SURFACE;
		orbisGlConf->context=EGL_NO_CONTEXT;
		orbisGlConf->config=NULL;
		orbisGlConf->width=width;
		orbisGlConf->height=height;
		orbisGlConf->screen.width=width;
		orbisGlConf->screen.height=height;
		orbisGlConf->display.width=width;
		orbisGlConf->display.height=height;
		orbisGlConf->position.x=orbisGlConf->display.width/2-orbisGlConf->screen.width/2;
		orbisGlConf->position.y=orbisGlConf->display.height/2-orbisGlConf->screen.height/2;
		orbisGlConf->screenScale=MatrixIdentity();
		SetupFramebuffer(orbisGlConf->display.width,orbisGlConf->display.height);
		orbisGlConf->orbisgl_initialized=-1;	
		return 0;
	}
	if(orbisGlConf->orbisgl_initialized==1)
	{
		return orbisGlConf->orbisgl_initialized;
	}
	//something weird happened
	return -1;			
}

void InitWindow(int width, int height, const char *title)
{
	int ret=orbisGlInit(width,height);
	if(ret>0)
	{
		LoadFontDefault();
    	Rectangle rec = GetFontDefault().recs[95];
    	// NOTE: We setup a 1px padding on char rectangle to avoid pixel bleeding on MSAA filtering
    	SetShapesTexture(GetFontDefault().texture, (Rectangle){ rec.x + 1, rec.y + 1, rec.width - 2, rec.height - 2 });
    	// Set default font texture filter for HighDPI (blurry)
    	SetTextureFilter(GetFontDefault().texture, FILTER_BILINEAR);
    }
    else
    {
    	debugNetPrintf(INFO,"[ORBISGL] orbisGlInit return error\n");
    }
}
void CloseWindow()
{
	UnloadFontDefault();
	orbisGlFinish();
}
// Load text data from file, returns a '\0' terminated string
// NOTE: text chars array should be freed manually
char *LoadFileText(const char *fileName)
{
    char *text = NULL;

    if (fileName != NULL)
    {
        int textFile = orbisNfsOpen(fileName,O_RDONLY,0);


        if (textFile>=0)
        {
            // WARNING: When reading a file as 'text' file,
            // text mode causes carriage return-linefeed translation...
            // ...but using fseek() should return correct byte-offset
            int size = orbisNfsLseek(textFile, 0, SEEK_END);
            orbisNfsLseek(textFile, 0, SEEK_SET);

            if (size > 0)
            {
                text = (char *)malloc(sizeof(char)*(size + 1));
                int count = orbisNfsRead(textFile, text, size);

                // WARNING: \r\n is converted to \n on reading, so,
                // read bytes count gets reduced by the number of lines
                if (count < size) text = realloc(text, count + 1);

                // Zero-terminate the string
                text[count] = '\0';

                debugNetPrintf(INFO, "[ORBISGL] %s FILEIO: [%s] Text file loaded successfully\n",__FUNCTION__,fileName);
            }
            else debugNetPrintf(ERROR, "[ORBISGL] %s FILEIO: [%s] Failed to read text file\n",__FUNCTION__,fileName);

            orbisNfsClose(textFile);
        }
        else debugNetPrintf(ERROR, "[ORBISGL] %s FILEIO: [%s] Failed to open text file\n",__FUNCTION__,fileName);
    }
    else debugNetPrintf(ERROR, "[ORBISGL] %s FILEIO: File name provided is not valid\n",__FUNCTION__);

    return text;
}

int orbisGlInit(unsigned int width,unsigned int height)
{
	int ret;
	int major,minor;
	
	SceWindow render_window={0,width,height};
	
	if(orbisGlCreateConf(width,height)==1)
	{
			return orbisGlConf->orbisgl_initialized;
	}
	if (orbisGlConf->orbisgl_initialized==1) 
	{
		debugNetPrintf(INFO,"[ORBISGL] liborbisGl is already initialized!\n");
		return orbisGlConf->orbisgl_initialized;
	}
	
	ret=scePigletSetConfigurationVSH(&orbisGlConf->pgl_config);
	if(!ret) 
	{
		debugNetPrintf(ERROR,"[ORBISGL] scePigletSetConfigurationVSH failed 0x%08X.\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] scePigletSetConfigurationVSH success.\n");
	
	orbisGlConf->device=eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(orbisGlConf->device==EGL_NO_DISPLAY) 
	{
		debugNetPrintf(ERROR,"[ORBISGL] eglGetDisplay failed.\n");
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglGetDisplay success.\n");
	
	ret=eglInitialize(orbisGlConf->device,&major,&minor);
	if(!ret)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglInitialize failed: 0x%08X\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] EGL version major:%d, minor:%d\n",major,minor);
	
	ret=eglBindAPI(EGL_OPENGL_ES_API);
	if(!ret)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglBindAPI failed: 0x%08X\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglBindAPI success.\n");
	
	ret=eglSwapInterval(orbisGlConf->device,0);
	if(!ret)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglSwapInterval failed: 0x%08X\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglSwapInterval success.\n");
	
	ret=eglChooseConfig(orbisGlConf->device,attribs,&orbisGlConf->config,1,&orbisGlConf->num_configs);
	if(!ret)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglChooseConfig failed: 0x%08X\n",ret);
		return -1;
	}
	if (orbisGlConf->num_configs!=1)
	{
		debugNetPrintf(ERROR,"[ORBISGL] No available configuration found.\n");
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglChooseConfig success.\n");
	

	orbisGlConf->surface=eglCreateWindowSurface(orbisGlConf->device,orbisGlConf->config,&render_window,window_attribs);
	if(orbisGlConf->surface==EGL_NO_SURFACE)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglCreateWindowSurface failed: 0x%08X\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglCreateWindowSurface success.\n");

	orbisGlConf->context=eglCreateContext(orbisGlConf->device,orbisGlConf->config,EGL_NO_CONTEXT,ctx_attribs);
	if(orbisGlConf->context==EGL_NO_CONTEXT)
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglCreateContext failed: 0x%08X\n", ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglCreateContext success.\n");

	ret=eglMakeCurrent(orbisGlConf->device,orbisGlConf->surface,orbisGlConf->surface,orbisGlConf->context);
	if(!ret) 
	{
		ret=eglGetError();
		debugNetPrintf(ERROR,"[ORBISGL] eglMakeCurrent failed: 0x%08X\n",ret);
		return -1;
	}
	debugNetPrintf(INFO,"[ORBISGL] eglMakeCurrent success.\n");


	//const char *gl_exts = (char *) glGetString(GL_EXTENSIONS);
	debugNetPrintf(INFO,"[ORBISGL] GL_VENDOR:   \"%s\"\n", glGetString(GL_VENDOR));
	debugNetPrintf(INFO,"[ORBISGL] GL_VERSION:  \"%s\"\n", glGetString(GL_VERSION));
	debugNetPrintf(INFO,"[ORBISGL] GL_RENDERER: \"%s\"\n", glGetString(GL_RENDERER));
	debugNetPrintf(INFO,"[ORBISGL] SL_VERSION:  \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	//debugNetPrintf(INFO,"[ORBISGL] GL_EXTs:     \"%s\"\n", gl_exts);
	
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
	debugNetPrintf(INFO, "[ORBISGL] Number of supported extensions: %i\n", numExt);
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
	if (RLGL.ExtSupported.vao) debugNetPrintf(INFO, "[ORBISGL] VAO extension detected, VAO functions initialized successfully\n");
	else debugNetPrintf(ERROR, "[ORBISGL] VAO extension not found, VAO usage not supported\n");
	if (RLGL.ExtSupported.texNPOT) debugNetPrintf(INFO, "[ORBISGL] NPOT textures extension detected, full NPOT textures supported\n");
	else debugNetPrintf(ERROR, "[ORBISGL] NPOT textures extension not found, limited NPOT support (no-mipmaps, no-repeat)\n");
	if (RLGL.ExtSupported.texCompDXT) debugNetPrintf(INFO, "[ORBISGL] DXT compressed textures supported\n");
	if (RLGL.ExtSupported.texCompETC1) debugNetPrintf(INFO, "[ORBISGL] ETC1 compressed textures supported\n");
	if (RLGL.ExtSupported.texCompETC2) debugNetPrintf(INFO, "[ORBISGL] ETC2/EAC compressed textures supported\n");
	if (RLGL.ExtSupported.texCompPVRT) debugNetPrintf(INFO, "[ORBISGL] PVRT compressed textures supported\n");
	if (RLGL.ExtSupported.texCompASTC) debugNetPrintf(INFO, "[ORBISGL] ASTC compressed textures supported\n");
	if (RLGL.ExtSupported.texAnisoFilter) debugNetPrintf(INFO, "[ORBISGL] Anisotropic textures filtering supported (max: %.0fX)\n", RLGL.ExtSupported.maxAnisotropicLevel);
	if (RLGL.ExtSupported.texMirrorClamp) debugNetPrintf(INFO, "[ORBISGL] Mirror clamp wrap texture mode supported\n");
	if (RLGL.ExtSupported.debugMarker) debugNetPrintf(INFO, "[ORBISGL] Debug Marker supported\n");
	// Initialize buffers, default shaders and default textures
	//----------------------------------------------------------
	// Init default white texture
	unsigned char pixels[4] = { 255, 255, 255, 255 };   // 1 pixel RGBA (4 bytes)
	RLGL.State.defaultTextureId = rlLoadTexture(pixels, 1, 1, UNCOMPRESSED_R8G8B8A8, 1);
	if (RLGL.State.defaultTextureId != 0) debugNetPrintf(INFO, "[ORBISGL] [TEX ID %i] Base white texture loaded successfully\n", RLGL.State.defaultTextureId);
	else debugNetPrintf(ERROR, "[ORBISGL] Base white texture could not be loaded\n");
	// Init default Shader (customized for GL 3.3 and ES2)
	RLGL.State.defaultShader = LoadShaderDefault();
	RLGL.State.currentShader = RLGL.State.defaultShader;
	//for debug static RLGL
	//checkshader(RLGL.State.defaultShader,2);
	//checkshader(RLGL.State.currentShader,3);
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
	#if defined(SUPPORT_DEFAULT_FONT)
	// Load default font
	// NOTE: External functions (defined in module: text)
	#endif
    // Set default font texture filter for HighDPI (blurry)
    //SetTextureFilter(GetFontDefault().texture, FILTER_BILINEAR);

	RLGL.State.shapesTexture = GetTextureDefault();
	RLGL.State.shapesTextureRec = (Rectangle){ 0.0f, 0.0f, 1.0f, 1.0f };
	debugNetPrintf(INFO, "[ORBISGL] default states initialized successfully\n");

	int fbWidth = orbisGlConf->render.width;
    int fbHeight = orbisGlConf->render.height;
    SetupViewport(fbWidth,fbHeight);
    orbisGlConf->currentFbo.width=orbisGlConf->screen.width;
    orbisGlConf->currentFbo.height=orbisGlConf->screen.height;

	// Init hi-res timer
	InitTimer();



	orbisGlConf->orbisgl_initialized=1;
	ClearBackground(RAYWHITE);
	debugNetPrintf(INFO,"[ORBISGL] liborbisGl2 initialized\n");
	return orbisGlConf->orbisgl_initialized;
}











