#version 430

uniform sampler2D stroke_texture;
uniform sampler2D bg_texture;

uniform vec2 win_size;

in vec2 tex_coord;
in vec4 color;

out vec4 fragcolor;

void main(void)
{
	// When drawing on paper, the pigment usually will cumulate in the grooves of the surface rather than distributed evenly.
	// Assume that the darker area of the paper is rougher and has more/deeper grooves, the strokes should be more vivid in those area.
	// The assumption is implemented by determining the alpha of the fragment with regard to the contrast-enhanced background teture.
	float paper_a = 0.7 - (0.33 * dot(texture2D(bg_texture, vec2(gl_FragCoord.x / win_size.x, gl_FragCoord.y / win_size.y)).rgb, vec3(1.0)) - 0.85) * 10.0;
	fragcolor = vec4(color.rgb, paper_a * texture2D(stroke_texture, tex_coord).r);
}




















