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

#ifndef __MODEL3D_H_INCLUDED__
#define __MODEL3D_H_INCLUDED__

#include <string>
#include <vector>

namespace Model3D
{
    /**
    Use this method to load any file from the filesystem
    This method will return a buffer, clients must free the a_buffer afterwards
    @param a_path specifies the file to load
    @param a_buffer will be used to load the data from the file as bytes
    @param a_bytes_read output the total number of bytes read
    @param binary read the file in binary mode
    @code
    std::string path="path/to/any/file";
    char **buffer = NULL;
    return_code = load_file(path, buffer);
    @endcode
    @return true if the file is loaded successfully otherwise look for error messages in std::out
    */
    bool load_file(const std::string& a_path, char **a_buffer, int &a_bytes_read, const bool binary = true);

    /**
    Objects of Material class contains material definitions
    A model can have zero or more materials
    if one or more materials are available then the third component
    of the first texture coordinates will contain the material id.
    all the materials available in the model can be sent as a uniform
    buffer object to the shaders indexed by the material id.
    @ingroup FrescoModel3D
    */

    class Material
    {
    public:
        /**
        Default constructor sets all pointers to NULL
        */
        Material();

        /**
        Default destructor sets all pointers to NULL
        */
        ~Material();

        float   *m_ambient;         //!< 4D Ambient color of the material
        float   *m_diffuse;         //!< 4D Diffuse color of the material
        float   *m_specular;        //!< 4D Specular color of the material
        float   *m_emmision;        //!< 4D Emissive color of the material
        float   *m_shine;           //!< Shine exponent of the material
        float   *m_transparency;    //!< Transparency of the material
    };


    /**
    Objects of Keyframe class contains transformation data for each
    bone in the model for a specific time. This is also called "keyframe"
    in 3d authoring tools like studio max or maya.
    @ingroup FrescoModel3D
    */

    class Keyframe
    {
    public:
        /**
        Default constructor sets all pointers to NULL
        */
        Keyframe();

        /**
        Default destructor sets all pointers to NULL
        */
        ~Keyframe();

        float   *m_time;        //!< Keyframe time in seconds for the all the bones
        float   *m_transforms;  //!< Transformations for all the bones in the current keyframe
    };

    /**
    Objects of Model3D class contains one complete model.
    This class can be used to load *.geom files from filesystem.
    A model might contain only geometry or geometry and animation.
    Geometry data consists of 3D Positions, One or more set of 3D Texture Coordinates,
    3D Normals, 3D Tangents and 3D Bitangents etc. Positions are a must but all other
    attributes are optional.
    A model might also contain optional one or more materials.
    If the model has animation data. It will be automatically loaded from the *.anim file.
    A model can have upto 50 bones. Each vertex can be influenced by a maximum of 4 bones.
    Animation data consists of keyframes for all bones.
    Each keyframe is a set of transformations for each bone and time.
    There can be one or more keyframes in an animation.
    All these classes are optimized for load time.
    @ingroup FrescoModel3D
    */

    class Model3D
    {
    public:

        friend class Model3DClientGL;

        /**
        Default constructor sets all pointers to NULL
        */
        Model3D();

        /**
        Default destructor sets all pointers to NULL
        It also frees the momory allocated to m_geometry_buffer and m_animation_buffer
        */
        ~Model3D();

        /**
        Use this method to load a 'geom' file from filesystem
        If the model has animation data. It will be automatically loaded from the '.anim' file.
        the '.anim' file is searched in the folder where the '.geom' resides.
        @warning Data written with model3d exporter should have all member variables properly aligned otherwise this will fail.
        @param a_path full path to the file containing the geometry data on the filesystem
        @return true if loading was successfull, false otherwise and look for error messages in the console
        */
        bool load(const std::string &a_path);

        /**
        This method returns the indices count in the model
        @return indices count
        */
        unsigned int get_indices_count() const;

        /**
        This method returns the keyframes count in the model
        @return keyframes count
        */
        unsigned int get_keyframes_count() const;

        /**
        This method returns a list of 3D Vertex Positions in the model
        @return 3D vertex positions
        */
        float* get_positions() const;

        /**
        This method returns a list of 3D Normals in the model
        @return 3D normals
        */
        float* get_normals() const;

        /**
        This method returns a list of 3D Texture Coordinates in the model
        @return 3D texture coordinates
        */
        float* get_texture_coordinates0() const;

        /**
        This method returns a list of 3D Tangents in the model
        @return 3D tangents
        */
        float* get_tangents() const;

        /**
        This method returns a list of indices in the model
        @return indices
        */
        unsigned int* get_indices() const;


    protected:
        bool            m_has_animation;            //!< True if the model has animation data
        bool            m_has_materials;            //!< True if the model has one or more materials
        bool            m_has_indices;              //!< True if the model has indexed data
        bool            m_has_normals;              //!< True if the model has normals
        bool            m_has_texture_coordinates0; //!< True if the model has texture coordinates 0

        unsigned int    *m_vertices_count;          //!< Total number of vertices in the model
        unsigned int    *m_indices_count;           //!< Total number of indices in the model
        unsigned int    *m_bones_count;             //!< Total number of bones in the model
        unsigned int    *m_keyframes_count;         //!< Total number of keyframes in the model
        unsigned int    *m_materials_count;         //!< Total number of materials in the model

        float           *m_positions;               //!< List of 3D Vertex Positions in the model
        float           *m_texture_coordinates0;    //!< List of 3D Texture Coordinates in the model
        float           *m_normals;                 //!< List of 3D Normals in the model
        float           *m_tangents;                //!< List of 3D Tangents in the model
        float           *m_weights;                 //!< List of 4D Weights in the model
        unsigned int    *m_bone_ids;                //!< List of 4D Bone Ids in the model
        Material        **m_materials;              //!< List of all the materials in the model

        unsigned int    *m_indices;                 //!< List of indices in the model

        Keyframe        **m_keyframes;              //!< List of all keyframes for all bones

        char            *m_geometry_buffer;         //!< All the geometry data is loaded in this buffer
        char            *m_tangent_buffer;          //!< All the tangent data is loaded in this buffer
        char            *m_animation_buffer;        //!< All the animation data is loaded in this buffer

        float           *m_bounding_box_minimum;    //!< Bounding box minimum of the model
        float           *m_bounding_box_maximum;    //!< Bounding box maximum of the model
    };
}

#endif // __MODEL3D_H_INCLUDED__
