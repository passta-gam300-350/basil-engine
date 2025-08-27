#version 450 core

// Input from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Uniforms
uniform vec3 u_ViewPos;
uniform vec3 u_LightPos = vec3(10.0, 10.0, 10.0);
uniform vec3 u_LightColor = vec3(1.0, 1.0, 1.0);

// Material uniforms (same for all instances)
uniform vec3 u_BaseColor = vec3(1.0, 1.0, 1.0);  // White base color to show textures
uniform float u_Metallic = 0.1;
uniform float u_Roughness = 0.6;

// Textures (optional - for compatibility with existing system)
uniform bool u_HasTexture0 = false;
uniform sampler2D u_Texture0;

// Output
out vec4 FragColor;

void main()
{
    // Base color from uniform
    vec3 albedo = u_BaseColor;
    
    // Sample texture if available
    if (u_HasTexture0) {
        vec3 textureColor = texture(u_Texture0, TexCoords).rgb;
        albedo *= textureColor;  // Modulate with base color
    }
    
    // Simple Blinn-Phong lighting with uniform material properties
    vec3 norm = normalize(Normal);
    
    // Ambient
    vec3 ambient = 0.15 * albedo;
    
    // Diffuse
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * albedo;
    
    // Specular (using uniform roughness)
    vec3 viewDir = normalize(u_ViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0 * (1.0 - u_Roughness));
    
    // Mix metallic/non-metallic response
    vec3 specular = mix(vec3(0.04), albedo, u_Metallic) * spec * u_LightColor;
    
    // Adjust diffuse for metallic surfaces
    diffuse *= (1.0 - u_Metallic);
    
    // Final color
    vec3 result = ambient + diffuse + specular;
    
    // Apply gamma correction
    result = pow(result, vec3(1.0/2.2));
    
    FragColor = vec4(result, 1.0);
}