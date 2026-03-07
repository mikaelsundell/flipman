/*
 * Common Shader Utilities
 *
 * Shared math and color helpers for effect shaders.
 * Intended to be included using:
 *
 *   @include "common.fx"
 *
 * Notes:
 *   - Functions are branch-free.
 *   - Designed for linear color space.
 *   - No uniform blocks defined here.
 */

// standard luminance (Rec.709)
float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// simple saturation adjustment
vec3 adjustSaturation(vec3 c, float amount)
{
    float Y = luminance(c);
    return mix(vec3(Y), c, amount);
}

// soft clamp to avoid harsh clipping
vec3 softClamp(vec3 c, float knee)
{
    return c / (1.0 + abs(c) / knee);
}

// highlight weighting helper
float highlightWeight(vec3 c, float low, float high)
{
    float Y = luminance(c);
    return smoothstep(low, high, Y);
}