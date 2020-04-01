#version 430

uniform sampler2D texture;
uniform int pass;
uniform vec2 mouse_pos;
uniform float time;
uniform bool edge_detect;

out vec4 fragcolor;           
in vec2 tex_coord;

      
void main(void)
{
	if(pass == 1)
	{
		fragcolor = texture2D(texture, tex_coord);
	}
	else if(pass == 2)
	{
		fragcolor = texelFetch(texture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
	}
	else
	{
		fragcolor = vec4(1.0, 0.0, 1.0, 1.0); //error
	}

}




















