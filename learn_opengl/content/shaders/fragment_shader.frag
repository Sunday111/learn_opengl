struct Attenuation
{
    float constant;
    float linear;
    float quadratic;
};

struct PointLight
{
    vec3 location;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    Attenuation attenuation;
};

struct DirectionalLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 location;
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
    float innerAngle;
    float outerAngle;
    Attenuation attenuation;
};

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct LightResult
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct CachedValues
{
    vec3 normal;
    vec3 viewDirection;
    vec3 materialDiffuse;
    vec3 materialSpecular;
};

uniform vec3 viewLocation;
uniform PointLight pointLights[cv_num_point_lights];
uniform DirectionalLight directionalLights[cv_num_directional_lights];
uniform SpotLight spotLights[cv_num_spot_lights];
uniform Material material;

in vec3 fragmentColor;
in vec2 fragmentTextureCoordinates;
in vec3 fragmentNormal;
in vec3 fragmentLocation;

out vec4 FragColor;

float ComputeAttenuation(Attenuation attenuation, float dist)
{
    float div = attenuation.constant;
    div += attenuation.linear * dist;
    div += attenuation.quadratic * dist * dist;
    return clamp(1.0f / div, 0, 1.0f);
}

LightResult ApplyPointLight(in PointLight light, in CachedValues cache)
{
    LightResult result;
    float lightDistance = length(light.location - fragmentLocation);
    float attenuation = ComputeAttenuation(light.attenuation, lightDistance);
    vec3 lightDirection = normalize(light.location - fragmentLocation);
    vec3 reflectDirection = reflect(-lightDirection, cache.normal);
    result.ambient = light.ambient * attenuation;
    result.diffuse = light.diffuse * attenuation *
                    max(dot(cache.normal, lightDirection), 0.0f);
    result.specular = light.specular * attenuation *
                    pow(max(dot(cache.viewDirection, reflectDirection), 0.0f), material.shininess);
    return result;
}

LightResult ApplyDirectionalLight(in DirectionalLight light, in CachedValues cache)
{
    LightResult result;
    vec3 lightDirection = normalize(-light.direction);
    vec3 reflectDirection = reflect(-lightDirection, cache.normal);
    result.ambient = light.ambient;
    result.diffuse = light.diffuse * 
                max(dot(cache.normal, lightDirection), 0.0f);
    result.specular = light.specular *
                pow(max(dot(cache.viewDirection, reflectDirection), 0.0f), material.shininess);
    return result;
}

LightResult ApplySpotLight(in SpotLight light, in CachedValues cache)
{
    LightResult result;
    vec3 lightDirection = normalize(light.location - fragmentLocation);
    vec3 reflectDirection = reflect(-lightDirection, cache.normal);
    float lightDistance = length(light.location - fragmentLocation);
    float attenuation = ComputeAttenuation(light.attenuation, lightDistance);
    float theta = dot(lightDirection, normalize(-light.direction));
    float epsilon = (light.innerAngle - light.outerAngle);
    float intensity = clamp((theta - light.outerAngle) / epsilon, 0.0, 1.0);
    result.ambient = vec3(0);
    result.diffuse = attenuation * intensity * light.diffuse *
                    max(dot(cache.normal, lightDirection), 0.0f);
    result.specular = attenuation * intensity * light.specular *
                    pow(max(dot(cache.viewDirection, reflectDirection), 0.0f), material.shininess);
    return result;
}

void AppendLightResult(inout LightResult a, in LightResult b)
{
    a.ambient += b.ambient;
    a.diffuse += b.diffuse;
    a.specular += b.specular;
}

LightResult ComputePointLights(in CachedValues cache)
{
    LightResult r;
    for(int i = 0; i < cv_num_point_lights; ++i) {
        AppendLightResult(r, ApplyPointLight(pointLights[i], cache));
    }

    r.ambient *= cache.materialDiffuse;
    r.diffuse *= cache.materialDiffuse;
    r.specular *= cache.materialSpecular;

    return r;
}

LightResult ComputeDirectionalLights(in CachedValues cache)
{
    LightResult r;
    for(int i = 0; i < cv_num_directional_lights; ++i) {
        AppendLightResult(r, ApplyDirectionalLight(directionalLights[i], cache));
    }

    r.ambient *= cache.materialDiffuse;
    r.diffuse *= cache.materialDiffuse;
    r.specular *= cache.materialSpecular;
    
    return r;
}

LightResult ComputeSpotLights(in CachedValues cache)
{
    LightResult r;
    for(int i = 0; i < cv_num_spot_lights; ++i) {
        AppendLightResult(r, ApplySpotLight(spotLights[i], cache));
    }

    r.diffuse *= cache.materialDiffuse;
    r.specular *= cache.materialSpecular;
    
    return r;
}

void main()
{
    CachedValues cache;
    cache.normal = normalize(fragmentNormal);
    cache.viewDirection = normalize(viewLocation - fragmentLocation);
    cache.materialDiffuse = vec3(texture(material.diffuse, fragmentTextureCoordinates));
    cache.materialSpecular = vec3(texture(material.specular, fragmentTextureCoordinates));

    LightResult lightsResult;
    AppendLightResult(lightsResult, ComputePointLights(cache));
    AppendLightResult(lightsResult, ComputeDirectionalLights(cache));
    AppendLightResult(lightsResult, ComputeSpotLights(cache));

    vec3 resultColor = (lightsResult.ambient + lightsResult.diffuse + lightsResult.specular) * fragmentColor;
    FragColor = vec4(resultColor, 1.0f);
}
