#version 450

//To be defined by the program
#define VERTICES_PER_DISK NB_V_DISK

#define M_PI 3.14159265358979323846

layout(points) in;
layout(line_strip, max_vertices=VERTICES_PER_DISK+1) out;


void main() {
    float r = gl_in[0].gl_Position[2];
    float theta = 2 * M_PI/float(VERTICES_PER_DISK);
    float c = cos(theta), s=sin(theta);
    float t;
    float _x=r;
    float _y=0;
    for(int i=0; i<=VERTICES_PER_DISK; i++)
    {
        gl_Position = vec4(gl_in[0].gl_Position.xy + vec2(_x, _y), 0.0, 1.0);
        EmitVertex();
        t = _x;
        _x = c * _x - s * _y;
        _y = s * t + c * _y;
    }
    EndPrimitive();
}
