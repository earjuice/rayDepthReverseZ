// passthrough fragment shader
#version 460 core



in VertexData {
	vec4 baseColor;
};
out vec4	oColor;

void main() {

	oColor	= vec4(1.);

}