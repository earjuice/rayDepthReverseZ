
#version 460 core

uniform vec3 iResolution;
uniform float iAspect;
uniform float iGlobalTime;


uniform vec3 iCamEyePos;
uniform vec3 iCamViewDir;
uniform float zNear, zFar;
uniform vec2 uProjectionParams;// = vec2(zFar / (zFar - zNear), (-zFar * zNear) / (zFar - zNear));

uniform float uViewDistance;
uniform mat3 uCameraDirection;
//uniform mat4 uCameraMatsViewProj;
uniform vec4 sphereColor=vec4(0.9, 0.8, 0.6, 1.0); 	//color:0.9,0.8,0.6,1.0
uniform vec3 sphpos= vec3(0.0); 			//ui:-10.0,10.0,1.0
uniform vec3 light = vec3(-10.0, 10.0, 1.0); 			//ui:-10.0,10.0,1.0
uniform float radius= 5.; 		//slider:0.0,2.0,1.0
uniform float shadowScale=2.; 	//slider:1.0,4.0,2.0
uniform float plHeight=5.; 	//slider:-5.0,5.0,0.0
uniform float ampAmnt=1.; 	//slider:0.0,10.0,0.0

const vec3 cGammaCorrection = vec3(0.4545454545);
vec4 sph1 = vec4(sphpos, radius);


in VertexData {
	vec2 texCoord;
};

out vec4	oColor;

// result suitable for assigning to gl_FragDepth
float depthSample(float linearDepth)
{
	float nonLinearDepth = (zFar + zNear - 2.0 * zNear * zFar / linearDepth) / (zFar - zNear);
	nonLinearDepth = (nonLinearDepth + 1.0) / 2.0;
	return nonLinearDepth;
}


vec3 gamma(in vec3 color)
{
	return pow(color, cGammaCorrection);
}

vec4 gamma(in vec4 color)
{
	return vec4(gamma(color.rgb), color.a);
}



float iSphere(in vec3 ro, in vec3 rd, in vec4 sph)
{
	//so a sphere centered at the origin has the equation |xyz| = r
	//meaning, |xyz|^2 = r^2, meaning <xyz,xyz> = r^2
	//now, xyz = ro + t * rd, |xyz|^2 = (ro + t*rd) * (ro + t*rd)
	//therefore r^2 = |ro|^2 + t^2 * |rd|^2 + 2.0 * t * <ro,rd>, |rd|^ 2 = 1.0
	//thus t^2 + 2*t*<ro,rd> + |ro|^2 - r^2 = 0.0;
	//this is a quadratic equation: a*x^2 + b*x + c = 0
	//Solution to quadratic: x = ( -b +- sqrt( b^2 - 4 * a * c ) ) / ( 2 * a )
	//solve for t
	vec3 oc = ro - sph.xyz;
	float r = sph.w ;
	float a = 1.0;
	float b = 2.0 * dot(oc, rd);
	float c = dot(oc, oc) - r * r;
	float h = b * b - 4.0 * a * c;
	if (h < 0.0) return -1.0;
	float t = (-b - sqrt(h)) / (2.0 * a);
	return t;
}

float iPlane(in vec3 ro, in vec3 rd)
{
	//equation of a plane, y = 0, ro.y + t * rd
	return -ro.y / rd.y;
}

vec3 nSphere(in vec3 pos, in vec4 sph)
{
	return (pos - sph.xyz) / sph.w;
}

vec3 nPlane(in vec3 pos)
{
	return vec3(0.0, 1.0, 0.0);
}

float intersect(in vec3 ro, in vec3 rd, out float resT) {
	resT = 1000.0;
	float id = -1.0;
	float tsph = iSphere(ro, rd, sph1);	//intersect with a sphere
	float tpla = iPlane(ro + vec3(plHeight), rd);	//intersect with a plane
	if (tsph > 0.0) {
		id = 1.0;
		resT = tsph;
	}
	if (tpla > 0.0 && tpla < resT) {
		id = 2.0;
		resT = tpla;
	}
	return id;
}

vec4 scene(vec3 ro, vec3 rd, vec2 uv)
{

	float t;
	float id = intersect(ro, rd, t);
	vec3 nor;
	vec3 col = vec3(.6,.2,.2);
	if (id > 0.5 && id < 1.5) {
		//we hit the sphere
		vec3 pos = ro + t * rd;
		nor = nSphere(pos, sph1);
		//oNormal = vec4(nor, 1.f);
		float dif = clamp(dot(nor, normalize(light - (pos - ro))), 0.0, 1.0);
		float ao = 0.5 + 0.5*nor.y;
		col = ( 1.0 - sphereColor.rgb ) * dif * ao + vec3( sphereColor.rgb ) * ao;
		//col = nor;
	}
	else if (id > 1.5) {
		// we hit the plane
		vec3 pos = ro + t * rd;
		nor = nPlane(pos);
		float dif = clamp(dot(nor, light), 0.0, 1.0);
		float height = clamp((5.0 - (sph1.y - sph1.w)) / 5.0, 0.0, 1.0);
		float amb = smoothstep(0.0, shadowScale * (height * sph1.w), length(pos.xz - sph1.xz));
		amb += (1.0 - amb) * (1.0 - height);
		col *= amb;
		//col=nor;
	}

	col = gamma(col);
	//return vec4( nor, t  );
	return vec4(col, t);
}


void main() {

	highp vec3 rayOrigin = iCamEyePos;
	vec2 sUV = gl_FragCoord.xy / iResolution.xy;

	vec2 uv = -1.0 + 2.0 * sUV; // screenPos can range from -1 to 1 texCoord.xy;//
	uv *= .5;
	uv.x *= iAspect;


	vec3 rayDirection = uCameraDirection * normalize(vec3(uv, -uViewDistance));
	//vec3 rayDirection = vec3(uCameraMatsViewProj * normalize(vec4(uv.x, uv.y, -uViewDistance, 1.)));

	//vec4 color = texture( iScreenTexture, sUV );
	//float texDepth = getTexDepth(sUV);//texture( iScreenDepthTexture, sUV );

#ifdef	REVERSE_Z
	//float depth = reverseSimple(texDepth);//convert from 32F reverse depth texture infinuteFar 1-0 to 0-... linear depth 
#else
	//float depth = linearDepth(texDepth);//linearSimple(texDepth);//
#endif //REVERSE_Z


										//scene render
	vec4 scn = scene(rayOrigin, rayDirection, uv);

	//float dCompare=compareDepth(scn.a,depth,1.f);
#ifdef	REVERSE_Z
	gl_FragDepth = zNear / scn.a;

#else
	gl_FragDepth = depthSample(scn.a);

#endif //REVERSE_Z

	oColor = vec4(scn.rgb, 1.);//iBackgroundColor;
	//oColor = vec4(rayDirection.rgb, 1.);//iBackgroundColor;
	//oColor = vec4(uv.x);//iBackgroundColor;


}

