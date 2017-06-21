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

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.content.*;
import android.view.*;

public class HighQualityTextJava extends Activity {

    // This is the view used to render to it.
    private GLSurfaceView  GLES20SurfaceView;

    // Called when the activity is first created.
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Assume GLES20 rendering is available on the device

        // SetUp the Surface view for the activity
        GLES20SurfaceView = new HighQualityTextSurfaceView(this);
        setContentView(GLES20SurfaceView);
    }

    @Override
    public void onPause() {
        super.onPause();
        GLES20SurfaceView.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
        GLES20SurfaceView.onResume();
    }
}

class HighQualityTextSurfaceView extends GLSurfaceView {

    private HighQualityTextRenderer theRenderer = new HighQualityTextRenderer();
    private float prevX = 0.0f;
    private float prevY = 0.0f;

    public HighQualityTextSurfaceView(Context context){
        super(context);

        setEGLConfigChooser(false);
        // Create an OpenGL ES 2.0 context.
        setEGLContextClientVersion(2);
        // Set the Renderer for drawing on the GLSurfaceView
        setRenderer(theRenderer);
    }

    public boolean onTouchEvent (MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            prevX = event.getRawX();
            prevY = event.getRawY();
            theRenderer.touchDown(prevX, prevY);
        }
        if (event.getAction() == MotionEvent.ACTION_UP) {
            theRenderer.touchUp(event.getRawX(), event.getRawY());
        }
        if (event.getAction() == MotionEvent.ACTION_MOVE) {
            float x = event.getRawX();
            float y = event.getRawY();
            theRenderer.touchMove(x, y, prevX, prevY);
            prevX = x;
            prevY = y;
        }
        return true;
    }
}
