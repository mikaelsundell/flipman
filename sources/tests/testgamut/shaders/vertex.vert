#version 440
layout(location = 0) in vec3 position;  // x, y, z from ImageBuffer
layout(location = 1) in vec3 color;     // RGB as float

layout(location = 0) out vec3 v_color;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float pointSize;
};

void main() {
    v_color = color;
    gl_Position = mvp * vec4(position, 1.0);
    gl_PointSize = pointSize;
}