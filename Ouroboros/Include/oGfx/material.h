// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once




// Random notes from the DOTA2 hero shadrer
//Texture0: RGB color + alpha microgeometry
//Texture1: RG normal map: will have 1 channel available <-- emissive
//
//Texture2:
//-R spec:  this is multiplied by a material constant for brightness of spec
//-G gloss: mult by some constant for glossiness (even if engine-internal)
//SpecTint: 0 means white spec, 1 means color of diffuse
//Metalness: darken diffuse color & rimlight... respect spectint
//
//

// Disney's PBR: [0,1]
// Subsurface
// Metallic
// Specular
// Specular tint
// Roughness
// Aniso
// Sheen
// Sheen tint
// Clearcoat
// Clearcoat gloss

// UE4 simplifies to:
// BaseColor
// Metallic
// Roughness (1-glossiness)
// Cavity (AO maps?)

// None describe transmission explicitly

// MIA:

//Basically four layers, top to bottom:
//-Reflections (which can be glossy or specular)
//-Diffuse (Oren Nayar <->Lambert)
//-Transparency (also glossy or specular)
//-Translucency (basically Lambertian shading on the ”flip side”)


namespace ouro { namespace gfx {

enum class material_texture
{
	diffuse_alpha,
	specular,
};

struct material_constants
{
	float specular_scale; // final specular = texture-sampled value [0,1] * this
	float gloss_scale;    // final gloss = texture-sampled [0,1] * this
	float emissive_scale; // final emissive = texture-sampled [0,1] * this
};


class material
{
public:

	texture2d_t diffuse;
	texture2d_t specular;
	texture2d_t normal;
};

}}
