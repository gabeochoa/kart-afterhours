#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec2 vertexTexCoord2;
in vec3 vertexNormal;
in vec4 vertexTangent;
in vec4 vertexColor;

uniform mat4 matProjection;
uniform mat4 matView;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    // Reference additional attributes so the compiler keeps them
    float _unused = vertexTexCoord2.x + vertexNormal.x + vertexTangent.x;
    _unused = _unused * 0.0;

    // Use MVP for final position so 'mvp' uniform remains active
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    // Touch other matrix uniforms so they are not optimized away
    vec4 _mntmp = matProjection[0] + matView[1] + matModel[2] + matNormal[3];
    _unused += _mntmp.x * 0.0;
}