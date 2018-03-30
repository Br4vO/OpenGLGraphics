#version 440
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexture;

layout(location = 0) out vec3 o_normal;
layout(location = 1) out vec3 o_light;
layout(location = 2) out vec3 o_camera;
layout(location = 3) out vec2 o_UV;

layout(location = 4) out vec3 pos_eye;
layout(location = 5) out vec3 n_eye;

layout(location = 6) out vec4 o_shadowCoord;



layout(location = 0) uniform mat4 MVP;
layout(location = 1) uniform mat3 MV;

layout(location = 2) uniform mat4 ModelMat;
layout(location = 14) uniform mat4 ViewMat;
layout(location = 3) uniform mat4 ProjMat;
layout(location = 4) uniform mat4 DepthBiasMVP;

layout(location = 5) uniform vec3 lightPosition;
layout(location = 6) uniform vec3 cameraPosition;

void main()
{
	vec4 worldPosition = (ModelMat * vec4(vertexPosition_modelspace, 1));

	o_normal = normalize(MV * vertexNormal);

	o_light = normalize(lightPosition - worldPosition.xyz);

	o_camera = normalize(cameraPosition - worldPosition.xyz);

	gl_Position =  ProjMat * ViewMat * worldPosition;

	o_UV = vertexTexture;

	pos_eye = vec3(ViewMat * ModelMat * vec4(vertexPosition_modelspace, 1.0));
	n_eye = vec3(ViewMat * ModelMat * vec4(vertexNormal, 0.0));

	o_shadowCoord = DepthBiasMVP * vec4(vertexPosition_modelspace, 1);
}