#version 450

in vec3 position;
in vec3 normal;
in vec3 color;
in vec4 offset_scale_rot;

out vec3 v_normal;
out vec3 v_color;
out vec4 v_worldpos;

uniform mat4 MVP;

void main() {
  //Define rotation matrix
  float c_r = cos(offset_scale_rot[3]);
  float s_r = sin(offset_scale_rot[3]);
  mat4 rot = mat4(c_r,  0.0, -s_r,  0.0,
                  0.0,  1.0,  0.0,  0.0,
                  s_r,  0.0,  c_r,  0.0,
                  0.0,  0.0,  0.0,  1.0); //Column major order

  //Define scale matrix
  mat4 scale = mat4(offset_scale_rot[2]);
  scale[3][3]=1;

  //Define translation matrix
  mat4 translate = mat4(1.0);
  translate[3]=vec4(offset_scale_rot[0], 0.0, offset_scale_rot[1], 1.0);

  //Vertex world position
  v_worldpos = translate * rot * scale * vec4(position, 1.0);

  //Vertex camera position
  gl_Position = MVP * v_worldpos;

  //Normal rotation
  v_normal = (rot * vec4(normal, 1.0)).xyz;

  //Directly passing color
  v_color = color;
}
