/* Copyright (c) 2013-2017, ARM Limited and Contributors
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
 * \brief A sample which illustrates the use of sync objects  to synchronise
 *  the use of shared objects between multiple contexts in multiple threads.
 *
 * A cube is rendered in the main application thread using a texture which
 * is updated in a second thread. Each thread has its own rendering context
 * and both contexts share their EGL objects. Sync objects are used to
 * synchronise the access to shared data.
 */

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

/* Fencing state. */
static bool useFence = true;

#include <string>
#include <jni.h>
#include <android/log.h>

#include "ThreadSync.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#include "Text.h"
#include "Shader.h"
#include "Matrix.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.threadsync/";
string vertexShaderFilename = "ThreadSync_cube.vert";
string fragmentShaderFilename = "ThreadSync_cube.frag";

/* Shader variables. */
GLuint vertexShaderID = 0;
GLuint fragmentShaderID = 0;
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocTextureMix = -1;
GLint iLocTexture = -1;
GLint iLocFillColor = -1;
GLint iLocTexCoord = -1;
GLint iLocProjection = -1;
GLint iLocModelview = -1;

/* Matrix transformation variables. */
static float angleX = 0;
static float angleY = 45;
static float angleZ = 45;
Matrix rotationX;
Matrix rotationY;
Matrix rotationZ;
Matrix translation;
Matrix modelView;
Matrix projection;

/* Application textures. */
GLuint iCubeTex = 0;

int windowWidth = -1;
int windowHeight = -1;

/* A text object to draw text on the screen. */
Text* text;
static string baseString = "Thread Synchronisation Example. ";
static string textString = "";

/* Context related variables. */
EGLContext mainContext = NULL;
EGLContext mainDisplay = NULL;
EGLSurface pBufferSurface = NULL;
EGLContext pBufferContext = NULL;

/* Secondary thread related variables. */
pthread_t secondThread;
bool exitThread = false;

/* Sync objects. */
GLsync secondThreadSyncObj = NULL;
GLsync mainThreadSyncObj = NULL;
GLuint64 timeout = GL_TIMEOUT_IGNORED;

/* Texture generation. */
unsigned char *textureData = NULL;
int swapStripes = -1;
int texWidth = 512;
int texHeight = 512;

unsigned char bakedColours[] =
{
    255,    0,      0,    255,
      0,  255,    0,    255,
    255,  255,    0,    255,
      0,  255,    255,    255,
};

/* User interaction. */
static bool touchStarted = false;

void touchStart(int x, int y)
{
    touchStarted = true;
    /* Empty. */
}

void touchMove (int x, int y)
{
    /* Empty. */
}

void touchEnd(int x, int y)
{
    if(touchStarted)
    {
        useFence = !useFence;
        touchStarted = false;

        text->clear();
        if(useFence)
        {
            textString = baseString + "Fencing enabled.";
            LOGI("Changed fencing from disabled to enabled.");
        }
        else
        {
            textString = baseString + "Fencing disabled.";
            LOGI("Changed fencing from enabled to disabled.")
        }
        text->addString(0, 0, textString.c_str(), 255, 255, 0, 255);
    }
}

/* Modify the texture. */
void animateTexture()
{
    static int col = 0;
    static int col1 = 1;
    static int col2= 2;
    static int col3= 3;

    float r1 = texHeight / 16;
    float r2 = texHeight / 8;
    float r3 = texHeight / 4;
    float r4 = texHeight / 2;
    float r5 = texHeight / 1;

    float r12 = r1 * r1;
    float r22 = r2 * r2;
    float r32 = r3 * r3;
    float r42 = r4 * r4;
    float r52 = r5 * r5;

    int offset = 0;
    float d2 = 0.0f;

    for(int y = 0; y < texHeight; y++)
    {
        for(int x = 0; x < texWidth; x++)
        {
            /* Squared distance from pixel to centre. */
            d2 = (y - texHeight / 2) * (y - texHeight / 2) + (x - texWidth / 2) * (x - texWidth / 2);

            col = col % 4;
            col1 = col1 % 4;
            col2 = col2 % 4;
            col3 = col3 % 4;

            if(d2 < r12)
            {
                textureData[offset] =     bakedColours[4 * col+0];
                textureData[offset + 1] = bakedColours[4 * col+1];
                textureData[offset + 2] = bakedColours[4 * col+2];
                textureData[offset + 3] = bakedColours[4 * col + 3];
            }
            else if(d2 < r22)
            {

                textureData[offset] =     bakedColours[4 * (col1) + 0];
                textureData[offset + 1] = bakedColours[4 * col1 + 1];
                textureData[offset + 2] = bakedColours[4 * col1 + 2];
                textureData[offset + 3] = bakedColours[4 * col1 + 3];
            }
            else if(d2 < r32)
            {
                textureData[offset] =     bakedColours[4 * col2 + 0];
                textureData[offset + 1] = bakedColours[4 * col2 + 1];
                textureData[offset + 2] = bakedColours[4 * col2 + 2];
                textureData[offset + 3] = bakedColours[4 * col2 + 3];
            }
            else if(d2 < r42)
            {
                textureData[offset] =     bakedColours[4 * col3 + 0];
                textureData[offset + 1] = bakedColours[4 * col3 + 1];
                textureData[offset + 2] = bakedColours[4 * col3 + 2];
                textureData[offset + 3] = bakedColours[4 * col3 + 3];
            }
            else if(d2 < r52)
            {
                textureData[offset] =     128;
                textureData[offset + 1] = 128;
                textureData[offset + 2] = 128;
                textureData[offset + 3] = 255;
            }

            offset += 4;
        }
    }

    col++;
    col1++;
    col2++;
    col3++;
}

/* Initialise texture buffer data. */
void initTexture(void)
{
    int bytesPerPixel = 4;
    int numBytes = texWidth * texHeight * bytesPerPixel;

    /* Allocate data buffer. */
    textureData = (unsigned char*) malloc(numBytes * sizeof(unsigned char));

    if(!textureData)
    {
        LOGE("Could not allocate memory for texture data.");
        exit(1);
    }

    /* Fill texture buffer with data. Circles with different colours. */
    animateTexture();

    /* Initialise texture. */
    GL_CHECK(glGenTextures(1, &iCubeTex));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, iCubeTex));

    /* Upload texture. */
    GL_CHECK(glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData));

    /* Set filtering. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
}

/* [workingFunction 1] */
/* Secondary thread's working function. */
static void *workingFunction(void *arg)
{
    /* Secondary thread's surface and rendering context creation. */
    /* [workingFunction 1] */
    /* [workingFunction 2] */
    EGLConfig config = findConfig(mainDisplay, true, true);
    pBufferSurface = eglCreatePbufferSurface(mainDisplay, config, pBufferAttributes);
    if(pBufferSurface == EGL_NO_SURFACE)
    {
        EGLint error = eglGetError();
        LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
        LOGE("Failed to create EGL pixel buffer surface at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }

    LOGI("PBuffer surface created successfully.\n");

    /* Unconditionally bind to OpenGL ES API. */
    eglBindAPI(EGL_OPENGL_ES_API);

    /* [Creating rendering context] */
    /* Sharing OpenGL ES objects with main thread's rendering context. */
    pBufferContext = eglCreateContext(mainDisplay, config, mainContext, contextAttributes);
    /* [Creating rendering context] */
    if(pBufferContext == EGL_NO_CONTEXT)
    {
        EGLint error = eglGetError();
        LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
        LOGE("Failed to create EGL pBufferContext at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }

    LOGI("PBuffer context created successfully sharing GLES objects with the main context.\n");
    GL_CHECK(eglMakeCurrent(mainDisplay, pBufferSurface, pBufferSurface, pBufferContext));
    LOGI("PBuffer context made current successfully.\n");
    /* [workingFunction 2] */
    /* [workingFunction 3] */
    /*
     * Flags to pass to glFenceSync must be zero as there are no flag defined yet.
     * The condition must be set to GL_SYNC_GPU_COMMANDS_COMPLETE.
     */
    GLbitfield flags = 0;
    GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;

    while(!exitThread)
    {
        /* Set texture change frequency to 60 frames/s. */
        usleep(1000000 / 60);

        /* Change texture. */
        animateTexture();

        if(useFence)
        {
            if (mainThreadSyncObj != NULL)
            {
                GL_CHECK(glWaitSync(mainThreadSyncObj, flags, timeout));
            }
            else
            {
                continue;
            }

            /* Upload texture. */
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, iCubeTex));
            GL_CHECK(glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData));

            /* This fence create a sync object which is signalled when the fence command reaches the end of the graphic pipeline. */
            secondThreadSyncObj = glFenceSync(condition, flags);

            EGLint error = eglGetError();
            if (secondThreadSyncObj == NULL || error == GL_INVALID_ENUM  || error == GL_INVALID_VALUE )
            {
                LOGE("glFenceSync failed at workingFunction.\n");
            }
        }
        else
        {
            /* Upload texture. */
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, iCubeTex));
            GL_CHECK(glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData));
        }
    }

    LOGI("Exiting secondary thread.\n");

    return NULL;
}
/* [workingFunction 3] */

/* Secondary thread's creation. */
void createTextureThread(void)
{
    int status;
    pthread_create(&secondThread,NULL, &workingFunction,NULL);
}

/* Main thread's graphics setup. */
bool setupGraphics(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    /* Full paths to the shader files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename;
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialise matrices. */
    projection = Matrix::matrixPerspective(45.0f, windowWidth / (float)windowHeight, 0.01f, 100.0f);
    /* Move cube 2 further away from camera. */
    translation = Matrix::createTranslation(0.0f, 0.0f, -2.0f);

    /* Initialise OpenGL ES. */
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glCullFace(GL_BACK));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialise the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), windowWidth, windowHeight);

    if(useFence)
    {
        textString = baseString + "Fencing enabled.";
    }
    else
    {
        textString = baseString + "Fencing disabled.";
    }
    text->addString(0, 0, textString.c_str(), 255, 255, 0, 255);

    /* Initialisation of some global variables needed in the case the application
     * is restarted due to orientation change.
     */
    mainContext = NULL;
    mainDisplay = NULL;
    pBufferSurface = NULL;
    pBufferContext = NULL;
    exitThread = false;
    textureData = NULL;

    initTexture();

    /* Process shaders. */
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

    /* Set up shaders. */
    programID = GL_CHECK(glCreateProgram());
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));

    /* Vertex positions. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "a_v4Position"));
    if(iLocPosition == -1)
    {
        LOGE("Attribute not found at %s:%i\n", __FILE__, __LINE__);
        return false;
    }
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Texture mix. */
    iLocTextureMix = GL_CHECK(glGetUniformLocation(programID, "u_fTex"));
    if(iLocTextureMix == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniform1f(iLocTextureMix, 0.0));
    }

    /* Texture. */
    iLocTexture = GL_CHECK(glGetUniformLocation(programID, "u_s2dTexture"));
    if(iLocTexture == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniform1i(iLocTexture, 0));
    }

    /* Vertex colours. */
    iLocFillColor = GL_CHECK(glGetAttribLocation(programID, "a_v4FillColor"));
    if(iLocFillColor == -1)
    {
        LOGD("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Texture coordinates. */
    iLocTexCoord = GL_CHECK(glGetAttribLocation(programID, "a_v2TexCoord"));
    if(iLocTexCoord == -1)
    {
        LOGD("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    /* Projection matrix. */
    iLocProjection = GL_CHECK(glGetUniformLocation(programID, "u_m4Projection"));
    if(iLocProjection == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, projection.getAsArray()));
    }

    /* Model-view matrix. */
    iLocModelview = GL_CHECK(glGetUniformLocation(programID, "u_m4Modelview"));
    if(iLocModelview == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }

    /* Initialise main display and context variables. */
    mainDisplay = eglGetCurrentDisplay();
    mainContext = eglGetCurrentContext();

    if(useFence)
    {
        /* Initialise mainThreadSyncObj to let the render function execute first,
         * otherwise we get a dead lock between the 2 sync objects.
         */
        GLbitfield flags = 0;
        GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;

        /* This fence creates a sync object which is signalled when the fence command
         * reaches the end of the graphic pipeline.
         */
        mainThreadSyncObj = glFenceSync(condition, flags);
        LOGI("Use of GL Fence enabled.")
    }
    else
    {
        LOGI("Use of GL Fence disabled.")
    }

    /* Secondary thread's creation. */
    createTextureThread();

    return true;
}

/* [renderFrame 1] */
void renderFrame(void)
{
    GLbitfield flags = 0;

    if(useFence)
    {
        if (secondThreadSyncObj != NULL)
        {
            GL_CHECK(glWaitSync(secondThreadSyncObj, flags, timeout));
        }
        else
        {
            return;
        }
    }

    /* Shader program. */
    GL_CHECK(glUseProgram(programID));

    /* [renderFrame 1] */
    /* Vertex data. */
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, cubeVertices));

    /* Colour data. */
    if(iLocFillColor != -1)
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, cubeColors));
    }

    /* Texture coordinate data. */
    if(iLocTexCoord != -1)
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
        GL_CHECK(glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, cubeTextureCoordinates));
    }

    /* [renderFrame 2] */
    /* Reset viewport to the EGL window surface's dimensions. */
    GL_CHECK(glViewport(0, 0, windowWidth, windowHeight));

    /* Clear the screen on the EGL surface. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    /* [renderFrame 2] */
    /* Construct different rotation for main cube. */
    rotationX = Matrix::createRotationX(angleX);
    rotationY = Matrix::createRotationY(angleY);
    rotationZ = Matrix::createRotationZ(angleZ);

    /* Rotate about origin, then translate away from camera. */
    modelView = translation * rotationX;
    modelView = modelView * rotationY;
    modelView = modelView * rotationZ;

    /* Load EGL window-specific projection and model view matrices. */
    GL_CHECK(glUniformMatrix4fv(iLocModelview, 1, GL_FALSE, modelView.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, projection.getAsArray()));

    /* For the main cube, we use texturing so set the texture mix factor to 1. */
    if(iLocTextureMix != -1)
    {
        GL_CHECK(glUniform1f(iLocTextureMix, 1.0));
    }

    /* [renderFrame 3] */
    /* Ensure the correct texture is bound to texture unit 0. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, iCubeTex));

    /* Set the sampler to point at the 0th texture unit. */
    GL_CHECK(glUniform1i(iLocTexture, 0));

    /* And draw the cube. */
    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(cubeIndices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, cubeIndices));
    /* [renderFrame 3] */
    /* Draw any text. */
    text->draw();

    /* Update cube's rotation angles for animating. */
    angleX += 0.75;
    angleY += 0.5;
    angleZ += 0.25;

    if(angleX >= 360) angleX -= 360;
    if(angleY >= 360) angleY -= 360;
    if(angleZ >= 360) angleZ -= 360;
    /* [renderFrame 4] */
    flags = 0;
    GLenum condition = GL_SYNC_GPU_COMMANDS_COMPLETE;
    /*
     * This fence creates a sync object which is signalled when the fence
     * command reaches the end of the graphic pipeline.
     */
    if(useFence)
    {
        if(mainThreadSyncObj == NULL)
        {
            LOGI("mainThreadSynobj == NULL at the end of renderframe.")
        }

        mainThreadSyncObj = glFenceSync(condition, flags);
    }
}
/* [renderFrame 4] */

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_uninit
    (JNIEnv *, jclass)
    {
        /* Finish secondary thread. */
        exitThread = true;
        pthread_join(secondThread, NULL);

        if(textureData != NULL)
        {
            free(textureData);
        }

        delete text;
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_touchStart
        (JNIEnv *env, jclass jcls, jint x, jint y)
    {
        touchStart(x, y);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_touchMove
        (JNIEnv *env, jclass jcls, jint x, jint y)
    {
        touchMove(x, y);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_threadsync_ThreadSync_touchEnd
        (JNIEnv *env, jclass jcls, jint x, jint y)
    {
        touchEnd(x, y);
    }
}
