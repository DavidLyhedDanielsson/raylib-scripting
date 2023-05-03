#version 120
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;

varying vec2 fragTexCoord;
varying vec3 fragNormal;

uniform mat4 mvp;
uniform mat4 matNormal;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragNormal = (matNormal * vec4(vertexNormal, 0.0)).xyz;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
