/* Copyright (c) 2015-2017, ARM Limited and Contributors
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

package com.arm.malideveloper.openglessdk.ocean;

import java.io.File;
import java.io.InputStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
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

class OceanView extends MaliSamplesView
{
    public OceanView(Context context) 
    { 
        super(context, GlesVersion.GLES3);
    }
	
    @Override protected void setRendererCallback()
    {
        setRenderer(new Renderer());
    }
    
    @Override protected void destroyContextCallback()
    {
    	Ocean.uninit();
    }

    protected class Renderer implements GLSurfaceView.Renderer 
    {
        public void onDrawFrame(GL10 gl) 
        {
        	Ocean.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) 
        {
        	Ocean.init(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) 
        {
        }
    }
};

public class Ocean extends MaliSamplesActivity 
{
    OceanView mView;
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

        extractAssets();
        
        mView = new OceanView(getApplication());
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

	private byte[] loadAsset(String file) {
		try {
			InputStream stream = getAssets().open(file);
			int len = stream.available();
			byte[] buf = new byte[len];
			stream.read(buf, 0, len);
			stream.close();
			return buf;
		} catch (IOException e) {
			return null;
		}
	}

	private void extractAssets()
	{
		try {
			String[] assets = getAssets().list("");

			for (String file : assets) {
				byte[] buf = loadAsset(file);
				if (buf == null)
					continue;

				String path = assetsDirectory + file;
				BufferedOutputStream writer = new BufferedOutputStream(new FileOutputStream(new File(path)));

				writer.write(buf, 0, buf.length);
				writer.flush();
				writer.close();

				Log.d(LOGTAG, "Asset " + path + " extracted successfully.");
			}
		}
		catch (IOException e) {
			Log.e(LOGTAG, "Failure in extractAssets(): " + e.toString());
		}
	}

    static 
    {
        // Load the NDK library for this example, built with ndk-build
        System.loadLibrary("Native");
    }
}
