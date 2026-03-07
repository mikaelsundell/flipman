#version 440
layout(location = 0) in vec2 position;
layout(location = 2) in vec2 uv_coord;
layout(location = 0) out vec2 vUV;
void main()
{
    vUV = uv_coord;
    gl_Position = vec4(position, 0.0, 1.0);
}
