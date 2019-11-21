#version 450

in vec4 offset_scale_rot;

//MinX, MinY, MaxX, MaxY
uniform vec4 bounds;

void main() {
    float scale_x = 2.0/(bounds[2]-bounds[0]);
    float scale_y = 2.0/(bounds[3]-bounds[1]);
    gl_Position = vec4((offset_scale_rot.x-bounds[0])*scale_x - 1.0, -((offset_scale_rot.y-bounds[1])*scale_y - 1.0), offset_scale_rot[2]*scale_x , 1.0);
}
