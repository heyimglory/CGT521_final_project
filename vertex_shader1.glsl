#version 430

uniform int pass;
uniform mat4 PVM;
uniform mat4 M;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec2 tex_coord;
out vec3 p; //world-space vertex position
out float depth;

void main(void)
{
	if(pass == 1)
	{
		gl_Position = vec4(pos_attrib, 1.0);
		tex_coord = tex_coord_attrib;
	}
	else
	{
		vec4 pos = PVM * vec4(pos_attrib, 1.0);
		gl_Position = pos;
		tex_coord = tex_coord_attrib;
		p = vec3(M * vec4(pos_attrib, 1.0));
		depth = pos.z / pos.w;
	}
}