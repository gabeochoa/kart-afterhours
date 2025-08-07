#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float time;
uniform vec2 resolution;
uniform sampler2D texture0;
uniform sampler2D maskTexture;
uniform vec4 colDiffuse;

uniform sampler2D texture1;
uniform sampler2D texture2;

out vec4 finalColor;

void main()
{
    vec4 src = texture(texture0, fragTexCoord);
    vec4 _d = texture(texture1, fragTexCoord) + texture(texture2, fragTexCoord);
    _d *= 0.0;
    vec4 mask = texture(maskTexture, fragTexCoord);
    float maskValue = (mask.r + mask.g + mask.b + mask.a) / 4.0;
    float anim = 0.05 * sin(time) + 0.05;
    float threshold = 0.1 + anim * (resolution.x / (resolution.x + resolution.y));
    if (maskValue > threshold) {
        finalColor = src * colDiffuse;
    } else {
        finalColor = vec4(0.0);
    }
}