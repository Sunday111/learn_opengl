/* Defines:
 *   vec3 cv_abmbient
 *   vec3 cv_specular
 *   vec3 cv_diffuse
 *   float cv_shininess
 */

uniform sampler2D modelTexture;
uniform vec3 lightColor;
uniform vec3 lightLocation;
uniform vec3 viewLocation;

in vec3 fragmentColor;
in vec2 fragmentTextureCoordinates;
in vec3 fragmentNormal;
in vec3 fragmentLocation;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(fragmentNormal);

    vec3 lightDirection = normalize(lightLocation - fragmentLocation);
    vec3 viewDirection = normalize(viewLocation - fragmentLocation);
    vec3 reflectDirection = reflect(-lightDirection, normal);

    vec3 diffuse = max(dot(normal, lightDirection), 0.0f) * cv_diffuse;
    vec3 specular = cv_specular * pow(max(dot(viewDirection, reflectDirection), 0.0f), cv_shininess);
    
    vec3 resultColor = (cv_abmbient + diffuse + specular) * lightColor * fragmentColor;
    FragColor = vec4(resultColor, 1.0f);
}
