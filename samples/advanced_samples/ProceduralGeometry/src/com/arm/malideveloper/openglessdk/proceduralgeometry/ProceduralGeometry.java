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

package com.arm.malideveloper.openglessdk.proceduralgeometry;

import android.content.res.AssetManager;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;

public class ProceduralGeometry extends Activity {

    ProceduralGeometryView mView;
    private static android.content.Context applicationContext = null;
    private static String assetsDirectory = null;
    private static String LOGTAG = "ProceduralGeometry";

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mView = new ProceduralGeometryView(getApplication());
        setContentView(mView);

        applicationContext = getApplicationContext();
        assetsDirectory = applicationContext.getFilesDir().getPath() + "/";

        extractAsset("backdrop.vs");
        extractAsset("backdrop.fs");

        extractAsset("generate.cs");
        extractAsset("centroid.cs");

        extractAsset("geometry.vs");
        extractAsset("geometry.gs");
        extractAsset("geometry.fs");

        extractAsset("texture11.jpg");
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
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

    public static native void init();
    public static native void resize(int width, int height);
    public static native void step();
    public static native void onpointerdown(float x, float y);
    public static native void onpointerup(float x, float y);

    static {
        System.loadLibrary("Native");
    }
}
