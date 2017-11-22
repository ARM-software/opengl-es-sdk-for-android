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

package com.arm.malideveloper.openglessdk.astctextureslowprecision;

import java.io.File;
import java.io.InputStream;
import java.io.RandomAccessFile;

import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetManager;
import android.util.Log;

public class AstcTexturesLowPrecision extends Activity
{
    /* [classMembers] */
    private static String LOGTAG = "AstcTextures";
    private static String assetDirectory = null;
    protected TutorialView graphicsView;

    private static android.content.Context applicationContext = null;
    /* [classMembers] */
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        graphicsView = new TutorialView(getApplication());

        /* [onCreateNew] */
        applicationContext = getApplicationContext();
        assetDirectory = applicationContext.getFilesDir().getPath() + "/";

        extractAsset("font.raw");
        extractAsset("font.png");
        extractAsset("font.vert");
        extractAsset("font.frag");
        extractAsset("shader.vert");
        extractAsset("shader.frag");
        extractAsset("CloudAndGloss4x4.astc");
        extractAsset("CloudAndGloss5x4.astc");
        extractAsset("CloudAndGloss5x5.astc");
        extractAsset("CloudAndGloss6x5.astc");
        extractAsset("CloudAndGloss6x6.astc");
        extractAsset("CloudAndGloss8x5.astc");
        extractAsset("CloudAndGloss8x6.astc");
        extractAsset("CloudAndGloss8x8.astc");
        extractAsset("CloudAndGloss10x5.astc");
        extractAsset("CloudAndGloss10x6.astc");
        extractAsset("CloudAndGloss10x8.astc");
        extractAsset("CloudAndGloss10x10.astc");
        extractAsset("CloudAndGloss12x10.astc");
        extractAsset("CloudAndGloss12x12.astc");
        extractAsset("Earth-Color4x4.astc");
        extractAsset("Earth-Color5x4.astc");
        extractAsset("Earth-Color5x5.astc");
        extractAsset("Earth-Color6x5.astc");
        extractAsset("Earth-Color6x6.astc");
        extractAsset("Earth-Color8x5.astc");
        extractAsset("Earth-Color8x6.astc");
        extractAsset("Earth-Color8x8.astc");
        extractAsset("Earth-Color10x5.astc");
        extractAsset("Earth-Color10x6.astc");
        extractAsset("Earth-Color10x8.astc");
        extractAsset("Earth-Color10x10.astc");
        extractAsset("Earth-Color12x10.astc");
        extractAsset("Earth-Color12x12.astc");
        extractAsset("Earth-Night4x4.astc");
        extractAsset("Earth-Night5x4.astc");
        extractAsset("Earth-Night5x5.astc");
        extractAsset("Earth-Night6x5.astc");
        extractAsset("Earth-Night6x6.astc");
        extractAsset("Earth-Night8x5.astc");
        extractAsset("Earth-Night8x6.astc");
        extractAsset("Earth-Night8x8.astc");
        extractAsset("Earth-Night10x5.astc");
        extractAsset("Earth-Night10x6.astc");
        extractAsset("Earth-Night10x8.astc");
        extractAsset("Earth-Night10x10.astc");
        extractAsset("Earth-Night12x10.astc");
        extractAsset("Earth-Night12x12.astc");

        /* [onCreateNew] */
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
    @Override protected void onDestroy()
    {
        super.onDestroy();
    }
    /* [extractAssetBeginning] */
    private void extractAsset(String assetName)
    {
        File fileTest = new File(assetDirectory + assetName);

        if(fileTest.exists())
        {
            Log.d(LOGTAG,assetName +  " already exists. No extraction needed.\n");
        }
        else
        {
            Log.d(LOGTAG, assetName + " doesn't exist. Extraction needed. \n");
            /* [extractAssetBeginning] */
            /* [tryCatch extractAsset] */
            try
            {
                RandomAccessFile out = new RandomAccessFile(assetDirectory + assetName,"rw");
                AssetManager am = applicationContext.getResources().getAssets();
                /* [tryCatch extractAsset] */
                /* [readWriteFile] */
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
                Log.e(LOGTAG, "Failure in extractAssets(): " + e.toString() + " " + assetDirectory+assetName);
            }
            if(fileTest.exists())
            {
                Log.d(LOGTAG,"File extracted successfully");
                /* [readWriteFile] */
            }
        }
    }
}
