#version 460 core

uniform mat4 ciProjectionMatrix;
uniform mat4 ciViewMatrix;
uniform mat4 ciModelMatrix;

in vec4			ciPosition;
in vec2			ciTexCoord0;

out VertexData{
	vec2 texCoord;
} outData;


void main()
{
	outData.texCoord = ciTexCoord0;

	gl_Position = ciProjectionMatrix * ciViewMatrix * ciModelMatrix  * ciPosition;

}