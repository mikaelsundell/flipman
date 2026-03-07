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
