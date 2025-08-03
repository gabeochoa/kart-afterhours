#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;
uniform float speed;

out vec4 finalColor;

void main()
{
    // Sample the original texture
    vec4 texColor = texture(texture0, fragTexCoord);
    
    // Calculate speed-based chromatic aberration
    float speedPercent = clamp(speed, 0.0, 1.0);
    float aberration_strength = speedPercent * 0.03; // Max 3% aberration at full speed
    finalColor = vec4(entityColor.rgb, entityColor.a);
} 