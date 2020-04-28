#version 430

//uniform sampler2D color_texture;
//uniform sampler2D depth_texture;
uniform sampler2D stroke_texture;
uniform sampler2D bg_texture;

uniform vec2 win_size;

in vec2 tex_coord;
in vec4 color;
//in float depth;

out vec4 fragcolor;

float get_paper_a(float x, float y)
{
	return 0.75 - (0.33 * dot(texture2D(bg_texture, vec2(x, y)).rgb, vec3(1.0)) - 0.85) * 10.0;
}

void main(void)
{
	float paper_a = get_paper_a(gl_FragCoord.x / win_size.x, gl_FragCoord.y / win_size.y);
	fragcolor = vec4(color.rgb, paper_a * texture2D(stroke_texture, tex_coord).r);
	//fragcolor = vec4(0.0, 1.0, 1.0, 1.0);
}




















