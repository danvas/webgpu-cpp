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

  std::cout << "Requesting device..." << std::endl;

  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  deviceDesc.label = "My Device";       // anything works here, that's your call
  deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
  deviceDesc.requiredLimits = nullptr;  // we do not require any specific limit
  deviceDesc.defaultQueue.nextInChain = nullptr;
  deviceDesc.defaultQueue.label = "The default queue";
  WGPUDevice device = requestDevice(adapter, &deviceDesc);

  auto onDeviceError = [](WGPUErrorType type, char const *message, void * /* pUserData */)
  {
    std::cout << "Uncaptured device error: type " << type;
    if (message)
      std::cout << " (" << message << ")";
    std::cout << std::endl;
  };
  wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

  std::cout << "Got device: " << device << std::endl;

  WGPUQueue queue = wgpuDeviceGetQueue(device);

  // The wgpuQueueOnSubmittedWorkDone is not implemented by our wgpu-native backend.
  // Using it will result in a null pointer exception so do not copy the above code block.
  // Uncomment the following when wgpu-native backend implements this function.
  // auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void * /* pUserData */)
  // {
  //   std::cout << "Queued work finished with status: " << status << std::endl;
  // };
  // wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  encoderDesc.label = "My command encoder";
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

  // Use the encoder to write instructions (debug placeholder for now)
  wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
  wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

  WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
  cmdBufferDescriptor.nextInChain = nullptr;
  cmdBufferDescriptor.label = "Command buffer";
  WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);

  // Finally submit the command queue
  std::cout << "Submitting command..." << std::endl;
  wgpuQueueSubmit(queue, 1, &command);
  while (!glfwWindowShouldClose(window))
  {
    // Check whether the user clicked on the close button (and any other
    // mouse/key event, which we don't use so far)
    glfwPollEvents();
  }

  // Cleanup WebGPU instance
  wgpuInstanceRelease(instance);
  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);

  glfwDestroyWindow(window);

  return 0;
}
