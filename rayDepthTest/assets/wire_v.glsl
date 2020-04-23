#version 460 core



uniform mat4 ciModelMatrix;
uniform mat4 uCameraMatsViewProj;


in vec4			ciPosition;
#ifdef INSTANCED
in mat4         vInstanceMatrix; // per-instance position variable
#endif

out VertexData{
	lowp vec4 baseColor;
} outData;

void main()
{
	outData.baseColor = vec4(1.);
#ifdef INSTANCED
	gl_Position = uCameraMatsViewProj * ciModelMatrix  * vInstanceMatrix * ciPosition;
#else
	gl_Position = uCameraMatsViewProj * ciModelMatrix  * ciPosition;
#endif

}