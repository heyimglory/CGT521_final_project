#version 430

uniform int pass;

layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec2 tex_coord_v[3];

out vec2 tex_coord;

void main() {
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