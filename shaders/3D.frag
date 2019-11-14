#version 450

in vec3 v_color;
in vec3 v_normal;
in vec4 v_worldpos;

layout(location=0) out vec4 output_color;

uniform vec3 light_position;

void main() {
  //Ambient
  vec3 ambient_color = 0.4f*v_color;

  //Diffuse
  vec3 light_direction = normalize(light_position-v_worldpos.xyz);
  float diffuse_intensity = clamp(dot(light_direction, v_normal), 0.0f, 1.0f);
  vec3 diffuse_color = diffuse_intensity*v_color;


  //Output color
  output_color = vec4(clamp(ambient_color+diffuse_color, 0.0f, 1.0f), 1.0);
}
