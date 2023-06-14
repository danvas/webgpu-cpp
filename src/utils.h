
#include "utils.cpp"

bool loadGeometry(const std::filesystem::path &path, std::vector<float> &pointData, std::vector<uint16_t> &indexData);
wgpu::ShaderModule loadShaderModule(const std::filesystem::path &path, wgpu::Device device);
