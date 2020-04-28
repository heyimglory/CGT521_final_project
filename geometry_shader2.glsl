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

float depth_offset = 0.003;

vec4 StoSE, StoLE, SEtoLE, n, p_StoSE;
vec4 line_start, line_end, line_step, line_dir;
vec4 to_next_start, to_next_end;
vec2 line_tex_coord[2];
float line_d[2];

vec4 stroke_start, stroke_end;
vec2 stroke_tex_coord, step_tex_coord;
float stroke_start_d, stroke_end_d;
vec4 stroke_color;

// check whether "p" has exceed "to" when start from "from"
bool not_exceed(vec4 p, vec4 from, vec4 to)
{
	vec3 side = to.xyz - from.xyz;
	vec3 moved = to.xyz - p.xyz;
	if(dot(side, moved) <= 0)
		return false;
	else
		return true;
}

// check whether "c1" and "c2" are similar colors
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

	int starting_point = 0;
	int lines_end_point = 1;
	int stroke_end_point = 2;

	// make sure the edge between starting_point to stroke_end_point is not the shotest edge
	if(distance(gl_in[starting_point].gl_Position, gl_in[lines_end_point].gl_Position) > distance(gl_in[starting_point].gl_Position, gl_in[stroke_end_point].gl_Position))
	{
		lines_end_point = 2;
		stroke_end_point = 1;
	}

	// discard triangle if the longer edge is shorter than stroke width
	if (distance(gl_in[starting_point].gl_Position, gl_in[stroke_end_point].gl_Position) < stroke_width)
	{
		return;
	}

	// determine the vectors
	StoSE = normalize(gl_in[stroke_end_point].gl_Position - gl_in[starting_point].gl_Position);
	StoLE = normalize(gl_in[lines_end_point].gl_Position - gl_in[starting_point].gl_Position);
	SEtoLE = normalize(gl_in[lines_end_point].gl_Position - gl_in[stroke_end_point].gl_Position);
	n = vec4(cross(StoLE.xyz, StoSE.xyz), 0.0);
	p_StoSE = vec4(cross(StoSE.xyz, n.xyz).xy, 0.0, 0.0);
	to_next_start = StoLE * stroke_inter;
	to_next_end = SEtoLE * stroke_inter * distance(gl_in[stroke_end_point].gl_Position, gl_in[lines_end_point].gl_Position) / distance(gl_in[lines_end_point].gl_Position, gl_in[starting_point].gl_Position);

	// traverse the area of the triangle with lines (potential location of strokes)
	line_end = gl_in[stroke_end_point].gl_Position;
	for(line_start = gl_in[starting_point].gl_Position; not_exceed(line_start, gl_in[starting_point].gl_Position, gl_in[lines_end_point].gl_Position); line_start += to_next_start)
	{
		line_dir = normalize(line_end - line_start);
		line_tex_coord[0] = mix(tex_coord_te[starting_point], tex_coord_te[lines_end_point], distance(line_start, gl_in[starting_point].gl_Position) / distance(gl_in[lines_end_point].gl_Position, gl_in[starting_point].gl_Position));
		line_tex_coord[1] = mix(tex_coord_te[stroke_end_point], tex_coord_te[lines_end_point], distance(line_start, gl_in[stroke_end_point].gl_Position) / distance(gl_in[lines_end_point].gl_Position, gl_in[stroke_end_point].gl_Position));

		stroke_color = texture2D(color_texture, line_tex_coord[0]);
		stroke_start = line_start;
		stroke_tex_coord = line_tex_coord[0];
		stroke_start_d = line_d[0];

		// form new strokes on the line when the colors are not similar
		for(line_step = line_start; not_exceed(line_step, line_start, line_end); line_step += line_dir * stroke_inter)
		{
			step_tex_coord = mix(line_tex_coord[0], line_tex_coord[1], distance(line_step, line_start) / distance(line_start, line_end));
			if(different_color(stroke_color, texture2D(color_texture, step_tex_coord)))
			{
				if(distance(line_step, stroke_start) > stroke_width)
				{
					stroke_end = line_step - line_dir * stroke_width;

					gl_Position = stroke_start - 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
					tex_coord = vec2(0.0, 0.0);
					color = stroke_color;
					EmitVertex();

					gl_Position = stroke_start + 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
					tex_coord = vec2(0.0, 1.0);
					color = stroke_color;
					EmitVertex();

					stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_start, line_end));

					gl_Position = stroke_end - 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
					tex_coord = vec2(1.0, 0.0);
					color = stroke_color;
					EmitVertex();

					gl_Position = stroke_end + 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
					tex_coord = vec2(1.0, 1.0);
					color = stroke_color;
					EmitVertex();

					EndPrimitive();

					stroke_color = texture2D(color_texture, step_tex_coord);
					stroke_start = line_step;
					stroke_tex_coord = step_tex_coord;
					stroke_start_d = mix(line_d[0], line_d[1], distance(line_step, line_start) / distance(line_end, line_start));
				}
			}
		}
		// when reach the end of the line, the current stroke must end
		if(!not_exceed(line_step, line_start, line_end))
		{
			if(distance(line_step, stroke_start) > stroke_inter)
			{
				stroke_end = line_end;

				gl_Position = stroke_start - 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
				tex_coord = vec2(0.0, 0.0);
				color = stroke_color;
				EmitVertex();

				gl_Position = stroke_start + 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
				tex_coord = vec2(0.0, 1.0);
				color = stroke_color;
				EmitVertex();

				stroke_end_d = mix(line_d[0], line_d[1], distance(stroke_end, line_start) / distance(line_end, line_start));

				gl_Position = stroke_end - 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
				tex_coord = vec2(1.0, 0.0);
				color = stroke_color;
				EmitVertex();

				gl_Position = stroke_end + 0.5 * stroke_width * normalize(vec4(p_StoSE.x, p_StoSE.y, 0.0, 0.0));
				tex_coord = vec2(1.0, 1.0);
				color = stroke_color;
				EmitVertex();

				EndPrimitive();
			}
		}
		line_end += to_next_end;
	}
}  