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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Pretty WebGPU", NULL, NULL);
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

  /**
   * The Swap Chain is something that is not exposed in the JavaScript version of the API.
   * Like the notion of surface that we have met already, by the way.
   * The web browser takes care of it and does not offer any option.
   */
  std::cout << "Creating swapchain device..." << std::endl;
  WGPUSwapChainDescriptor swapChainDesc = {};
  swapChainDesc.width = 640;
  swapChainDesc.height = 480;

  WGPUTextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
  swapChainDesc.format = swapChainFormat;
  // We use the swap chain textures as targets for a Render Pass;
  // dictates the way the GPU organizes its memory
  swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
  // Tell which texture from the waiting queue must be presented at each frame: Immediate, Mailbox, or Fifo
  swapChainDesc.presentMode = WGPUPresentMode_Fifo;
  WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
  std::cout << "Swapchain: " << swapChain << std::endl;

  while (!glfwWindowShouldClose(window))
  {
    // Check whether the user clicked on the close button (and any other
    // mouse/key event, which we don't use so far)
    glfwPollEvents();
    WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
    if (!nextTexture)
    {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      break;
    }
    WGPUCommandEncoderDescriptor commandEncoderDesc = {};
    commandEncoderDesc.nextInChain = nullptr;
    commandEncoderDesc.label = "nvCommand encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

    // Describe a render pass, which targets the texture view
    WGPURenderPassDescriptor renderPassDesc = {};
    WGPURenderPassColorAttachment renderPassColorAttachment = {};

    // The attachment is tied to the view returned by the swap chain, so
    // that the render pass draws directly on the screen.
    renderPassColorAttachment.view = nextTexture;

    // Not relevant here because we don't use multi-sampling
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{0.9, 0.1, 0.2, 1.0};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    // No depth buffer (for now)
    renderPassDesc.depthStencilAttachment = nullptr;

    // We don't use timers (for now)
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;

    renderPassDesc.nextInChain = nullptr;

    // Create the render pass. We end it immediately because we use its builtin
    // mechanism for clearing the screen wehn it begins (see descriptor)
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    wgpuRenderPassEncoderEnd(renderPass);

    wgpuTextureViewRelease(nextTexture);

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuQueueSubmit(queue, 1, &command);

    // We can tell the swap chain to present the next texture.
    wgpuSwapChainPresent(swapChain);
  }

  // Cleanup WebGPU instance
  wgpuInstanceRelease(instance);
  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  wgpuSwapChainRelease(swapChain);

  glfwDestroyWindow(window);

  return 0;
}
