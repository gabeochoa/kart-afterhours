#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D maskTexture;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 source = texture(texture0, fragTexCoord);
    vec4 mask = texture(maskTexture, fragTexCoord);
    
    // Check if the mask has any color (text area)
    float maskValue = (mask.r + mask.g + mask.b + mask.a) / 4.0;
    
    // Only show source where mask has content
    if (maskValue > 0.1) {
        finalColor = source;
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 0.0); // Transparent
    }
} 