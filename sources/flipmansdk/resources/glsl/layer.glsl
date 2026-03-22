/*
 * Effect Processing Fragment Shader
 *
 * Processes an input image through a configurable shader pipeline
 * consisting of color transforms and procedural effects.
 *
 * The shader reconstructs a color value from one or more source
 * textures and then executes a sequence of optional processing
 * stages before writing the result to the framebuffer.
 *
 * The shader source is designed to be dynamically extended through
 * code injection markers which allow external systems to insert
 * uniforms and processing code at runtime.
 *
 * Pipeline:
 *   1. Reconstruct source color from the input texture(s).
 *   2. Apply Input Device Transform (IDT) if defined.
 *   3. Apply custom effect code.
 *   4. Apply Output Display Transform (ODT) if defined.
 *   5. Write the final color to the framebuffer.
 *
 * Behavior:
 *   - Operates in normalized UV space (0–1).
 *   - Supports single-texture and multi-texture inputs
 *     (e.g. RGBA textures or bi-planar NV12 video).
 *   - Maintains the alpha channel unless modified by effect code.
 *   - Supports dynamic shader extension through placeholder tokens.
 *   - Suitable for real-time image processing pipelines.
 *
 * Dynamic Injection Points:
 *
 *   Texture stage
 *   @textureUniforms  - Texture sampler declarations.
 *   @textureApply     - Reconstructs the input color and must
 *                       produce: vec4 color.
 *
 *   Effect stage
 *   @effectUniforms   - Additional uniforms for effect parameters.
 *   @effectCode       - Custom shader functions or logic.
 *
 *   Color transforms
 *   @idtCode          - Input color transform implementation.
 *   @odtCode          - Output color transform implementation.
 *
 *   Stage execution
 *   @idtApply         - Invocation of the IDT stage.
 *   @effectApply      - Invocation of the effect stage.
 *   @odtApply         - Invocation of the ODT stage.
 *
 * Usage:
 *   Used as the core processing shader in image effect pipelines
 *   where textures of varying formats are converted into a common
 *   color representation and processed by GPU-based effects before
 *   presentation.
 *
 * Notes:
 *   - The texture stage must produce a variable named `color`.
 *   - Subsequent stages operate on this color value regardless of
 *     the original texture format.
 */

#version 440

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform Global
{
    mat4  mvp;
    vec2  resolution;
    float time;
    float pad0;
} global;

@texUniform
@effectUniform

@include "common.glsl"

@effectCode
@idtCode
@odtCode
void main()
{
    @texCall
    
    @idtCall
    @effectCall
    @odtCall
    
    fragColor = color;
}
