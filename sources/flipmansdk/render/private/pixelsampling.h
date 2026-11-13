// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 Mikael Sundell

#pragma once

namespace flipman::sdk::render {

// Shared native interleaved sampling for the standalone converter and fused
// layer shader. geometry = width, height, stride bytes, channel count.
inline constexpr const char* pixelBufferSampling = R"(
layout(std430, binding=1) readonly buffer FmPixels { uint words[]; } fmPixels;
float fmImageComponent(uint offset, int sampleType)
{
    uint word = fmPixels.words[offset >> 2u];
    if (sampleType == 0)
        return float((word >> ((offset & 3u) * 8u)) & 255u) / 255.0;
    if (sampleType == 1) {
        vec2 pair = unpackHalf2x16(word);
        return (offset & 2u) == 0u ? pair.x : pair.y;
    }
    return uintBitsToFloat(word);
}
vec4 fmImagePixel(ivec2 pixel, ivec4 geometry, int sampleType, bool bgr)
{
    pixel = clamp(pixel, ivec2(0), geometry.xy - ivec2(1));
    uint bytes = sampleType == 0 ? 1u : (sampleType == 1 ? 2u : 4u);
    uint base = uint(pixel.y * geometry.z + pixel.x * geometry.w * int(bytes));
    vec4 value = vec4(fmImageComponent(base, sampleType), 1.0, 1.0, 1.0);
    if (geometry.w > 1) value.g = fmImageComponent(base + bytes, sampleType);
    if (geometry.w > 2) value.b = fmImageComponent(base + 2u * bytes, sampleType);
    if (geometry.w > 3) value.a = fmImageComponent(base + 3u * bytes, sampleType);
    return bgr ? value.bgra : value;
}
)";

}  // namespace flipman::sdk::render
