#version 430

uniform sampler2D d_texture;
uniform sampler2D n_texture;
uniform sampler2D r_texture;
uniform sampler2D m_texture;
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
		fragcolor = texture2D(d_texture, tex_coord);
	}
	else if(pass == 2)
	{
		fragcolor = texelFetch(d_texture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
	}
	else
	{
		fragcolor = vec4(1.0, 0.0, 1.0, 1.0); //error
	}

}




















