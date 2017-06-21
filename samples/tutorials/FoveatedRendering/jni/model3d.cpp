/* Copyright (c) 2017, ARM Limited and Contributors
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

#include <model3d.h>
#include <android/log.h>
#define LOG_TAG "Asset_Loader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


#include <iostream>
#include <fstream>

namespace Model3D
{
    bool load_file(const std::string& a_path, char **a_buffer, int &a_bytes_read, const bool binary)
    {
        std::fstream file;
		if(binary)
			file.open(a_path.c_str(), std::ios::in | std::ios::binary);
		else
			file.open( a_path.c_str(), std::ios::in );

        if (!file.is_open())
        {
            LOGE("Error! opening file %s", a_path.c_str());
            return false;
        }

        file.seekg(0, std::ios_base::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios_base::beg);

        if(file_size == 0)
        {
			LOGE( "Error! reading file size %s ", a_path.c_str());
            return false;
        }

        *a_buffer = new char[file_size];

        if(*a_buffer == NULL)
        {
			LOGE( "Error! Out of memory allocating *a_buffer");
            return false;
        }

        file.read(*a_buffer, file_size);
        file.close();

        a_bytes_read = static_cast<int>(file_size);

        return true;
    }

    Material::Material()
    {
        this->m_ambient      = NULL;
        this->m_diffuse      = NULL;
        this->m_specular     = NULL;
        this->m_emmision     = NULL;
        this->m_shine        = NULL;
        this->m_transparency = NULL;
    }

    Material::~Material()
    {
        this->m_ambient      = NULL;
        this->m_diffuse      = NULL;
        this->m_specular     = NULL;
        this->m_emmision     = NULL;
        this->m_shine        = NULL;
        this->m_transparency = NULL;
    }

    Keyframe::Keyframe()
    {
        this->m_time        = NULL;
        this->m_transforms  = NULL;
    }

    Keyframe::~Keyframe()
    {
        this->m_time        = NULL;
        this->m_transforms  = NULL;
    }

    Model3D::Model3D()
    {
        this->m_has_animation            = false;
        this->m_has_materials            = false;
        this->m_has_indices              = false;
        this->m_has_normals              = false;
        this->m_has_texture_coordinates0 = false;

        this->m_vertices_count          = NULL;
        this->m_indices_count           = NULL;
        this->m_bones_count             = NULL;
        this->m_keyframes_count         = NULL;
        this->m_materials_count         = NULL;

        this->m_positions               = NULL;
        this->m_texture_coordinates0    = NULL;
        this->m_normals                 = NULL;
        this->m_bone_ids                = NULL;
        this->m_weights                 = NULL;
        this->m_materials               = NULL;
        this->m_indices                 = NULL;
        this->m_keyframes               = NULL;

        this->m_geometry_buffer         = NULL;
        this->m_tangent_buffer          = NULL;
        this->m_animation_buffer        = NULL;

        this->m_bounding_box_minimum    = NULL;
        this->m_bounding_box_maximum    = NULL;
    }

    Model3D::~Model3D()
    {
        if (this->m_has_materials)
        {
            for (unsigned int i = 0; i < *this->m_materials_count; ++i)
            {
                delete this->m_materials[i];
            }
            delete[] this->m_materials;
        }

        if (this->m_has_animation)
        {
            for (unsigned int i = 0; i < *this->m_keyframes_count; ++i)
            {
                delete this->m_keyframes[i];
            }
            delete[] this->m_keyframes;
        }

        this->m_vertices_count   = NULL;
        this->m_indices_count    = NULL;
        this->m_bones_count      = NULL;
        this->m_keyframes_count  = NULL;
        this->m_materials_count  = NULL;

        this->m_positions               = NULL;
        this->m_texture_coordinates0    = NULL;
        this->m_normals                 = NULL;
        this->m_bone_ids                = NULL;
        this->m_weights                 = NULL;
        this->m_materials               = NULL;
        this->m_indices                 = NULL;
        this->m_keyframes               = NULL;

        this->m_bounding_box_minimum    = NULL;
        this->m_bounding_box_maximum    = NULL;

		delete[] this->m_geometry_buffer;
		delete[] this->m_tangent_buffer;
        delete[] this->m_animation_buffer;

        this->m_geometry_buffer      = NULL;
        this->m_tangent_buffer       = NULL;
        this->m_animation_buffer     = NULL;
    }

    bool Model3D::load(const std::string& a_path)
    {
        int bytes_read;
        if (!load_file(a_path, &this->m_geometry_buffer, bytes_read))
        {
            LOGE("Error! opening geometry file %s", a_path.c_str());
            return false;
		}

        char *buffer = this->m_geometry_buffer;

        // test the magic_id for 'geom'
        if (buffer[0] != 'g' ||
            buffer[1] != 'e' ||
            buffer[2] != 'o' ||
            buffer[3] != 'm')
        {
			LOGE( "%s is not a valid 'geom' file", a_path.c_str() );
            delete []this->m_geometry_buffer;
            return false;
        }

        buffer += sizeof(unsigned int);

        this->m_has_indices                 = *((unsigned int*)buffer) & 1;
        this->m_has_texture_coordinates0    = *((unsigned int*)buffer) & (1 << 8);
        this->m_has_materials               = *((unsigned int*)buffer) & (1 << 12);
        this->m_has_normals                 = *((unsigned int*)buffer) & (1 << 16);
        this->m_has_animation               = *((unsigned int*)buffer) & (1 << 24);

        buffer += sizeof(unsigned int);

		buffer += sizeof( float ) * 16 ;

        // Read bounding box
        this->m_bounding_box_minimum = (float*)buffer;
        buffer += (sizeof(float) * 3);
        this->m_bounding_box_maximum = (float*)buffer;
        buffer += (sizeof(float) * 3);

        // Read vertices count
		this->m_vertices_count = (unsigned int*) buffer;
        buffer += sizeof(unsigned int);
        this->m_positions = (float*)buffer;
        buffer += (sizeof(float) * 3 * (*this->m_vertices_count));

		LOGI( "Vx count=%u", *this->m_vertices_count );

        // If texture coordinates exist with this geometry then set the pointers
        if (this->m_has_texture_coordinates0)
        {
            this->m_texture_coordinates0 = (float*)buffer;
            buffer += (sizeof(float) * 3 * (*this->m_vertices_count));
        }

        // If normals exist with this geometry then load it
        if (this->m_has_normals)
        {
            this->m_normals = (float*)buffer;
            buffer += (sizeof(float) * 3 * (*this->m_vertices_count));
        }

        // If animation exists then it must have bone ids and vertex weights we need to load that
        if (this->m_has_animation)
        {
            // Bone Ids
            this->m_bone_ids = (unsigned int*)buffer;
            buffer += (sizeof(unsigned int) * 4 * (*this->m_vertices_count));

            // Vertex Weights
            this->m_weights = (float*)buffer;
            buffer += (sizeof(float) * 4 * (*this->m_vertices_count));
        }

        // If the data is indexed then load set the pointers
        if (this->m_has_indices)
        {
            this->m_indices_count = (unsigned int*)buffer;
            buffer += sizeof(unsigned int);
            this->m_indices = (unsigned int*)buffer;
            buffer += (sizeof(unsigned int) * 3 * (*this->m_indices_count));
        }

        // If there are materials we need to set the pointers
        if (this->m_has_materials)
        {
            this->m_materials_count = (unsigned int*)buffer;
            buffer += sizeof(unsigned int);

            this->m_materials = new Material*[*this->m_materials_count];

            for (unsigned int i = 0; i < *this->m_materials_count; ++i)
            {
                this->m_materials[i] = new Material();

                this->m_materials[i]->m_ambient      = (float*)buffer; buffer += (sizeof(float) * 4);
                this->m_materials[i]->m_diffuse      = (float*)buffer; buffer += (sizeof(float) * 4);
                this->m_materials[i]->m_specular     = (float*)buffer; buffer += (sizeof(float) * 4);
                this->m_materials[i]->m_emmision     = (float*)buffer; buffer += (sizeof(float) * 4);
                this->m_materials[i]->m_shine        = (float*)buffer; buffer += sizeof(float);
                this->m_materials[i]->m_transparency = (float*)buffer; buffer += sizeof(float);
            }
        }

		// Read bitangents from file
		if ( this->m_has_normals && this->m_has_texture_coordinates0 )
		{
			int bytes_read_tan;
			if ( !load_file( a_path + "tan", &this->m_tangent_buffer, bytes_read_tan ) )
			{
				LOGE( "Error! opening tangent file %s", (a_path + "tan").c_str() );
				return false;
			}

			char *bufferTan = this->m_tangent_buffer;

			// test the magic_id for 'geomtan'
			if ( bufferTan[0] != 'g' ||
				 bufferTan[1] != 'e' ||
				 bufferTan[2] != 'o' ||
				 bufferTan[3] != 'm' ||
				 bufferTan[4] != 't' ||
				 bufferTan[5] != 'a' ||
				 bufferTan[6] != 'n' )
			{
				LOGE( "%s is not a valid 'geomtan' file", (a_path + "tan").c_str() );
				delete[]this->m_tangent_buffer;
				return false;
			}

			bufferTan += 7;
			this->m_tangents = (float*) bufferTan;
		}
		LOGI("Finished loading");

        // If animation exists with this geometry then set the pointers
        if (this->m_has_animation)
        {
            // This assumes the animation file is the same name but instead of .geom the extension is .anim
            std::string animation_filename = a_path.substr(0, a_path.length() - 5);
            animation_filename = animation_filename.append(".anim");

            if (!load_file(animation_filename, &this->m_animation_buffer, bytes_read))
            {
                LOGE( "Error! opening animation file %s", animation_filename.c_str());
                return false;
            }

            char *buffer = this->m_animation_buffer;

            // test the magic_id for 'anim'
            if (buffer[0] != 'a' ||
                buffer[1] != 'n' ||
                buffer[2] != 'i' ||
                buffer[3] != 'm')
            {
				LOGE( "%s is not a valid 'anim' file", a_path.c_str());
                delete []this->m_animation_buffer;
                return false;
            }

            buffer += sizeof(unsigned int);

            // Set bones count
            this->m_bones_count = (unsigned int*)buffer;
            buffer += sizeof(unsigned int);

            // Set keyframes count
            this->m_keyframes_count = (unsigned int*)buffer;
            buffer += sizeof(unsigned int);

            this->m_keyframes = new Keyframe*[*this->m_keyframes_count];
            char *transforms_buffer = buffer + (*this->m_keyframes_count * sizeof(float));

            for (unsigned int i = 0; i < *this->m_keyframes_count; ++i)
            {
                this->m_keyframes[i] = new Keyframe();
                this->m_keyframes[i]->m_time = (float*)buffer;
                this->m_keyframes[i]->m_transforms = (float*)transforms_buffer;

                buffer += sizeof(float);
                transforms_buffer += sizeof(float) * 16 * (*this->m_bones_count);
            }
		}
		LOGI( "Here" );

        return true;
    }

    unsigned int Model3D::get_indices_count() const
    {
        return *this->m_indices_count;
    }

    unsigned int Model3D::get_keyframes_count() const
    {
        return *this->m_keyframes_count;
    }

    float* Model3D::get_positions() const
    {
        return this->m_positions;
    }

    float* Model3D::get_normals() const
    {
        return this->m_normals;
    }

    float* Model3D::get_texture_coordinates0() const
    {
        return this->m_texture_coordinates0;
    }

    float* Model3D::get_tangents() const
    {
        return this->m_tangents;
    }

    unsigned int* Model3D::get_indices() const
    {
        return this->m_indices;
    }
}
