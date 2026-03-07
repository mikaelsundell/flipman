/*
 * Sharpen Effect
 *
 * Applies a simple spatial sharpening filter using a 3x3 kernel.
 * The effect enhances local contrast by boosting the difference
 * between the center pixel and its neighbors.
 *
 * Parameters:
 *   strength - Sharpening strength multiplier.
 *
 * Behavior:
 *   - Samples immediate neighboring pixels.
 *   - Enhances edges while preserving flat areas.
 *   - Works best in linear color space.
 *   - Preserves alpha channel.
 *
 * Notes:
 *   This is a lightweight kernel suitable for real-time playback.
 */

@param float strength 0.5 0.0 2.0

@include "common.fx"

vec4 effect(vec4 color, vec2 uv)
{
    vec2 px = 1.0 / resolution;

    vec3 center = texture(tex, uv).rgb;

    vec3 north = texture(tex, uv + vec2(0.0, -px.y)).rgb;
    vec3 south = texture(tex, uv + vec2(0.0,  px.y)).rgb;
    vec3 east  = texture(tex, uv + vec2( px.x, 0.0)).rgb;
    vec3 west  = texture(tex, uv + vec2(-px.x, 0.0)).rgb;

    vec3 neighbors = (north + south + east + west) * 0.25;

    vec3 sharpened = center + (center - neighbors) * strength;

    return vec4(sharpened, color.a);
}