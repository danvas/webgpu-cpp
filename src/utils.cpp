#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <webgpu/webgpu.hpp>

using namespace wgpu;
namespace fs = std::filesystem;

bool loadGeometry(
    const fs::path &path,
    std::vector<float> &pointData,
    std::vector<uint16_t> &indexData,
    int dimensions)
{
    // Check if the file exists
    if (!fs::exists(path))
    {
        // print message
        std::cout << "File does not exist" << std::endl;

        // cout the path value as string
        std::cout << path << std::endl;

        return false;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section
    {
        None,
        Points,
        Indices,
    };
    Section currentSection = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof())
    {
        getline(file, line);

        // overcome the `CRLF` problem
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line == "[points]")
        {
            currentSection = Section::Points;
        }
        else if (line == "[indices]")
        {
            currentSection = Section::Indices;
        }
        else if (line[0] == '#' || line.empty())
        {
            // Do nothing, this is a comment
        }
        else if (currentSection == Section::Points)
        {
            std::istringstream iss(line);
            // Get x, y, z, r, g, b
            for (int i = 0; i < dimensions + 3; ++i)
            {
                iss >> value;
                pointData.push_back(value);
            }
        }
        else if (currentSection == Section::Indices)
        {
            std::istringstream iss(line);
            // Get corners #0 #1 and #2
            for (int i = 0; i < 3; ++i)
            {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

ShaderModule loadShaderModule(const fs::path &path, Device device)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    ShaderModuleWGSLDescriptor shaderCodeDesc;
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    ShaderModuleDescriptor shaderDesc;
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
    shaderCodeDesc.code = shaderSource.c_str();
#else
    shaderCodeDesc.source = shaderSource.c_str();
#endif

    return device.createShaderModule(shaderDesc);
}