#version 430            
uniform mat4 PVM;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec2 tex_coord;
out float depth;

void main(void)
{
	vec4 pos = PVM * vec4(pos_attrib, 1.0);
	gl_Position = pos;
	tex_coord = tex_coord_attrib;
	depth = pos.z / pos.w;
}