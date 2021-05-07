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

#include "Text.h"
#include "AstcTextures.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace AstcTextures
{ 
    const char Text::textureFilename[] = "font.raw";

    const float Text::scale = 1.0f;

    const int Text::textureCharacterWidth = 8;
    const int Text::textureCharacterHeight = 16;

    /* Please see header for specification. */
    void loadData(const char* filename, unsigned char** textureData)
    {
        LOGI("Texture loadData started for %s...\n", filename);

        FILE* file = fopen(filename, "rb");

        if (file == NULL)
        {
            LOGE("Failed to open '%s'\n", filename);
            exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_END);

        unsigned int   length = ftell(file);
        unsigned char* loadedTexture = NULL;

        MALLOC_CHECK(unsigned char*, loadedTexture, length);

        fseek(file, 0, SEEK_SET);

        size_t read = fread(loadedTexture, sizeof(unsigned char), length, file);

        if (read != length)
        {
            LOGE("Failed to read in '%s'\n", filename);
            exit(EXIT_FAILURE);
        }

        fclose(file);

        *textureData = loadedTexture;
    }

    /* Please see header for specification. */
    Text::Text(const char* resourceDirectory, int windowWidth, int windowHeight)
    {
        vertexShaderID     = 0;
        fragmentShaderID   = 0;
        programID          = 0;
        numberOfCharacters = 0;

        textVertex             = NULL;
        textTextureCoordinates = NULL;
        color                  = NULL;
        textIndex              = NULL;

        /* Create an orthographic projection. */
        projectionMatrix = Matrix::matrixOrthographic(0, (float)windowWidth, 0, (float)windowHeight, 0, 1);

        /* Create program object and initialize it. */
        programID = create_program(fontVertexShaderSource, fontFragmentShaderSource);

        GL_CHECK(glUseProgram(programID));

        /* Vertex positions. */
        m_iLocPosition = get_and_check_attrib_location(programID, "a_v4Position");

        /* Text colors. */
        m_iLocTextColor = get_and_check_attrib_location(programID, "a_v4FontColor");

        /* TexCoords. */
        m_iLocTexCoord = get_and_check_attrib_location(programID, "a_v2TexCoord");

        /* Projection matrix. */
        m_iLocProjection = get_and_check_uniform_location(programID, "u_m4Projection");

        GL_CHECK(glUniformMatrix4fv(m_iLocProjection, 1, GL_FALSE, projectionMatrix.getAsArray()));

        /* Set the sampler to point at the 0th texture unit. */
        m_iLocTexture = get_and_check_uniform_location(programID, "u_s2dTexture");

        GL_CHECK(glUniform1i(m_iLocTexture, 0));

        /* Load texture. */
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glGenTextures(1, &textureID));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));

        /* Set filtering. */
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        const int textureLength = strlen(resourceDirectory) + strlen(textureFilename);
        char*     texture       = NULL;

        MALLOC_CHECK(char*, texture, textureLength);
        strcpy(texture, resourceDirectory);
        strcat(texture, textureFilename);

        unsigned char* textureData = NULL;

        loadData(texture, &textureData);

        FREE_CHECK(texture);

        GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 48, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData));

        FREE_CHECK(textureData);
    }

    /* Please see header for specification. */
    void Text::clear(void)
    {
        numberOfCharacters = 0;

        FREE_CHECK(textVertex);
        FREE_CHECK(textTextureCoordinates);
        FREE_CHECK(color);
        FREE_CHECK(textIndex);
    }

    /* Please see header for specification. */
    void Text::addString(int xPosition, int yPosition, const char* string, int red, int green, int blue, int alpha)
    {
        int length = (int)strlen(string);
        int iTexCoordPos = 4 * 2 * numberOfCharacters;
        int iVertexPos = 4 * 3 * numberOfCharacters;
        int iColorPos = 4 * 4 * numberOfCharacters;
        int iIndex = 0;
        int iIndexPos = 0;

        numberOfCharacters += length;

        /* Realloc memory. */
        REALLOC_CHECK(float*,   textVertex,             numberOfCharacters * 4 * 3 * sizeof(float));
        REALLOC_CHECK(float*,   textTextureCoordinates, numberOfCharacters * 4 * 2 * sizeof(float));
        REALLOC_CHECK(float*,   color,                  numberOfCharacters * 4 * 4 * sizeof(float));
        REALLOC_CHECK(GLshort*, textIndex,              (numberOfCharacters * 6 - 2) * sizeof(GLshort));

        /* Re-init entire index array. */
        textIndex[iIndex++] = 0;
        textIndex[iIndex++] = 1;
        textIndex[iIndex++] = 2;
        textIndex[iIndex++] = 3;

        iIndexPos = 4;
        for (int cIndex = 1; cIndex < numberOfCharacters; cIndex ++)
        {
            textIndex[iIndexPos++] = iIndex - 1;
            textIndex[iIndexPos++] = iIndex;
            textIndex[iIndexPos++] = iIndex++;
            textIndex[iIndexPos++] = iIndex++;
            textIndex[iIndexPos++] = iIndex++;
            textIndex[iIndexPos++] = iIndex++;
        }

        for (int iChar = 0; iChar < (signed int)strlen(string); iChar ++)
        {
            char cChar = string[iChar];
            int iCharX = 0;
            int iCharY = 0;
            Vec2 sBottom_left;
            Vec2 sBottom_right;
            Vec2 sTop_left;
            Vec2 sTop_right;

            /* Calculate tex coord for char here. */
            cChar -= 32;
            iCharX = cChar % 32;
            iCharY = cChar / 32;
            iCharX *= textureCharacterWidth;
            iCharY *= textureCharacterHeight;
            sBottom_left.x = iCharX;
            sBottom_left.y = iCharY;
            sBottom_right.x = iCharX + textureCharacterWidth;
            sBottom_right.y = iCharY;
            sTop_left.x = iCharX;
            sTop_left.y = iCharY + textureCharacterHeight;
            sTop_right.x = iCharX + textureCharacterWidth;
            sTop_right.y = iCharY + textureCharacterHeight;

            /* Add vertex position data here. */
            textVertex[iVertexPos++] = xPosition + iChar * textureCharacterWidth * scale;
            textVertex[iVertexPos++] = (float)yPosition;
            textVertex[iVertexPos++] = 0;

            textVertex[iVertexPos++] = xPosition + (iChar + 1) * textureCharacterWidth * scale;
            textVertex[iVertexPos++] = (float)yPosition;
            textVertex[iVertexPos++] = 0;

            textVertex[iVertexPos++] = xPosition + iChar * textureCharacterWidth * scale;
            textVertex[iVertexPos++] = yPosition + textureCharacterHeight * scale;
            textVertex[iVertexPos++] = 0;

            textVertex[iVertexPos++] = xPosition + (iChar + 1) * textureCharacterWidth * scale;
            textVertex[iVertexPos++] = yPosition + textureCharacterHeight * scale;
            textVertex[iVertexPos++] = 0;

            /* Texture coords here. Because textures are read in upside down, flip Y coords here. */
            textTextureCoordinates[iTexCoordPos++] = sBottom_left.x / 256.0f;
            textTextureCoordinates[iTexCoordPos++] = sTop_left.y / 48.0f;

            textTextureCoordinates[iTexCoordPos++] = sBottom_right.x / 256.0f;
            textTextureCoordinates[iTexCoordPos++] = sTop_right.y / 48.0f;

            textTextureCoordinates[iTexCoordPos++] = sTop_left.x / 256.0f;
            textTextureCoordinates[iTexCoordPos++] = sBottom_left.y / 48.0f;

            textTextureCoordinates[iTexCoordPos++] = sTop_right.x / 256.0f;
            textTextureCoordinates[iTexCoordPos++] = sBottom_right.y / 48.0f;

            /* Color data. */
            color[iColorPos++] = red / 255.0f;
            color[iColorPos++] = green / 255.0f;
            color[iColorPos++] = blue / 255.0f;
            color[iColorPos++] = alpha / 255.0f;

            /* Copy to the other 3 vertices. */
            memcpy(&color[iColorPos],     &color[iColorPos - 4], 4 * sizeof(float));
            memcpy(&color[iColorPos + 4], &color[iColorPos],     4 * sizeof(float));
            memcpy(&color[iColorPos + 8], &color[iColorPos + 4], 4 * sizeof(float));
            iColorPos += 3 * 4;
        }
    }

    /* Please see header for specification. */
    void Text::draw(void)
    {
        /* Push currently bound vertex array object. */
        GLint vertexArray = 0;

        GL_CHECK(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArray));
        GL_CHECK(glBindVertexArray(0));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        /* Push currently used program object. */
        GLint currentProgram = 0;

        GL_CHECK(glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram));

        GL_CHECK(glUseProgram(programID));

        if (m_iLocPosition == -1 || m_iLocTextColor == -1 || m_iLocTexCoord == -1 || m_iLocProjection == -1)
        {
            LOGI("At least one of the attributes and/or uniforms is missing. Have you invoked Text(const char*, int, int) constructor?");
            exit(EXIT_FAILURE);
        }

        if (numberOfCharacters == 0)
        {
            return;
        }

        GL_CHECK(glEnableVertexAttribArray(m_iLocPosition));
        GL_CHECK(glEnableVertexAttribArray(m_iLocTextColor));
        GL_CHECK(glEnableVertexAttribArray(m_iLocTexCoord));

        GL_CHECK(glVertexAttribPointer(m_iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, textVertex));
        GL_CHECK(glVertexAttribPointer(m_iLocTextColor, 4, GL_FLOAT, GL_FALSE, 0, color));
        GL_CHECK(glVertexAttribPointer(m_iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textTextureCoordinates));
        GL_CHECK(glUniformMatrix4fv(m_iLocProjection, 1, GL_FALSE, projectionMatrix.getAsArray()));

        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));

        GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, numberOfCharacters * 6 - 2, GL_UNSIGNED_SHORT, textIndex));

        GL_CHECK(glDisableVertexAttribArray(m_iLocTextColor));
        GL_CHECK(glDisableVertexAttribArray(m_iLocTexCoord));
        GL_CHECK(glDisableVertexAttribArray(m_iLocPosition));

        /* Pop previously used program object. */
        GL_CHECK(glUseProgram(currentProgram));

        /* Pop previously bound vertex array object. */
        GL_CHECK(glBindVertexArray(vertexArray));
    }

    /* Please see header for specification. */
    Text::Text() :
        m_iLocPosition(-1),
        m_iLocProjection(-1),
        m_iLocTextColor(-1),
        m_iLocTexCoord(-1),
        m_iLocTexture(-1),
        vertexShaderID(0),
        fragmentShaderID(0),
        programID(0),
        textureID(0)
    {
        clear();
    }

    /* Please see header for specification. */
    Text::~Text(void)
    {
        clear();
        
        GL_CHECK(glDeleteTextures(1, &textureID));
    }
}
