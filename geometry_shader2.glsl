#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;
float stroke_width = 0.01;
float stroke_inter = 0.01;

layout (triangles) in;
layout (triangle_strip, max_vertices = 140) out;

in vec3 p_model[3];
in vec2 tex_coord_v[3];
in float depth_v[3];

out vec2 tex_coord;
out float depth;

float depth_offset = 0.1;

vec4 HtoL, HtoM, LtoM, n, p_HtoL;
vec4 line_start, line_end, line_step, line_dir;
vec4 to_next_start, to_next_end;
vec2 line_tex_coord[2];
float line_d[2];

vec4 stroke_start, stroke_end;
vec2 stroke_tex_coord, step_tex_coord;
float stroke_start_d, stroke_end_d;
vec4 stroke_color;

bool not_exceed(vec4 p, vec4 from, vec4 to)
{
	vec3 side = to.xyz - from.xyz;
	vec3 moved = to.xyz - p.xyz;
	if(dot(side, moved) <= 0)
		return false;
	else
		return true;
}

bool different_color(vec4 c1, vec4 c2)
{
	if(abs(c1.r - c2.r) > 0.02 && abs(c1.g - c2.g) > 0.02 && abs(c1.b - c2.b) > 0.02)
		return true;
	else
		return false;
}

float is_ccw(vec4 HtoM, vec4 LtoM, vec4 n)
{
	if(cross(HtoM.xyz, LtoM.xyz).z < 0)
		return -1.0;
	else
		return 1.0;
}

void main(void)
{
	/*if(depth_v[0] - texture2D(depth_texture, tex_coord_v[0]).r > depth_offset && depth_v[1] - texture2D(depth_texture, tex_coord_v[1]).r > depth_offset && depth_v[2] - texture2D(depth_texture, tex_coord_v[2]).r > depth_offset)
		return;*/

	int highest = 0;
	int middle = 1;
	int lowest = 2;

	gl_Position = gl_in[0].gl_Position;
	tex_coord = tex_coord_v[0];
	depth = depth_v[0];
	EmitVertex();

	gl_Position = gl_in[1].gl_Position;
	tex_coord = tex_coord_v[1];
	depth = depth_v[1];
	EmitVertex();

	gl_Position = gl_in[2].gl_Position;
	tex_coord = tex_coord_v[2];
	depth = depth_v[2];
	EmitVertex();

	EndPrimitive();

	for(int i=0; i<3; i++)
	{
		if(i == 0)
		{
			highest = 0;
			middle = 1;
			lowest = 2;
		}
		else if(i == 1)
		{
			highest = 1;
			middle = 2;
			lowest = 0;
		}
		else
		{
			highest = 2;
			middle = 0;
			lowest = 1;
		}

		HtoL = normalize(gl_in[lowest].gl_Position - gl_in[highest].gl_Position);
		HtoM = normalize(gl_in[middle].gl_Position - gl_in[highest].gl_Position);
		LtoM = normalize(gl_in[middle].gl_Position - gl_in[lowest].gl_Position);
		n = vec4(cross(HtoM.xyz, HtoL.xyz), 0.0);
		p_HtoL = vec4(cross(HtoL.xyz, n.xyz), 0.0);
		to_next_start = HtoM * (stroke_width + stroke_inter);
		to_next_end = LtoM * (stroke_width + stroke_inter) * distance(gl_in[lowest].gl_Position, gl_in[middle].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position);

		line_end = gl_in[lowest].gl_Position;
		for(line_start = gl_in[highest].gl_Position; not_exceed(line_start, gl_in[highest].gl_Position, gl_in[middle].gl_Position); line_start += to_next_start)
		{
			line_dir = normalize(line_end - line_start);
			line_tex_coord[0] = mix(tex_coord_v[highest], tex_coord_v[middle], distance(line_start, gl_in[highest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position));
			line_tex_coord[1] = mix(tex_coord_v[lowest], tex_coord_v[middle], distance(line_start, gl_in[lowest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[lowest].gl_Position));
			line_d[0] = mix(depth_v[highest], depth_v[middle], distance(line_start, gl_in[highest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position));
			line_d[1] = mix(depth_v[lowest], depth_v[middle], distance(line_start, gl_in[lowest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[lowest].gl_Position));

			stroke_color = texture2D(color_texture, line_tex_coord[0]);
			stroke_start = line_start;
			stroke_tex_coord = line_tex_coord[0];
			stroke_start_d = line_d[0];

			for(line_step = line_start; not_exceed(line_step, line_start, line_end); line_step += line_dir * stroke_inter)
			{
				step_tex_coord = mix(line_tex_coord[0], line_tex_coord[1], distance(line_step, line_start) / distance(line_start, line_end));
				if(different_color(stroke_color, texture2D(color_texture, step_tex_coord)))
				{
					if(distance(line_step, stroke_start) > stroke_inter)
					{
						stroke_end = line_step - line_dir * stroke_width;

						gl_Position = stroke_start - 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
						tex_coord = stroke_tex_coord;
						depth = stroke_start_d - depth_offset;
						EmitVertex();

						gl_Position = stroke_start + 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
						tex_coord = stroke_tex_coord;
						depth = stroke_start_d - depth_offset;
						EmitVertex();

						stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_start, line_end));

						gl_Position = stroke_end + 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
						tex_coord = stroke_tex_coord;
						depth = stroke_end_d - depth_offset;
						EmitVertex();

						gl_Position = stroke_end - 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
						tex_coord = stroke_tex_coord;
						depth = stroke_end_d - depth_offset;
						EmitVertex();

						EndPrimitive();

						stroke_color = texture2D(color_texture, step_tex_coord);
						stroke_start = line_step;
						stroke_tex_coord = step_tex_coord;
						stroke_start_d = mix(line_d[0], line_d[1], distance(line_step, line_start) / distance(line_end, line_start));
					}
				}
			}
			if(!not_exceed(line_step, line_start, line_end))
			{
				if(distance(line_step, stroke_start) > stroke_inter)
				{
					stroke_end = line_end;

					gl_Position = stroke_start - 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
					tex_coord = stroke_tex_coord;
					depth = stroke_start_d - depth_offset;
					EmitVertex();

					gl_Position = stroke_start + 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
					tex_coord = stroke_tex_coord;
					depth = stroke_start_d - depth_offset;
					EmitVertex();

					stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_end, line_start));

					gl_Position = stroke_end + 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
					tex_coord = stroke_tex_coord;
					depth = stroke_end_d - depth_offset;
					EmitVertex();

					gl_Position = stroke_end - 0.5 * stroke_width * p_HtoL * is_ccw(HtoM, LtoM, n);
					tex_coord = stroke_tex_coord;
					depth = stroke_end_d - depth_offset;
					EmitVertex();

					EndPrimitive();
				}
			}
			line_end += to_next_end;
		}
	}
}  