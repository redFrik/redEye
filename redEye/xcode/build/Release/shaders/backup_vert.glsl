#version 120

uniform vec2 iResolution;
uniform float iGlobalTime;

//varying vec4 col;

void main() {
	gl_FrontColor = gl_Color;//*(sin(iGlobalTime*60)*0.5+0.5);
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
    //vec4 v= vec4(gl_Vertex);
    //gl_FrontColor= gl_Color;
	//gl_Position= gl_ModelViewProjectionMatrix*v;
	//gl_Position = ftransform();
}
