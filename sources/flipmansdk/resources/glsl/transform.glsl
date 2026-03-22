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
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 0) out vec2 uv;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
};

void main()
{
    uv = texCoord;
    gl_Position = mvp * vec4(position, 1.0);
}
