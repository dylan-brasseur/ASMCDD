#version 450

in vec2 point;

//MinX, MinY, MaxX, MaxY
uniform vec4 bounds;

void main() {
    //Scaling the values to fit
    float scale_x = 2.0/(bounds[2]-bounds[0]);
    float scale_y = 2.0/(bounds[3]-bounds[1]);
    gl_Position = vec4((point.x-bounds[0])*scale_x - 1.0, ((point.y-bounds[1])*scale_y - 1.0), 0.0 , 1.0);
}