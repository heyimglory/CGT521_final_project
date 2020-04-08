#version 430

uniform sampler2D color_texture;
uniform sampler2D depth_texture;

out vec4 fragcolor;           
in vec2 tex_coord;
in float depth;
      
void main(void)
{
	/*if(depth - texture2D(depth_texture, tex_coord).r > 0.01)
	{
		discard;
	}
	else
	{*/
		fragcolor = texture2D(color_texture, tex_coord);
		fragcolor = vec4(tex_coord, 0.0, 1.0);
	//}
}




















