/*
 * Basic Gaussian Blur
 *
 * Applies a small 5-tap Gaussian blur to the source texture.
 * Intended as a minimal demonstration of Gaussian convolution.
 *
 * Parameters:
 *   radius - blur radius in pixels
 */

@param float radius 1.0 0.0 10.0

@include "common.fx"

vec4 effect(vec4 color, vec2 uv)
{
    vec2 texel = 1.0 / global.resolution;
    vec4 result = vec4(0.0);

    // 5-tap gaussian kernel
    float w0 = 0.227027;
    float w1 = 0.316216;
    float w2 = 0.070270;

    result += texture(tex, uv) * w0;
    result += texture(tex, uv + texel * vec2(radius, 0.0)) * w1;
    result += texture(tex, uv - texel * vec2(radius, 0.0)) * w1;
    result += texture(tex, uv + texel * vec2(radius * 2.0, 0.0)) * w2;
    result += texture(tex, uv - texel * vec2(radius * 2.0, 0.0)) * w2;

    return result;
}