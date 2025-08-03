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
    // Sample the original texture
    vec4 textureColor = texture(texture0, fragTexCoord);
    finalColor = entityColor;
}