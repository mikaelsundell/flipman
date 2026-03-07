/*
 * Fullscreen Quad Vertex Shader
 *
 * Transforms a fullscreen quad into clip space and forwards UV
 * coordinates to the fragment stage for texture sampling.
 *
 * The vertex positions are expected to already be defined in clip
 * space coordinates (-1 to 1). Therefore the shader performs no
 * additional transformations and directly writes the position to
 * gl_Position.
 *
 * Behavior:
 *   - Pass-through vertex shader for screen-space rendering.
 *   - Forwards UV coordinates unchanged to the fragment shader.
 *   - Assumes vertices form a fullscreen quad (typically rendered
 *     using a triangle strip).
 *   - No matrices or camera transforms are applied.
 *
 * Usage:
 *   Commonly used in post-processing passes, texture blits, and
 *   fullscreen effects where geometry transformation is unnecessary.
 */

#version 440
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec2 vUV;
void main()
{
    vUV = uv_coord;
    gl_Position = vec4(position, 0.0, 1.0);
}
