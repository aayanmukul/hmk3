#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D emissive0;
    float     shininess;
    int hasDiffuse;
    int hasSpecular;
    int hasEmissive;
};
uniform Material material;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

#define MAX_POINT_LIGHTS 2
struct PointLight {
    vec3  position;
    float constant;
    float linear;
    float quadratic;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
};
uniform PointLight pointLights[MAX_POINT_LIGHTS];

uniform vec3 viewPos;

// Fog parameters (bonus feature)
uniform bool  fogEnabled;
uniform vec3  fogColor;
uniform float fogDensity;

vec3 getDiffuseColor()
{
    if (material.hasDiffuse == 1)
        return texture(material.diffuse0, TexCoords).rgb;
    return vec3(0.7);
}

vec3 getSpecularColor()
{
    if (material.hasSpecular == 1)
        return texture(material.specular0, TexCoords).rgb;
    return vec3(0.3);
}

vec3 calcDirLight(DirLight light, vec3 norm, vec3 viewDir,
                  vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(-light.direction);

    // Ambient
    vec3 ambient = light.ambient * diffColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffColor;

    // Specular (Blinn-Phong half-vector)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * specColor;

    return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir,
                    vec3 diffColor, vec3 specColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float dist = length(light.position - fragPos);
    float atten = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    // Ambient
    vec3 ambient = light.ambient * diffColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffColor;

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * specColor;

    return atten * (ambient + diffuse + specular);
}

void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 diffColor = getDiffuseColor();
    vec3 specColor = getSpecularColor();

    vec3 result = calcDirLight(dirLight, norm, viewDir, diffColor, specColor);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
        result += calcPointLight(pointLights[i], norm, FragPos, viewDir,
                                 diffColor, specColor);

    if (material.hasEmissive == 1)
        result += texture(material.emissive0, TexCoords).rgb;

    // Exponential distance fog (bonus)
    if (fogEnabled) {
        float dist = length(viewPos - FragPos);
        float fogFactor = exp(-fogDensity * dist);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
    }

    FragColor = vec4(result, 1.0);
}
