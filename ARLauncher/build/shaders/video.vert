#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform vec2 videoAspect;  // (videoWidth/videoHeight, windowWidth/windowHeight)
uniform vec2 videoOffset; // (offsetX, offsetY) для центрирования

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    
    // Масштабируем позицию для сохранения соотношения сторон
    vec2 scaledPos = aPos.xy;
    if (videoAspect.x > videoAspect.y) {
        // Видео шире - подгоняем по ширине
        scaledPos.y *= videoAspect.y / videoAspect.x;
        scaledPos.y += videoOffset.y;
    } else {
        // Видео выше - подгоняем по высоте
        scaledPos.x *= videoAspect.x / videoAspect.y;
        scaledPos.x += videoOffset.x;
    }
    
    gl_Position = vec4(scaledPos, aPos.z, 1.0);
}

