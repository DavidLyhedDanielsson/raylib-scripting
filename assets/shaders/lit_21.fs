#version 120

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
