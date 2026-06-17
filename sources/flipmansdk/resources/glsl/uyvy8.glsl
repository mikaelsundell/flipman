/*
 * UYVY8 Conversion Compute Shader
 *
 * Converts an RGBA floating-point source texture into an 8-bit
 * 4:2:2 UYVY / 2vuy output buffer.
 *
 * The shader reads two horizontally adjacent RGB pixels per compute
 * invocation, converts them to Rec.709 YCbCr, averages chroma between
 * the two pixels, and writes one packed 32-bit UYVY word to the
 * destination storage buffer.
 *
 * Pipeline:
 *   1. Read two neighboring RGBA pixels from the source texture.
 *   2. Clamp RGB values to the normalized 0–1 range.
 *   3. Convert both pixels from RGB to Rec.709 YCbCr.
 *   4. Average Cb and Cr between the two pixels for 4:2:2 chroma.
 *   5. Quantize Y, Cb and Cr to 8-bit legal-range video levels.
 *   6. Pack the result as UYVY:
 *        U | Y0 << 8 | V << 16 | Y1 << 24
 *   7. Write the packed word to the destination buffer using the
 *      configured row stride.
 *
 * Behavior:
 *   - Operates in integer pixel coordinates.
 *   - Uses one compute invocation per two horizontal output pixels.
 *   - Supports arbitrary row stride through the Convert uniform block.
 *   - Outputs 8-bit 4:2:2 YUV in UYVY / 2vuy byte order.
 *   - Uses Rec.709 luma coefficients.
 *   - Uses legal video range:
 *       Y  = 16–235
 *       Cb = 16–240 centered at 128
 *       Cr = 16–240 centered at 128
 *   - Suitable for DeckLink bmdFormat8BitYUV and QuickTime 2vuy paths.
 *
 * Bindings:
 *   binding 0 - Convert uniform block.
 *               size   = output image size in pixels.
 *               stride = output row stride in bytes.
 *
 *   binding 1 - Source RGBA texture.
 *               Expected to contain normalized floating-point RGB data.
 *
 *   binding 2 - Destination storage buffer.
 *               Receives packed 32-bit UYVY words.
 *
 * Notes:
 *   - The destination buffer must be large enough for:
 *       stride * height bytes.
 *   - The stride is expressed in bytes, not pixels.
 *   - The buffer is addressed as uint words.
 *   - Each uint represents two output pixels.
 *   - Odd image widths duplicate the last pixel for the final pair.
 */

#version 440

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std140, binding = 0) uniform Convert
{
    vec2 size;
    uint stride;
    uint pad0;
} convert;

layout(binding = 1) uniform sampler2D src;

layout(std430, binding = 2) buffer Dst
{
    uint data[];
} dst;

vec3 rgbToRec709Ycbcr(vec3 rgb)
{
    rgb = clamp(rgb, 0.0, 1.0);

    float y  = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    float cb = (rgb.b - y) / 1.8556;
    float cr = (rgb.r - y) / 1.5748;

    return vec3(y, cb, cr);
}

uint quantizeYLegal(float y)
{
    return uint(clamp(round(16.0 + 219.0 * y), 0.0, 255.0));
}

uint quantizeCLegal(float c)
{
    return uint(clamp(round(128.0 + 224.0 * c), 0.0, 255.0));
}

void main()
{
    uvec2 pair = gl_GlobalInvocationID.xy;
    uvec2 size = uvec2(convert.size);

    uint x0 = pair.x * 2u;
    uint x1 = min(x0 + 1u, size.x - 1u);
    uint y  = pair.y;

    if (x0 >= size.x || y >= size.y)
        return;

    vec3 rgb0 = texelFetch(src, ivec2(x0, y), 0).rgb;
    vec3 rgb1 = texelFetch(src, ivec2(x1, y), 0).rgb;

    vec3 ycc0 = rgbToRec709Ycbcr(rgb0);
    vec3 ycc1 = rgbToRec709Ycbcr(rgb1);

    uint y0 = quantizeYLegal(ycc0.x);
    uint y1 = quantizeYLegal(ycc1.x);
    uint u  = quantizeCLegal((ycc0.y + ycc1.y) * 0.5);
    uint v  = quantizeCLegal((ycc0.z + ycc1.z) * 0.5);

    uint packed = u | (y0 << 8) | (v << 16) | (y1 << 24);

    uint byteOffset = y * convert.stride + pair.x * 4u;
    uint wordIndex = byteOffset / 4u;

    dst.data[wordIndex] = packed;
}