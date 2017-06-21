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

package com.arm.malideveloper.openglessdk.highqualitytextjava;

import com.arm.malideveloper.openglessdk.highqualitytextjava.HighQualityTextRenderer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff.Mode;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;

public class TextObject {

    private float theAnimRotZ = 0.0f;
    private Vector4f thePosition = new Vector4f();
    private String theText = new String("Empty");;
    private float textSize = 20;
    private int bitmapWidth;
    private int bitmapHeight;
    private boolean isUpdateNeeded = true;
    private int theViewportWidth = 0;
    private int theViewportHeight = 0;

    // Constructor initialize all necessary members
    public TextObject() {
        super();
        //
        bitmapWidth = 1;
        bitmapHeight = 1;
    }

    public void setText(String aText) {
        theText = aText;
        isUpdateNeeded = true;
    }

    public void setPosition(float aX, float aY, float aZ) {
        thePosition.set(aX, aY, aZ, 1.0f);
        isUpdateNeeded = true;
    }

    public void setRelPos(float aX, float aY, float aZ) {
        setPosition(thePosition.x + aX,
                    thePosition.y + aY,
                    thePosition.z + aZ);

        // Preventing going too far-from or close-to the screen
        if (thePosition.z > 0.9f)
            thePosition.z = 0.9f;
        if (thePosition.z < -4.0f)
            thePosition.z = -4.0f;
    }

    void update() {
        if (isUpdateNeeded == false)
            return;
        /* [Update Text Size] */
        // 1. Calculate bounding box in screen coordinates with current matrices
        Vector4f cLT = new Vector4f(-0.5f,-0.5f, 0.0f, 1.0f);
        Vector4f cLB = new Vector4f(-0.5f, 0.5f, 0.0f, 1.0f);
        Vector4f cRT = new Vector4f( 0.5f,-0.5f, 0.0f, 1.0f);
        Vector4f cRB = new Vector4f( 0.5f, 0.5f, 0.0f, 1.0f);

        // Instead of calculating matrices again lets reuse ones which were already calculated
        // for rendering purpose. One important thing is the update() method must be called
        // after render() method
        cLT.makePixelCoords(mMVPMatrix, theViewportWidth, theViewportHeight);
        cLB.makePixelCoords(mMVPMatrix, theViewportWidth, theViewportHeight);
        cRT.makePixelCoords(mMVPMatrix, theViewportWidth, theViewportHeight);
        cRB.makePixelCoords(mMVPMatrix, theViewportWidth, theViewportHeight);

        // 2. Evaluate font size based on the height of bounding box corners
        Vector4f vl = Vector4f.sub(cLB, cLT);
        Vector4f vr = Vector4f.sub(cRB, cRT);
        textSize = (vl.length3() + vr.length3()) / 2.0f;
        /* [Update Text Size] */

        // 3. Generate texture with the new evaluated font size
        drawCanvasToTexture(theText, textSize);

        android.util.Log.i("INFO", "bmpSize[" + bitmapWidth + ", " + bitmapHeight + "]");

        isUpdateNeeded = false;
    }

    // Load shaders, create vertices, texture coordinates etc.
    public void init() {
        // Initialize the triangle vertex array
        initShapes();

        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
        HighQualityTextRenderer.checkGLError("loadShaders");

        mProgram = GLES20.glCreateProgram();             // Create empty OpenGL Program
        GLES20.glAttachShader(mProgram, vertexShader);   // Add the vertex shader to program
        HighQualityTextRenderer.checkGLError("glAttachShader:vert");
        GLES20.glAttachShader(mProgram, fragmentShader); // Add the fragment shader to program
        HighQualityTextRenderer.checkGLError("glAttachShader:frag");
        GLES20.glLinkProgram(mProgram);                  // Creates OpenGL program executables
        HighQualityTextRenderer.checkGLError("glLinkProgram");
        GLES20.glUseProgram(mProgram);
        HighQualityTextRenderer.checkGLError("glUseProgram");

        // Get handle to the vertex shader's vPosition member
        maPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");
        HighQualityTextRenderer.checkGLError("glGetAttribLocation:vPosition");
        // Get handle to the vertex shader's vPosition member
        maTexCoordsHandle = GLES20.glGetAttribLocation(mProgram, "vTexCoord");
        HighQualityTextRenderer.checkGLError("glGetAttribLocation:vTexCoord");
        // get handle to uniform parameter
        muMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix");
        HighQualityTextRenderer.checkGLError("glGetUniformLocation:uMVPMatrix");
        muTextureHandle = GLES20.glGetUniformLocation(mProgram, "u_s2dTexture");
        HighQualityTextRenderer.checkGLError("glGetUniformLocation:u_s2dTexture");

        GLES20.glHint(GLES20.GL_GENERATE_MIPMAP_HINT, GLES20.GL_NICEST);
        HighQualityTextRenderer.checkGLError("glHint");
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        HighQualityTextRenderer.checkGLError("glActiveTexture");
        GLES20.glGenTextures(1, textureId, 0);
        HighQualityTextRenderer.checkGLError("glGenTextures");
        //
        GLES20.glUniform1i(muTextureHandle, 0);
        HighQualityTextRenderer.checkGLError("glUniform1i");
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId[0]);
        HighQualityTextRenderer.checkGLError("glBindTexture");
        // Setup texture parameters
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        HighQualityTextRenderer.checkGLError("glTexParameterf:GL_TEXTURE_MAG_FILTER");
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR_MIPMAP_LINEAR);
        HighQualityTextRenderer.checkGLError("glTexParameterf:GL_TEXTURE_MIN_FILTER");
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
        HighQualityTextRenderer.checkGLError("glTexParameterf:GL_TEXTURE_WRAP_S");
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
        HighQualityTextRenderer.checkGLError("glTexParameterf:GL_TEXTURE_WRAP_T");
    }


        public void render() {
            // Add program to OpenGL environment
            GLES20.glUseProgram(mProgram);

            theAnimRotZ += 0.5f;

            // Apply a ModelView Projection transformation
            Matrix.setIdentityM(mMMatrix, 0);
            Matrix.rotateM(mMMatrix, 0, theAnimRotZ, 0.0f, 0.0f, 1.0f);
            Matrix.scaleM(mMMatrix, 0,  (float)bitmapWidth / (float)bitmapHeight, (float)1.0f, 1.0f);
            Matrix.translateM(mMMatrix, 0, thePosition.x, thePosition.y, thePosition.z);

            Matrix.multiplyMM(mMVPMatrix, 0, mVMatrix, 0, mMMatrix, 0);
            Matrix.multiplyMM(mMVPMatrix, 0, mProjMatrix, 0, mMVPMatrix, 0);

            GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);

            // Prepare the triangle data
            GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, 12, quadVB);
            GLES20.glEnableVertexAttribArray(maPositionHandle);
            // Prepare the triangle data
            GLES20.glVertexAttribPointer(maTexCoordsHandle, 3, GLES20.GL_FLOAT, false, 12, quadCB);
            GLES20.glEnableVertexAttribArray(maTexCoordsHandle);

            // Draw the triangle
            GLES20.glDisable(GLES20.GL_CULL_FACE);
            GLES20.glEnable(GLES20.GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_ONE, GLES20.GL_ONE_MINUS_SRC_ALPHA);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        }

        void updateCamera(int aWidth,
                          int aHeight) {
            theViewportWidth = aWidth;
            theViewportHeight = aHeight;

            float ratio = (float)aWidth / (float)aHeight;

            // This projection matrix is applied to object coodinates
            Matrix.frustumM(mProjMatrix, 0, -ratio, ratio, -1, 1, 0.1f, 100.0f);
            Matrix.setLookAtM(mVMatrix, 0, 0, 0, 1, 0f, 0f, 0f, 0f, 1.0f, 0.0f);
            //
            isUpdateNeeded = true;
        }

        /* [Draw Canvas To Texture] */
        private void drawCanvasToTexture(
                String aText,
                float aFontSize) {

            if (aFontSize < 8.0f)
                aFontSize = 8.0f;

            if (aFontSize > 500.0f)
                aFontSize = 500.0f;

            Paint textPaint = new Paint();
            textPaint.setTextSize(aFontSize);
            textPaint.setFakeBoldText(false);
            textPaint.setAntiAlias(true);
            textPaint.setARGB(255, 255, 255, 255);
			// If a hinting is available on the platform you are developing, you should enable it (uncomment the line below).
            //textPaint.setHinting(Paint.HINTING_ON);
            textPaint.setSubpixelText(true);
            textPaint.setXfermode(new PorterDuffXfermode(Mode.SCREEN));

            float realTextWidth = textPaint.measureText(aText);

            // Creates a new mutable bitmap, with 128px of width and height
            bitmapWidth = (int)(realTextWidth + 2.0f);
            bitmapHeight = (int)aFontSize + 2;

            Bitmap textBitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, Bitmap.Config.ARGB_8888);
            textBitmap.eraseColor(Color.argb(0, 255, 255, 255));
            // Creates a new canvas that will draw into a bitmap instead of rendering into the screen
            Canvas bitmapCanvas = new Canvas(textBitmap);
            // Set start drawing position to [1, base_line_position]
            // The base_line_position may vary from one font to another but it usually is equal to 75% of font size (height).
            bitmapCanvas.drawText(aText, 1, 1.0f + aFontSize * 0.75f, textPaint);

            GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId[0]);
            HighQualityTextRenderer.checkGLError("glBindTexture");
            // Assigns the OpenGL texture with the Bitmap
            GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, textBitmap, 0);
            // Free memory resources associated with this texture
            textBitmap.recycle();

            // After the image has been subloaded to texture, regenerate mipmaps
            GLES20.glGenerateMipmap(GLES20.GL_TEXTURE_2D);
            HighQualityTextRenderer.checkGLError("glGenerateMipmap");
        }
        /* [Draw Canvas To Texture] */

        private void initShapes(){

            float quadVerts[] = {
                // X, Y, Z
                -0.5f, -0.5f, 0,
                -0.5f,  0.5f, 0,
                 0.5f, -0.5f, 0,
                 0.5f,  0.5f, 0
            };

            float quadCoords[] = {
                // X, Y, Z
                0.0f, 1.0f, 0,
                0.0f, 0.0f, 0,
                1.0f, 1.0f, 0,
                1.0f, 0.0f, 0
            };

            // Initialize vertex Buffer for triangle
            ByteBuffer vbb = ByteBuffer.allocateDirect(
                    // (# of coordinate values * 4 bytes per float)
                    quadVerts.length * 4);
            vbb.order(ByteOrder.nativeOrder()); // Use the device hardware's native byte order
            quadVB = vbb.asFloatBuffer();       // Create a floating point buffer from the ByteBuffer
            quadVB.put(quadVerts);              // Add the coordinates to the FloatBuffer
            quadVB.position(0);                 // Set the buffer to read the first coordinate

            vbb = ByteBuffer.allocateDirect(
                    // (# of coordinate values * 4 bytes per float)
                    quadCoords.length * 4);
            vbb.order(ByteOrder.nativeOrder()); // Use the device hardware's native byte order
            quadCB = vbb.asFloatBuffer();       // Create a floating point buffer from the ByteBuffer
            quadCB.put(quadCoords);             // Add the coordinates to the FloatBuffer
            quadCB.position(0);                 // Set the buffer to read the first coordinate
        }

        private int loadShader(int type, String shaderCode)
        {
            // Create a vertex shader type (GLES20.GL_VERTEX_SHADER)
            // or a fragment shader type (GLES20.GL_FRAGMENT_SHADER)
            int shader = GLES20.glCreateShader(type);

            // Add the source code to the shader and compile it
            GLES20.glShaderSource(shader, shaderCode);
            GLES20.glCompileShader(shader);

            return shader;
        }

    private int muMVPMatrixHandle;
    private int muTextureHandle;
    private float[] mMVPMatrix = new float[16];
    private float[] mMMatrix = new float[16];
    private float[] mVMatrix = new float[16];
    private float[] mProjMatrix = new float[16];


    private int mProgram;
    private int maPositionHandle;
    private int maTexCoordsHandle;
    private int[] textureId = new int[1];
    private FloatBuffer quadVB;
    private FloatBuffer quadCB;

    private final String vertexShaderCode =
        // This matrix member variable provides a hook to manipulate
        // the coordinates of the objects that use this vertex shader
        "uniform mat4 uMVPMatrix;   \n" +

        "attribute vec4 vPosition;  \n" +
        "attribute vec4 vTexCoord;  \n" +

        "varying vec4 v_v4TexCoord; \n" +

        "void main(){               \n" +

        // The matrix must be included as a modifier of gl_Position
        " v_v4TexCoord = vTexCoord; \n" +
        " gl_Position = uMVPMatrix * vPosition; \n" +

        "}  \n";

    private final String fragmentShaderCode =
        "precision mediump float;  \n" +
        "uniform sampler2D u_s2dTexture; \n" +
        "varying vec4 v_v4TexCoord; \n" +
        "void main(){              \n" +
        " gl_FragColor = texture2D(u_s2dTexture, v_v4TexCoord.xy); \n" +
        "}                         \n";
}
