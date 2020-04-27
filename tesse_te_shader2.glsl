#version 430 

layout (triangles, equal_spacing, ccw) in;

in vec2 tex_coord_tc[];
in float depth_tc[];

out vec2 tex_coord_te;
out float depth_te;

void main()
{
	const float u = gl_TessCoord[0];
	const float v = gl_TessCoord[1];
	const float w = gl_TessCoord[2];

	const vec4 p0 = gl_in[0].gl_Position;
	const vec4 p1 = gl_in[1].gl_Position;
	const vec4 p2 = gl_in[2].gl_Position;

	gl_Position = u * p0 + v * p1 + w * p2;
	tex_coord_te = u * tex_coord_tc[0] + v * tex_coord_tc[1] + w * tex_coord_tc[2];
	depth_te = u * depth_tc[0] + v * depth_tc[1] + w * depth_tc[2];
}
