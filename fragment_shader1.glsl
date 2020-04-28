#version 430

const vec3 l = vec3(0.0, 1.0, 1.0); //directional light direction

uniform int pass;

uniform sampler2D bg_texture;
uniform sampler2D d_texture;
uniform sampler2D n_texture;
uniform sampler2D r_texture;
uniform sampler2D m_texture;

uniform mat4 M;

uniform vec3 cam_pos; //World-space eye position

layout(location=0) out vec4 fragcolor;
layout(location=1) out vec4 fragdepth;

in vec2 tex_coord;
in vec3 p;
in float depth;

void main(void)
{
	// just draw the background
	if (pass == 1)
	{
		fragcolor = texture2D(bg_texture, tex_coord);
	}
	// basically phong lighting
	else
	{
		vec4 d_tex = texture2D(d_texture, tex_coord);
		vec3 n = normalize((M * texture2D(n_texture, tex_coord)).xyz);
		float r_tex = texture2D(r_texture, tex_coord).r;
		float m_tex = texture2D(m_texture, tex_coord).r;

		vec3 amb = 0.2 * d_tex.rgb;
		vec3 diff = r_tex * d_tex.rgb * max(dot(n, normalize(l)), 0.0);
		vec3 spec = m_tex * vec3(pow(max(dot(reflect(normalize(-l), n), normalize(cam_pos - p)), 0.0), 2));

		fragcolor = vec4(amb + diff + spec, 1.0);
		fragdepth = vec4(depth);
	}
}
