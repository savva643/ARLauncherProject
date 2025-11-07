#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uiTexture;
uniform float opacity;

vec3 applyGlass(vec3 baseColor, vec2 uv)
{
    float vignette = smoothstep(0.8, 0.0, distance(uv, vec2(0.5)));
    vec3 tint = mix(vec3(0.9, 0.95, 1.0), baseColor, 0.4);
    return mix(tint, vec3(1.0), vignette * 0.12);
}

void main()
{
    vec4 texColor = texture(uiTexture, TexCoord);
    if (texColor.a <= 0.01)
        discard;

    vec3 glassColor = applyGlass(texColor.rgb, TexCoord);
    FragColor = vec4(glassColor, texColor.a * opacity);
}
