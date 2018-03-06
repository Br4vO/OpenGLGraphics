#version 420


layout(location = 0) in vec3 i_normal;
layout(location = 1) in vec3 i_light;
layout(location = 2) in vec3 i_camera;
layout(location = 3) in vec2 i_UV;

layout(location = 4) in vec3 pos_eye;
layout(location = 5) in vec3 n_eye;

layout(location = 6) in vec4 i_shadowCoord;

uniform vec3 lightAmbientIntensity;
uniform vec3 lightDiffuseIntensity;
uniform vec3 lightSpecularIntensity; 

uniform vec3 matAmbientReflectance;
uniform vec3 matDiffuseReflectance;
uniform vec3 matSpecularReflectance; 
uniform float matShininess; 

uniform mat4 ViewMat;

uniform sampler2DShadow shadow;

uniform samplerCube cube_texture;

out vec4 color;

vec3 ambientLighting()
{
   return matAmbientReflectance * lightAmbientIntensity;
}

vec3 diffuseLighting(in vec3 N, in vec3 L)
{
   float diffuseTerm = clamp(dot(N, L), 0, 1) ;
   return matDiffuseReflectance * lightDiffuseIntensity * diffuseTerm;
}

vec3 specularLighting(in vec3 N, in vec3 L, in vec3 V)
{
   float specularTerm = 0;
   if(dot(N, L) > 0)
   {
      // half vector
      vec3 H = normalize(L + V);
      specularTerm = pow(dot(N, H), matShininess);
   }
   return matSpecularReflectance * lightSpecularIntensity * specularTerm;
}

void main()
{
   vec3 L = normalize(i_light);
   vec3 V = normalize(i_camera);
   vec3 N = normalize(i_normal);

  vec3 incident_eye = normalize(pos_eye);
  vec3 normal = normalize(n_eye);

   vec3 R = reflect(incident_eye, normal);
   R = vec3(inverse(ViewMat) * vec4(R, 0.0));

   vec3 Iamb = ambientLighting();
   vec3 Idif = diffuseLighting(N, L);
   vec3 Ispe = specularLighting(N, L, V);

   
   float bias = 0.005;
   float visibility = textureProj( shadow, i_shadowCoord, bias);
   if (visibility < i_shadowCoord.z/i_shadowCoord.w)
	visibility = 0.0f;

   
	//color = texture2D(texUnitS, i_UV) * texture2D(texUnitD, i_UV) ;
	//color = texture(cube_texture, R) * texture(texUnitD, i_UV);
	color = vec4(0.5,0.5,0.5,1) * vec4((Iamb + (visibility*Idif) + (visibility*Ispe)), 1);
	//color.xyz = vec3(visibility);
	//color.a = 1;
	//color = texture2D(shadow, i_UV);
}