#version 430

uniform sampler2D d_texture;
uniform sampler2D n_texture;
uniform sampler2D r_texture;
uniform sampler2D m_texture;

layout(location=0) out vec4 fragcolor;      
layout(location=1) out vec4 fragdepth;

in vec2 tex_coord;
in float depth;
      
void main(void)
{
	fragcolor = texture2D(d_texture, tex_coord);
	fragdepth = vec4(depth);
}




















