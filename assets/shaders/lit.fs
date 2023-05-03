// Found in rlgl.h
// #version 330

// in vec2 fragTexCoord;
// // in vec4 fragColor;
// in vec3 fragNormal;

// out vec4 finalColor;

// uniform sampler2D texture0;
// uniform vec4 colDiffuse;

// const vec3 lightDir = normalize(vec3(0.1, 1.0, 0.1));

// void main()
// {
//     vec3 texelColor = texture(texture0, fragTexCoord).xyz;
//     float diffuseFac = max(dot(lightDir, fragNormal), 0.0);
//     float ambientFac = 0.75;
//     float lightFac = min(diffuseFac + ambientFac, 1.0);

//     finalColor = vec4(colDiffuse.rgb * lightFac, colDiffuse.a);
// }

#if defined(GRAPHICS_API_OPENGL_ES2)
// #version 100
precision mediump float;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
const vec3 lightDir = normalize(vec3(0.1, 1.0, 0.1));
void main()
{
    vec3 texelColor = texture2D(texture0, fragTexCoord).xyz;
    float diffuseFac = max(dot(lightDir, fragNormal), 0.0);
    float ambientFac = 0.75;
    float lightFac = min(diffuseFac + ambientFac, 1.0);

    gl_FragColor = vec4(colDiffuse.rgb * lightFac, colDiffuse.a);
}
#elif defined(GRAPHICS_API_OPENGL_21)
// #version 120
varying vec2 fragTexCoord;
varying vec3 fragNormal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
const vec3 lightDir = normalize(vec3(0.1, 1.0, 0.1));
void main()
{
    vec3 texelColor = texture2D(texture0, fragTexCoord).xyz;
    float diffuseFac = max(dot(lightDir, fragNormal), 0.0);
    float ambientFac = 0.75;
    float lightFac = min(diffuseFac + ambientFac, 1.0);

    gl_FragColor = vec4(colDiffuse.rgb * lightFac, colDiffuse.a);
}
#elif defined(GRAPHICS_API_OPENGL_33)
// #version 330
in vec2 fragTexCoord;
in vec3 fragNormal;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
const vec3 lightDir = normalize(vec3(0.1, 1.0, 0.1));
void main()
{
    // vec4 texelColor = texture(texture0, fragTexCoord);
    // finalColor = texelColor * colDiffuse * fragColor;

    vec3 texelColor = texture(texture0, fragTexCoord).xyz;
    float diffuseFac = max(dot(lightDir, fragNormal), 0.0);
    float ambientFac = 0.75;
    float lightFac = min(diffuseFac + ambientFac, 1.0);

    finalColor = vec4(colDiffuse.rgb * lightFac, colDiffuse.a);
}
#endif