/* Copyright (c) 2012-2017, ARM Limited and Contributors
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

package com.arm.malideveloper.openglessdk;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import android.os.Bundle;
import android.util.Log;
import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;

public abstract class MaliSamplesActivity extends Activity
{
    public static android.content.Context mAppContext = null;
    protected static String LOG_TAG = "MaliSamplesActivity";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        mAppContext = getApplicationContext();

        /* Extract font assets. */
        extractAssetToRes("font.frag");
        extractAssetToRes("font.png");
        extractAssetToRes("font.raw");
        extractAssetToRes("font.vert");
    }

    private void extractAssetToRes(String filename)
    {
        String res = "/data/data/"+this.getPackageName();
        extractAsset(res, filename);
    }

    public static int[] openAsset( String path )
    {
        AssetFileDescriptor ad = null;
        try
        {
            if ( null == mAppContext )
            {
                return null;
            }
            ad = mAppContext.getResources().getAssets().openFd( path );
            int off = (int) ad.getStartOffset();
            int len = (int) ad.getLength();
            int res[] = { off, len };
            ad.close();
            return res;
        } 
        catch(IOException e)
        {
            Log.e(LOG_TAG, "MaliSamples.openAsset(): " + e.toString());
        }
        return null;
    }

    public static void extractAsset(String resource_file_dir, String asset_path)
    {
        String resource_file_path = resource_file_dir + "/" + asset_path;
        try
        {
            File resfile = new File(resource_file_path);
            if(resfile.exists())
            {
                return;
            }
            File dirs = new File(resource_file_dir);
            dirs.mkdirs();
            resfile.createNewFile();
            RandomAccessFile out = new RandomAccessFile(resfile, "rw");
            
             /* Get the local asset manager. */
            AssetManager am = mAppContext.getResources().getAssets();
    
            /* Open the input stream for reading. */
            InputStream inputStream = am.open( asset_path ); 
    
            byte buffer[] = new byte[1024];
            int count = inputStream.read(buffer, 0, 1024);
            
            /* Do the reading. */
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
            Log.e(LOG_TAG, "MaliSamples.extractAsset(): " + e.toString() + " " + resource_file_path);
        }
        Log.d(LOG_TAG, resource_file_path);
        File resfile = new File(resource_file_path);
        if(resfile.exists())
        {
            Log.d(LOG_TAG, "File loaded successfully");
        }
    }
}