#ifndef _OBJ_LOADER_HPP_
#define _OBJ_LOADER_HPP_

#include <array>
#include <vector>
#include <string>

typedef std::array<float, 3> float3;
typedef std::array<float, 4> float4;

struct model
{
    std::vector<float4> vertices;
    std::vector<float3> texcoords;
    std::vector<float3> normals;
};

struct material
{
    float3 ambient_color;
    float3 diffuse_color;
    float3 specular_color;
    float specular_coefficient;
    float transparency;

    std::string ambient_map_path;
    std::string diffuse_map_path;
    std::string specular_color_map_path;
    std::string alpha_map_path;
    std::string bump_map_path;
    std::string displacement_map_path;
};

model load_model_from_file(const std::string& path);
material load_material_from_file(const std::string& path);

#endif // _OBJ_LOADER_HPP_