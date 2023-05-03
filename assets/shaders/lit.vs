// Found in rlgl.h
// #version 330

// in vec3 vertexPosition;
// in vec2 vertexTexCoord;
// in vec3 vertexNormal;

// out vec2 fragTexCoord;
// out vec3 fragNormal;

// uniform mat4 mvp;
// uniform mat4 matNormal;

// void main()
// {
//     fragTexCoord = vertexTexCoord;
//     // fragColor = vertexColor;
//     fragNormal = (matNormal * vec4(vertexNormal, 0.0)).xyz;

//     gl_Position = mvp * vec4(vertexPosition, 1.0);
// }

#if defined(GRAPHICS_API_OPENGL_ES2)
// #version 100
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
#elif defined(GRAPHICS_API_OPENGL_21)
// #version 120
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
#elif defined(GRAPHICS_API_OPENGL_33)
// #version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
out vec2 fragTexCoord;
out vec3 fragNormal;
#endif
uniform mat4 mvp;
uniform mat4 matNormal;
void main()
{
    fragTexCoord = vertexTexCoord;
    fragNormal = (matNormal * vec4(vertexNormal, 0.0)).xyz;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}