/* Copyright (c) 2014-2017, ARM Limited and Contributors
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

package com.arm.malideveloper.openglessdk.tessellation;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;
import android.os.Bundle;
import android.os.Build;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.res.AssetManager;
import android.util.Log;
import android.view.MotionEvent;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.widget.Toast;
import android.content.res.AssetManager;
import android.content.pm.PackageManager;
import android.view.Window;
import android.view.WindowManager;
import android.opengl.GLSurfaceView.EGLContextFactory;
import android.opengl.GLES31;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGL10;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class Tessellation extends Activity
{
    private static android.content.Context applicationContext = null;
    private static String                  assetsDirectory    = null;
    private static String                  LOGTAG             = "libNative";
    private GLSurfaceView view;

    @Override protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
           WindowManager.LayoutParams.FLAG_FULLSCREEN);

        view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        view.setRenderer(new Renderer());
        setContentView(view);

        applicationContext = getApplicationContext();
        assetsDirectory    = applicationContext.getFilesDir().getPath() + "/";

        extractAsset("backdrop.vs");
        extractAsset("backdrop.fs");

        extractAsset("shader.vs");
        extractAsset("shader.tcs");
        extractAsset("shader.tes");
        extractAsset("shader.fs");

        extractAsset("magicmoon_heightmap.png");
        extractAsset("magicmoon_diffusemap.png");

        extractAsset("swirly_heightmap.png");
        extractAsset("swirly_diffusemap.png");

        extractAsset("voronoi_env_heightmap.png");
        extractAsset("voronoi_env_diffusemap.png");

        extractAsset("voronoi_sharp_heightmap.png");
        extractAsset("voronoi_sharp_diffusemap.png");

        extractAsset("wavey_heightmap.png");
        extractAsset("wavey_diffusemap.png");
    }

    @Override protected void onPause()
    {
        super.onPause();
        view.onPause();
    }

    @Override protected void onResume()
    {
        super.onResume();
        view.onResume();
    }

    private void extractAsset(String assetName)
    {
        File file = new File(assetsDirectory + assetName);

        if(file.exists()) {
            Log.d(LOGTAG, assetName +  " already exists. No extraction needed.\n");
        } else {
            Log.d(LOGTAG, assetName + " doesn't exist. Extraction needed. \n");

            try {
                RandomAccessFile randomAccessFile = new RandomAccessFile(assetsDirectory + assetName,"rw");
                AssetManager     assetManager     = applicationContext.getResources().getAssets();
                InputStream      inputStream      = assetManager.open(assetName);

                byte buffer[] = new byte[1024];
                int count     = inputStream.read(buffer, 0, 1024);

                while (count > 0) {
                    randomAccessFile.write(buffer, 0, count);

                    count = inputStream.read(buffer, 0, 1024);
                }

                randomAccessFile.close();
                inputStream.close();
            } catch(Exception e) {
                Log.e(LOGTAG, "Failure in extractAssets(): " + e.toString() + " " + assetsDirectory + assetName);
            }

            if(file.exists()) {
                Log.d(LOGTAG,"File extracted successfully");
            }
        }
    }

    private static class Renderer implements GLSurfaceView.Renderer
    {
        public void onDrawFrame(GL10 gl)
        {
            NativeLibrary.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
            NativeLibrary.resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
            NativeLibrary.init();
        }
    }
}
