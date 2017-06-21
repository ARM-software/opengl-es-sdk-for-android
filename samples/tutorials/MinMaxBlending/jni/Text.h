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

#ifndef TEXT_H
#define TEXT_H

#include "Matrix.h"

#include <GLES3/gl3.h>

namespace AstcTextures
{
    /**
     * \brief Vertex shader source code for text rendering.
     */
    const char fontVertexShaderSource[] =
    {
        "#version 300 es\n"
        "uniform   mat4 u_m4Projection;\n"
        "in        vec4 a_v4Position;\n"
        "in        vec4 a_v4FontColor;\n"
        "in        vec2 a_v2TexCoord;\n"
        "out       vec4 v_v4FontColor;\n"
        "out       vec2 v_v2TexCoord;\n"
        "void main() {\n"
        "    v_v4FontColor = a_v4FontColor;\n"
        "    v_v2TexCoord  = a_v2TexCoord;\n"
        "    gl_Position   = u_m4Projection * a_v4Position;\n"
        "}\n"
    };

    /**
     * \brief Fragment shader source code for text rendering.
     */
    const char fontFragmentShaderSource[] =
    {
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform  sampler2D u_s2dTexture;\n"
        "in       vec2      v_v2TexCoord;\n"
        "in       vec4      v_v4FontColor;\n"
        "out      vec4      color;\n"
        "void main() {\n"
        "    vec4 v4Texel = texture(u_s2dTexture, v_v2TexCoord);\n"
        "    color = v_v4FontColor * v4Texel;\n"
        "}\n"
    };

    /**
     * \brief Load texture data from a file into memory.
     *
     * \param[in]  filename    The filename of the texture to load.
     * \param[out] textureData Pointer to the texture that has been loaded.
     */
    void loadData(const char* filename, unsigned char** textureData);

    /**
     * \brief Type representing texture coordinates.
     */
    typedef struct Vec2
    {
        int x;
        int y;
    } Vec2;

    /**
     * \brief Functions for drawing text in OpenGL ES
     *
     * Uses a texture with images of alphanumeric and punctuation symbols.
     * The class converts strings into texture coordinates in order to render the correct symbol from the texture for each character of the string.
     */
    class Text
    {
        private:
            static const char textureFilename[];
            static const char vertexShaderFilename[];
            static const char fragmentShaderFilename[];

            /**
             * \brief Scaling factor to use when rendering the text. 
             * \warning 
             * Experimental: allows drawing characters bigger than the texture was.
             * Range 0.75-3.0 seems to work reasonably.
             */
            static const float scale;

            Matrix projectionMatrix;
            int numberOfCharacters;
            float* textVertex;
            float* textTextureCoordinates;
            float* color;
            GLshort* textIndex;
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
             *
             * \param[in] resourceDirectory Path to the resources. Where the textures and shaders are located.
             * \param[in] windowWidth The width of the window (in pixles) that the text is being used in.
             * \param[in] windowHeight The height of the window (in pixles) that the text is being used in.
             */
            Text(const char* resourceDirectory, int windowWidth, int windowHeight);

            /**
             * \brief Overloaded default constructor.
             */
            Text(void);

            /**
             * \brief Overloaded default destructor.
             */
            ~Text(void);

            /**
             * \brief Removes the current string from the class.
             *
             * Should be called before adding a new string to render using addString().
             */
            void clear(void);

            /**
             * \brief Add a std::string to be drawn to the screen.
             *
             * \param[in] xPosition The X position (in pixels) to start drawing the text. Measured from the left of the screen.
             * \param[in] yPosition The Y position (in pixels) to start drawing the text. Measured from the bottom of the screen.
             * \param[in] string    The string to be rendered on the screen.
             * \param[in] red       The red component of the text colour (accepts values 0-255).
             * \param[in] green     The green component of the text colour (accepts values 0-255).
             * \param[in] blue      The blue component of the text colour (accepts values 0-255).
             * \param[in] alpha     The alpha component of the text colour (accepts values 0-255). Affects the transparency of the text.
             */
            void addString(int xPosition, int yPosition, const char* string, int red, int green, int blue, int alpha);

            /**
             * \brief Draw the text to the screen.
             * 
             * Should be called each time through the render loop so that the text is drawn every frame.
             */
            void draw(void);
    };
}
#endif /* TEXT_H */
