#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 backgroundColor;
uniform float opacity;
uniform float blurAmount;
uniform float borderWidth;
uniform vec3 borderColor;

void main()
{
    // Glassmorphism эффект: полупрозрачный фон с размытием
    vec3 color = backgroundColor;
    
    // Простой эффект стекла (можно улучшить с помощью blur texture)
    float alpha = opacity * 0.7; // Полупрозрачность
    
    // Граница
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoord, center);
    if (dist > 0.5 - borderWidth && dist < 0.5) {
        color = mix(color, borderColor, 0.5);
    }
    
    FragColor = vec4(color, alpha);
}

