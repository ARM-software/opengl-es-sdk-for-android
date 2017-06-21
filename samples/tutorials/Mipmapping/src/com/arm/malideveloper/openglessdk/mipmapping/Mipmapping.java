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

package com.arm.malideveloper.openglessdk.mipmapping;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;

import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetManager;
import android.util.Log;

public class Mipmapping extends Activity
{
    private static String LOGTAG = "Mipmapping";
    protected TutorialView graphicsView;
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        String privateAssetDirectory = getFilesDir().getAbsolutePath();

        String textureName = "level0.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level1.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level2.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level3.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level4.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level5.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level6.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level7.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level8.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level9.raw";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level0.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level1.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level2.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level3.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level4.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level5.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level6.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level7.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level8.pkm";
        extractAsset(textureName, privateAssetDirectory);
        textureName = "level9.pkm";
        extractAsset(textureName, privateAssetDirectory);
        graphicsView = new TutorialView(getApplication());
        setContentView(graphicsView);
    }
    @Override protected void onPause()
    {
        super.onPause();
        graphicsView.onPause();
    }
    @Override protected void onResume()
    {
        super.onResume();
        graphicsView.onResume();
    }
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
            catch(Exception e)
            {
                Log.e(LOGTAG, "Failure in extractAssets(): " + e.toString() + " " + assetPath+assetName);
            }
            if(fileTest.exists())
            {
                Log.d(LOGTAG,"File Extracted successfully");
            }
        }
    }

}