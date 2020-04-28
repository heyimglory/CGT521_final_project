#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform float stroke_width;
uniform float stroke_inter;

layout (triangles) in;
layout (triangle_strip, max_vertices = 100) out;

in vec2 tex_coord_te[3];
in float depth_te[3];

out vec2 tex_coord;
out vec4 color;
//out float depth;

float depth_offset = 0.003;

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
	if(abs(c1.r - c2.r) > 0.05 && abs(c1.g - c2.g) > 0.05 && abs(c1.b - c2.b) > 0.05)
		return true;
	else
		return false;
}

void main(void)
{
	// discard triangle if all the ponts are occluded
	if(depth_te[0] > texture2D(depth_texture, tex_coord_te[0]).r + depth_offset && depth_te[1] > texture2D(depth_texture, tex_coord_te[1]).r + depth_offset 
	&& depth_te[2] > texture2D(depth_texture, tex_coord_te[2]).r + depth_offset)
		return;

	int highest = 0;
	int middle = 1;
	int lowest = 2;

	// make sure highest to lowest is not the shotest edge
	if(distance(gl_in[highest].gl_Position, gl_in[middle].gl_Position) > distance(gl_in[highest].gl_Position, gl_in[lowest].gl_Position))
	{
		middle = 2;
		lowest = 1;
	}

	// discard triangle if smaller than stroke width
	if (distance(gl_in[highest].gl_Position, gl_in[lowest].gl_Position) < stroke_width)
	{
		return;
	}

	// determine the vectors
	HtoL = normalize(gl_in[lowest].gl_Position - gl_in[highest].gl_Position);
	HtoM = normalize(gl_in[middle].gl_Position - gl_in[highest].gl_Position);
	LtoM = normalize(gl_in[middle].gl_Position - gl_in[lowest].gl_Position);
	n = vec4(cross(HtoM.xyz, HtoL.xyz), 0.0);
	p_HtoL = vec4(cross(HtoL.xyz, n.xyz).xy, 0.0, 0.0);
	to_next_start = HtoM * stroke_inter;
	to_next_end = LtoM * stroke_inter * distance(gl_in[lowest].gl_Position, gl_in[middle].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position);

	line_end = gl_in[lowest].gl_Position;
	for(line_start = gl_in[highest].gl_Position; not_exceed(line_start, gl_in[highest].gl_Position, gl_in[middle].gl_Position); line_start += to_next_start)
	{
		line_dir = normalize(line_end - line_start);
		line_tex_coord[0] = mix(tex_coord_te[highest], tex_coord_te[middle], distance(line_start, gl_in[highest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[highest].gl_Position));
		line_tex_coord[1] = mix(tex_coord_te[lowest], tex_coord_te[middle], distance(line_start, gl_in[lowest].gl_Position) / distance(gl_in[middle].gl_Position, gl_in[lowest].gl_Position));

		stroke_color = texture2D(color_texture, line_tex_coord[0]);
		stroke_start = line_start;
		stroke_tex_coord = line_tex_coord[0];
		stroke_start_d = line_d[0];

		for(line_step = line_start; not_exceed(line_step, line_start, line_end); line_step += line_dir * stroke_inter)
		{
			step_tex_coord = mix(line_tex_coord[0], line_tex_coord[1], distance(line_step, line_start) / distance(line_start, line_end));
			if(different_color(stroke_color, texture2D(color_texture, step_tex_coord)))
			{
				if(distance(line_step, stroke_start) > stroke_width)
				{
					stroke_end = line_step - line_dir * stroke_width;

					gl_Position = stroke_start - 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
					tex_coord = vec2(0.0, 0.0);
					color = stroke_color;
					//depth = stroke_start_d - depth_offset;
					EmitVertex();

					gl_Position = stroke_start + 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
					tex_coord = vec2(0.0, 1.0);
					color = stroke_color;
					//depth = stroke_start_d - depth_offset;
					EmitVertex();

					stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_start, line_end));

					gl_Position = stroke_end - 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
					tex_coord = vec2(1.0, 0.0);
					color = stroke_color;
					//depth = stroke_end_d - depth_offset;
					EmitVertex();

					gl_Position = stroke_end + 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
					tex_coord = vec2(1.0, 1.0);
					color = stroke_color;
					//depth = stroke_end_d - depth_offset;
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

				gl_Position = stroke_start - 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
				tex_coord = vec2(0.0, 0.0);
				color = stroke_color;
				//depth = stroke_start_d - depth_offset;
				EmitVertex();

				gl_Position = stroke_start + 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
				tex_coord = vec2(0.0, 1.0);
				color = stroke_color;
				//depth = stroke_start_d - depth_offset;
				EmitVertex();

				stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_end, line_start));

				gl_Position = stroke_end - 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
				tex_coord = vec2(1.0, 0.0);
				color = stroke_color;
				//depth = stroke_end_d - depth_offset;
				EmitVertex();

				gl_Position = stroke_end + 0.5 * stroke_width * normalize(vec4(p_HtoL.x, p_HtoL.y, 0.0, 0.0));
				tex_coord = vec2(1.0, 1.0);
				color = stroke_color;
				//depth = stroke_end_d - depth_offset;
				EmitVertex();

				EndPrimitive();
			}
		}
		line_end += to_next_end;
	}
}  