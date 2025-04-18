#version 410 core

layout (location = 0) in vec2 aPos;       // Vertex position from VBO (-1 to +1)
layout (location = 1) in vec2 aTexCoords; // Texture coordinate from VBO (0 to 1)

out vec2 TexCoords; // Pass texture coordinate to fragment shader

uniform vec2 scale = vec2(1.0, 1.0); // Aspect ratio scale (calculated in C)

void main()
{
    // Scale the vertex position based on aspect ratio to fit texture correctly
    vec2 scaledPos = aPos * scale;

    // Pass scaled vertex position directly to clip space
    gl_Position = vec4(scaledPos, 0.0, 1.0);

    // Pass the texture coordinate directly
    TexCoords = aTexCoords;
}
