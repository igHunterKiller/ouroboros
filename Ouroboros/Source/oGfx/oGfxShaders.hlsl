// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/gpu_signature_slots.h>
#include <oGfx/vertex_layouts.h>

#define oGFX_SAMPLER_SLOT(x) register(s##x)
#define oGFX_SRV_SLOT(x) register(t##x)

// _____________________________________________________________________________
// oGPU signature resources

Texture2D t_depth : oGFX_SRV_SLOT(oGFX_SRV_DEPTH);

// _____________________________________________________________________________
// oGPU samplers

SamplerState PointClamp   : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_POINT_CLAMP);
SamplerState PointWrap    : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_POINT_WRAP);
SamplerState LinearClamp  : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_LINEAR_CLAMP);
SamplerState LinearWrap   : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_LINEAR_WRAP);
SamplerState AnisoClamp   : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_ANISO_CLAMP);
SamplerState AnisoWrap    : oGFX_SAMPLER_SLOT(oGFX_SAMPLER_ANISO_WRAP);

// _____________________________________________________________________________
// Vertex Shaders

#define VS_START(type) INTcommon VS##type(uint instance : SV_InstanceID, VTX##type In) { INTcommon Out = (INTcommon)0; Out.LSposition = In.position; Out.WSposition = gfx_ls2ws(instance, In.position); Out.SSposition = gfx_ls2ss(instance, In.position);
#define VS_END return Out; }
#define VS_BTN transform_btn(instance, In.normal, In.tangent, Out.WSnormal, Out.WStangent, Out.WSbitangent);

// Handedness is stored in w component of tangent_and_facing. This should be 
// called out of the vertex shader and the out values passed through to the 
// pixel shader. For world-space pass the world matrix. For view space pass the 
// WorldView matrix. Read more: http://www.terathon.com/code/tangent.html.
inline void transform_btn(oIN(float4x4, tx)
	, oIN(float3, normal)
	, oIN(float4, tangent_and_facing)
	, oOUT(float3, out_bitangent)
	, oOUT(float3, out_tangent)
	, oOUT(float3, out_normal))
{
	float3x3 r = (float3x3)tx;
	out_normal = mul(r, normal);
	out_tangent = mul(r, tangent_and_facing.xyz);
	out_bitangent = cross(out_normal, out_tangent) * tangent_and_facing.w;
}

VS_START(pos)
VS_END

VS_START(pos_nrm_tan)
	VS_BTN
VS_END

VS_START(pos_uv0)
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
VS_END

VS_START(pos_uvw)
	Out.texcoord0 = In.texcoord0;
	return Out;
VS_END

VS_START(pos_col)
	Out.color = In.color0;
VS_END

VS_START(pos_uv0_col)
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
	Out.color = In.color0;
VS_END

VS_START(pos_nrm_tan_uv0)
	VS_BTN
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
VS_END

VS_START(pos_nrm_tan_uvw)
	VS_BTN
	Out.texcoord0 = In.texcoord0;
VS_END

VS_START(pos_nrm_tan_col)
	VS_BTN
	Out.color = In.color0;
VS_END

VS_START(pos_nrm_tan_uv0_uv1)
	VS_BTN
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
	Out.texcoord1 = In.texcoord1;
VS_END

VS_START(pos_nrm_tan_uv0_col)
	VS_BTN
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
	Out.color = In.color0;
VS_END

VS_START(pos_nrm_tan_uvw_col)
	VS_BTN
	Out.texcoord0 = In.texcoord0;
	Out.color = In.color0;
VS_END

VS_START(pos_nrm_tan_uv0_uv1_col)
	VS_BTN
	Out.texcoord0 = float3(In.texcoord0, 0.0f);
	Out.texcoord1 = In.texcoord1;
	Out.color = In.color0;
VS_END

INTpos VSpos_passthru(uint instance : SV_InstanceID, VTXpos In)
{
	INTpos Out = (INTpos)0;
	Out.SSposition = float4(In.position, 1);
	return Out;
}

INTpos_col VSpos_col_passthru(uint instance : SV_InstanceID, VTXpos_col In)
{
	INTpos_col Out = (INTpos_col)0;
	Out.SSposition = float4(In.position, 1);
	Out.color = In.color0;
	return Out;
}

INTpos_col VSpos_col_prim(uint instance : SV_InstanceID, VTXpos_col In)
{
	INTpos_col Out = (INTpos_col)0;
	Out.SSposition = gfx_ls2ss(instance, In.position);
	Out.color = In.color0;
	return Out;
}

INTpos_uv0 VSfullscreen_tri(uint id : SV_VertexID)
{
	INTpos_uv0 Out = (INTpos_uv0)0;
	Out.texcoord0 = float2((id << 1) & 2, id & 2); 
	Out.SSposition = float4(Out.texcoord0 * float2(2, -2) + float2(-1, 1), 0, 1);
	return Out;
}

INTcommon VSpos_as_uvw(uint instance : SV_InstanceID, VTXpos_uvw In)
{
	INTcommon Out = (INTcommon)0;
	Out.LSposition = In.position;
	Out.WSposition = gfx_ls2ws(instance, In.position);
	Out.SSposition = gfx_ls2ss(instance, In.position);
	Out.texcoord0 = In.position;
	return Out;
}

// _____________________________________________________________________________
// Solid Color Pixel Shaders

// Solid Colors
float4 PSblack()   : SV_Target { return float4(0.0,0.0,0.0,1); }
float4 PSgray()    : SV_Target { return float4(0.5,0.5,0.5,1); }
float4 PSwhite()   : SV_Target { return float4(1.0,1.0,1.0,1); }
float4 PSred()     : SV_Target { return float4(1.0,0.0,0.0,1); }
float4 PSgreen()   : SV_Target { return float4(0.0,1.0,0.0,1); }
float4 PSblue()    : SV_Target { return float4(0.0,0.0,1.0,1); }
float4 PSyellow()  : SV_Target { return float4(1.0,1.0,0.0,1); }
float4 PSmagenta() : SV_Target { return float4(1.0,0.0,1.0,1); }
float4 PScyan()    : SV_Target { return float4(0.0,1.0,1.0,1); }

// _____________________________________________________________________________
// Simple Interpolants Pixel Shaders

Texture1D Simple1D : register(t0);
Texture1DArray Simple1DArray : register(t0);
Texture2D Simple2D : register(t0);
Texture2DArray Simple2DArray : register(t0);
Texture3D Simple3D : register(t0);
TextureCube SimpleCube : register(t0);
TextureCubeArray SimpleCubeArray : register(t0);

float4 PStexture1d(INTcommon In) : SV_Target { return Simple1D.Sample(LinearWrap, In.texcoord0.x); }
float4 PStexture1d_array(INTcommon In) : SV_Target { return Simple1DArray.Sample(LinearWrap, In.texcoord0.x); }
float4 PStexture2d(INTcommon In) : SV_Target { return Simple2D.Sample(LinearWrap, In.texcoord0.xy); }
float4 PStexture2d_array(uint instance : SV_InstanceID, INTcommon In) : SV_Target { return Simple2DArray.Sample(LinearWrap, float3(In.texcoord0.xy, oGfxGetSlice(instance))); }
float4 PStexture3d(INTcommon In) : SV_Target { return Simple3D.Sample(LinearWrap, In.texcoord0); }
float4 PStexture_cube(INTcommon In) : SV_Target { return SimpleCube.Sample(LinearWrap, In.texcoord0); }
float4 PStexture_cube_array(uint instance : SV_InstanceID, INTcommon In) : SV_Target { return SimpleCubeArray.Sample(LinearWrap, float4(In.texcoord0, oGfxGetSlice(instance))); }
float4 PSconstant_color(INTcommon In) : SV_Target { return oGfxGetColor(In.instance); }
float4 PSvertex_color(INTcommon In) : SV_Target { return In.color; }
float4 PSvertex_color_prim(INTpos_col In) : SV_Target { return In.color; }
float4 PStexcoordu(INTcommon In) : SV_Target { return float4(frac(In.texcoord0.x), 0, 0, 1); }
float4 PStexcoordv(INTcommon In) : SV_Target { return float4(0, frac(In.texcoord0.y), 0, 1); }
float4 PStexcoord(INTcommon In) : SV_Target { return float4(frac(In.texcoord0.xy), 0, 1); }
float4 PStexcoord3u(INTcommon In) : SV_Target { return float4(frac(In.texcoord0.x), 0, 0, 1); }
float4 PStexcoord3v(INTcommon In) : SV_Target { return float4(0, frac(In.texcoord0.y), 0, 1); }
float4 PStexcoord3w(INTcommon In) : SV_Target { return float4(0, 0, frac(In.texcoord0.z), 1); }
float4 PStexcoord3(INTcommon In) : SV_Target { return float4(frac(In.texcoord0), 1); }

// _____________________________________________________________________________
// Misc

float PSmouse_depth(float4 SSposition : SV_Position, float2 texcoord0 : TEXCOORD0) : SV_Target
{
	const float2 mouse_texcoords = oGfx_misc_constants.a.xy;
	const float hyper_depth      = t_depth.Sample(PointClamp, mouse_texcoords).x;

	return gfx_linearize_depth(hyper_depth);
}

float PSlinearize_depth(float4 SSposition : SV_Position) : SV_Target
{
	const int3  screen_position = (int3)SSposition.xyz;
	const float hyper_depth     = t_depth.Load(int3(screen_position.x, screen_position.y, 0)).x;

	return gfx_linearize_depth(hyper_depth);
}

float4 PSvertex_color_stipple(INTcommon In) : SV_Target
{
	// if the geometry would be occluded, draw a stipple pattern
	// (thus turn depth test off for this since it will be done
	// here and special rendering is done if it fails

	const int3  screen_position       = (int3)In.SSposition.xyz;
  const float fragment_linear_depth = In.SSposition.w;
 	const float scene_linear_depth    = t_depth.Load(int3(screen_position.x, screen_position.y, 0)).x;
	const bool  occluded              = fragment_linear_depth > scene_linear_depth;

  if (occluded && ((screen_position.x ^ screen_position.y) & 2))
    discard;

	return In.color;
}

// _____________________________________________________________________________
// Geometry Shaders

// expands a vertex position into a line represent a vector
void oGSexpand_vector(float3 WSposition, float3 WSvector, float4 color, inout LineStream<INTpos_col> out_line)
{
	static const float3 kVectorScale = 0.025;
	INTpos_col Out = (INTpos_col)0;
	Out.color = color;
	Out.SSposition = gfx_ws2ss(WSposition);
	out_line.Append(Out);
	Out.SSposition = gfx_ws2ss(WSvector * kVectorScale + WSposition);
	out_line.Append(Out);
}

[maxvertexcount(2)]
void GSvertex_normals(point INTcommon In[1], inout LineStream<INTpos_col> out_line)
{
	oGSexpand_vector(In[0].WSposition, In[0].WSnormal, float4(0,1,0,1), out_line);
}

[maxvertexcount(2)]
void GSvertex_tangents(point INTcommon In[1], inout LineStream<INTpos_col> out_line)
{
	oGSexpand_vector(In[0].WSposition, In[0].WStangent.xyz, float4(1,0,0,1), out_line);
}

#if 0

#include <oCompute/oComputeConstants.h>
#include <oCompute/oComputePhong.h>
#include <oCompute/oComputeProcedural.h>
#include <oCompute/oComputeUtil.h>

#include <oGfx/oGfxDrawConstants.h>
#include <oGfx/oGfxLightConstants.h>
#include <oGfx/oGfxMaterialConstants.h>
#include <oGfx/oGfxViewConstants.h>
#include <oGfx/oGfxVertexLayouts.h>

struct oGFX_INSTANCE
{
	float3 Translation;
	quatf Rotation;
};

#define MAX_NUM_INSTANCES (64)

cbuffer cb_GfxInstances { oGFX_INSTANCE GfxInstances[MAX_NUM_INSTANCES]; }

struct oGFX_VS_OUT_UNLIT
{
	float4 SSPosition : SV_Position;
	float4 Color : CLR0;
};

struct oGFX_VS_OUT_LIT
{
	float4 SSPosition : SV_Position;
	float3 WSPosition : POS0;
	float3 LSPosition : POS1;
	float VSDepth : VSDEPTH;
	float3 WSNormal : NML0;
	float3 VSNormal : NML1;
	float3 WSTangent : TAN0;
	float3 WSBitangent : TAN1;
	float2 Texcoord : TEX0;
};

struct oGFX_GBUFFER_FRAGMENT
{
	float4 Color : SV_Target0;
	float2 VSNormalXY : SV_Target1;
	float LinearDepth : SV_Target2;
	float2 LinearDepth_DDX_DDY : SV_Target3;
	float4 Mask0 : SV_Target4; // R=Outline strength, GBA=Unused
};

oGFX_VS_OUT_LIT VSRigid(vertex_pos_nrm_tan_uv0 In)
{
	oGFX_VS_OUT_LIT Out = (oGFX_VS_OUT_LIT)0;
	Out.SSPosition = gfx_ls2ss(In.position);
	Out.WSPosition = oGfxLStoWS(In.position);
	Out.LSPosition = In.position;
	Out.VSDepth = oGfxLStoVS(In.position).z;
	oGfxRotateBasisLStoWS(In.normal, In.tangent, Out.WSNormal, Out.WSTangent, Out.WSBitangent);
	Out.VSNormal = oGfxRotateLStoVS(In.normal);
	Out.Texcoord = In.texcoord;
	return Out;
}
	
oGFX_VS_OUT_LIT VSRigidInstanced(vertex_pos_nrm_tan_uv0 In, uint _InstanceID : SV_InstanceID)
{
	oGFX_INSTANCE Instance = GfxInstances[_InstanceID];
	oGFX_VS_OUT_LIT Out = (oGFX_VS_OUT_LIT)0;
	Out.SSPosition = oGfxWStoSS(Out.WSPosition);
	Out.WSPosition = qmul(Instance.Rotation, In.position) + Instance.Translation;
	Out.LSPosition = In.position;
	Out.VSDepth = oGfxWStoVS(Out.WSPosition).z;
	oQRotateTangentBasisVectors(Instance.Rotation, In.normal, In.tangent, Out.WSNormal, Out.WSTangent, Out.WSBitangent);
	Out.VSNormal = oGfxRotateWStoVS(qmul(Instance.Rotation, In.normal));
	Out.Texcoord = In.texcoord;
	return Out;
}

void VSShadow(in float3 LSPosition : POSITION, out float4 _Position : SV_Position, out float _VSDepth : VIEWSPACEZ)
{
	_Position = gfx_ls2ss(LSPosition);
	_VSDepth = oGfxNormalizeDepth(oGfxLStoVS(LSPosition).z);
}

static const float3 oCHALKYBLUE = float3(0.44, 0.57, 0.75);
static const float3 oSMOKEYWHITE = float3(0.88, 0.9, 0.96);

float4 PSGrid(oGFX_VS_OUT_LIT In) : SV_Target
{
	float GridFactor = oCalcFadingGridShadeIntensity2D(In.LSPosition.xy, oCalcMipSelection(In.LSPosition.xy), 0.2, 10, 0.05);
	float3 Color = lerp(oSMOKEYWHITE, oCHALKYBLUE, GridFactor);
	return float4(Color, 1);
}

// The Hero Shader is a reimplementation of the material interface described by
// Valve for DOTA2 here:
// http://media.steampowered.com/apps/dota2/workshop/Dota2ShaderMaskGuide.pdf

//cbuffer
//{
static const float GfxMaterialConstants_IoR = IoRAir;
static const float GfxMaterialConstants_FresnelFalloff = 1.0;
static const float GfxMaterialConstants_BumpScale = 1.0;
//};

oGFX_GBUFFER_FRAGMENT PSHero(oGFX_VS_OUT_LIT In)
{
	oGfxMaterialConstants M = oGfxGetMaterial(In.Texcoord);

	// Calculate relevant values in world-space 

	float3 WSEyePosition = oGfxGetEyePosition();
	float3 WSEyeVector = normalize(WSEyePosition - In.WSPosition);

	// Just grab the first light for now
	oGfxLight Light = oGfxGetLight(0);
	float3 WSLightVector = normalize(Light.WSPosition - In.WSPosition);

	// @tony: It'd be good to add a detail normal map as well, but will that
	// need a separate mask from the color detail map? Maybe it's just part of a 
	// detail material.
  float3 WSUnnormalizedNormal = oDecodeTSNormal(
		In.WSTangent
		, In.WSBitangent
		, In.WSNormal
		, M.SampledTSNormal.xyz
		, GfxMaterialConstants_BumpScale);

	// Do phong-style lighting
	float3 WSNormal;
	float4 Lit = oLitHalfLambertToksvig(WSUnnormalizedNormal, WSLightVector, WSEyeVector, M.SpecularExponent, WSNormal);

	// Decode lit and material mask values into more familiar coefficients
	// NOTE: DetailMap, Rimlight, Fresnel color not yet implmented
	const float Ke = M.SelfIlluminationIntensity;
	const float Kd = (1 - M.MetalnessIntensity) * Lit.y;
	const float Ks = M.SpecularIntensity * Lit.z;
	const float Kf = M.DiffuseFresnelIntensity * oFresnel(
		WSEyeVector
		, WSNormal
		, IoRAir
		, GfxMaterialConstants_IoR
		, GfxMaterialConstants_FresnelFalloff);

	const float3 Ce = M.Color;
	const float3 Cd = M.Color;
	const float3 Cs = lerp(oWHITE3, Cd, M.SpecularTintIntensity);
	
	// Valve has this from a 3D texture (how is it 3D? there's 2D texcoords AFAICT 
	// and only one intensity value). This probably will come from engine values,
	// such as a realtime reflection map, so stub this for now.
	const float3 Cf = oWHITE3;

	// Assign output
	oGFX_GBUFFER_FRAGMENT Out = (oGFX_GBUFFER_FRAGMENT)0;
	Out.VSNormalXY = oFullToHalfSphere(normalize(In.VSNormal));
	Out.LinearDepth = In.VSDepth;

	// @tony: this is not proving to be all that useful...
	Out.LinearDepth_DDX_DDY = float2(abs(100* max(ddx(In.Texcoord))), abs(100 * max(ddy(In.Texcoord))));
	Out.Mask0 = float4(0,0,0,0); // outline mask is off by default

	// ambient component ignored/emulated by emissive
	Out.Color = float4(Ke*Ce + Kd*Cd + Ks*Cs + Kf*Cf, M.Opacity);
	return Out;
}

struct VSOUT
{
	float4 Color : SV_Target;
};

oGFX_GBUFFER_FRAGMENT PSMaterial(oGFX_VS_OUT_LIT In)
{
	oGFX_GBUFFER_FRAGMENT Out = (oGFX_GBUFFER_FRAGMENT)0;	

	float4 Diffuse = oGfxColorSample(In.Texcoord);

	float3 E = oGfxGetEyePosition();
	float3 EyeVector = normalize(E - In.WSPosition);
	oGfxLight Light = oGfxGetLight(0);
	float3 L = normalize(Light.WSPosition - In.WSPosition);
	float3 HalfVector = normalize(EyeVector + L);

  float3 WSUnnormalizedNormal = oDecodeTSNormal(In.WSTangent, In.WSBitangent, In.WSNormal
		, oGfxNormalSample(In.Texcoord).xyz
		, 1.0f); // FIXME: Figure out what mask channel this comes from
		
	
	oRGBf rgb = oPhongShade(normalize(WSUnnormalizedNormal)
									, L
									, EyeVector
									, 1
									, oBLACK3
									// Figure out where these other parameters come from in the mask texture
									, oBLACK3
									, Diffuse.rgb
									, oBLACK3
									, oZERO3
									, oZERO3
									, oWHITE3
									, 1
									, 0.1f);

	Out.Color = float4(rgb, 1);
	Out.VSNormalXY = oFullToHalfSphere(normalize(In.VSNormal));
	Out.LinearDepth = oGfxNormalizeDepth(In.VSDepth);
	Out.Mask0 = float4(0,0,0,0);
	return Out;
}

uint PSObjectID(float4 Position : SV_Position) : SV_Target
{
	return oGfxGetObjectID();
}

void PSShadow(in float4 _Position : SV_Position, in float _VSDepth : VIEWSPACEZ, out float _Depth : SV_Depth) 
{ 
	_Depth = _VSDepth;
}

float4 PSTexcoord(oGFX_VS_OUT_LIT In) : SV_Target
{
	return float4(In.Texcoord.xy, 0, 1);
}

Texture2D ColorTexture : register(t0);
SamplerState Sampler : register(s0);

float4 PSTextureColor(oGFX_VS_OUT_LIT In) : SV_Target
{
	return ColorTexture.Sample(Sampler, In.Texcoord);
}

Texture2D DiffuseTexture : register(t0);

oGFX_GBUFFER_FRAGMENT PSVertexPhong(oGFX_VS_OUT_LIT In)
{
	float4 Diffuse = DiffuseTexture.Sample(Sampler, In.Texcoord);
	
	float3 E = oGfxGetEyePosition();
	float3 ViewVector = normalize(E - In.WSPosition);
	oGfxLight Light = oGfxGetLight(0);
	float3 L = normalize(Light.WSPosition - In.WSPosition);

	oGFX_GBUFFER_FRAGMENT Out = (oGFX_GBUFFER_FRAGMENT)0;	
	oRGBf rgb = oPhongShade(normalize(In.WSNormal)
									, L
									, ViewVector
									, 1
									, oBLACK3
									 // Figure out where these other parameters come from in the mask texture
									, oBLACK3
									, Diffuse.rgb
									, oBLACK3
									, oZERO3
									, oZERO3
									, oWHITE3
									, 1
									, 0.1f);

	Out.Color = float4(rgb, 1);
	return Out;
}

float4 PSVSDepth(oGFX_VS_OUT_LIT In) : SV_Target
{
	return float4(oGfxNormalizeDepth(In.VSDepth).xxx, 1);
}

float4 PSWSPixelNormal(oGFX_VS_OUT_LIT In) : SV_Target
{
  float3 WSNormal = normalize(oDecodeTSNormal(
		In.WSTangent
		, In.WSBitangent
		, In.WSNormal
		, oGfxNormalSample(In.Texcoord).rgb
		, 1.0));
	return float4(colorize_vector(WSNormal), 1);
}

float4 PSWSVertexNormal(oGFX_VS_OUT_LIT In) : SV_Target
{
	return float4(colorize_vector(normalize(In.WSNormal)), 1);
}

#endif