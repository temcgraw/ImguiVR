#version 450        

layout(location = 0) in vec3 pos_attrib;

layout(location = 0) uniform mat4 PVM;

out vec2 tex_coord;

void main(void)
{
	gl_Position = PVM*vec4(pos_attrib, 1.0);
	tex_coord = 0.5*(pos_attrib.xy+vec2(1.0)); //assume the quad coords are in [-1, 1]
}