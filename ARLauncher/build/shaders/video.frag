#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D videoTexture;
uniform float opacity;

void main()
{
    vec3 color = texture(videoTexture, TexCoord).rgb;
    FragColor = vec4(color, opacity);
}

