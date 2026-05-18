/*
 * Checker Pattern Effect
 *
 * Generates a procedural checkerboard pattern in normalized UV space.
 *
 * Parameters:
 *   scale     - number of checker cells across the UV domain
 *   colorA    - first checker color
 *   colorB    - second checker color
 *   mixAmount - blend factor between original image and checker pattern
 */

@param float scale 8.0 1.0 64.0
@param vec3 colorA 1.0 1.0 1.0 0.0 1.0
@param vec3 colorB 0.0 0.0 0.0 0.0 1.0
@param float mixAmount 1.0 0.0 1.0

vec4 effect(vec4 color, vec2 uv)
{
    vec2 grid = floor(uv * scale);
    float checker = mod(grid.x + grid.y, 2.0);
    vec3 pattern = mix(colorA, colorB, checker);
    vec3 result = mix(color.rgb, pattern, mixAmount);
    return vec4(result, color.a);
}