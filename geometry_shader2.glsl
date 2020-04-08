#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;
float stroke_width = 0.03;
float stroke_inter = 0.02;

layout (triangles) in;
layout (triangle_strip, max_vertices = 146) out;

in vec3 p_model[3];
in vec2 tex_coord_v[3];
in float depth_v[3];

out vec2 tex_coord;
out float depth;

float depth_offset = 0.001;

vec4 HtoL, HtoM, LtoM, n, p_HtoL;
vec4 line_start, line_end, line_step;
vec4 to_next_start, to_next_end;
vec4 stroke_start, stroke_end, stroke_dir;

bool not_exceed(vec4 p, vec4 from, vec4 to)
{
	vec3 side = to.xyz - from.xyz;
	vec3 moved = to.xyz - p.xyz;
	if(dot(side, moved) < 0)
		return false;
	else
		return true;
}

void main(void)
{
	if(depth_v[0] - texture2D(depth_texture, tex_coord_v[0]).r > depth_offset && depth_v[1] - texture2D(depth_texture, tex_coord_v[1]).r > depth_offset && depth_v[2] - texture2D(depth_texture, tex_coord_v[2]).r > depth_offset)
		return;

	int highest = 0;
	int lowest = 2;
	int middle = 1;
	if(p_model[1].y > p_model[highest].y)
	{
		highest = 1;
	}
	if(p_model[2].y > p_model[highest].y)
	{
		highest = 2;
	}
	if(p_model[0].y < p_model[lowest].y)
	{
		lowest = 0;
	}
	if(p_model[2].y < p_model[lowest].y)
	{
		lowest = 2;
	}
	middle = 3 - highest - lowest;
	HtoL = normalize(gl_in[lowest].gl_Position - gl_in[highest].gl_Position);
	HtoM = normalize(gl_in[middle].gl_Position - gl_in[highest].gl_Position);
	LtoM = normalize(gl_in[middle].gl_Position - gl_in[lowest].gl_Position);
	n = vec4(cross(HtoM.xyz, HtoL.xyz), 0.0);
	p_HtoL = vec4(cross(HtoL.xyz, n.xyz), 0.0);
	to_next_start = HtoM * (stroke_width + stroke_inter);
	to_next_end = LtoM * (stroke_width + stroke_inter) * distance(gl_in[lowest].gl_Position, gl_in[middle].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position);

	line_start = gl_in[highest].gl_Position;
	line_end = gl_in[lowest].gl_Position;
	for(line_start = gl_in[highest].gl_Position; not_exceed(line_start, gl_in[highest].gl_Position, gl_in[middle].gl_Position); line_start += to_next_start)
	{
		gl_Position = line_start - 0.5 * stroke_width * p_HtoL;
		tex_coord = tex_coord_v[highest];
		depth = depth_v[highest];
		EmitVertex();

		gl_Position = line_start + 0.5 * stroke_width * p_HtoL;
		tex_coord = tex_coord_v[highest];
		depth = depth_v[highest];
		EmitVertex();

		gl_Position = line_end + 0.5 * stroke_width * p_HtoL;
		tex_coord = tex_coord_v[lowest];
		depth = depth_v[lowest];
		EmitVertex();	

		gl_Position = line_end - 0.5 * stroke_width * p_HtoL;
		tex_coord = tex_coord_v[lowest];
		depth = depth_v[lowest];
		EmitVertex();
    
		EndPrimitive();

		line_end += to_next_end;
	}
}  