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
  let offset = vec2f(-0.6875, -0.463);
  out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
  out.color = in.color; // forward to the fragment shader
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    // We apply a gamma-correction to the color
	let linear_color = pow(in.color, vec3f(2.2));
    return vec4f(linear_color, 1.0);
}
