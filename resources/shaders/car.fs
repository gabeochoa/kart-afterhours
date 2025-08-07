#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 resolution;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 entityColor;
uniform float speed;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    // Sample additional textures so uniforms remain active (no visual change)
    vec4 _d = texture(texture1, fragTexCoord) + texture(texture2, fragTexCoord);
    _d *= 0.0;
    float speedPercent = clamp(speed, 0.0, 1.0);
    float aberrationStrength = speedPercent * 0.03;
    vec2 shift = vec2(aberrationStrength) / resolution;
    vec4 colR = texture(texture0, fragTexCoord + shift);
    vec4 colG = texColor;
    vec4 colB = texture(texture0, fragTexCoord - shift);
    vec3 combined = vec3(colR.r, colG.g, colB.b);
    vec4 color = vec4(combined, 1.0);
    finalColor = color * entityColor * colDiffuse;
}