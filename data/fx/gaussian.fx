/*
 * Basic Gaussian Blur
 *
 * Applies a 5-tap horizontal Gaussian blur.
 *
 * Parameters:
 *   radius - blur radius in pixels
 *   debugOffset - show only an offset sample to verify source sampling
 */

@param float radius 20.0 0.0 100.0
@param bool debugOffset false

@include "common.h"

vec4 effect(vec4 color, vec2 uv)
{
    vec2 texel = 1.0 / global.resolution;

    if (debugOffset) {
        return source(uv + texel * vec2(radius, 0.0));
    }

    vec4 result = vec4(0.0);

    float w0 = 0.227027;
    float w1 = 0.316216;
    float w2 = 0.070270;

    result += source(uv) * w0;
    result += source(uv + texel * vec2(radius, 0.0)) * w1;
    result += source(uv - texel * vec2(radius, 0.0)) * w1;
    result += source(uv + texel * vec2(radius * 2.0, 0.0)) * w2;
    result += source(uv - texel * vec2(radius * 2.0, 0.0)) * w2;

    return vec4(result.rgb, color.a);
}