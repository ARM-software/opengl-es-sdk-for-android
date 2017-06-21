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

package com.arm.malideveloper.openglessdk.rendertotexturejava;

import java.io.InputStream;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.content.Context;

/**
 * SpinningCube3D contains all the geometry of the Cube It also contains the
 * required vertex/normal/texture-coordinates buffers
 */
public class SpinningCube3D {

    // TODO: Added error checks

    /** Rendering context for the Spinning Cube */
    private Context m_Context;

    /** Rotation angle in X and Y axis*/
    private int m_RotateX = 0;
    private int m_RotateY = 360;

    /** Number of vertices in the Cube */
    private int m_VertexCount = 23;

    /** Number of indices for the vertices in the Cube */
    private int m_IndexCount = 36;

    /** Texture ID for the cube diffuse color */
    private int m_CubeDiffuseTextureId;

    /** Texture ID for the FBO attached texture */
    private int m_CubeFBOTextureId;


    /** Vertex buffer object and Index buffer object IDs */
    /** Frame Buffer FBO ID */
    private int[] m_RenderToTextureFBO = new int[1];

    /** Vertex Attributes VBO ID */
    private int[] m_VertexAttributeVBO = new int[1];

    /** Vertex Attributes IBO ID */
    private int[] m_VertexAttributeIBO = new int[1];

    /** Shader Program ID */
    private int m_ShaderProgramID;

    /** Vertices attributes IDs */
    private int m_VerticesPositionAttributeId;
    private int m_VerticesTextureAttributeId;

    /** Model View Projection matrix ID */
    private int m_ModelViewProjectionId;

    /** Shader sources for the Shaders used in Spinning Cube */
    /** This could also be loaded from Shader resource files */
    private final String m_VertexShaderSource =
            "uniform mat4 uModelViewProjection;\n" +
            "attribute vec4 aPosition;\n" +
            "attribute vec2 aTextureCoordinate;\n" +
            "varying vec2 vTextureCoordinate;\n" +
            "void main(){\n" +
            "gl_Position = uModelViewProjection * aPosition;\n" +
            "vTextureCoordinate = aTextureCoordinate;\n" +
            "}\n";

    private final String m_FragmentShaderSource =
            "precision mediump float;\n" +
            "varying vec2 vTextureCoordinate;\n" +
            "uniform sampler2D sTextureSampler;\n" +
            "void main(){\n" +
            "gl_FragColor = texture2D(sTextureSampler, vTextureCoordinate);\n" +
            "}\n";

    /** Different matrices required for correct transformations */
    private float[] m_ModelViewProjectionMatrix = new float[16];
    private float[] m_ModelingMatrix = new float[16];
    private float[] m_ModelViewMatrix = new float[16];
    private float[] m_ProjectionMatrix = new float[16];

    /** Width and Height of the screen buffer */
    private int m_Width;
    private int m_Height;

    /** Width and Height of the frame buffer */
    private int m_FBOWidth = 256;
    private int m_FBOHeight = 256;

    /** Default constructor */
    public SpinningCube3D() {
        super();
    }

    /** One argument constructor initializes the cube data structures*/
    public SpinningCube3D(Context a_Context) {
        super();

        /** Save the context for later use */
        this.m_Context = a_Context;
    }

    /** Create the spinning cube and setup OpenGL ES states */
    public void createSpinningCube() {

        /** OpenGL ES initialization is done here */
        /** Enable back face culling to avoid drawing faces that are invisible */
        GLES20.glEnable(GLES20.GL_CULL_FACE);

        /** Enable depth testing */
        GLES20.glEnable(GLES20.GL_DEPTH_TEST);

        /** Create the Shader Program, Vertex and Fragment shaders */
        createShaderProgram();

        /** Create the texture and upload it to OpenGL ES */
        createTexture();

        /** Build the Vertex and Index buffers objects and load data into it */
        buildVertexAndIndexBufferObjects();

        /** Initialized the viewing matrix */
        /** From        (0,0,-2)
         *  To          (0,0,0)
         *  Up Vector   (0,1,0)
         */
        Matrix.setLookAtM(this.m_ModelViewMatrix, 0, 0, 0, -2, 0f, 0f, 0f, 0f, 1.0f, 0.0f);
    }

    /** Create the texture and upload it to OpenGL ES */
    private void createTexture() {

        /** Create the texture, setup its properties and upload it to OpenGL ES */
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        this.m_CubeDiffuseTextureId = textureIds[0];

        /** Bind the texture and Setup its properties */
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, this.m_CubeDiffuseTextureId);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);

        /** Load the resource as a bitmap */
        InputStream inputStream = this.m_Context.getResources().openRawResource(R.raw.bubbles);
        Bitmap bitmapTexture = BitmapFactory.decodeStream(inputStream);

        /** Upload the texture to OpenGL ES */
        GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmapTexture, 0);

        /** Mark the bitmap for deletion */
        bitmapTexture.recycle();
    }

    private void createShaderProgram() {

        /** Create the vertex shader */
        int vertexShader = GLES20.glCreateShader(GLES20.GL_VERTEX_SHADER);

        /** Upload the vertex shader source to OpenGL ES and compile it */
        GLES20.glShaderSource(vertexShader, this.m_VertexShaderSource);
        GLES20.glCompileShader(vertexShader);

        /** Create the fragment shader */
        int fragmentShader = GLES20.glCreateShader(GLES20.GL_FRAGMENT_SHADER);

        /** Upload the fragment shader source to OpenGL ES and compile it */
        GLES20.glShaderSource(fragmentShader, this.m_FragmentShaderSource);
        GLES20.glCompileShader(fragmentShader);

        /** Create the shader program and attache the vertex and fragment shaders to it */
        this.m_ShaderProgramID = GLES20.glCreateProgram();

        GLES20.glAttachShader(this.m_ShaderProgramID, vertexShader);
        GLES20.glAttachShader(this.m_ShaderProgramID, fragmentShader);

        /** Link the program */
        GLES20.glLinkProgram(this.m_ShaderProgramID);

        /** Get Vertices Position attribute index from GLES */
        this.m_VerticesPositionAttributeId = GLES20.glGetAttribLocation(this.m_ShaderProgramID, "aPosition");
        if (this.m_VerticesPositionAttributeId == -1) {
            throw new RuntimeException("aPosition attribute location invalid");
        }

        /** Get Vertices Texture Coordinate attribute index from GLES*/
        this.m_VerticesTextureAttributeId = GLES20.glGetAttribLocation(this.m_ShaderProgramID, "aTextureCoordinate");
        if (this.m_VerticesTextureAttributeId == -1) {
            throw new RuntimeException("aTextureCoordinate attribute location invalid");
        }

        /** Get Model View Projection matrix index from GLES*/
        this.m_ModelViewProjectionId = GLES20.glGetUniformLocation(this.m_ShaderProgramID, "uModelViewProjection");
        if (this.m_ModelViewProjectionId == -1) {
            throw new RuntimeException("uModelViewProjection location invalid");
        }
    }

    public void render() {

        /** Use only GLES20 methods */

        int floatSize = Float.SIZE >> 3;

        /** Start rendering using the OpenGL ES 2.0 shader program created earlier */
        GLES20.glUseProgram(this.m_ShaderProgramID);

        /** Bind the Vertex and Index buffers */
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, this.m_VertexAttributeVBO[0]);
        GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, this.m_VertexAttributeIBO[0]);

        /** Set the pointers to the vertex position attributes */
        /** The magic number 3 means 3-float-components for each vertex position */
        GLES20.glVertexAttribPointer(this.m_VerticesPositionAttributeId, 3, GLES20.GL_FLOAT, false, 0, 0);
        GLES20.glEnableVertexAttribArray(this.m_VerticesPositionAttributeId);

        /** set the pointers to the vertex texture coordinates attributes */
        /** The magic number 2 means 3-float-components for each vertex texture coordinate */
        /** The magic number 3 means 3-float-components for each vertex position */
        GLES20.glVertexAttribPointer(this.m_VerticesTextureAttributeId, 2, GLES20.GL_FLOAT, false, 0 , this.m_VertexCount * 3 * floatSize);
        GLES20.glEnableVertexAttribArray(this.m_VerticesTextureAttributeId);

        /** Update the rotation angles and check against maximum and minimum values */
        this.m_RotateX += 2;
        this.m_RotateY -= 2;

        if (this.m_RotateX > 360) {
            this.m_RotateX = 0;
        }
        if (this.m_RotateY < 0) {
            this.m_RotateY = 360;
        }

        float[] m_ModelingMatrix1 = new float[16];
        float[] m_ModelingMatrix2 = new float[16];
        float[] m_ModelingMatrix3 = new float[16];

        /** Create matrices from the euler angles */
        Matrix.setRotateM(m_ModelingMatrix1, 0, this.m_RotateX, 1.0f, 0.0f, 0.0f);
        Matrix.setRotateM(m_ModelingMatrix2, 0, this.m_RotateY, 0.0f, 1.0f, 0.0f);

        /** Combine the matrices into the Modeling Matrix*/
        Matrix.multiplyMM(m_ModelingMatrix3, 0, m_ModelingMatrix1, 0, m_ModelingMatrix2, 0);
        Matrix.multiplyMM(this.m_ModelingMatrix, 0, m_ModelViewMatrix, 0, m_ModelingMatrix3, 0);

        /** Setup with view port with the width and height and reset the projection matrix accordingly */
        this.setProjection(this.m_FBOWidth, this.m_FBOHeight);

        /** Create model view projection matrix */
        Matrix.multiplyMM(this.m_ModelViewProjectionMatrix, 0, this.m_ProjectionMatrix, 0, this.m_ModelingMatrix, 0);

        /** Upload the Model View Projection matrix to the bound program */
        GLES20.glUniformMatrix4fv(this.m_ModelViewProjectionId, 1, false, this.m_ModelViewProjectionMatrix, 0);

        /** set the Texture unit 0 active and bind the texture to it */
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);

        /** Enable frame buffer to start rendering into it */
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, this.m_RenderToTextureFBO[0]);

        /** Create an Orange back ground */
        GLES20.glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
        GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

        /** Bind the texture */
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, this.m_CubeDiffuseTextureId);

        /** Draw the cube using the indices. There are total 36 vertices to draw */
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 36, GLES20.GL_UNSIGNED_SHORT, 0);

        /** Un-bind the frame buffer and render to the screen frame buffer */
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);

        /** Use blue background color */
        GLES20.glClearColor(0.0f, 0.2f, 0.3f, 1.0f);
        GLES20.glClear( GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

        /** Setup with view port with the width and height and reset the projection matrix accordingly */
        this.setProjection(this.m_Width, this.m_Height);

        /** Create model view projection matrix */
        Matrix.multiplyMM(this.m_ModelViewProjectionMatrix, 0, this.m_ProjectionMatrix, 0, this.m_ModelingMatrix, 0);

        /** Upload the Model View Projection matrix to the bound program */
        GLES20.glUniformMatrix4fv(this.m_ModelViewProjectionId, 1, false, this.m_ModelViewProjectionMatrix, 0);

        /** Bind the texture */
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, this.m_CubeFBOTextureId);

        /** Draw the cube using the indices. There are total 36 vertices to draw */
        GLES20.glDrawElements(GLES20.GL_TRIANGLES, 36, GLES20.GL_UNSIGNED_SHORT, 0);
    }

    private void buildVertexAndIndexBufferObjects() {

        int floatSize = Float.SIZE >> 3;
        int shortSize = Short.SIZE >> 3;

        float cubeVerticesAttributes[] = new float[] {
            /** Vertices Positions */
            -0.5f, -0.5f,  0.5f,    /* 0  */
             0.5f, -0.5f,  0.5f,    /* 1  */
            -0.5f,  0.5f,  0.5f,    /* 2  */
             0.5f,  0.5f,  0.5f,    /* 3  */
            -0.5f, -0.5f, -0.5f,    /* 4  */
             0.5f, -0.5f, -0.5f,    /* 5  */
            -0.5f,  0.5f, -0.5f,    /* 6  */
             0.5f,  0.5f, -0.5f,    /* 7  */
             0.5f, -0.5f,  0.5f,    /* 8  */
             0.5f,  0.5f,  0.5f,    /* 9  */
             0.5f, -0.5f, -0.5f,    /* 10 */
            -0.5f, -0.5f, -0.5f,    /* 11 */
             0.5f,  0.5f, -0.5f,    /* 12 */
            -0.5f,  0.5f, -0.5f,    /* 13 */
            -0.5f,  0.5f, -0.5f,    /* 14 */
            -0.5f, -0.5f,  0.5f,    /* 15 */
            -0.5f,  0.5f,  0.5f,    /* 16 */
            -0.5f,  0.5f,  0.5f,    /* 17 */
             0.5f,  0.5f,  0.5f,    /* 18 */
             0.5f,  0.5f, -0.5f,    /* 19 */
            -0.5f, -0.5f,  0.5f,    /* 20 */
            -0.5f, -0.5f, -0.5f,    /* 21 */
             0.5f, -0.5f,  0.5f,    /* 22 */

            /** Vertices Texture Coordinates */
            0.0f, 0.0f,             /* 0  */
            1.0f, 0.0f,             /* 1  */
            0.0f, 1.0f,             /* 2  */
            1.0f, 1.0f,             /* 3  */
            0.0f, 0.0f,             /* 4  */
            1.0f, 0.0f,             /* 5  */
            0.0f, 1.0f,             /* 6  */
            1.0f, 1.0f,             /* 7  */
            0.0f, 0.0f,             /* 8  */
            0.0f, 1.0f,             /* 9  */
            0.0f, 0.0f,             /* 10 */
            1.0f, 0.0f,             /* 11 */
            0.0f, 1.0f,             /* 12 */
            1.0f, 1.0f,             /* 13 */
            0.0f, 1.0f,             /* 14 */
            1.0f, 0.0f,             /* 15 */
            1.0f, 1.0f,             /* 16 */
            0.0f, 0.0f,             /* 17 */
            1.0f, 0.0f,             /* 18 */
            1.0f, 1.0f,             /* 19 */
            0.0f, 1.0f,             /* 20 */
            0.0f, 0.0f,             /* 21 */
            1.0f, 1.0f,             /* 22 */

            /** Add other Vertices attributes Here */
        };

        short cubeTriangleIndices[] = new short[] {
            /** Triangle Indices */
            /** Front First*/
            0, 1, 2,                /* 1  */
            /** Front Second*/
            2, 1, 3,                /* 2  */

            /** Right First*/
            8, 5, 9,                /* 3  */
            /** Right Second*/
            9, 5, 7,                /* 4  */

            /** Back First*/
            10, 11, 12,             /* 5  */
            /** Back Second*/
            12, 11, 13,             /* 6  */

            /** Left First*/
            14, 4, 15,              /* 7  */
            /** Left Second*/
            14, 15, 16,             /* 8  */

            /** Top First*/
            17, 18, 6,              /* 9  */
            /** Top Second*/
            6, 18, 19,              /* 10 */

            /** Bottom First*/
            20, 21, 5,              /* 11 */
            /** Bottom Second*/
            20, 5, 22               /* 12 */
        };

        /** Buffers for vertices and indices */
        FloatBuffer vertexDataBuffer;
        ShortBuffer indexDataBuffer;

        /** Copy the vertices and indices into the buffers */
        vertexDataBuffer = FloatBuffer.wrap(cubeVerticesAttributes);
        indexDataBuffer = ShortBuffer.wrap(cubeTriangleIndices);

        /** Generate the Vertex Buffers and get valid buffer IDs from OpenGL */
        GLES20.glGenBuffers(1, this.m_VertexAttributeVBO, 0);
        GLES20.glGenBuffers(1, this.m_VertexAttributeIBO, 0);

        /** Load the VBO data into the driver */
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, this.m_VertexAttributeVBO[0]);
        GLES20.glBufferData(GLES20.GL_ARRAY_BUFFER, (this.m_VertexCount * 3 * floatSize) + (this.m_VertexCount * 2 * floatSize)  , vertexDataBuffer, GLES20.GL_STATIC_DRAW);

        /** Load the IBO data into the driver */
        GLES20.glBindBuffer(GLES20.GL_ELEMENT_ARRAY_BUFFER, this.m_VertexAttributeIBO[0]);
        GLES20.glBufferData(GLES20.GL_ELEMENT_ARRAY_BUFFER, this.m_IndexCount * shortSize , indexDataBuffer, GLES20.GL_STATIC_DRAW);

        /** Create the texture, setup its properties for the FBO */
        int[] textureIds = new int[1];
        GLES20.glGenTextures(1, textureIds, 0);
        this.m_CubeFBOTextureId = textureIds[0];

        /** Bind the texture for FBO and Setup its properties */
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, this.m_CubeFBOTextureId);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_REPEAT);
        GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_REPEAT);
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_RGBA, this.m_FBOWidth, this.m_FBOHeight, 0, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);

        /** Create the Frame Buffer Objects for rendering into it */
        /** Initialize FBO */
        GLES20.glGenFramebuffers(1, this.m_RenderToTextureFBO, 0);

        /** Bind our frame buffer */
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, this.m_RenderToTextureFBO[0]);

        /** Attach texture to the frame buffer */
        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0, GLES20.GL_TEXTURE_2D, this.m_CubeFBOTextureId, 0);

        /** Check FBO is complete and OK */
        int iResult = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
        if(iResult != GLES20.GL_FRAMEBUFFER_COMPLETE)
        {
            throw new RuntimeException("Error: Frame Buffer Status is not complete. Terminated.");
        }

        /** Un-bind frame buffer */
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
    }

    public void setWidthHeight(int a_Width, int a_Height){
        this.m_Width = a_Width;
        this.m_Height = a_Height;
    }

    public void setProjection(int a_Width, int a_Height) {

        /** Set the view port */
        GLES20.glViewport(0, 0, a_Width, a_Height);

        /** Create the projection matrix using the aspect ratio */
        float aspectRatio = (float) a_Width / (float) a_Height;
        Matrix.frustumM(this.m_ProjectionMatrix, 0, -aspectRatio, aspectRatio, -1, 1, 1, 10);
    }
}
