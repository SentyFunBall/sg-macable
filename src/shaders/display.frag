#version 410 core

out vec4 FragColor;

in vec2 TexCoords; // Input from vertex shader (0,0 bottom-left to 1,1 top-right)

uniform sampler2D screenTexture;
// uniform float aspect; // Temporarily remove aspect ratio uniform

void main()
{
    // 1. Flip Y coordinate to match top-left origin expectation
    vec2 finalTexCoords = vec2(TexCoords.x, 1.0 - TexCoords.y);

    // 2. Temporarily disable aspect ratio correction and discard
    /*
    vec2 scale = vec2(1.0, 1.0);
    if (aspect > 1.0) { // Window wider than texture -> pillarbox
        scale.x = 1.0 / aspect; // Scale X coordinate down
    } else { // Window taller than texture -> letterbox
        scale.y = aspect; // Scale Y coordinate down
    }
    vec2 finalTexCoords = (flippedCoords - 0.5) * scale + 0.5;

    if (finalTexCoords.x < 0.0 || finalTexCoords.x > 1.0 || finalTexCoords.y < 0.0 || finalTexCoords.y > 1.0) {
        discard;
        return;
    }
    */

    // 3. Sample the texture with only the Y-flipped coordinates
    FragColor = texture(screenTexture, finalTexCoords);
}
