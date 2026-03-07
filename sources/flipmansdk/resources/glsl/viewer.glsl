/*
 * Effect Processing Fragment Shader
 *
 * Processes an input texture through a configurable shader pipeline
 * consisting of color transforms and procedural effects.
 *
 * The shader samples the input texture, optionally applies an
 * input color transform (IDT), executes a custom effect stage,
 * and finally applies an output transform (ODT) before writing
 * the result to the framebuffer.
 *
 * The shader source is designed to be dynamically extended through
 * code injection markers which allow external systems to insert
 * uniforms and processing code at runtime.
 *
 * Pipeline:
 *   1. Sample input texture.
 *   2. Apply Input Device Transform (IDT) if defined.
 *   3. Apply custom effect code.
 *   4. Apply Output Display Transform (ODT) if defined.
 *   5. Apply global opacity.
 *
 * Behavior:
 *   - Operates in normalized UV space (0–1).
 *   - Maintains alpha channel unless modified by effect code.
 *   - Supports dynamic shader extension through placeholder tokens.
 *   - Suitable for real-time image processing pipelines.
 *
 * Dynamic Injection Points:
 *   @effectUniforms  - Additional uniforms for effect parameters.
 *   @effectCode      - Custom shader functions or logic.
 *   @idtCode         - Input color transform implementation.
 *   @odtCode         - Output color transform implementation.
 *   @idtApply        - Invocation of the IDT stage.
 *   @effectApply     - Invocation of the effect stage.
 *   @odtApply        - Invocation of the ODT stage.
 *
 * Usage:
 *   Used as the core processing shader in image effect pipelines
 *   where color transforms and GPU-based effects are applied
 *   to an input texture before presentation.
 */

#version 440
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;
layout(binding = 1) uniform sampler2D tex;

layout(std140, binding = 0) uniform Global
{
    mat4  qt_Matrix;
    float qt_Opacity;
    vec2  resolution;
    float time;
    float pad0;
} global;

@effectUniforms

@effectCode

@idtCode

@odtCode

void main()
{
    vec4 color = texture(tex, vUV);
    
    @idtApply
    @effectApply
    @odtApply
    
    fragColor = color * global.qt_Opacity;
}
