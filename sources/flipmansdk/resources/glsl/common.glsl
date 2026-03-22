/*
 * NV12 Conversion Utilities
 *
 * Helpers for converting NV12 video frames to RGB.
 *
 * NV12 format:
 *   - Y plane contains luma.
 *   - UV plane contains interleaved chroma at half resolution.
 *
 * The conversion assumes:
 *   - Rec.709 coefficients
 *   - Video (limited) range
 *   - Y ∈ [16/255, 235/255]
 *   - U,V ∈ [16/255, 240/255]
 *
 * Intended usage:
 *
 *   float y  = texture(texY, uv).r;
 *   vec2 uvv = texture(texUV, uv).rg;
 *   vec3 rgb = nv12ToRgb(y, uvv);
 *
 * Notes:
 *   - Branch free implementation.
 *   - Suitable for real-time video shaders.
 *   - Output RGB is linear relative to the YUV transform.
 */

vec3 nv12ToRgb(float y, vec2 uv)
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
