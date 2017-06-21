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

package com.arm.malideveloper.openglessdk.multisampledfbo;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.Window;
import android.view.WindowManager;

import com.arm.malideveloper.openglessdk.*;

class MultisampledFBOView extends MaliSamplesView implements GestureDetector.OnGestureListener, ScaleGestureDetector.OnScaleGestureListener
{
	private static final String TAG = MultisampledFBOView.class.getSimpleName();
	private boolean initOk = false;

	private GestureDetector gestureDetector;
	private ScaleGestureDetector scaleGestureDetector;

	private float scaleFactor = 1.0f;

	public MultisampledFBOView(Context context)
	{
		/* Ensure we ask for a GLES3 context instead of a GLES2 context. */
		super(context, GlesVersion.GLES3);

		gestureDetector = new GestureDetector(context, this);
		scaleGestureDetector = new ScaleGestureDetector(context, this);
	}

	@Override protected void setRendererCallback()
	{
		setRenderer(new Renderer());
	}

	@Override protected void destroyContextCallback()
	{
		MultisampledFBO.uninit();
	}

	@Override public boolean onTouchEvent(MotionEvent ev)
	{
		this.gestureDetector.onTouchEvent(ev);
		this.scaleGestureDetector.onTouchEvent(ev);
		return true;
	}

	protected class Renderer implements GLSurfaceView.Renderer
	{
		@Override public void onDrawFrame(GL10 gl)
		{
			if (initOk) MultisampledFBO.step();
		}

		@Override public void onSurfaceChanged(GL10 gl, int width, int height)
		{
			initOk = MultisampledFBO.init(width, height);
			if (initOk)
				Log.d(TAG, "Initialization complete.");
			else
				Log.e(TAG, "Initialization FAILED!");
		}

		@Override public void onSurfaceCreated(GL10 gl, EGLConfig config)
		{
		}
	}

	@Override public boolean onDown(MotionEvent e)
	{
		return true;
	}

	@Override public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY)
	{
		return true;
	}

	@Override public void onLongPress(MotionEvent e)
	{
		MultisampledFBO.switchColor();
	}

	@Override public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY)
	{
		if (e2.getAction() == MotionEvent.ACTION_MOVE)
			MultisampledFBO.setDragRotation(distanceX, distanceY);
		return true;
	}

	@Override public void onShowPress(MotionEvent e)
	{
	}

	@Override public boolean onSingleTapUp(MotionEvent e)
	{
		MultisampledFBO.toggleAnim();
		return true;
	}

	@Override public boolean onScale(ScaleGestureDetector scaleGestureDetector)
	{
		scaleFactor *= scaleGestureDetector.getScaleFactor();
		scaleFactor = Math.max(0.1f, Math.min(scaleFactor, 2.0f));
		MultisampledFBO.setScaleFactor(scaleFactor);
		return true;
	}

	@Override public boolean onScaleBegin(ScaleGestureDetector arg0)
	{
		return true;
	}

	@Override public void onScaleEnd(ScaleGestureDetector arg0)
	{
	}
}

public class MultisampledFBO extends MaliSamplesActivity
{
	boolean volUpInLongPress = false;
	boolean volDownInLongPress = false;

	MultisampledFBOView mView;

	public static native boolean init(int width, int height);
	public static native void step();
	public static native void toggleAnim();
	public static native void switchColor();
	public static native void switchSamples();
	public static native void switchTextureFormat();
	public static native void switchTextureSize();
	public static native void toggleTextureFiltering();
	public static native void setScaleFactor(float scaleFactor);
	public static native void setDragRotation(float rotationX, float rotationY);
	public static native void uninit();

	@Override protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		this.requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
		   WindowManager.LayoutParams.FLAG_FULLSCREEN);

		mView = new MultisampledFBOView(getApplication());
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

	@Override public boolean dispatchKeyEvent(KeyEvent event)
	{
		int action = event.getAction();
		int keyCode = event.getKeyCode();

		switch (keyCode)
		{
		case KeyEvent.KEYCODE_VOLUME_UP:
			if (action == KeyEvent.ACTION_DOWN && event.isLongPress())
			{
				volUpInLongPress = true;

				/* We must ensure these calls occur on the UI thread. */
				mView.queueEvent(new Runnable()
				{
					@Override
					public void run()
					{
						switchTextureFormat();
					}
				});
			}

			if (action == KeyEvent.ACTION_UP)
			{
				if (!volUpInLongPress)
				{
					mView.queueEvent(new Runnable()
					{
						@Override
						public void run()
						{
							switchSamples();
						}
					});
				}
				else
				{
					volUpInLongPress = false;
				}
			}
			return true;

		case KeyEvent.KEYCODE_VOLUME_DOWN:
			if (action == KeyEvent.ACTION_DOWN && event.isLongPress())
			{
				volDownInLongPress = true;
				mView.queueEvent(new Runnable()
				{
					@Override
					public void run()
					{
						toggleTextureFiltering();
					}
				});
			}
			if (action == KeyEvent.ACTION_UP)
			{
				if(!volDownInLongPress)
				{
					mView.queueEvent(new Runnable()
					{
						@Override
						public void run()
						{
							switchTextureSize();
						}
					});
					}
				else
				{
					volDownInLongPress = false;
				}
			}
			return true;

		default:
			return super.dispatchKeyEvent(event);
		}
	}

	static
	{
		// Load the NDK library for this example, built with ndk-build
		System.loadLibrary("Native");
	}
}