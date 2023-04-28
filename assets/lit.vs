// Found in rlgl.h
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
// in vec4 vertexColor;
in vec3 vertexNormal;

out vec2 fragTexCoord;
// out vec4 fragColor;
out vec3 fragNormal;

uniform mat4 mvp;
uniform mat4 matNormal;

void main()
{
    fragTexCoord = vertexTexCoord;
    // fragColor = vertexColor;
    fragNormal = (matNormal * vec4(vertexNormal, 0.0)).xyz;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}