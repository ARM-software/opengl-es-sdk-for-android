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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Skybox.h"
#include "Image.h"

/* Please see the header for specification. */
ImageFile load_ppm_file(const char* ppm_file_name)
{
    ImageFile image = { 0, 0, NULL };

    /* Constant numbers. */
    int max_color_value        = 255;
    int num_of_bytes_per_pixel = 3;
    int read_buffer_length     = 256;

    /* Constant strings. */
    const char comment_id    = '#';
    const char pixmap_mark[] = "P6\n";

    int   id_255          = 0;
    int   height          = 0;
    char* line_str        = NULL;
    int   n_loaded_pixels = 0;
    char* pixels          = NULL;
    char* returned_str    = NULL;
    int   returned_value  = 0;
    int   width           = 0;

    FILE* pFile = fopen(ppm_file_name, "rb");

    if (pFile == NULL)
    {
        LOGF("Error opening .ppm file.");

        exit(EXIT_FAILURE);
    }

    MALLOC_CHECK(char*, line_str, read_buffer_length);

    /* Read the first line. */
    returned_str = fgets(line_str, read_buffer_length, pFile);

    if (returned_str == NULL)
    {
    	LOGF("Error reading .ppm file.");

        exit(EXIT_FAILURE);
    }

    /* Verify whether the file begins with a "magic number" identifying the .ppm file type. */
    returned_value = strncmp(line_str, pixmap_mark, sizeof(pixmap_mark));

    if (returned_value != 0)
    {
    	LOGF("File does not contain P6 string in the header.");

    	exit(EXIT_FAILURE);
    }

    returned_str = fgets(line_str, read_buffer_length, pFile);

    /* Ignore any comments after P6 identifier, beginning with '#' */
    while (strncmp(line_str, &comment_id, sizeof(comment_id)) == 0)
    {
        returned_str = fgets(line_str, read_buffer_length, pFile);

        if (returned_str == NULL)
        {
        	LOGF("Error reading .ppm file.");

        	exit(EXIT_FAILURE);
        }
    }

    /* Read the pixmap dimensions. */
    returned_value = sscanf(line_str, "%d %d", &width, &height);

    /* Make sure both width and height have been read correctly. */
    if (returned_value != 2)
    {
    	LOGF("Error reading image width/height from the .ppm file.");

    	exit(EXIT_FAILURE);
    }

    /* Check if the maximum color value is 255. */
    returned_value = fscanf(pFile, "%d", &id_255);

    if (!(returned_value == 1 && id_255 == max_color_value))
    {
    	LOGF("Error reading 255 mark in the .ppm file.");

    	exit(EXIT_FAILURE);
    }

    fseek(pFile, 1, SEEK_CUR);

    /* Each pixel consists of 3 bytes for GL_RGB storage. */
    pixels = (char*) calloc(width * height, num_of_bytes_per_pixel);

    if (pixels == NULL)
    {
    	LOGF("Error allocating memory for pixels buffer.");

    	exit(EXIT_FAILURE);
    }

    /* Load image into the pixel buffer. */
    n_loaded_pixels = fread(pixels, num_of_bytes_per_pixel, width * height, pFile);

    if (n_loaded_pixels != width * height)
    {
    	LOGF("Error reading .ppm file.");

    	exit(EXIT_FAILURE);
    }

    /* Finally, put all needed info into the Image struct. */
    image.width  = width;
    image.height = height;
    image.pixels = pixels;

    FREE_CHECK(line_str);

    return image;
}
