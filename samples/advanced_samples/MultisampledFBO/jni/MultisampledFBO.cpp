/* Copyright (c) 2012-2017, ARM Limited and Contributors
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

/**
 * \file MultisampledFBO.cpp
 * \brief A sample which shows how to use multisampled frame buffer objects with render-to-texture.
 *
 * This demo draws a side-by-side comparison of a non-antialiased framebuffer object, and another FBO
 * with multisampling enabled. The Utah Teapot is used an example model to illustrate how edges are
 * affected by antialiasing.
 *
 * Additionally the demo shows how to check for float and half-float support, and setup
 * framebuffer objects using these formats.
 *
 * The device is queried for the following extensions, and the results logged:
 *    GL_EXT_multisampled_render_to_texture - for actually rendering to texture with MSAA
 *    GL_EXT_color_buffer_half_float - for GL_RGBA16F texture support
 *    GL_EXT_color_buffer_float - for GL_RGBA32F texture support
 *    OES_texture_half_float_linear - for linear filtering on GL_RGBA16F textures
 *    OES_texture_float_linear - for linear filtering on GL_RGBA32F textures
 *
 * To resolve the multisampled FBOs and present them to the user, we render them to textured quads,
 * instead of using glBlitFramebuffer(). This is because with our tile-based rendering, using
 * glBlitFrameBuffer() would incur additional memory copying overheads and would be suboptimal.
 * Rendering textured quads is faster and also allows us the flexibility to do shader-based post-
 * processing as well as being able to use a custom projection.
 *
 * The demo initially uses the highest supported texture format with the maximum number of samples.
 * The user is able to interact with the demo and change the settings as follows:
 *    Tap on screen: toggle rotation animation
 *    Long-press on screen: cycle through different colors
 *    Pinch-to-zoom gesture: change distance of model
 *    Drag on screen: rotate model
 *    Volume up: switch multisampling level
 *    Volume down: switch texture resolution
 *    Long-press on volume up: switch texture format
 *    Long press on volume down: toggle texture filtering on/off
 */

#include <GLES3/gl3.h>

#ifdef __ANDROID__
/**
 * For Android, we are using a more recent local version of gl2ext.h from Khronos instead of NDK headers to get
 * GL_EXT_color_buffer_half_float and GL_EXT_color_buffer_float, which aren't added to Android NDK until SDK21.
 * We could build using SDK21, but then this code would not run on devices with API <21.
 *
 * See: http://stackoverflow.com/questions/28890570/using-gl-ext-color-buffer-half-float-with-android-ndk
 * 		http://stackoverflow.com/questions/27338318/cannot-load-library-reloc-library1285-cannot-locate-rand
 */
#include "inc/gl2ext.h"
#else
#include <GLES2/gl2ext.h>
#endif

#include <EGL/egl.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <math.h>

#include <jni.h>
#include <android/log.h>

#include "MultisampledFBO.h"
#include "Teapot.h"

#include "Text.h"
#include "AndroidPlatform.h"
#include "Shader.h"
#include "Matrix.h"

/* OpenGL ES extension functions. */
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC glFramebufferTexture2DMultisampleEXT = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC glRenderbufferStorageMultisampleEXT = NULL;

using std::string;
using std::ostringstream;
using namespace MaliSDK;

/* Asset directories and filenames. */
const string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.multisampledfbo/";
const string teapotVertexShaderFilename = "MultisampledFBO_teapot.vert";
const string teapotFragmentShaderFilename = "MultisampledFBO_teapot.frag";
string quadVertexShaderFilename = "MultisampledFBO_quad.vert";
string quadFragmentShaderFilename = "MultisampledFBO_quad.frag";

/* Some colors and contrasting backgrounds. */
typedef struct
{
	GLfloat r;
	GLfloat g;
	GLfloat b;
} Color;

const Color colors[] =
{
		{ 0.0706f, 0.5490f, 0.6706f }, 	/* ARM Teal*/
		{ 0.9882f, 0.9961f, 0.0157f }, 	/* Yellow */
		{ 0.9882f, 0.6039f, 0.0157f }, 	/* Amber */
		{ 0.9882f, 0.4000f, 0.0196f }, 	/* Orange */
		{ 0.9882f, 0.1961f, 0.0157f }, 	/* Dark Orange */
		{ 0.8000f, 0.0078f, 0.0157f }, 	/* Red */
		{ 0.6118f, 0.0078f, 0.3922f }, 	/* Magenta */
};

const Color backgrounds[] =
{
		{ 0.0000f, 0.0000f, 0.0000f }, 	/* Black */
		{ 0.3922f, 0.0200f, 0.3922f }, 	/* Purple */
		{ 0.0156f, 0.0118f, 0.3922f }, 	/* Dark Blue */
		{ 0.0157f, 0.1961f, 0.6118f }, 	/* Blue */
		{ 0.0157f, 0.4000f, 0.3922f }, 	/* Teal */
		{ 0.2039f, 0.6039f, 0.0157f }, 	/* Green */
		{ 0.3922f, 0.8078f, 0.0157f }, 	/* Light Green */
};

/* Texture format enums and strings. */
const GLenum textureFormats[] = { GL_RGBA4, GL_RGBA8, GL_RGBA16F, GL_RGBA32F };
const GLchar* const textureFormatStrings[] = { "GL_RGBA4", "GL_RGBA8", "GL_RGBA16F", "GL_RGBA32F" };

/* Texture format supported by device. */
GLboolean textureFormatSupported[] = { false, false, false, false };

/* Max levels of multisampling for individual texture formats. */
GLint textureFormatSamples[] = { 0, 0, 0, 0 };

/* FBO texture sizes. */
GLuint textureSizes[] = { 32, 64, 128, 256, 512, 1024 };

/* Max level of multisampling for all texture formats. */
GLint maxSamples = 0;

/* Flags for whether we can use linear filtering on float/half-float textures. */
GLboolean canFilterHalfFloat;
GLboolean canFilterFloat;

GLuint currentColor = 0;
GLint currentSamples = 0;
GLuint currentTextureFormat = 0;
GLuint currentTextureSize = 5;
GLboolean linearFilteringEnabled = true;

/* Shader variables. */
GLuint teapotProgramID;
GLuint quadProgramID;

GLint iLocTeapotPosition = -1;
GLint iLocTeapotNormal = -1;
GLint iLocTeapotColor = -1;
GLint iLocTeapotMVMatrix = -1;
GLint iLocTeapotMVPMatrix = -1;
GLint iLocTeapotNormalMatrix = -1;

GLint iLocQuadPosition = -1;
GLint iLocQuadTexCoord = -1;
GLint iLocQuadMVPMatrix = -1;

/* Framebuffers and textures. */
GLuint frameBufferNoAA;
GLuint frameBufferMSAA;

GLuint texColorBufferNoAA;
GLuint texColorBufferMSAA;

GLuint depthBufferNoAA;
GLuint depthBufferMSAA;

/* Teapot dimensions */
GLfloat minX = 0.0f;
GLfloat minY = 0.0f;
GLfloat minZ = 0.0f;
GLfloat maxX = 0.0f;
GLfloat maxY = 0.0f;
GLfloat maxZ = 0.0f;
GLfloat maxXYZ = 0.0f;

GLfloat rotationDegree1 = 0.0f;
GLfloat rotationDegree2 = 0.0f;
GLuint counter = 0;

GLint windowWidth = -1;
GLint windowHeight = -1;

/* Flag to enable animation. */
GLboolean animate = true;

/* Scale factor for pinch-to-zoom. */
GLfloat pinchZoomScaleFactor = 1.0f;

/* Rotation offsets for drag gestures. */
GLfloat dragRotateX = 0.0f;
GLfloat dragRotateY = 0.0f;

/* Text object to draw text on the screen. */
Text* text;

bool extensionAvailable(const char* extName)
{
	string extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	return extensions.find(extName) != string::npos;
}

void findTeapotDimensions()
{
	minX = maxX = teapotPositions[0];
	minY = maxY = teapotPositions[1];
	minZ = maxZ = teapotPositions[2];
	for (unsigned int i = 3; i < sizeof teapotPositions / sizeof (GLfloat); i+=3)
	{
		if (teapotPositions[i] > maxX) maxX = teapotPositions[i];
		if (teapotPositions[i] < minX) minX = teapotPositions[i];
		if (teapotPositions[i+1] > maxY) maxY = teapotPositions[i+1];
		if (teapotPositions[i+1] < minY) minY = teapotPositions[i+1];
		if (teapotPositions[i+2] > maxZ) maxZ = teapotPositions[i+2];
		if (teapotPositions[i+2] < minZ) minZ = teapotPositions[i+2];
	}

	maxXYZ = fmaxf(maxX, fmaxf(maxY, maxZ));
}

void setTextureFiltering(GLuint* texture)
{
	/* If the new texture format doesn't support linear filtering, disable it. */
	GLboolean invalidFilteringMode =    (textureFormats[currentTextureFormat] == GL_RGBA16F && !canFilterHalfFloat)
									 || (textureFormats[currentTextureFormat] == GL_RGBA32F && !canFilterFloat);
	if (linearFilteringEnabled && invalidFilteringMode)
	{
		linearFilteringEnabled = false;
		return;
	}
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, *texture));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linearFilteringEnabled ? GL_LINEAR : GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linearFilteringEnabled ? GL_LINEAR : GL_NEAREST));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

bool setupFBO(GLuint* fbo, GLuint* colorBuffer, GLuint* depthBuffer, GLuint textureSize, GLenum internalFormat, GLuint samples)
{
	LOGD("Creating a %dx%d %smultisampled FBO (%d samples).", textureSize, textureSize, samples ? "" : "non-", samples);

	/* Generate framebuffer object. */
	GL_CHECK(glDeleteFramebuffers(1, fbo));
	GL_CHECK(glGenFramebuffers(1, fbo));
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, *fbo));

	/* Generate and attach texture for color buffer. */
	GL_CHECK(glDeleteTextures(1, colorBuffer));
	GL_CHECK(glGenTextures(1, colorBuffer));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, *colorBuffer));
	GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, textureSize, textureSize));
	GL_CHECK(glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorBuffer, 0, samples));

	setTextureFiltering(colorBuffer);
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

	/* Generate and attach depth buffer. */
	GL_CHECK(glDeleteRenderbuffers(1, depthBuffer))
	GL_CHECK(glGenRenderbuffers(1, depthBuffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, *depthBuffer));
	GL_CHECK(glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT16, textureSize, textureSize));
	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *depthBuffer));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	/* Ensure the framebuffer is 'complete'. */
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LOGE("%s framebuffer is incomplete! Error code %d", (samples ? "Multisampled" : "Non-multisampled"), glGetError());
		return false;
	}

	return true;
}

void setupText()
{
	ostringstream maxSamplesStringStream;
	ostringstream texFormatStringStream;
	ostringstream descriptionStringStream;

	maxSamplesStringStream << "Max number of samples (all texture formats): " << maxSamples;
	texFormatStringStream << "Using texture format: " << textureFormatStrings[currentTextureFormat]
													  << " (" << textureSizes[currentTextureSize] << "x"
													  << textureSizes[currentTextureSize] << ", "
													  << currentSamples << " samples, "
													  << (linearFilteringEnabled ? "GL_LINEAR" : "GL_NEAREST") << " filtering)";
	descriptionStringStream << "Left: No anti-aliasing. Right: Multisampled anti-aliasing (" << currentSamples << " samples)";

	text->clear();

	text->addString(0, windowHeight - text->textureCharacterHeight, maxSamplesStringStream.str().c_str(), 255, 255, 255, 255);
	text->addString(0, windowHeight - text->textureCharacterHeight * 2, texFormatStringStream.str().c_str(), 255, 255, 255, 255);
	text->addString(0, windowHeight - text->textureCharacterHeight * 3, "Tap to screen to toggle animation. Long-press to cycle colors. Pinch-to-zoom, drag to rotate.", 0, 255, 255, 255);
	text->addString(0, windowHeight - text->textureCharacterHeight * 4, "Volume up: switch multisampling level. Volume down: switch texture resolution.", 0, 255, 255, 255);
	text->addString(0, windowHeight - text->textureCharacterHeight * 5, "Long press vol up: switch texture format. Long press vol down: toggle texture filtering", 0, 255, 255, 255);

	text->addString(0, text->textureCharacterHeight, descriptionStringStream.str().c_str(), 255, 255, 0, 255);
	text->addString(0, 0, "Multisampled framebuffer objects.", 0, 255, 255, 255);
}

bool setupGraphics(int width, int height)
{
	/* Initialize OpenGL ES. */
	LOGD("setupGraphics(%d, %d)", width, height);

    /* Check we have the extensions we need. */
	LOGD("Available extensions:");
	LOGD("GL_EXT_multisampled_render_to_texture = %s", extensionAvailable("GL_EXT_multisampled_render_to_texture") ? "YES" : "NO");
	LOGD("GL_EXT_color_buffer_half_float = %s", extensionAvailable("GL_EXT_color_buffer_half_float") ? "YES" : "NO");
	LOGD("GL_EXT_color_buffer_float = %s", extensionAvailable("GL_EXT_color_buffer_float") ? "YES" : "NO");

	canFilterHalfFloat = extensionAvailable("OES_texture_half_float_linear");
	canFilterFloat = extensionAvailable("OES_texture_float_linear");
	LOGD("OES_texture_half_float_linear = %s", canFilterHalfFloat ? "YES" : "NO");
	LOGD("OES_texture_float_linear = %s", canFilterFloat ? "YES" : "NO");

	/* Query maximum number of samples for all formats. */
	LOGD("Max samples for internal texture formats:");
	GL_CHECK(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));
	LOGD("All (MAX_SAMPLES) = %d", maxSamples);

	/* Query maximum number of samples for various texture formats. */
	for (unsigned int i = 0; i < sizeof textureFormats / sizeof (GLenum); i++)
	{
		/* Try to get the number of samples. If there was an error (likely GL_INVALID_ENUM), then assume unsupported. */
		glGetInternalformativ(GL_RENDERBUFFER, textureFormats[i], GL_SAMPLES, 1, &textureFormatSamples[i]);
		textureFormatSupported[i] = !glGetError();
		if (textureFormatSupported[i])
		{
			LOGD("%s = %d", textureFormatStrings[i], textureFormatSamples[i]);
		}
		else
		{
			LOGD("%s = %s", textureFormatStrings[i], "Not supported");
		}
	}

	/* Initialize multisampling extension function pointers. */
	glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC) eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");
	if (!glFramebufferTexture2DMultisampleEXT)
	{
		LOGE("Couldn't get function pointer to glFramebufferTexture2DMultisampleEXT()!");
		return false;
	}

	glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC) eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
	if (!glRenderbufferStorageMultisampleEXT)
	{
		LOGE("Couldn't get function pointer to glRenderbufferStorageMultisampleEXT()!");
		return false;
	}

	/* Save window dimensions for calculating aspect ratios. */
	windowWidth = width;
	windowHeight = height;

	/* Full paths to the shader files */
	string teapotVertexShaderPath = resourceDirectory + teapotVertexShaderFilename;
	string teapotFragmentShaderPath = resourceDirectory + teapotFragmentShaderFilename;
	string quadVertexShaderPath = resourceDirectory + quadVertexShaderFilename;
	string quadFragmentShaderPath = resourceDirectory + quadFragmentShaderFilename;

	/* Handles to shaders. */
	GLuint teapotVertexShaderID = 0;
	GLuint teapotFragmentShaderID = 0;
	GLuint quadVertexShaderID = 0;
	GLuint quadFragmentShaderID = 0;

	/* Enable the depth buffer (must remember to clear it on each redraw) */
	GL_CHECK(glEnable(GL_DEPTH_TEST));

	/* Use less-than or equal for depth testing */
	GL_CHECK(glDepthFunc(GL_LESS));

	/* Find the best texture format supporting the greatest number of samples. */
	GLint bestSamples = 0;
	GLint bestTextureFormat = 0;
	for (unsigned int i = 0; i < sizeof textureFormats / sizeof (GLuint); i++)
	{
		if (textureFormatSamples[i] >= bestSamples)
		{
			bestSamples = textureFormatSamples[i];
			bestTextureFormat = i;
		}
	}

	currentSamples = bestSamples;
	currentTextureFormat = bestTextureFormat;

	/* Generate FBOs and render buffers for non-antialiased and multisampled rendering. */
	if (!setupFBO(&frameBufferNoAA, &texColorBufferNoAA, &depthBufferNoAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], 0)) return false;
	if (!setupFBO(&frameBufferMSAA, &texColorBufferMSAA, &depthBufferMSAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], currentSamples)) return false;

	/* Initialize the Text object and add some text. */
	text = new Text(resourceDirectory.c_str(), width, height);

	/* Update screen text. */
	setupText();

	/* Process shaders. */
	Shader::processShader(&teapotVertexShaderID, teapotVertexShaderPath.c_str(), GL_VERTEX_SHADER);
	Shader::processShader(&teapotFragmentShaderID, teapotFragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
	Shader::processShader(&quadVertexShaderID, quadVertexShaderPath.c_str(), GL_VERTEX_SHADER);
	Shader::processShader(&quadFragmentShaderID, quadFragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

	LOGD("teapotVertexShaderID = %d", teapotVertexShaderID);
	LOGD("teapotFragmentShaderID = %d", teapotFragmentShaderID);
	LOGD("quadVertexShaderID = %d", quadVertexShaderID);
	LOGD("quadFragmentShaderID = %d", quadFragmentShaderID);

	/* Initialise shader programs. */
	teapotProgramID = GL_CHECK(glCreateProgram());
	quadProgramID = GL_CHECK(glCreateProgram());
	if (teapotProgramID == 0 || quadProgramID == 0)
	{
		LOGE("Could not create program.");
		return false;
	}

	GL_CHECK(glAttachShader(teapotProgramID, teapotVertexShaderID));
	GL_CHECK(glAttachShader(teapotProgramID, teapotFragmentShaderID));
	GL_CHECK(glLinkProgram(teapotProgramID));

	GL_CHECK(glAttachShader(quadProgramID, quadVertexShaderID));
	GL_CHECK(glAttachShader(quadProgramID, quadFragmentShaderID));
	GL_CHECK(glLinkProgram(quadProgramID));

	/* Cleanup individual shaders as they're no longer needed. */
	GL_CHECK(glDeleteShader(teapotVertexShaderID));
	GL_CHECK(glDeleteShader(teapotFragmentShaderID));
	GL_CHECK(glDeleteShader(quadVertexShaderID));
	GL_CHECK(glDeleteShader(quadFragmentShaderID));

	/* Get handles to teapot shader variables. */
	GL_CHECK(iLocTeapotPosition = glGetAttribLocation(teapotProgramID, "a_v3Position"));
	GL_CHECK(iLocTeapotNormal = glGetAttribLocation(teapotProgramID, "a_v3Normal"));
	GL_CHECK(iLocTeapotColor = glGetUniformLocation(teapotProgramID, "u_v3Color"));
	GL_CHECK(iLocTeapotMVMatrix = glGetUniformLocation(teapotProgramID, "u_m4MV"));
	GL_CHECK(iLocTeapotMVPMatrix = glGetUniformLocation(teapotProgramID, "u_m4MVP"));
	GL_CHECK(iLocTeapotNormalMatrix = glGetUniformLocation(teapotProgramID, "u_m4Normal"));

	LOGD("glGetAttribLocation(\"a_v3Position\") = %d", iLocTeapotPosition);
	LOGD("glGetAttribLocation(\"a_v3Normal\") = %d", iLocTeapotNormal);
	LOGD("glGetAttribLocation(\"u_v3Color\") = %d", iLocTeapotColor);
	LOGD("glGetUniformLocation(\"u_m4MV\") = %d", iLocTeapotMVMatrix);
	LOGD("glGetUniformLocation(\"u_m4MVP\") = %d", iLocTeapotMVPMatrix);
	LOGD("glGetUniformLocation(\"u_m4Normal\") = %d", iLocTeapotNormalMatrix);

	/* Get handles to quad shader variables. */
	GL_CHECK(iLocQuadPosition = glGetAttribLocation(quadProgramID, "a_v3Position"));
	GL_CHECK(iLocQuadTexCoord = glGetAttribLocation(quadProgramID, "a_v2TexCoord"));
	GL_CHECK(iLocQuadMVPMatrix = glGetUniformLocation(quadProgramID, "u_m4MVP"));

	LOGD("glGetAttribLocation(\"a_v3Position\") = %d", iLocQuadPosition);
	LOGD("glGetAttribLocation(\"a_v2TexCoord\") = %d", iLocQuadTexCoord);
	LOGD("glGetUniformLocation(\"u_m4MVP\") = %d", iLocQuadMVPMatrix);

	/* Set clear screen color (dark grey). */
	GL_CHECK(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
	GL_CHECK(glClearDepthf(1.0f));

	/* Find min/max dimensions of teapot. */
	findTeapotDimensions();

	return true;
}

void drawTeapot()
{
	Matrix modelViewMatrix;
	Matrix projectionMatrix;
	Matrix mvpMatrix;
	Matrix normalMatrix;

	/* Set clear screen color and clear the framebuffer. */
	Color bg = backgrounds[currentColor];
	GL_CHECK(glClearColor(bg.r, bg.g, bg.b, 1.0f));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	/* Set viewport to size of FBO texture. */
	GL_CHECK(glViewport(0, 0, textureSizes[currentTextureSize], textureSizes[currentTextureSize]));

	/*
	 *  Use max dimensions of the model to center it.
	 *  We find the centre of the teapot by averaging the min and max Y coordinates,
	 *  then push it down on the Y axis by this value before rotating so that the
	 *  teapot spins about its centre.
	 */
	GLfloat centerY = (maxY + minY) / 2.0f;

	/* Calculate model, view, and projection matrices. */
	modelViewMatrix = Matrix::identityMatrix;
	modelViewMatrix = Matrix::createTranslation(0.0f, -centerY, 0.0f) * modelViewMatrix;

	modelViewMatrix = Matrix::createRotationX(rotationDegree1) * modelViewMatrix;
	modelViewMatrix = Matrix::createRotationZ(rotationDegree2) * modelViewMatrix;
	modelViewMatrix = Matrix::createRotationY(dragRotateX) * modelViewMatrix;
	modelViewMatrix = Matrix::createRotationX(dragRotateY) * modelViewMatrix;
	modelViewMatrix = Matrix::createTranslation(0.0f, 0.0f, -2.5f + pinchZoomScaleFactor) * modelViewMatrix;
	modelViewMatrix = Matrix::createTranslation(0.0f, 0.0f, sinf(rotationDegree2 * M_PI/180.0f) / 2.0f) * modelViewMatrix;

	/* Change degree counters if animating. */
	if (animate)
	{
		rotationDegree1 += 1.0f;
		rotationDegree2 += 0.3f;
		if (rotationDegree1 > 360.0f) rotationDegree1 -= 360.0f;
		if (rotationDegree2 > 360.0f) rotationDegree2 -= 360.0f;
	}

	/* Calculate the projection matrix. */
	projectionMatrix = Matrix::matrixPerspective(45.0f, 1.0f, 0.1f, 1000.0f);
	mvpMatrix = projectionMatrix * modelViewMatrix;

	/* Calculate the normal matrix. */
	normalMatrix = Matrix::matrixInvert(&modelViewMatrix);
	Matrix::matrixTranspose(&normalMatrix);

	/* Send matrices to the shaders. */
	GL_CHECK(glUniformMatrix4fv(iLocTeapotMVMatrix, 1, GL_FALSE, modelViewMatrix.getAsArray()));
	GL_CHECK(glUniformMatrix4fv(iLocTeapotMVPMatrix, 1, GL_FALSE, mvpMatrix.getAsArray()));
	GL_CHECK(glUniformMatrix4fv(iLocTeapotNormalMatrix, 1, GL_FALSE, normalMatrix.getAsArray()));

	/* Send current color to the shaders. */
	Color color = colors[currentColor];
	GL_CHECK(glUniform3f(iLocTeapotColor, color.r, color.g, color.b));

	/* Set up vertex attributes. */
	GL_CHECK(glVertexAttribPointer(iLocTeapotPosition, 3, GL_FLOAT, GL_FALSE, 0, teapotPositions));
	GL_CHECK(glVertexAttribPointer(iLocTeapotNormal, 3, GL_FLOAT, GL_FALSE, 0, teapotNormals));

	GL_CHECK(glEnableVertexAttribArray(iLocTeapotPosition));
	GL_CHECK(glEnableVertexAttribArray(iLocTeapotNormal));

	/* Draw the teapot geometry. */
	GL_CHECK(glDrawElements(GL_TRIANGLES, sizeof(teapotIndices) / sizeof (GLuint), GL_UNSIGNED_INT, teapotIndices));

	GL_CHECK(glDisableVertexAttribArray(iLocTeapotPosition));
	GL_CHECK(glDisableVertexAttribArray(iLocTeapotNormal));
}

/* Code to draw one frame. */
void renderFrame(void)
{
	/**************************************************************************/
	/* DRAW TEAPOT                                                            */
	/**************************************************************************/

	/* Switch to the teapot shader program. */
	GL_CHECK(glUseProgram(teapotProgramID));

	/* Switch to non-antialiased FBO */
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferNoAA));
	drawTeapot();

	/* Switch to multisampled FBO */
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, frameBufferMSAA));
	drawTeapot();

	/**************************************************************************/
	/* DRAW QUADS                                                             */
	/**************************************************************************/

	/* Switch to the quad shader program. */
	GL_CHECK(glUseProgram(quadProgramID));

	/* Switch back to default framebuffer. */
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_CHECK(glViewport (0, 0, windowWidth, windowHeight));

	/* Enable attributes for position and texture coordinates. */
	GL_CHECK(glEnableVertexAttribArray(iLocQuadPosition));
	GL_CHECK(glEnableVertexAttribArray(iLocQuadTexCoord));

	/* Populate attributes for position, color and texture coordinates etc. */
	GL_CHECK(glVertexAttribPointer(iLocQuadPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), quadVertices));
	GL_CHECK(glVertexAttribPointer(iLocQuadTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), quadVertices + 3));

	/* Calculate and pass matrix to the shader. */
	Matrix modelViewMatrix = Matrix::identityMatrix;

	/* Pull the camera back. */
	modelViewMatrix[14] -= 1.0f;

	float aspectRatio =  windowWidth / (float) windowHeight;
	Matrix projectionMatrix = Matrix::matrixPerspective(45.0f, aspectRatio, 0.1f, 1000.0f);
	Matrix mvpMatrix = projectionMatrix * modelViewMatrix;

	GL_CHECK(glUniformMatrix4fv(iLocQuadMVPMatrix, 1, GL_FALSE, mvpMatrix.getAsArray()));

	/* Clear the screen and draw the elements. */
	GL_CHECK(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	/* Enable texture. */
	GL_CHECK(glActiveTexture(GL_TEXTURE0));

	/* Select NoAA color buffer to use as a texture and draw left quad. */
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, texColorBufferNoAA));
	GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, vertexIndices));

	/* Select MSAA color buffer to use as a texture and draw right quad. */
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, texColorBufferMSAA));
	GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, vertexIndices + 6));

	/* Draw fonts - blending is required for text). */
	GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	text->draw();
	GL_CHECK(glDisable(GL_BLEND));
}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_init
	(JNIEnv *env, jclass jcls, jint width, jint height)
	{
		/* Make sure that all resource files are in place. */
		AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), teapotVertexShaderFilename.c_str());
		AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), teapotFragmentShaderFilename.c_str());
		AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), quadVertexShaderFilename.c_str());
		AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), quadFragmentShaderFilename.c_str());

		setupGraphics(width, height);
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_step
	(JNIEnv *env, jclass jcls)
	{
		renderFrame();
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_toggleAnim
	(JNIEnv *env, jclass jcls)
	{
		/* Toggle teapot animation. */
		animate = !animate;
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_switchColor
	(JNIEnv *env, jclass jcls)
	{
		/* Switch colors */
		currentColor++;
		currentColor = currentColor % (sizeof colors / sizeof (Color));
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_switchSamples
	(JNIEnv *env, jclass jcls)
	{
		/* Switch to next level of multisampling. */
		if (currentSamples == textureFormatSamples[currentTextureFormat])
			currentSamples = 0;
		else if (currentSamples == 0)
			currentSamples = 2;
		else
			currentSamples *= 2;

		setupFBO(&frameBufferNoAA, &texColorBufferNoAA, &depthBufferNoAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], 0);
		setupFBO(&frameBufferMSAA, &texColorBufferMSAA, &depthBufferMSAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], currentSamples);
		setupText();
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_switchTextureFormat
	(JNIEnv *env, jclass jcls)
	{
		/* Switch to next supported texture format. */
		do
		{
			currentTextureFormat++;
			currentTextureFormat = currentTextureFormat % (sizeof textureFormats / sizeof (GLenum));
		}
		while (!textureFormatSupported[currentTextureFormat]);

		/* If the new texture format doesn't support the current sampling level, use its max. */
		currentSamples = std::min(currentSamples, textureFormatSamples[currentTextureFormat]);

		/* The new texture format may not support filtering, so reset it. */
		setTextureFiltering(&texColorBufferNoAA);
		setTextureFiltering(&texColorBufferMSAA);
		setupFBO(&frameBufferNoAA, &texColorBufferNoAA, &depthBufferNoAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], 0);
		setupFBO(&frameBufferMSAA, &texColorBufferMSAA, &depthBufferMSAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], currentSamples);
		setupText();
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_switchTextureSize
	(JNIEnv *env, jclass jcls)
	{
		/* Switch to next texture size. */
		currentTextureSize++;
		currentTextureSize = currentTextureSize % (sizeof textureSizes / sizeof (GLuint));

		setupFBO(&frameBufferNoAA, &texColorBufferNoAA, &depthBufferNoAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], 0);
		setupFBO(&frameBufferMSAA, &texColorBufferMSAA, &depthBufferMSAA, textureSizes[currentTextureSize], textureFormats[currentTextureFormat], currentSamples);
		setupText();
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_toggleTextureFiltering
	(JNIEnv *env, jclass jcls)
	{
		/* Toggle texture filtering. */
		linearFilteringEnabled = !linearFilteringEnabled;
		setTextureFiltering(&texColorBufferNoAA);
		setTextureFiltering(&texColorBufferMSAA);
		setupText();
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_setScaleFactor
	(JNIEnv *env, jclass jcls, jfloat scaleFactor)
	{
		/* Update scale factor. */
		pinchZoomScaleFactor = scaleFactor;
		LOGD("Scale factor now %.2f", pinchZoomScaleFactor);
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_setDragRotation
	(JNIEnv *env, jclass jcls, jfloat rotationX, jfloat rotationY)
	{
		animate = false;

		/* Update drag rotation. */
		dragRotateX += rotationX / 2.0f;
		dragRotateY += rotationY / 2.0f;
	}

	JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_multisampledfbo_MultisampledFBO_uninit
	(JNIEnv *, jclass)
	{
		delete text;

		/* Clean up FBOs. */
		GL_CHECK(glDeleteFramebuffers(1, &frameBufferNoAA));
		GL_CHECK(glDeleteFramebuffers(1, &frameBufferMSAA));
	}
}
