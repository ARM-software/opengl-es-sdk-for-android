/* Copyright (c) 2011-2017, ARM Limited and Contributors
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

package com.arm.malideveloper.openglessdk.occlusionculling;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;
import android.content.res.AssetManager;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import com.arm.malideveloper.openglessdk.*;

class OcclusionCullingView extends MaliSamplesView
{
    public OcclusionCullingView(Context context) 
    { 
        super(context, GlesVersion.GLES3);
    }
	
    @Override protected void setRendererCallback()
    {
        setRenderer(new Renderer());
    }
    
    @Override protected void destroyContextCallback()
    {
    	OcclusionCulling.uninit();
    }

    protected class Renderer implements GLSurfaceView.Renderer 
    {
        public void onDrawFrame(GL10 gl) 
        {
        	OcclusionCulling.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) 
        {
        	OcclusionCulling.init(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) 
        {
        }
    }
};

public class OcclusionCulling extends MaliSamplesActivity 
{
    OcclusionCullingView mView;
    private static android.content.Context applicationContext = null;
    private static String                  assetsDirectory    = null;
    private static String                  LOGTAG             = "libNative";
    
    public static native void init(int width, int height);
    public static native void step();
    public static native void uninit();
    
    @Override protected void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        
        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
        WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        applicationContext = getApplicationContext();
        assetsDirectory    = applicationContext.getFilesDir().getPath() + "/";

        extractAsset("font.frag");
        extractAsset("font.vert");
        extractAsset("font.raw");

        extractAsset("depth.fs");
        extractAsset("depth.vs");
        extractAsset("depth_mip.fs");

        extractAsset("hiz_cull.cs");
        extractAsset("hiz_cull_no_lod.cs");
        extractAsset("physics.cs");

        extractAsset("quad.vs");
        extractAsset("quad.fs");
        extractAsset("scene.vs");
        extractAsset("scene.fs");
        extractAsset("scene_sphere.vs");
        extractAsset("scene_sphere.fs");
        
        mView = new OcclusionCullingView(getApplication());
        mView.getHolder().setFixedSize(1280, 720);
        setContentView(mView);
    }

    @Override protected void onPause() 
    {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() 
    {
        super.onResume();
        mView.onResume();
    }

    @Override protected void onDestroy()
    {
    	super.onDestroy();
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


    static 
    {
        // Load the NDK library for this example, built with ndk-build
        System.loadLibrary("Native");
    }
}


