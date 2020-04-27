#version 430

uniform float stroke_width;
uniform float stroke_inter;

layout (vertices = 3) out;  //number of output verts of the tess. control shader

in vec2 tex_coord_v[];
in float depth_v[];

out vec2 tex_coord_tc[];
out float depth_tc[];

void main()
{
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	tex_coord_tc[gl_InvocationID] = tex_coord_v[gl_InvocationID];
	depth_tc[gl_InvocationID] = depth_v[gl_InvocationID];

	float tess_lv_0 = distance(gl_out[0].gl_Position, gl_out[1].gl_Position) / (stroke_width + stroke_inter);
	float tess_lv_1 = distance(gl_out[1].gl_Position, gl_out[2].gl_Position) / (stroke_width + stroke_inter);
	float tess_lv_2 = distance(gl_out[1].gl_Position, gl_out[0].gl_Position) / (stroke_width + stroke_inter);

	//set tessellation levels
	gl_TessLevelOuter[0] = tess_lv_0;
	gl_TessLevelOuter[1] = tess_lv_1;
	gl_TessLevelOuter[2] = tess_lv_2;

	gl_TessLevelInner[0] = min(min(tess_lv_0, tess_lv_1), tess_lv_2);
}
