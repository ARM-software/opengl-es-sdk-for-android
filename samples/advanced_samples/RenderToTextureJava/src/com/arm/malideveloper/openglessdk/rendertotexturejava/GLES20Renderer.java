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

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;


public class GLES20Renderer implements GLSurfaceView.Renderer {

    /** Geometry to draw */
    private SpinningCube3D m_SpinningCube3D;

    public GLES20Renderer(Context context) {

        /** Initialize the Spinning Cube */
        this.m_SpinningCube3D = new SpinningCube3D(context);
    }

    @Override
    public void onDrawFrame(GL10 a_glUnused) {

        /** Use GLES20 instead of GL10's a_glUnused */
        /** Start rendering the cube */
        this.m_SpinningCube3D.render();
    }

    @Override
    public void onSurfaceChanged(GL10 a_glUnused, int a_Width, int a_Height) {

        /** Use GLES20 instead of GL10's a_glUnused */
        /** Create the Projection matrix and setup view port for the spinning cube */
        this.m_SpinningCube3D.setWidthHeight(a_Width, a_Height);
        this.m_SpinningCube3D.setProjection(a_Width, a_Height);
    }

    @Override
    public void onSurfaceCreated(GL10 a_glUnused, EGLConfig a_EGLConfig) {

        /** Use GLES20 instead of GL10's a_glUnused */
        /** Create the Spinning Cube 3D structure and buffers */
        this.m_SpinningCube3D.createSpinningCube();
    }
}
