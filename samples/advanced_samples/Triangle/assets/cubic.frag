#version 320 es

precision mediump float;
in vec4 in_TEXCOORD0;
uniform sampler2D u_InputTexture;
uniform float uUnit;
uniform float vUnit;
out vec4 out_Target0;

float BiCubicPoly1(float x, float a)
{
    x = abs(x);
    float res = (a+float(2))*x*x*x - (a+float(3))*x*x + float(1);
    return res;
}
float BiCubicPoly2(float x, float a)
{
    x = abs(x);
    float res = a*x*x*x - float(5)*a*x*x + float(8)*a*x - float(4)*a;
    return res;
}

void main()
{
vec2 basic; 
vec2 det; 
basic = in_TEXCOORD0.xy * vec2(uUnit, vUnit) - vec2(0.5,0.5); 
det = fract(basic); 
out_Target0 = vec4(0.0,0.0,0.0,0.0)
+BiCubicPoly2(det.x-float(-1), float(-0.5))*BiCubicPoly2(det.y-float(-1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(-1), float(-1)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(-1), float(-0.5))*BiCubicPoly1(det.y-float(0), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(-1), float(0)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(-1), float(-0.5))*BiCubicPoly1(det.y-float(1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(-1), float(1)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(-1), float(-0.5))*BiCubicPoly2(det.y-float(2), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(-1), float(2)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(0), float(-0.5))*BiCubicPoly2(det.y-float(-1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(0), float(-1)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(0), float(-0.5))*BiCubicPoly1(det.y-float(0), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(0), float(0)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(0), float(-0.5))*BiCubicPoly1(det.y-float(1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(0), float(1)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(0), float(-0.5))*BiCubicPoly2(det.y-float(2), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(0), float(2)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(1), float(-0.5))*BiCubicPoly2(det.y-float(-1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(1), float(-1)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(1), float(-0.5))*BiCubicPoly1(det.y-float(0), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(1), float(0)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(1), float(-0.5))*BiCubicPoly1(det.y-float(1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(1), float(1)))/vec2(uUnit, vUnit))
+BiCubicPoly1(det.x-float(1), float(-0.5))*BiCubicPoly2(det.y-float(2), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(1), float(2)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(2), float(-0.5))*BiCubicPoly2(det.y-float(-1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(2), float(-1)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(2), float(-0.5))*BiCubicPoly1(det.y-float(0), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(2), float(0)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(2), float(-0.5))*BiCubicPoly1(det.y-float(1), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(2), float(1)))/vec2(uUnit, vUnit))
+BiCubicPoly2(det.x-float(2), float(-0.5))*BiCubicPoly2(det.y-float(2), float(-0.5))*texture(u_InputTexture, in_TEXCOORD0.xy + ( - det + vec2(float(2), float(2)))/vec2(uUnit, vUnit));
}