#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;
uniform sampler2D stroke_texture;

in vec2 tex_coord;
in vec4 color;
in float depth;

out vec4 fragcolor;
      
void main(void)
{
	fragcolor = vec4(color.rgb, texture2D(stroke_texture, tex_coord).r);
}




















