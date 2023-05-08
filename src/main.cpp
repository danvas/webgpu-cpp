#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <glfw3webgpu.h>
#include "webgpu-release.h"
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#define wgpuInstanceRelease wgpuInstanceDrop
#endif

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const *options)
{
  // A simple structure holding the local information shared with the
  // onAdapterRequestEnded callback.
  struct UserData
  {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  // Callback called by wgpuInstanceRequestAdapter when the request returns
  // This is a C++ lambda function, but could be any function defined in the
  // global scope. It must be non-capturing (the brackets [] are empty) so
  // that it behaves like a regular C function pointer, which is what
  // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
  // is to convey what we want to capture through the pUserData pointer,
  // provided as the last argument of wgpuInstanceRequestAdapter and received
  // by the callback as its last argument.
  auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const *message, void *pUserData)
  {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestAdapterStatus_Success)
    {
      userData.adapter = adapter;
    }
    else
    {
      std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData.requestEnded = true;
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(
      instance /* equivalent of navigator.gpu */,
      options,
      onAdapterRequestEnded,
      (void *)&userData);

  // In theory we should wait until onAdapterReady has been called, which
  // could take some time (what the 'await' keyword does in the JavaScript
  // code). In practice, we know that when the wgpuInstanceRequestAdapter()
  // function returns its callback has been called.
  assert(userData.requestEnded);

  return userData.adapter;
}

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
