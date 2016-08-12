#version 450        

layout(location = 0) uniform mat4 PVM;
layout(location = 1) uniform mat4 P;
layout(location = 2) uniform mat4 Q1;
layout(location = 3) uniform mat4 Q2;
layout(location = 4) uniform float time = 0.0f;

layout(location = 5) uniform vec4 axis = vec4(0.0f);
layout(location = 6) uniform float scale = 1.0f;
layout(location = 7) uniform vec2 clip = vec2(1.0, 10.0);

layout(location = 8) uniform int mode = 0;
layout(location = 9) uniform float params[10];
layout(location = 19) uniform vec4 color[4];

layout(location = 0) in vec3 pos_attrib;
layout(location = 2) in vec3 tex_coord_attrib;

out vec3 vpos;
out mat3 rotation;

void main(void)
{
	gl_Position = P*vec4(pos_attrib, 1.0);
	vpos = tex_coord_attrib;

	
	float ct = cos(0.5*params[9]*time);
	float st = sin(0.5*params[9]*time);
	rotation = mat3(ct, -st, 0.0, st, ct, 0.0, 0.0, 0.0, 1.0);
}
