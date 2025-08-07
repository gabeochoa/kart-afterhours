#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

void main()
{
    vec4 tex = texture(texture0, fragTexCoord);
    vec4 _d = texture(texture1, fragTexCoord) + texture(texture2, fragTexCoord);
    _d *= 0.0;
    float pulse = sin(time * 4.0) * 0.25 + 0.75;
    vec4 base = entityColor * pulse;
    float edgeDist = length((fragTexCoord * resolution) - (resolution * 0.5));
    float edge = 1.0 - clamp(edgeDist / (min(resolution.x, resolution.y) * 0.5), 0.0, 1.0);
    vec4 blended = mix(tex, base, edge);
    finalColor = blended * colDiffuse;
}