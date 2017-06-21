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

package com.arm.malideveloper.openglessdk.fileloading;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;

import android.os.Bundle;
import android.os.Environment;
import android.app.Activity;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.util.Log;

public class FileLoading extends Activity
{
    private String LOGTAG = "FirstLoading";

    /* [savePreferences] */
    @Override protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        SharedPreferences savedValues = getSharedPreferences("savedValues", MODE_PRIVATE);
        int programRuns = savedValues.getInt("programRuns", 1);

        Log.d(LOGTAG, "This application has been run " + programRuns + " times");
        programRuns++;

        SharedPreferences.Editor editor = savedValues.edit();
        editor.putInt("programRuns", programRuns);
        editor.commit();
        /* [savePreferences] */

        /* [publicPrivateCacheFileLocations] */
        String privateAssetDirectory = getFilesDir().getAbsolutePath();
        String publicAssetDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
        String cacheAssetDirectory = getCacheDir().getAbsolutePath();

        Log.i(LOGTAG, "privateAssetDirectory's path is equal to: " + privateAssetDirectory);
        Log.i(LOGTAG, "publicAssetDirectory's path is equal to: " + publicAssetDirectory);
        Log.i(LOGTAG, "cacheAssetDirectory's path is equal to: " + cacheAssetDirectory);
        /* [publicPrivateCacheFileLocations] */

        /* [callExtractAssets] */
        String privateApplicationFileName = "privateApplicationFile.txt";
        String publicApplicationFileName = "publicApplicationFile.txt";
        String cacheApplicationFileName = "cacheApplicationFile.txt";

        extractAsset(privateApplicationFileName, privateAssetDirectory);
        extractAsset(publicApplicationFileName, publicAssetDirectory);
        extractAsset(cacheApplicationFileName, cacheAssetDirectory);
        /* [callExtractAssets] */

        /* [initNativeCall] */
        NativeLibrary.init(privateAssetDirectory + "/" + privateApplicationFileName, publicAssetDirectory + "/" +  publicApplicationFileName, cacheAssetDirectory + "/" + cacheApplicationFileName);
        /* [initNativeCall] */
    }

    @Override protected void onPause()
    {
        super.onPause();
    }

    @Override protected void onResume()
    {
        super.onResume();
    }

    /* [extractAssetBeginning] */
    private void extractAsset(String assetName, String assetPath)
    {
        File fileTest = new File(assetPath, assetName);

        if(fileTest.exists())
        {
            Log.d(LOGTAG, assetName +  " already exists no extraction needed\n");
        }
        else
        {
            Log.d(LOGTAG, assetName + " doesn't exist extraction needed \n");
    /* [extractAssetBeginning] */
    /* [extractAssets] */
            try
            {
                RandomAccessFile out = new RandomAccessFile(fileTest,"rw");
                AssetManager am = getResources().getAssets();

                InputStream inputStream = am.open(assetName);
                byte buffer[] = new byte[1024];
                int count = inputStream.read(buffer, 0, 1024);

                while (count > 0)
                {
                    out.write(buffer, 0, count);
                    count = inputStream.read(buffer, 0, 1024);
                }
                out.close();
                inputStream.close();
            }
      /* [extractAssets] */
            /*  [extractAssetsErrorChecking] */
            catch(Exception e)
            {
                Log.e(LOGTAG, "Failure in extractAssets(): " + e.toString() + " " + assetPath+assetName);
            }
            if(fileTest.exists())
            {
                Log.d(LOGTAG,"File Extracted successfully");
            }
            /*  [extractAssetsErrorChecking] */
        }
    }
}
