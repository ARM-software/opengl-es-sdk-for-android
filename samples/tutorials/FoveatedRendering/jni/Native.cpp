/* Copyright (c) 2017, ARM Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define FOVEATED
#define RATIO 0.3
#define MASK
#define RED

#ifdef FOVEATED
	#ifndef RATIO
		#define RATIO 0.3
	#endif

	#define VIEWS 4
#else
	#undef RATIO
	#define RATIO 1.0

	#ifdef MULTIVIEW
		#define VIEWS 2
	#else
		#define VIEWS 1
	#endif
#endif

#include "Utils.h"

#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>

#include "model3d.h"

#include "Matrix.h"

#define LOG_TAG "Foveated_Sample"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define GL_CHECK(x)                                                                              \
    x;                                                                                           \
    {                                                                                            \
        GLenum glError = glGetError();                                                           \
        if(glError != GL_NO_ERROR) {                                                             \
            LOGE("glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
            exit(1);                                                                             \
        }                                                                                        \
    }

using namespace MaliSDK;
GLuint fboWidth;
GLuint fboHeight;
GLuint screenWidth;
GLuint screenHeight;

// Switch to define the structure holding the main view framebuffers
// Regular contains two FB, for left and right eye while the others
// use a single one based on multiview
#ifdef REGULAR
GLuint frameBufferObjectId[2];
#else
GLuint frameBufferObjectId;
#endif

// Array of texture 2D containing left and right eye
#ifdef REGULAR
GLuint frameBufferTextureId[2];
GLuint frameBufferDepthTextureId[2];
#else
GLuint frameBufferTextureId;
GLuint frameBufferDepthTextureId;
#endif

GLuint multiviewProgram;
GLuint multiviewVertexLocation;
GLuint multiviewVertexNormalLocation;
GLuint multiviewVertexTangentLocation;
GLuint multiviewVertexUVLocation;
GLuint multiviewViewLocation;
GLuint multiviewModelViewLocation;
GLuint multiviewModelViewProjectionLocation;
GLuint multiviewProjectionLocation;
GLuint multiviewModelLocation;
GLuint multiviewTimeLocation;

GLuint textureIdDiffuse;
GLuint multiviewTextureDiffuse;

GLuint textureIdNormal;
GLuint multiviewTextureNormal;

GLuint textureIdMetallicRoughness;
GLuint multiviewTextureMetallicRoughness;

GLuint textureIdBump;
GLuint multiviewTextureBump;

GLuint texturedQuadProgram;
GLuint texturedQuadVertexLocation;
GLuint texturedQuadLowResTexCoordLocation;
GLuint texturedQuadHighResTexCoordLocation;
GLuint texturedQuadSamplerLocation;
GLuint texturedQuadLayerIndexLocation;
GLuint textureQuadFoveatedRatio;
GLuint textureQuadWidthHeight;

GLuint maskProgram;
GLuint maskVertexLocation;
GLuint maskTypeLocation;
GLuint masklInsetVertexArray;
GLuint maskOutsetVertexArray;

Matrix projectionMatrix[VIEWS];
Matrix viewMatrix[VIEWS];
Matrix viewProjectionMatrix[VIEWS];
Matrix modelViewMatrix[VIEWS];
Matrix modelViewProjectionMatrix[VIEWS];
Matrix modelMatrix;
float angle = 0;

const uint8_t circleStepConst = 16;

// The textures should be multiple of 2
const uint16_t TextureSize = 2048;

std::string assetFolder;
Model3D::Model3D room;

std::vector<GLushort> I_OutsetCircle;


#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102

typedef void( *PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR )(GLenum, GLenum, GLuint, GLint, GLint, GLsizei);
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR glFramebufferTextureMultiviewOVR;

// Define with multisampling multiview
typedef void( *PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR )(GLenum, GLenum, GLuint, GLint, GLsizei, GLint, GLsizei);
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR glFramebufferTextureMultisampleMultiviewOVR;

// Texture 2D Multisample
typedef void( *PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT )(GLenum, GLenum, GLenum, GLuint, GLint, GLsizei);
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT glFramebufferTexture2DMultisampleEXT;

typedef void( *PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT )(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT glRenderbufferStorageMultisampleEXT;

/* Textured quad fragmentShader */
static const char texturedQuadFragmentShader[] =
"#version 300 es\n"

"precision mediump float;\n"
"precision mediump int;\n"
"precision mediump sampler2DArray;\n"

"in vec2 vLowResTexCoord;\n"
"in vec2 vHighResTexCoord;\n"

"out vec4 fragColor;\n"

#ifdef REGULAR
"uniform sampler2D tex;\n"
#else
"uniform sampler2DArray tex;\n"
#endif

"uniform int layerIndex;\n"
"uniform vec2 width_height;\n"

"void main()\n"
"{\n"
#ifdef FOVEATED
"   vec2 distVec = (vec2(0.5) - vHighResTexCoord) / vec2(1, width_height.x/width_height.y);\n"
"   float squaredDist = dot(distVec, distVec);\n"

"   if( squaredDist > 0.25) { \n"
"      fragColor = textureLod(tex, vec3(vLowResTexCoord, layerIndex), 0.0);\n"
"   } \n"
"   else { \n"
"      fragColor = textureLod(tex, vec3(vHighResTexCoord, layerIndex + 2), 0.0);\n"
"   } \n"

#ifdef RED
"   if( squaredDist > 0.23 && squaredDist < 0.27) { \n"
"      fragColor += vec4(0.2,0.0,0.0,1.0);\n"
"   } \n"
#endif
#else
#ifdef REGULAR
"   vec4 lowResSample = texture(tex, vLowResTexCoord);\n"
#else
"   vec4 lowResSample = texture(tex, vec3(vLowResTexCoord, layerIndex));\n"
#endif
"   fragColor = lowResSample;\n"
#endif
"}\n";

/* Textured quad geometry */
float texturedQuadCoordinates[] =
{
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f
};

/* Textured quad low resolution texture coordinates */
float texturedQuadLowResTexCoordinates[] =
{
	0, 0,
	1, 0,
	1, 1,

	0, 0,
	1, 1,
	0, 1
};

/* Textured quad high resolution texture coordinates */
float texturedQuadHighResTexCoordinates[] =
{
	0, 0,
	1, 0,
	1, 1,

	0, 0,
	1, 1,
	0, 1
};

void generateDepthCircleVBO( const uint8_t circleStep, const float radius )
{
	// Generate the VAO for inset and outset masks
	glGenVertexArrays( 1, &masklInsetVertexArray );
	glGenVertexArrays( 1, &maskOutsetVertexArray );

	// Create the increment for the shape
	float increment = (2 * M_PI) / (circleStep);			// Increment for the circle

	GLfloat *VX_InsetCircle = new GLfloat[circleStep * 3];		// Inset mask
	GLfloat *VX_OutsetCircle = new GLfloat[(circleStep + 4) * 3];	// Outset mask

	static const GLfloat plane[] = {		// Plane around outset
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		-1.0f,  -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};

	memcpy( VX_OutsetCircle, plane, sizeof( plane ) );	// Copy the plane to the outset array

	// Create the coordinates data
	double loopStep = 0;
	for ( int j = 0; j < circleStep; ++j )
	{
		// Draw the inset mask
		// The inset mask is dependent on the radius of the fovea region
		// A small ratio (0.9) is applied in order to prevent uncovered area between the two masks
		/*
				X
			   XXX
			 XXXXXXX
			XXXXXXXXX
			 XXXXXXX
			   XXX
				X
		 */
		VX_InsetCircle[(j * 3) + 0] = cos( loopStep ) * radius*0.9;
		VX_InsetCircle[(j * 3) + 1] = sin( loopStep ) * radius*0.9;
		VX_InsetCircle[(j * 3) + 2] = 0.0f;

		// Draw the outset mask
		// The outset mask does not depend on the radius of fovea as it is only here to cover the corners
		// A small ratio (1.1) is applied in order to prevent uncovered area between the two masks
		/*
			XXXXX XXXXX
			XXX     XXX
			XX       XX
			X         X
			XX       XX
			XXX     XXX
			XXXXX XXXXX
		 */
		VX_OutsetCircle[((4 + j) * 3) + 0] = cos( loopStep ) * (1.1);
		VX_OutsetCircle[((4 + j) * 3) + 1] = sin( loopStep ) * (1.1);
		VX_OutsetCircle[((4 + j) * 3) + 2] = 0.0f;

		loopStep += increment;
	}

	// Upload data to the VAO
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, masklInsetVertexArray ) );
	GL_CHECK( glBufferData( GL_ARRAY_BUFFER, circleStep * 3 * sizeof( GLfloat ), VX_InsetCircle, GL_STATIC_DRAW ) );
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, maskOutsetVertexArray ) );
	GL_CHECK( glBufferData( GL_ARRAY_BUFFER, (circleStep + 4) * 3 * sizeof( GLfloat ), VX_OutsetCircle, GL_STATIC_DRAW ) );


	// Create the index list for the outset mask
	for ( int i = 0; i < 4; ++i )
	{
		for ( int j = 0; j < circleStep / 4; j++ )
		{
			I_OutsetCircle.push_back( i );
			I_OutsetCircle.push_back( (i*(circleStep / 4)) + j + 4 );
			int temp = (i*(circleStep / 4)) + j + 4 + 1;

			I_OutsetCircle.push_back( (temp == 4 + circleStep ? 4 : temp) );
		}
		I_OutsetCircle.push_back( i );
		if ( i + 1 == 4 )
		{
			I_OutsetCircle.push_back( 4 );
			I_OutsetCircle.push_back( 0 );
		}
		else
		{
			I_OutsetCircle.push_back( 4 + ((i + 1)*(circleStep / 4)) );
			I_OutsetCircle.push_back( i + 1 );
		}
	}

	delete[] VX_InsetCircle;
	delete[] VX_OutsetCircle;
}

GLuint loadShader( GLenum shaderType, const char* shaderSource, const int* length = NULL )
{
	GLuint shader = GL_CHECK( glCreateShader( shaderType ) );
	if ( shader != 0 )
	{
		GL_CHECK( glShaderSource( shader, 1, &shaderSource, length ) );
		GL_CHECK( glCompileShader( shader ) );
		GLint compiled = 0;
		GL_CHECK( glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled ) );
		if ( compiled != GL_TRUE )
		{
			GLint infoLen = 0;
			GL_CHECK( glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &infoLen ) );

			if ( infoLen > 0 )
			{
				char * logBuffer = (char*) malloc( infoLen );

				if ( logBuffer != NULL )
				{
					GL_CHECK( glGetShaderInfoLog( shader, infoLen, NULL, logBuffer ) );
					LOGE( "Could not Compile Shader %d:\n%s\n", shaderType, logBuffer );
					free( logBuffer );
					logBuffer = NULL;
				}

				GL_CHECK( glDeleteShader( shader ) );
				shader = 0;
			}
		}
	}

	return shader;
}

char* loadShaderFromFile( const std::string &filename, int* shaderLength )
{
	// Load the shader file from the sdcard
	char* shaderCode;

	Model3D::load_file( filename, &shaderCode, *shaderLength, false );

	// Load the shader
	return shaderCode;
}

GLuint createProgram( const char* vertexSource, const char * fragmentSource, const int* vertexLength = NULL, const int* fragmentLength = NULL )
{
	GLuint vertexShader = loadShader( GL_VERTEX_SHADER, vertexSource, vertexLength );
	if ( vertexShader == 0 )
	{
		return 0;
	}

	GLuint fragmentShader = loadShader( GL_FRAGMENT_SHADER, fragmentSource, fragmentLength );
	if ( fragmentShader == 0 )
	{
		return 0;
	}

	GLuint program = GL_CHECK( glCreateProgram() );

	if ( program != 0 )
	{
		GL_CHECK( glAttachShader( program, vertexShader ) );
		GL_CHECK( glAttachShader( program, fragmentShader ) );
		GL_CHECK( glLinkProgram( program ) );
		GLint linkStatus = GL_FALSE;
		GL_CHECK( glGetProgramiv( program, GL_LINK_STATUS, &linkStatus ) );
		if ( linkStatus != GL_TRUE )
		{
			GLint bufLength = 0;
			GL_CHECK( glGetProgramiv( program, GL_INFO_LOG_LENGTH, &bufLength ) );
			if ( bufLength > 0 )
			{
				char* logBuffer = (char*) malloc( bufLength );

				if ( logBuffer != NULL )
				{
					GL_CHECK( glGetProgramInfoLog( program, bufLength, NULL, logBuffer ) );
					LOGE( "Could not link program:\n%s\n", logBuffer );
					free( logBuffer );
					logBuffer = NULL;
				}
			}
			GL_CHECK( glDeleteProgram( program ) );
			program = 0;
		}
	}
	return program;
}

bool createTexture( std::string filename, GLuint *textureLocation )
{
	// Load the image in a buffer
	char* pixelData;
	int byteRead;

	Model3D::load_file( filename, &pixelData, byteRead, false );

	// Create and put the texture in memory
	GL_CHECK( glGenTextures( 1, textureLocation ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D, *textureLocation ) );
	GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
	GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );

	// Texture used here are power of two sized, check the data alignment if you want to replace them
	GL_CHECK( glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, TextureSize, TextureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, (unsigned char*) pixelData ) );

	// Destroy the image;
	delete[] pixelData;

	return true;
}

bool setupFBO( int width, int height )
{
	#ifdef REGULAR
	for ( uint8_t i = 0; i < 2; ++i )
	{
		// Generate colour texture
		GL_CHECK( glGenTextures( 1, &frameBufferTextureId[i] ) );
		GL_CHECK( glBindTexture( GL_TEXTURE_2D, frameBufferTextureId[i] ) );
		GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		GL_CHECK( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
		GL_CHECK( glTexStorage2D( GL_TEXTURE_2D, 1, GL_RGBA8, width, height ) );

		// Generate depth texture

		GL_CHECK( glGenRenderbuffers( 1, &frameBufferDepthTextureId[i] ) );
		GL_CHECK( glBindRenderbuffer( GL_RENDERBUFFER, frameBufferDepthTextureId[i] ) );
		GL_CHECK( glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, width, height ) );

		GL_CHECK( glGenFramebuffers( 1, &frameBufferObjectId[i] ) );
		GL_CHECK( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBufferObjectId[i] ) );

		GL_CHECK( glFramebufferTexture2DMultisampleEXT( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTextureId[i], 0, 4 ) );
		GL_CHECK( glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBufferDepthTextureId[i] ) );

		GLenum result = GL_CHECK( glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
		if ( result != GL_FRAMEBUFFER_COMPLETE )
		{
			LOGE( "Framebuffer incomplete at %s:%i %i\n", __FILE__, __LINE__, (int) result );
			/* Unbind framebuffer. */
			GL_CHECK( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
			return false;
		}
	}
	#else

	// Generate colour texture
	GL_CHECK( glGenTextures( 1, &frameBufferTextureId ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D_ARRAY, frameBufferTextureId ) );
	GL_CHECK( glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
	GL_CHECK( glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
	GL_CHECK( glTexStorage3D( GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, VIEWS ) );

	// Generate depth texture
	GL_CHECK( glGenTextures( 1, &frameBufferDepthTextureId ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D_ARRAY, frameBufferDepthTextureId ) );
	GL_CHECK( glTexStorage3D( GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, VIEWS ) );


	GL_CHECK( glGenFramebuffers( 1, &frameBufferObjectId ) );
	GL_CHECK( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, frameBufferObjectId ) );

	GL_CHECK( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frameBufferTextureId, 0, 4, 0, VIEWS ) );
	GL_CHECK( glFramebufferTextureMultisampleMultiviewOVR( GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frameBufferDepthTextureId, 0, 4, 0, VIEWS ) );

	GLenum result = GL_CHECK( glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
	if ( result != GL_FRAMEBUFFER_COMPLETE )
	{
		LOGE( "Framebuffer incomplete at %s:%i\n", __FILE__, __LINE__ );
		/* Unbind framebuffer. */
		GL_CHECK( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
		return false;
	}
	#endif

	return true;
}

bool setupGraphics( int width, int height )
{
	#ifdef FOVEATED
	#ifdef MASK
	LOGI( "Running with masked foveated rendering." );
	#else
	LOGI( "Running with foveated rendering." );
	#endif
	#elif defined(MULTIVIEW)
	LOGI( "Running with multiview." );
	#else
	LOGI( "Running with regular stereo." );
	#endif

	int vertexShaderLength;
	int fragmentShaderLength;

	glFramebufferTexture2DMultisampleEXT =
		(PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT) eglGetProcAddress( "glFramebufferTexture2DMultisampleEXT" );
	if ( !glFramebufferTexture2DMultisampleEXT )
	{
		LOGI( "Cannot get proc address for glFramebufferTexture2DMultisampleEXT.\n" );
		exit( EXIT_FAILURE );
	}

	glRenderbufferStorageMultisampleEXT =
		(PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT) eglGetProcAddress( "glRenderbufferStorageMultisampleEXT" );
	if ( !glRenderbufferStorageMultisampleEXT )
	{
		LOGI( "Cannot get proc address for glRenderbufferStorageMultisampleEXT.\n" );
		exit( EXIT_FAILURE );
	}

	// Make sure the prerequisites are met
	const GLubyte* extensions = GL_CHECK( glGetString( GL_EXTENSIONS ) );
	const char * found_extension = strstr( (const char*) extensions, "GL_OVR_multiview" );
	if ( NULL == found_extension )
	{
		LOGI( "OpenGL ES 3.0 implementation does not support GL_OVR_multiview extension.\n" );
		exit( EXIT_FAILURE );
	}
	else
	{
		glFramebufferTextureMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR) eglGetProcAddress( "glFramebufferTextureMultiviewOVR" );
		if ( !glFramebufferTextureMultiviewOVR )
		{
			LOGI( "Cannot get proc address for glFramebufferTextureMultiviewOVR.\n" );
			exit( EXIT_FAILURE );
		}

		glFramebufferTextureMultisampleMultiviewOVR =
			(PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR) eglGetProcAddress( "glFramebufferTextureMultisampleMultiviewOVR" );
		if ( !glFramebufferTextureMultisampleMultiviewOVR )
		{
			LOGI( "Cannot get proc address for glFramebufferTextureMultisampleMultiviewOVR.\n" );
			exit( EXIT_FAILURE );
		}
	}

	GL_CHECK( glDisable( GL_CULL_FACE ) );
	GL_CHECK( glEnable( GL_DEPTH_TEST ) );
	GL_CHECK( glDepthFunc( GL_LEQUAL ) );

	/* Setting screen width and height for use when rendering. */
	screenWidth = width;
	screenHeight = height;

	// Set fbo size based on screen width/height
	fboWidth = (screenWidth / 2) * RATIO;
	fboHeight = screenHeight * RATIO;

	LOGI( "Resolution is %u-%u on %u-%u screen resolution", fboWidth, fboHeight, screenWidth, screenHeight );

	if ( !setupFBO( fboWidth, fboHeight ) )
	{
		LOGE( "Could not create multiview FBO" );
		return false;
	}

	/* Creating program for drawing textured quad. */
	texturedQuadProgram = createProgram( loadShaderFromFile( assetFolder + "multiviewPlane.vs", &vertexShaderLength ), texturedQuadFragmentShader, &vertexShaderLength );
	if ( texturedQuadProgram == 0 )
	{
		LOGE( "Could not create textured quad program" );
		return false;
	}

	/* Get attribute and uniform locations for textured quad program. */
	texturedQuadVertexLocation = GL_CHECK( glGetAttribLocation( texturedQuadProgram, "attributePosition" ) );
	texturedQuadLowResTexCoordLocation = GL_CHECK( glGetAttribLocation( texturedQuadProgram, "attributeLowResTexCoord" ) );
	texturedQuadHighResTexCoordLocation = GL_CHECK( glGetAttribLocation( texturedQuadProgram, "attributeHighResTexCoord" ) );
	texturedQuadSamplerLocation = GL_CHECK( glGetUniformLocation( texturedQuadProgram, "tex" ) );
	texturedQuadLayerIndexLocation = GL_CHECK( glGetUniformLocation( texturedQuadProgram, "layerIndex" ) );
	textureQuadFoveatedRatio = GL_CHECK( glGetUniformLocation( texturedQuadProgram, "foveatedRatio" ) );
	textureQuadWidthHeight = GL_CHECK( glGetUniformLocation( texturedQuadProgram, "width_height" ) );

	/* Creating program for drawing object with multiview. */
	#ifdef FOVEATED
	std::string roomVertexShader = assetFolder + "roomFoveated.vs";
	#elif defined(MULTIVIEW)
	std::string roomVertexShader = assetFolder + "roomMultiview.vs";
	#else
	std::string roomVertexShader = assetFolder + "roomRegular.vs";
	#endif // FOVEATED

	multiviewProgram = createProgram( loadShaderFromFile( roomVertexShader, &vertexShaderLength ), loadShaderFromFile( assetFolder + "roomPBR.fs", &fragmentShaderLength ), &vertexShaderLength, &fragmentShaderLength );
	if ( multiviewProgram == 0 )
	{
		LOGE( "Could not create multiview program" );
		return false;
	}

	/* Get attribute and uniform locations for room program. */
	multiviewVertexLocation = GL_CHECK( glGetAttribLocation( multiviewProgram, "vertexPosition" ) );
	multiviewVertexNormalLocation = GL_CHECK( glGetAttribLocation( multiviewProgram, "vertexNormal" ) );
	multiviewVertexTangentLocation = GL_CHECK( glGetAttribLocation( multiviewProgram, "vertexTangent" ) );
	multiviewVertexUVLocation = GL_CHECK( glGetAttribLocation( multiviewProgram, "uvCoordinates" ) );

	multiviewTextureDiffuse = GL_CHECK( glGetUniformLocation( multiviewProgram, "TexDiffuse" ) );
	multiviewTextureNormal = GL_CHECK( glGetUniformLocation( multiviewProgram, "TexNormal" ) );
	multiviewTextureMetallicRoughness = GL_CHECK( glGetUniformLocation( multiviewProgram, "TexMetallicRoughness" ) );
	multiviewTextureBump = GL_CHECK( glGetUniformLocation( multiviewProgram, "TexBump" ) );

	multiviewViewLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "View" ) );
	multiviewProjectionLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "Projection" ) );
	multiviewModelLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "Model" ) );
	multiviewModelViewLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "ModelView" ) );
	multiviewModelViewProjectionLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "ModelViewProjection" ) );
	multiviewTimeLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "Time" ) );

	//multiviewTimeLocation = GL_CHECK( glGetUniformLocation( multiviewProgram, "time" ) );

	#if defined FOVEATED && defined MASK
	maskProgram = createProgram( loadShaderFromFile( assetFolder + "mask.vs", &vertexShaderLength ), loadShaderFromFile( assetFolder + "mask.fs", &fragmentShaderLength ), &vertexShaderLength, &fragmentShaderLength );
	if ( maskProgram == 0 )
	{
		LOGE( "Could not create mask program %i", maskProgram );
		return false;
	}
	maskVertexLocation = GL_CHECK( glGetAttribLocation( maskProgram, "vertexPosition" ) );
	maskTypeLocation = GL_CHECK( glGetUniformLocation( maskProgram, "maskType" ) );

	generateDepthCircleVBO( circleStepConst, RATIO );
	#endif

	/*
	 * Set up the perspective matrices for each view. Rendering is done twice in each eye position with different
	 * field of view. The narrower field of view should give half the size for the near plane in order to
	 * render the center of the scene at a higher resolution. The resulting high resolution and low resolution
	 * images will later be interpolated to create an image with higher resolution in the center of the screen
	 * than on the outer parts of the screen.
	 * 1.5707963268 rad = 90 degrees.
	 * 0.9272952188 rad = 53.1301024 degrees. This angle gives half the size for the near plane.
	 */
	const float FOV = M_PI_2;
	double insetFOV = std::atan( std::tan( FOV / 2 ) * RATIO ) * 2;

	projectionMatrix[0] = Matrix::matrixPerspective( FOV, (float) (width/2) / (float) height, 0.1f, 1000.0f );
	projectionMatrix[1] = Matrix::matrixPerspective( FOV, (float) (width / 2) / (float) height, 0.1f, 1000.0f );
	#ifdef FOVEATED
	projectionMatrix[2] = Matrix::matrixPerspective( insetFOV, (float) (width / 2) / (float) height, 0.1f, 1000.0f );
	projectionMatrix[3] = Matrix::matrixPerspective( insetFOV, (float) (width / 2) / (float) height, 0.1f, 1000.0f );
	#endif

	/* Setting up model view matrices for each of the */
	Vec3f leftCameraPos = { -0.5f, 0.0f, 5.0f };
	Vec3f rightCameraPos = { 0.5f, 0.0f, 5.0f };
	Vec3f lookAt = { 0.0f, 0.0f, 0.0f };
	Vec3f upVec = { 0.0f, 1.0f, 0.0f };
	viewMatrix[0] = Matrix::matrixCameraLookAt( leftCameraPos, lookAt, upVec );
	viewMatrix[1] = Matrix::matrixCameraLookAt( rightCameraPos, lookAt, upVec );
	#ifdef FOVEATED
	viewMatrix[2] = Matrix::matrixCameraLookAt( leftCameraPos, lookAt, upVec );
	viewMatrix[3] = Matrix::matrixCameraLookAt( rightCameraPos, lookAt, upVec );
	#endif

	if ( !room.load( assetFolder + "room.geom" ) )
	{
		LOGE( "Cannot find room geom asset.\n" );
		exit( EXIT_FAILURE );
	}
	LOGI( "Asset Loaded" );

	// Create and load textures
	createTexture( assetFolder + "T_Exterior_D.raw", &textureIdDiffuse );
	createTexture( assetFolder + "T_Exterior_N.raw", &textureIdNormal );
	createTexture( assetFolder + "T_Exterior_M.raw", &textureIdMetallicRoughness );
	createTexture( assetFolder + "T_Exterior_B.raw", &textureIdBump );

	LOGI( "Texture Loaded" );
	return true;
}

void renderToFBO( const int width, const int height, const GLuint frameBufferID )
{
	/* Rendering to FBO. */
	GL_CHECK( glViewport( 0, 0, width, height ) );

	/* Bind our framebuffer for rendering. */
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, frameBufferID ) );

	GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT ) );

	#if defined FOVEATED && defined MASK
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	// Render the mask
	GL_CHECK( glUseProgram( maskProgram ) );
	GL_CHECK( glEnableVertexAttribArray( maskVertexLocation ) );
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, masklInsetVertexArray ) );
	GL_CHECK( glVertexAttribPointer( maskVertexLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 ) );
	GL_CHECK( glUniform1ui( maskTypeLocation, 1 ) );
	GL_CHECK( glDrawArrays( GL_TRIANGLE_FAN, 0, circleStepConst ) );
	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, maskOutsetVertexArray ) );
	GL_CHECK( glVertexAttribPointer( maskVertexLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0 ) );
	GL_CHECK( glUniform1ui( maskTypeLocation, 2 ) );
	GL_CHECK( glDrawElements( GL_TRIANGLES, I_OutsetCircle.size(), GL_UNSIGNED_SHORT, &I_OutsetCircle[0] ) );
	GL_CHECK( glDisableVertexAttribArray( maskVertexLocation ) );

	GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	#endif

	// Upload diffuse texture
	GL_CHECK( glActiveTexture( GL_TEXTURE1 ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D, textureIdDiffuse ) );
	GL_CHECK( glActiveTexture( GL_TEXTURE2 ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D, textureIdNormal ) );
	GL_CHECK( glActiveTexture( GL_TEXTURE3 ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D, textureIdMetallicRoughness ) );
	GL_CHECK( glActiveTexture( GL_TEXTURE4 ) );
	GL_CHECK( glBindTexture( GL_TEXTURE_2D, textureIdBump ) );

	modelMatrix = Matrix::createTranslation( 0.0, 30.0, -45.0 ) * Matrix::createScaling( 0.2, 0.2, 0.2 ) * Matrix::createRotationX( -90.0 ) * Matrix::createRotationZ( 90.0 );
	
	for ( int i = 0; i < VIEWS; ++i )
	{
		modelViewMatrix[i] = viewMatrix[i] * modelMatrix;
		modelViewProjectionMatrix[i] = projectionMatrix[i] * modelViewMatrix[i];
	}

	GL_CHECK( glUseProgram( multiviewProgram ) );

	GL_CHECK( glUniformMatrix4fv( multiviewViewLocation, VIEWS, GL_FALSE, viewMatrix[0].getAsArray() ) );
	GL_CHECK( glUniformMatrix4fv( multiviewProjectionLocation, VIEWS, GL_FALSE, projectionMatrix[0].getAsArray() ) );
	GL_CHECK( glUniformMatrix4fv( multiviewModelViewLocation, VIEWS, GL_FALSE, modelViewMatrix[0].getAsArray() ) );
	GL_CHECK( glUniformMatrix4fv( multiviewModelViewProjectionLocation, VIEWS, GL_FALSE, modelViewProjectionMatrix[0].getAsArray() ) );

	// Upload vertex location
	GL_CHECK( glVertexAttribPointer( multiviewVertexLocation, 3, GL_FLOAT, GL_FALSE, 0, room.get_positions() ) );
	GL_CHECK( glEnableVertexAttribArray( multiviewVertexLocation ) );

	// Upload vertex normal
	GL_CHECK( glVertexAttribPointer( multiviewVertexNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, room.get_normals() ) );
	GL_CHECK( glEnableVertexAttribArray( multiviewVertexNormalLocation ) );

	// Upload vertex UV
	GL_CHECK( glVertexAttribPointer( multiviewVertexUVLocation, 3, GL_FLOAT, GL_FALSE, 0, room.get_texture_coordinates0() ) );
	GL_CHECK( glEnableVertexAttribArray( multiviewVertexUVLocation ) );

	// Upload vertex tangent
	GL_CHECK( glVertexAttribPointer( multiviewVertexTangentLocation, 3, GL_FLOAT, GL_FALSE, 0, room.get_tangents() ) );
	GL_CHECK( glEnableVertexAttribArray( multiviewVertexTangentLocation ) );

	/* Upload model view projection matrices. */
	GL_CHECK( glUniformMatrix4fv( multiviewModelLocation, 1, GL_FALSE, modelMatrix.getAsArray() ) );

	GL_CHECK( glUniform1i( multiviewTextureDiffuse, 1 ) );
	GL_CHECK( glUniform1i( multiviewTextureNormal, 2 ) );
	GL_CHECK( glUniform1i( multiviewTextureMetallicRoughness, 3 ) );
	GL_CHECK( glUniform1i( multiviewTextureBump, 4 ) );

	// Upload time
	GL_CHECK( glUniform1f( multiviewTimeLocation, Stats::totalNbFrame ) );


	/* Draw the room. */
	GL_CHECK( glDrawElements( GL_TRIANGLES, room.get_indices_count() * 3, GL_UNSIGNED_INT, room.get_indices() ) );

	// Invalidate the depth buffer
	GLenum invalidateList[] = { GL_DEPTH_ATTACHMENT };
	GL_CHECK( glInvalidateFramebuffer( GL_FRAMEBUFFER, 1, invalidateList ) );

	/* Go back to the backbuffer for rendering to the screen. */
	GL_CHECK( glBindFramebuffer( GL_FRAMEBUFFER, 0 ) );
}

void renderFrame()
{
	// Start the render frame timer
	Stats::StartFrame();


	#ifdef REGULAR
	for ( int i = 0; i < 2; ++i )
	{
		renderToFBO( fboWidth, fboHeight, frameBufferObjectId[i] );

	}
	#else
	/*
	* Render the scene to the multiview texture. This will render to 4 different layers of the texture,
	* using different projection and view matrices for each layer.
	*/
	renderToFBO( fboWidth, fboHeight, frameBufferObjectId );
	#endif


	GL_CHECK( glClearColor( 0.0f, 0.0f, 0.0f, 1.0f ) );
	GL_CHECK( glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT ) );

	/*
	 * Render the multiview texture layers to separate viewports. Each viewport corresponds to one eye,
	 * and will use two different texture layers from the multiview texture, one with a wide field of view
	 * and one with a narrow field of view.
	 */
	for ( int i = 0; i < 2; i++ )
	{
		glViewport( i * screenWidth / 2, 0, screenWidth / 2, screenHeight );

		/* Use the texture array that was drawn to using multiview. */
		GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );
		#ifdef REGULAR
		GL_CHECK( glBindTexture( GL_TEXTURE_2D, frameBufferTextureId[i] ) );
		#else
		GL_CHECK( glBindTexture( GL_TEXTURE_2D_ARRAY, frameBufferTextureId ) );
		#endif

		GL_CHECK( glUseProgram( texturedQuadProgram ) );

		/* Upload vertex attributes. */
		GL_CHECK( glVertexAttribPointer( texturedQuadVertexLocation, 3, GL_FLOAT, GL_FALSE, 0, texturedQuadCoordinates ) );
		GL_CHECK( glEnableVertexAttribArray( texturedQuadVertexLocation ) );
		GL_CHECK( glVertexAttribPointer( texturedQuadLowResTexCoordLocation, 2, GL_FLOAT,
										 GL_FALSE, 0, texturedQuadLowResTexCoordinates ) );
		GL_CHECK( glEnableVertexAttribArray( texturedQuadLowResTexCoordLocation ) );

		#ifdef FOVEATED
		GL_CHECK( glVertexAttribPointer( texturedQuadHighResTexCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, texturedQuadHighResTexCoordinates ) );
		GL_CHECK( glEnableVertexAttribArray( texturedQuadHighResTexCoordLocation ) );
		#endif

		/*
		 * Upload uniforms. The layerIndex is used to choose what layer of the array texture to sample from.
		 * The shader will use the given layerIndex and layerIndex + 2, where layerIndex gives the layer with
		 * the wide field of view, where the entire scene has been rendered, and layerIndex + 2 gives the layer
		 * with the narrow field of view, where only the center of the scene has been rendered.
		 */
		GL_CHECK( glUniform1i( texturedQuadSamplerLocation, 0 ) );
		GL_CHECK( glUniform1i( texturedQuadLayerIndexLocation, i ) );
		GL_CHECK( glUniform1f( textureQuadFoveatedRatio, RATIO ) );
		GL_CHECK( glUniform2f( textureQuadWidthHeight, screenWidth / 2.0, screenHeight ) );

		/* Draw textured quad using the multiview texture. */
		GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, 6 ) );

		Stats::EndFrame();

		if ( Stats::nbFrame >= 60 )
		{
			LOGI( "Current FPS: %f", Stats::GetFPS() );
			Stats::Clear();
		}
	}
}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_foveatedrendering_NativeLibrary_init(
			JNIEnv * env, jobject obj, jint width, jint height, jstring localPath );
	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_foveatedrendering_NativeLibrary_step(
			JNIEnv * env, jobject obj );
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_foveatedrendering_NativeLibrary_init(
		JNIEnv * env, jobject obj, jint width, jint height, jstring localPath )
{
	assetFolder = "/data/data/com.arm.malideveloper.openglessdk.foveatedrendering/files/";

	setupGraphics( width, height );
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_foveatedrendering_NativeLibrary_step(
		JNIEnv * env, jobject obj )
{
	renderFrame();
}
