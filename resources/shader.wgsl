/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
    color: vec4f,
    time: f32,
};

@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

// The `@location(0)` attribute means that this input variable is described
// by the vertex buffer layout at index 0 in the `pipelineDesc.vertex.buffers`
// array.
// The type `vec2f` must comply with what we will declare in the layout.
// The argument name `in_vertex_position` is up to you, it is only internal to
// the shader code!
struct VertexInput {
    @location(0) position: vec3f,
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
  let angle = uMyUniforms.time; // you can multiply it go rotate faster
  let alpha = cos(angle);
  let beta = sin(angle);
  var position = vec3<f32>(
    in.position.x,
    alpha * in.position.y + beta * in.position.z,
    alpha * in.position.z - beta * in.position.y,
  );
  out.position = vec4<f32>(position.x, position.y * ratio, 0.0, 1.0);
  out.color = in.color; // forward to the fragment shader
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // We multiply the scene's color with our global uniform (this is one
    // possible use of the color uniform, among many others).
    let color = in.color * uMyUniforms.color.rgb;
    // Gamma-correction
    let corrected_color = pow(color, vec3f(2.2));
    return vec4f(corrected_color, uMyUniforms.color.a);
}
