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

#ifndef HDR_IMAGE_LOADER_H
#define HDR_IMAGE_LOADER_H

namespace MaliSDK
{
    /**
     * \brief Class to load an manage HDR images.
     *
     * This class implements a loader for the Picture Radiance format.
     * Will only load HDR images with FORMAT=32-bit_rle_rgbe and coordinates specified in -Y +X.
     * See http://radsite.lbl.gov/radiance/refer/filefmts.pdf for more information.
     */
    class HDRImage
    {
        public:
            /**
             * \brief Default constructor.
             */
            HDRImage(void);

            /**
             * \brief Constructor which loads a HDR image from a file.
             *
             * \param[in] filePath The path to the HDR image to load.
             */
            HDRImage(const std::string& filePath);

            /**
             * \brief Copy constructor to copy the contents of one HDRImage to another.
             *
             * \param[in] another The HDRImage to copy from.
             */
            HDRImage(HDRImage& another);

            /**
             * \brief Destructor.
             */
            virtual ~HDRImage(void);

            /**
             * \brief Load a HDRImage from a file.
             *
             * \param[in] filePath The path to the HDR image to load.
             */
            void loadFromFile(const std::string& filePath);
          
            /**
             * \brief Overloading assignment operater to do deep copy of the HDRImage data.
             *
             * \param[in] another The HDRImage to copy from.
             */
            HDRImage& operator=(const HDRImage &another);

            /**
             * \brief The HDR image data.
             *
             * Data is stored a floating point RBG values for all the pixels.
             * Total size is width * height * 3 floating point values.
             */
            float* rgbData;

            /**
             * \brief The width of the HDR image.
             */
            int width;

            /**
             * \brief The height of the HDR image.
             */
            int height;

        private:

            struct RGBEPixel
            {
                unsigned char r;
                unsigned char g;
                unsigned char b;
                unsigned char e;
            };

            static void convertRGBEPixel(const RGBEPixel& pixel, float* rgbData);

            static float convertSingleComponent(unsigned char value, int exponent);

            static bool decodeLine(FILE* file, int lineLength, RGBEPixel* scanLine);

            static void writeDecodedComponent(int componentIndicator, unsigned char value, RGBEPixel* pixel);
    };

}
#endif /* HDR_IMAGE_LOADER_H */