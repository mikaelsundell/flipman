/*
 * Checker Pattern Effect
 *
 * Generates a procedural checkerboard pattern in normalized UV space.
 * The pattern is resolution-independent and controlled by a configurable
 * scale factor defining the number of squares across the image.
 *
 * Parameters:
 *   scale      - Number of checker cells across the UV domain.
 *   colorA     - First checker color.
 *   colorB     - Second checker color.
 *   mixAmount  - Blend factor between original image and checker pattern.
 *
 * Behavior:
 *   - Operates in UV space (0–1 range expected).
 *   - Branch-free implementation for GPU efficiency.
 *   - Preserves input alpha channel.
 *   - Suitable for debugging UV mapping, tiling behavior,
 *     and shader pipeline validation.
 *
 * Usage:
 *   Commonly used as a diagnostic overlay or procedural background.
 */

@param float scale 8.0 1.0 64.0
@param vec3  colorA 1.0 1.0 1.0
@param vec3  colorB 0.0 0.0 0.0
@param float mixAmount 1.0 0.0 1.0

vec4 effect(vec4 color, vec2 uv)
{
    vec2 grid = floor(uv * scale);
    float checker = mod(grid.x + grid.y, 2.0);
    vec3 pattern = mix(colorA, colorB, checker);
    vec3 result = mix(color.rgb, pattern, mixAmount);
    return vec4(result, color.a);
}