/*
 * Warm Color Grading Effect
 *
 * Applies a highlight-weighted warm tone shift to the incoming color.
 * The effect increases red/orange bias while subtly reducing blue,
 * producing a perceptual warming similar to film-style grading.
 *
 * Parameters:
 *   warm       - Strength of the warming effect.
 *   intensity  - Global intensity multiplier applied before warming.
 *
 * Behavior:
 *   - Operates in RGB space (recommended in linear color space).
 *   - Warms highlights more than shadows using luminance weighting.
 *   - Preserves perceived contrast by avoiding uniform RGB scaling.
 *   - Preserves input alpha channel.
 *   - Branch-free implementation for real-time GPU execution.
 *
 * Usage:
 *   Intended for creative grading, look development, or subtle
 *   temperature adjustments prior to display transform.
 *
 * Notes:
 *   For physically accurate white balance adjustments, a matrix-based
 *   chromatic adaptation transform should be used instead.
 */

@param float warm 1.0 0.0 2.0

@include "common.fx"

vec4 effect(vec4 color, vec2 uv)
{
    vec3 c = color.rgb;
    float weight = highlightWeight(c, 0.1, 0.9);
    vec3 warmTone = mix(
        vec3(0.02, 0.01, -0.03),
        vec3(0.10, 0.03, -0.06),
        weight
    );
    c += warmTone * warm;
    c = adjustSaturation(c, 1.02);
    return vec4(c, color.a);
}