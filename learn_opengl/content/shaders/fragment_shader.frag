struct PointLight {
    vec3 location;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
};

uniform vec3 viewLocation;
uniform PointLight light;
uniform Material material;

in vec3 fragmentColor;
in vec2 fragmentTextureCoordinates;
in vec3 fragmentNormal;
in vec3 fragmentLocation;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(fragmentNormal);

    vec3 lightDirection = normalize(light.location - fragmentLocation);
    vec3 viewDirection = normalize(viewLocation - fragmentLocation);
    vec3 reflectDirection = reflect(-lightDirection, normal);

    vec3 materialDiffuse = vec3(texture(material.diffuse, fragmentTextureCoordinates));

    vec3 ambient = light.ambient * materialDiffuse;

    vec3 diffuse;
    if (materialDiffuse.x > 0.1) {
        diffuse = materialDiffuse * light.diffuse;
        diffuse *= max(dot(normal, lightDirection), 0.0f);
    }
    else {
        diffuse = light.diffuse;
    }

    vec3 specular = material.specular * light.specular;
    specular *= pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);

    vec3 resultColor = (ambient + diffuse + specular) * fragmentColor;
    FragColor = vec4(resultColor, 1.0f);
}
