/*
 * RGBA8 Conversion Compute Shader
 *
 * Converts an RGBA floating-point source texture into a packed
 * 8-bit RGBA output buffer.
 *
 * The shader reads one pixel from the source texture per compute
 * invocation, clamps the color to the normalized 0–1 range, converts
 * each channel to unsigned 8-bit integer precision, and writes the
 * packed RGBA result into a storage buffer.
 *
 * Pipeline:
 *   1. Read one RGBA pixel from the source texture.
 *   2. Clamp the color to the normalized 0–1 range.
 *   3. Convert each channel to 8-bit unsigned integer precision.
 *   4. Pack R, G, B and A into one 32-bit word.
 *   5. Write the packed pixel to the destination buffer using the
 *      configured row stride.
 *
 * Behavior:
 *   - Operates in integer pixel coordinates.
 *   - Uses one compute invocation per output pixel.
 *   - Supports arbitrary row stride through the Convert uniform block.
 *   - Writes pixels as packed RGBA byte order:
 *       R | G << 8 | B << 16 | A << 24
 *   - Matches the CPU float-to-UInt8 conversion behavior by rounding
 *     normalized values to the nearest 8-bit code value.
 *   - Suitable for GPU-side conversion before asynchronous readback.
 *
 * Bindings:
 *   binding 0 - Convert uniform block.
 *               size   = output image size in pixels.
 *               stride = output row stride in bytes.
 *
 *   binding 1 - Source RGBA texture.
 *               Expected to contain normalized floating-point RGBA data.
 *
 *   binding 2 - Destination storage buffer.
 *               Receives packed 32-bit RGBA8 pixels.
 *
 * Notes:
 *   - The destination buffer must be large enough for:
 *       stride * height bytes.
 *   - The stride is expressed in bytes, not pixels.
 *   - The buffer is addressed as uint words, so RGBA8 pixels map
 *     naturally to one uint per pixel.
 *   - Values outside 0–1 are clipped before conversion.
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

uint packUnorm4x8Manual(vec4 c)
{
    uvec4 v = uvec4(clamp(round(c * 255.0), 0.0, 255.0));
    return v.r | (v.g << 8) | (v.b << 16) | (v.a << 24);
}

void main()
{
    uvec2 p = gl_GlobalInvocationID.xy;
    uvec2 size = uvec2(convert.size);

    if (p.x >= size.x || p.y >= size.y)
        return;

    vec4 c = texelFetch(src, ivec2(p), 0);
    c = clamp(c, 0.0, 1.0);

    uint byteOffset = p.y * convert.stride + p.x * 4u;
    uint wordIndex = byteOffset / 4u;

    dst.data[wordIndex] = packUnorm4x8Manual(c);
}
