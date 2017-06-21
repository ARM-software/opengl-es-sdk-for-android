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

package com.arm.malideveloper.openglessdk.etcTexture;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;
import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetManager;
import android.util.Log;

public class EtcTexture extends Activity
{
    private static android.content.Context applicationContext = null;
    private static String                  assetsDirectory    = null;
    private static String                  LOGTAG             = "EtcTexture";
    protected TutorialView                 tutorialView;
    
    @Override protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        tutorialView = new TutorialView(getApplication());

        /* [onCreateNew] */
        applicationContext = getApplicationContext();
        assetsDirectory    = applicationContext.getFilesDir().getPath() + "/";

        extractAsset("fragment_shader_source.frag");
        extractAsset("vertex_shader_source.vert");
        extractAsset("font.frag");
        extractAsset("font.vert");
        extractAsset("font.raw");
        extractAsset("BinaryAlpha.pkm");
        extractAsset("BumpMap.pkm");
        extractAsset("BumpMapSigned.pkm");
        extractAsset("HeightMap.pkm");
        extractAsset("HeightMapSigned.pkm");
        extractAsset("SemiAlpha.pkm");
        extractAsset("Texture.pkm");

        /* [onCreateNew] */
        setContentView(tutorialView);
    }
    @Override protected void onPause()
    {
        super.onPause();
        tutorialView.onPause();
    }
    @Override protected void onResume()
    {
        super.onResume();
        tutorialView.onResume();
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
}