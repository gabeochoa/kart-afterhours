#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;

out vec4 finalColor;

void main()
{
    // Create a simple enhanced entity shader
    vec4 baseColor = entityColor;
    
    // Add a subtle glow effect
    float glow = 0.15;
    vec3 glowColor = baseColor.rgb * glow;
    
    // Add some edge highlighting
    float edge = 1.0 - length(fragTexCoord - vec2(0.5, 0.5)) * 1.5;
    edge = max(edge, 0.0);
    
    // Combine all effects
    vec3 finalColorRGB = baseColor.rgb + glowColor * edge;
    
    finalColor = vec4(finalColorRGB, baseColor.a);
} 