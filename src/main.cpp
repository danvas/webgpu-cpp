#include "webgpu-release.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>

#include <cassert>
#include <iostream>
#include <vector>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#define wgpuInstanceRelease wgpuInstanceDrop
#endif

int main(int, char **)
{
  std::cout << "Hello, world!" << std::endl;
  // Create the equivalent of the navigator.gpu
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // Create the instance using descriptor
  WGPUInstance instance = wgpuCreateInstance(&desc);

  // Check if actually instance
  if (instance == nullptr)
  {
    std::cerr << "Could not create instance!" << std::endl;
    return 1;
  }

  // Display object
  std::cout << "WGPUInstance: " << instance << std::endl;

  if (!glfwInit())
  {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return 1;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(640, 480, "This is WebGPU", NULL, NULL);
  if (!window)
  {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return 1;
  }

  std::cout << "Requesting adapter..." << std::endl;

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;
  WGPUSurface surface = glfwGetWGPUSurface(instance, window);
  adapterOpts.compatibleSurface = surface;

  WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

  std::cout << "Got adapter: " << adapter << std::endl;

  std::vector<WGPUFeatureName> features;

  // Call the function a first time with a null return address, just to get the entry count.
  size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

  // Allocate memory (could be a new, or a malloc() if this were a C program)
  features.resize(featureCount);

  // Call the function a second time, with a non-null return address
  wgpuAdapterEnumerateFeatures(adapter, features.data());

  std::cout << "Adapter features:" << std::endl;
  for (auto feat : features)
  {
    std::cout << " - " << feat << std::endl;
  }

  while (!glfwWindowShouldClose(window))
  {
    // Check whether the user clicked on the close button (and any other
    // mouse/key event, which we don't use so far)
    glfwPollEvents();
  }

  // Cleanup WebGPU instance
  wgpuInstanceRelease(instance);
  wgpuAdapterRelease(adapter);

  glfwDestroyWindow(window);

  return 0;
}
