/*
 * Texture Blit Shader
 *
 * Performs a direct texture sample using interpolated UV coordinates
 * and writes the sampled color to the output fragment.
 *
 * This shader is typically used as the final stage in an offscreen
 * rendering pipeline where an intermediate render target is copied
 * (blitted) to the current framebuffer.
 *
 * Behavior:
 *   - Operates in normalized UV space (0–1 range expected).
 *   - Performs a single texture lookup with no color modification.
 *   - Preserves all color channels including alpha.
 *   - Suitable for final presentation of rendered content or
 *     debugging intermediate render targets.
 *
 * Usage:
 *   Commonly used as a pass-through fragment shader when presenting
 *   an offscreen render texture to the screen or another render target.
 */

#version 440

layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D tex;

void main()
{
    fragColor = texture(tex, uv_coord);
}