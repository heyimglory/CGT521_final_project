#version 430

uniform int pass;
uniform sampler2D texture;

layout (triangles) in;
layout (triangle_strip) out;

in vec2 tex_coord_v[3];

out vec2 tex_coord;

void main(void)
{
	gl_Position = gl_in[0].gl_Position; 
	tex_coord = tex_coord_v[0];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	tex_coord = tex_coord_v[1];
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	tex_coord = tex_coord_v[2];
	EmitVertex();
    
	EndPrimitive();
}  