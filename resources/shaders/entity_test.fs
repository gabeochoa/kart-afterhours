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
    vec4 texColor = texture(texture0, fragTexCoord);
    vec4 _d = texture(texture1, fragTexCoord) + texture(texture2, fragTexCoord);
    _d *= 0.0;
    float pulse = sin(time * 3.0 + resolution.x * 0.001) * 0.1 + 0.9;
    finalColor = texColor * entityColor * colDiffuse * pulse;
}