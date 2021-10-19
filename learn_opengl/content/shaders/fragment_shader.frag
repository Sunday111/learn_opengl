struct Attenuation {
    float constant;
    float linear;
    float quadratic;
};

struct PointLight {
    vec3 location;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    Attenuation attenuation;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 location;
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
    float innerAngle;
    float outerAngle;
    Attenuation attenuation;
};

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

uniform vec3 viewLocation;
uniform PointLight pointLight;
uniform DirectionalLight directionalLight;
uniform SpotLight spotLight;
uniform Material material;

in vec3 fragmentColor;
in vec2 fragmentTextureCoordinates;
in vec3 fragmentNormal;
in vec3 fragmentLocation;

out vec4 FragColor;

float ComputeAttenuation(Attenuation attenuation, float dist) {
    float div = attenuation.constant;
    div += attenuation.linear * dist;
    div += attenuation.quadratic * dist * dist;
    return 1.0f / div;
}

void main() {
    vec3 normal = normalize(fragmentNormal);
    vec3 viewDirection = normalize(viewLocation - fragmentLocation);

    vec3 ambient = vec3(0);
    vec3 diffuse = vec3(0);
    vec3 specular = vec3(0);

    vec3 materialDiffuse = vec3(texture(material.diffuse, fragmentTextureCoordinates));
    vec3 materialSpecular = vec3(texture(material.specular, fragmentTextureCoordinates));

    // point light
    {
        float lightDistance = length(pointLight.location - fragmentLocation);
        float attenuation = ComputeAttenuation(pointLight.attenuation, lightDistance);
        vec3 lightDirection = normalize(pointLight.location - fragmentLocation);
        vec3 reflectDirection = reflect(-lightDirection, normal);
        ambient += pointLight.ambient * materialDiffuse * attenuation;
        diffuse += pointLight.diffuse * materialDiffuse * attenuation *
                        max(dot(normal, lightDirection), 0.0f);
        specular += pointLight.specular * materialSpecular * attenuation *
                        pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);
    }

    // directional light
    {
        vec3 lightDirection = normalize(-directionalLight.direction);
        vec3 reflectDirection = reflect(-lightDirection, normal);
        ambient += directionalLight.ambient * materialDiffuse;
        diffuse += directionalLight.diffuse * materialDiffuse * 
                    max(dot(normal, lightDirection), 0.0f);
        specular += directionalLight.specular * materialSpecular *
                    pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);
    }

    // spotlight
    {
        vec3 lightDirection = normalize(spotLight.location - fragmentLocation);
        vec3 reflectDirection = reflect(-lightDirection, normal);
        float lightDistance = length(pointLight.location - fragmentLocation);
        float attenuation = clamp(ComputeAttenuation(spotLight.attenuation, lightDistance), 0.0f, 1.0f);
        float theta = dot(lightDirection, normalize(-spotLight.direction));
        float epsilon = (spotLight.innerAngle - spotLight.outerAngle);
        float intensity = clamp((theta - spotLight.outerAngle) / epsilon, 0.0, 1.0);
        diffuse += attenuation * intensity * spotLight.diffuse * materialDiffuse *
                        max(dot(normal, lightDirection), 0.0f);
        specular += attenuation * intensity * spotLight.specular * materialSpecular *
                        pow(max(dot(viewDirection, reflectDirection), 0.0f), material.shininess);
    }

    vec3 resultColor = (ambient + diffuse + specular) * fragmentColor;
    FragColor = vec4(resultColor, 1.0f);
}
