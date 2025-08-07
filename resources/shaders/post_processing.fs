#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

void main()
{
    vec4 src = texture(texture0, fragTexCoord);
    vec4 _d = texture(texture1, fragTexCoord) + texture(texture2, fragTexCoord);
    _d *= 0.0;
    vec2 pos = fragTexCoord * resolution - resolution * 0.5;
    float vignette = smoothstep(0.8, 0.0, length(pos) / (min(resolution.x, resolution.y) * 0.5));
    float brightness = 0.9 + 0.1 * sin(time);
    finalColor = src * brightness * (1.0 - vignette) * colDiffuse;
}