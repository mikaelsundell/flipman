/*
 * YCbCr Conversion Utilities
 *
 * Helpers for converting video-range YCbCr formats to RGB.
 *
 * Supported formats:
 *   - NV12: 4:2:0 biplanar, Y plane + interleaved UV plane.
 *   - UYVY: 4:2:2 packed, byte order U Y0 V Y1.
 *
 * Assumes:
 *   - Video / limited range
 *   - Y ∈ [16/255, 235/255]
 *   - U,V ∈ [16/255, 240/255]
 */

vec3 _ycbcr601ToRgb(float y, vec2 uv)
{
    float Y = 1.16438356 * (y - 0.0625);
    float U = uv.x - 0.5;
    float V = uv.y - 0.5;

    return vec3(
        Y + 1.59602678 * V,
        Y - 0.39176229 * U - 0.81296764 * V,
        Y + 2.01723214 * U
    );
}

vec3 _ycbcr709ToRgb(float y, vec2 uv)
{
    float Y = 1.16438356 * (y - 0.0625);
    float U = uv.x - 0.5;
    float V = uv.y - 0.5;

    return vec3(
        Y + 1.79274107 * V,
        Y - 0.21324861 * U - 0.53290933 * V,
        Y + 2.11240179 * U
    );
}

vec3 _ycbcr2020ToRgb(float y, vec2 uv)
{
    float Y = 1.16438356 * (y - 0.0625);
    float U = uv.x - 0.5;
    float V = uv.y - 0.5;

    return vec3(
        Y + 1.67867411 * V,
        Y - 0.18732610 * U - 0.65042432 * V,
        Y + 2.14177232 * U
    );
}

vec3 _nv12ToRgb601(float y, vec2 uv)
{
    return _ycbcr601ToRgb(y, uv);
}

vec3 _nv12ToRgb709(float y, vec2 uv)
{
    return _ycbcr709ToRgb(y, uv);
}

vec3 _nv12ToRgb2020(float y, vec2 uv)
{
    return _ycbcr2020ToRgb(y, uv);
}

vec3 _uyvyToRgb601(vec4 uyvy, int pixelX)
{
    float y = ((pixelX & 1) == 0) ? uyvy.g : uyvy.a;
    vec2 uv = vec2(uyvy.r, uyvy.b);

    return _ycbcr601ToRgb(y, uv);
}

vec3 _uyvyToRgb709(vec4 uyvy, int pixelX)
{
    float y = ((pixelX & 1) == 0) ? uyvy.g : uyvy.a;
    vec2 uv = vec2(uyvy.r, uyvy.b);

    return _ycbcr709ToRgb(y, uv);
}

vec3 _uyvyToRgb2020(vec4 uyvy, int pixelX)
{
    float y = ((pixelX & 1) == 0) ? uyvy.g : uyvy.a;
    vec2 uv = vec2(uyvy.r, uyvy.b);

    return _ycbcr2020ToRgb(y, uv);
}
