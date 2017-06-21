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

package com.arm.malideveloper.openglessdk.threadsync;
 
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import com.arm.malideveloper.openglessdk.*;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

class ThreadSyncView extends MaliSamplesView
{
    public ThreadSyncView(Context context, MaliSamplesView.GlesVersion version) 
    { 
        super(context, version);
    }
	
    @Override protected void setRendererCallback()
    {
        setRenderer(new Renderer());
    }
    
    @Override protected void destroyContextCallback()
    {
    	ThreadSync.uninit();
    }

    protected class Renderer implements GLSurfaceView.Renderer 
    {
        public void onDrawFrame(GL10 gl) 
        {
        	ThreadSync.step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) 
        {
        	ThreadSync.init(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) 
        {
            // Optional code here...
        }
    }
};

public class ThreadSync extends MaliSamplesActivity implements OnTouchListener
{
	ThreadSyncView mView;
    
    public static native void init(int width, int height);
    public static native void step();
    public static native void uninit();
    
    public static native void touchStart(int x, int y);
    public static native void touchMove (int x, int y);
    public static native void touchEnd  (int x, int y);
    
    @Override protected void onCreate(Bundle icicle) 
    {
        super.onCreate(icicle);
        
        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
           WindowManager.LayoutParams.FLAG_FULLSCREEN);
        
        mView = new ThreadSyncView(getApplication(),MaliSamplesView.GlesVersion.GLES3);
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

    static 
    {
        // Load the NDK library for this example, built with ndk-build
        System.loadLibrary("Native");
    }
    
    @Override public boolean onTouch(View v, MotionEvent event) 
    {
      //Log.d( "ThreadSync Fur", "onTouch" );
      // TODO Auto-generated method stub
      return false;
    }
    
    @Override public boolean onTouchEvent(MotionEvent event)
    {
      //Log.i( "Fur", "onTouchEvent" );
      switch (event.getAction())
          {
          case MotionEvent.ACTION_DOWN:
          //Log.i( "Dials", "onTouchEvent/DOWN" );
          touchStart((int)event.getX(),(int)event.getY());
        break;
          case MotionEvent.ACTION_MOVE:
          //Log.i( "Dials", "onTouchEvent/MOVE" );
          touchMove((int)event.getX(),(int)event.getY());
        break;
            
          case MotionEvent.ACTION_UP:
          //Log.i( "Dials", "onTouchEvent/UP" );
          touchEnd((int)event.getX(),(int)event.getY());
        break;
          default:
        break;
          }
      return true;
    }
    
    
}


