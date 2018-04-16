#version 440


#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : require

#define EPSILON 0.0001

layout(location = 0) in vec3 i_normal;
layout(location = 1) in vec3 i_light;
layout(location = 2) in vec3 i_camera;
layout(location = 3) in vec2 i_UV;

layout(location = 4) in vec3 pos_eye;
layout(location = 5) in vec3 n_eye;

layout(location = 6) in vec4 i_shadowCoord;

layout(location = 7) uniform vec3 lightAmbientIntensity;
layout(location = 8) uniform vec3 lightDiffuseIntensity;
layout(location = 9) uniform vec3 lightSpecularIntensity; 

layout(location = 10) uniform vec3 matAmbientReflectance;
layout(location = 11) uniform vec3 matDiffuseReflectance;
layout(location = 12) uniform vec3 matSpecularReflectance; 
layout(location = 13) uniform float matShininess; 

layout(location = 14) uniform mat4 ViewMat;

uniform sampler2DShadow shadow;

uniform samplerCube cube_texture;

layout(location = 0) out vec4 color;

vec3 ambientLighting()
{
   return matAmbientReflectance * lightAmbientIntensity;
}

vec3 diffuseLighting(vec3 N, vec3 L)
{
   float diffuseTerm = clamp(dot(N, L), 0, 1) ;
   return matDiffuseReflectance * lightDiffuseIntensity * diffuseTerm;
}

vec3 specularLighting(vec3 N, vec3 L, vec3 V)
{
   float specularTerm = 0;
   if(dot(N, L) > 0)
   {
      // half vector
      vec3 H = normalize(L + V);
      specularTerm = pow(clamp(dot(N, H), 0, 1), matShininess);
	  //specularTerm = pow(1, matShininess);
   }
   return matSpecularReflectance * lightSpecularIntensity * specularTerm;
  // return vec3(specularTerm);
}

float CalcShadowFactor(vec4 LightSpacePos)
{
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords;
    UVCoords.x = 0.5 * ProjCoords.x + 0.5;
    UVCoords.y = 0.5 * ProjCoords.y + 0.5;
    float z = 0.5 * ProjCoords.z + 0.5;

    float xOffset = 1.0/2048;
    float yOffset = 1.0/2048;

    float Factor = 0.0;
	int nrOfSampling = 0;
	int widthSampling = 3;


    for (int y = -widthSampling; y <= widthSampling; y++) {
        for (int x = -widthSampling; x <= widthSampling; x++) 
		{

			float current;
			{
				vec2 Offsets = vec2(x * xOffset, y * yOffset);
				vec3 UVC = vec3(UVCoords + Offsets, z);
				current = texture(shadow, UVC);
			}

			float fX;
			{
				vec2 Offsets = vec2((x+1) * xOffset, y * yOffset);
				vec3 UVC = vec3(UVCoords + Offsets, z);
				fX = texture(shadow, UVC);
			}

			float fY;
			{
				vec2 Offsets = vec2(x * xOffset, (y+1) * yOffset);
				vec3 UVC = vec3(UVCoords + Offsets, z);
				fY = texture(shadow, UVC);
			}

			float biasX = fX - current;
			float biasY = fY - current;

            vec2 Offsets = vec2(x * xOffset, y * yOffset);
			float bias = (biasX ) + (biasY) /2;
            vec3 UVC = vec3(UVCoords + Offsets, z );

			float test = texture(shadow, UVC);

			if (test < 1)
				Factor += 0;
			else
				Factor += 0.2;
			nrOfSampling ++;
        }
    }

    return (0.5 + (Factor / nrOfSampling));
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
  // float visibility = textureProj( shadow, i_shadowCoord, bias);
  // if (visibility < i_shadowCoord.z/i_shadowCoord.w)
	//visibility = 0.0f;

	float visibility = CalcShadowFactor(i_shadowCoord);

	//if (visibility < 1)
	//	visibility = 0;
   
	//color = texture2D(texUnitS, i_UV) * texture2D(texUnitD, i_UV) ;
	//color = texture(cube_texture, R) * texture(texUnitD, i_UV);
	color = vec4(0.5,0.5,0.5,1) * vec4((Iamb + (visibility*Idif) + (visibility*Ispe)), 1);
	//color = vec4(Ispe, 1);
	//color = vec4(0.5 * normalize(L + V) + 0.5, 1);
	//color.xyz = vec3(visibility);
	//color.a = 1;
}