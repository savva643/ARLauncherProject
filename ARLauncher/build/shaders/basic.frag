#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform float opacity;
uniform vec3 viewPos;
uniform bool useLighting;

void main()
{
    vec3 color = objectColor;
    
    if (useLighting) {
        // Простое освещение
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float diff = max(dot(normalize(Normal), lightDir), 0.0);
        color = color * (0.3 + 0.7 * diff);
    }
    
    FragColor = vec4(color, opacity);
}

