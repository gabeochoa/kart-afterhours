#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 source = texture(texture0, fragTexCoord);
    
    // Subtle color enhancement for racing game
    vec3 enhanced = source.rgb;
    
    // Slight saturation boost for more vibrant colors
    float luminance = dot(enhanced, vec3(0.299, 0.587, 0.114));
    enhanced = mix(enhanced, enhanced * 1.1, 0.2);
    
    // Add subtle contrast boost
    enhanced = pow(enhanced, vec3(0.95));
    
    // Add a very subtle vignette effect (darker edges)
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    float vignette = 1.0 - dot(uv, uv) * 0.15;
    enhanced *= vignette;
    
    // Add subtle color grading - slightly warmer tones
    enhanced.r *= 1.05;
    enhanced.b *= 0.98;
    
    finalColor = vec4(enhanced, source.a);
}
