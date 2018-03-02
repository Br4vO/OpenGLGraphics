#version 420

layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexture;

layout(location = 0) out vec3 o_normal;
layout(location = 1) out vec3 o_light;
layout(location = 2) out vec3 o_camera;
layout(location = 3) out vec2 o_UV;

layout(location = 4) out vec3 pos_eye;
layout(location = 5) out vec3 n_eye;



uniform mat4 MVP;
uniform mat3 MV;

uniform mat4 ModelMat;
uniform mat4 ViewMat;
uniform mat4 ProjMat;

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

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
}