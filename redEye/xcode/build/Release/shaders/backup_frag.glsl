#version 120

uniform vec2 iResolution;
uniform float iGlobalTime;

varying vec4 col;

void main() {
    vec2 v= gl_FragCoord.xy;
    gl_FragColor= gl_Color*(1.0-(abs(length(v-(iResolution*0.5)))/iResolution.x));
}
