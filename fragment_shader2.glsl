#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

uniform bool show_coord;

out vec4 fragcolor;           
in vec2 tex_coord;
in float depth;
      
void main(void)
{
	fragcolor = texture2D(color_texture, tex_coord);
	if(show_coord)
		fragcolor = vec4(tex_coord, 0.0, 0.7);
	//fragcolor = vec4(0.0, 1.0, 1.0, 1.0);
}




















