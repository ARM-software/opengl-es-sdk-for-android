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

#ifndef TEXT_H
#define TEXT_H

#include "Matrix.h"

#if GLES_VERSION == 2
#include <GLES2/gl2.h>
#elif GLES_VERSION == 3
#include <GLES3/gl3.h>
#else 
#error "GLES_VERSION must be defined as either 2 or 3"
#endif

#include <string>

namespace MaliSDK
{
    /**
     * \brief Functions for drawing text in OpenGL ES
     *
     * Uses a texture with images of alphanumeric and punctuation symbols.
     * The class converts strings into texture coordinates in order to render the correct symbol from the texture for each character of the string.
     */
    class Text
    {
    private:
        static const std::string textureFilename;
        static const std::string vertexShaderFilename;
        static const std::string fragmentShaderFilename;
        
        /**
         * \brief Scaling factor to use when rendering the text. 
         * \warning 
         * Experimental: allows drawing characters bigger than the texture was.
         * Range 0.75-3.0 seems to work reasonably.
         */
        static const float scale;
        

        
        Matrix projectionMatrix;
        int numberOfCharacters;
        float *textVertex;
        float *textTextureCoordinates;
        float *color;
        GLshort *textIndex;
        int m_iLocPosition;
        int m_iLocProjection;
        int m_iLocTextColor;
        int m_iLocTexCoord;
        int m_iLocTexture;
        GLuint vertexShaderID;
        GLuint fragmentShaderID;
        GLuint programID;
        GLuint textureID;

    public: 

        /**
         * \brief The width (in pixels) of the characters in the text texture.
         * \warning Change only if the text texture is changed and the width of the characters is different.
         */
        static const int textureCharacterWidth;
        
        /**
         * \brief The height (in pixels) of the characters in the text texture.
         * \warning Change only if the text texture is changed and the height of the characters is different.
         */
        static const int textureCharacterHeight;
        /**
         * \brief Constructor for Text.
         * \param[in] resourceDirectory Path to the resources. Where the textures and shaders are located.
         * \param[in] windowWidth The width of the window (in pixles) that the text is being used in.
         * \param[in] windowHeight The height of the window (in pixles) that the text is being used in.
         */
        Text(const char * resourceDirectory, int windowWidth, int windowHeight);
        
        /**
         * \brief Default destructor.
         */
        virtual ~Text(void);
        
        /**
         * \brief Removes the current string from the class.
         *
         * Should be called before adding a new string to render using addString().
         */
        void clear(void);

        /**
         * \brief Add a std::string to be drawn to the screen.
         * \param[in] xPosition The X position (in pixels) to start drawing the text. Measured from the left of the screen.
         * \param[in] yPosition The Y position (in pixels) to start drawing the text. Measured from the bottom of the screen.
         * \param[in] string The string to be rendered on the screen.
         * \param[in] red The red component of the text colour (accepts values 0-255).
         * \param[in] green The green component of the text colour (accepts values 0-255).
         * \param[in] blue The blue component of the text colour (accepts values 0-255).
         * \param[in] alpha The alpha component of the text colour (accepts values 0-255). Affects the transparency of the text.
         */
        void addString(int xPosition, int yPosition, const char *string, int red, int green, int blue, int alpha);

        /**
         * \brief Draw the text to the screen.
         * 
         * Should be called each time through the render loop so that the text is drawn every frame.
         */
        void draw(void);
    };
}
#endif /* TEXT_H */