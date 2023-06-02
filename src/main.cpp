/**
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <glfw3webgpu.h>
#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <iostream>
#include <cassert>

#include "webgpu-release.h"
#include "utils.h"

using namespace wgpu;

int main(int, char **)
{
  Instance instance = createInstance(InstanceDescriptor{});
  if (!instance)
  {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return 1;
  }

  if (!glfwInit())
  {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
  if (!window)
  {
    std::cerr << "Could not open window!" << std::endl;
    return 1;
  }

  std::cout << "Requesting adapter..." << std::endl;
  Surface surface = glfwGetWGPUSurface(instance, window);
  RequestAdapterOptions adapterOpts;
  adapterOpts.compatibleSurface = surface;
  Adapter adapter = instance.requestAdapter(adapterOpts);
  std::cout << "Got adapter: " << adapter << std::endl;

  SupportedLimits supportedLimits;
  adapter.getLimits(&supportedLimits);

  std::cout << "Requesting device..." << std::endl;
  // Don't forget to = Default
  RequiredLimits requiredLimits = Default;
  // We use at most 1 vertex attribute for now
  requiredLimits.limits.maxVertexAttributes = 2;
  // We should also tell that we use 1 vertex buffers
  requiredLimits.limits.maxVertexBuffers = 1;
  // Maximum size of a buffer is 6 vertices of 2 float each
  requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
  // Maximum stride between 2 consecutive vertices in the vertex buffer
  requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
  requiredLimits.limits.maxInterStageShaderComponents = 3;
  // This must be set even if we do not use storage buffers for now
  requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
  requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
  DeviceDescriptor deviceDesc;
  deviceDesc.label = "My Device";
  deviceDesc.requiredFeaturesCount = 0;
  // FIXME: following causes runtime error...
  deviceDesc.requiredLimits = &requiredLimits;
  // Could not get WebGPU adapter: LimitsExceeded(FailedLimit { name: "min_storage_buffer_offset_alignment", requested: 0, allowed: 256 })
  // Got device: <wgpu::Device 0x0>
  // thread '<unnamed>' panicked at 'invalid DeviceId', src/lib.rs:210:5
  // note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
  // fatal runtime error: failed to initiate panic, error 5
  deviceDesc.defaultQueue.label = "The default queue";
  Device device = adapter.requestDevice(deviceDesc);
  std::cout << "Got device: " << device << std::endl;

  adapter.getLimits(&supportedLimits);
  std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

  device.getLimits(&supportedLimits);
  std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

  // Personally I get:
  //   adapter.maxVertexAttributes: 16
  //   device.maxVertexAttributes: 8

  // Add an error callback for more debug info
  auto h = device.setUncapturedErrorCallback([](ErrorType type, char const *message)
                                             {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl; });

  Queue queue = device.getQueue();

  std::cout << "Creating swapchain..." << std::endl;
#ifdef WEBGPU_BACKEND_WGPU
  TextureFormat swapChainFormat = surface.getPreferredFormat(adapter);
#else
  TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;
#endif
  SwapChainDescriptor swapChainDesc;
  swapChainDesc.width = 640;
  swapChainDesc.height = 480;
  swapChainDesc.usage = TextureUsage::RenderAttachment;
  swapChainDesc.format = swapChainFormat;
  swapChainDesc.presentMode = PresentMode::Fifo;
  SwapChain swapChain = device.createSwapChain(surface, swapChainDesc);
  std::cout << "Swapchain: " << swapChain << std::endl;

  std::cout << "Creating shader module..." << std::endl;
  const char *shaderSource = R"(
// The `@location(0)` attribute means that this input variable is described
// by the vertex buffer layout at index 0 in the `pipelineDesc.vertex.buffers`
// array.
// The type `vec2f` must comply with what we will declare in the layout.
// The argument name `in_vertex_position` is up to you, it is only internal to
// the shader code!
struct VertexInput {
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

/**
 * A structure with fields labeled with builtins and locations can also be used
 * as *output* of the vertex shader, which is also the input of the fragment
 * shader.
 */
struct VertexOutput {
    @builtin(position) position: vec4f,
    // The location here does not refer to a vertex attribute, it just means
    // that this field must be handled by the rasterizer.
    // (It can also refer to another field of another struct that would be used
    // as input to the fragment shader.)
    @location(0) color: vec3f,
};


@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  let ratio = 640.0 / 480.0; // The width and height of the target surface
  out.position = vec4f(in.position.x, in.position.y * ratio, 0.0, 1.0);
  out.color = in.color; // forward to the fragment shader
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.color, 1.0);
}
)";

  ShaderModuleWGSLDescriptor shaderCodeDesc;
  shaderCodeDesc.chain.next = nullptr;
  shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
  ShaderModuleDescriptor shaderDesc;
  shaderDesc.nextInChain = &shaderCodeDesc.chain;

#ifdef WEBGPU_BACKEND_WGPU
  shaderDesc.hintCount = 0;
  shaderDesc.hints = nullptr;
  shaderCodeDesc.code = shaderSource;
#else
  shaderCodeDesc.source = shaderSource;
#endif

  ShaderModule shaderModule = device.createShaderModule(shaderDesc);
  std::cout << "Shader module: " << shaderModule << std::endl;

  std::cout << "Creating render pipeline..." << std::endl;
  RenderPipelineDescriptor pipelineDesc;

  // Vertex fetch interleaved attributes ("Option A")
  std::vector<VertexAttribute> vertexAttribs(2);

  // Position attribute
  vertexAttribs[0].shaderLocation = 0;
  vertexAttribs[0].format = VertexFormat::Float32x2;
  vertexAttribs[0].offset = 0;

  // Color attribute
  vertexAttribs[1].shaderLocation = 1;
  vertexAttribs[1].format = VertexFormat::Float32x3; // different type!
  vertexAttribs[1].offset = 2 * sizeof(float);       // non null offset!

  VertexBufferLayout vertexBufferLayout;
  // [...] Build vertex buffer layout
  vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
  vertexBufferLayout.attributes = vertexAttribs.data();
  // Bigger stride
  vertexBufferLayout.arrayStride = 5 * sizeof(float);
  vertexBufferLayout.stepMode = VertexStepMode::Vertex;

  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;

  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vs_main";
  pipelineDesc.vertex.constantCount = 0;
  pipelineDesc.vertex.constants = nullptr;

  pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
  pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
  pipelineDesc.primitive.frontFace = FrontFace::CCW;
  pipelineDesc.primitive.cullMode = CullMode::None;

  FragmentState fragmentState;
  pipelineDesc.fragment = &fragmentState;
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = "fs_main";
  fragmentState.constantCount = 0;
  fragmentState.constants = nullptr;

  BlendState blendState;
  blendState.color.srcFactor = BlendFactor::SrcAlpha;
  blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
  blendState.color.operation = BlendOperation::Add;
  blendState.alpha.srcFactor = BlendFactor::Zero;
  blendState.alpha.dstFactor = BlendFactor::One;
  blendState.alpha.operation = BlendOperation::Add;

  ColorTargetState colorTarget;
  colorTarget.format = swapChainFormat;
  colorTarget.blend = &blendState;
  colorTarget.writeMask = ColorWriteMask::All;

  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;

  pipelineDesc.depthStencil = nullptr;

  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = false;

  PipelineLayoutDescriptor layoutDesc;
  layoutDesc.bindGroupLayoutCount = 0;
  layoutDesc.bindGroupLayouts = nullptr;
  PipelineLayout layout = device.createPipelineLayout(layoutDesc);
  pipelineDesc.layout = layout;

  RenderPipeline pipeline = device.createRenderPipeline(pipelineDesc);
  std::cout << "Render pipeline: " << pipeline << std::endl;
  std::vector<float> pointData;
  std::vector<uint16_t> indexData;

  bool success = loadGeometry(RESOURCE_DIR "webgpu.txt", pointData, indexData);
  if (!success)
  {
    std::cerr << "Could not load geometry!" << std::endl;
    return 1;
  }
  else
  {
    std::cout << "Loaded " << pointData.size() / 5 << " vertices and "
              << indexData.size() << " indices." << std::endl;

    for (int i = 0; i < static_cast<int>(pointData.size()); i++)
    {
      std::cout << pointData[i] << " ";
    }
  }

  // Create vertex buffer
  BufferDescriptor bufferDesc;
  bufferDesc.size = pointData.size() * sizeof(float);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  bufferDesc.mappedAtCreation = false;
  Buffer vertexBuffer = device.createBuffer(bufferDesc);
  queue.writeBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

  int indexCount = static_cast<int>(indexData.size());

  // Create index buffer
  bufferDesc.size = indexData.size() * sizeof(uint16_t);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  bufferDesc.mappedAtCreation = false;
  Buffer indexBuffer = device.createBuffer(bufferDesc);

  // Upload geometry data to the buffer
  queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    TextureView nextTexture = swapChain.getCurrentTextureView();
    if (!nextTexture)
    {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      return 1;
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

    RenderPassDescriptor renderPassDesc;

    WGPURenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{0.9, 0.1, 0.2, 1.0};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    renderPass.setPipeline(pipeline);

    // Set vertex buffer while encoding the render pass
    renderPass.setVertexBuffer(0, vertexBuffer, 0, pointData.size() * sizeof(float));
    renderPass.setIndexBuffer(indexBuffer, IndexFormat::Uint16, 0, pointData.size() * sizeof(uint16_t));

    // We use the `vertexCount` variable instead of hard-coding the vertex count
    renderPass.drawIndexed(indexCount, 1, 0, 0, 0);

    renderPass.end();

    wgpuTextureViewRelease(nextTexture);

    CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(command);

    swapChain.present();
  }

  wgpuSwapChainRelease(swapChain);
  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  wgpuInstanceRelease(instance);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}