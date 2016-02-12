// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Encapsulate the math for how to interact with a planar pointer (i.e. mouse) 
// to generate the view matrix for an eye point on a virtual sphere around a 
// lookat point. Arcball can be constrained to two axes or left unconstrained. 
// Unconstrained is the "classic" arcball like what you would find on the web. 
// The constrained versions are found in tools like 3DSMax and Maya.

#pragma once
#include <oMath/hlsl.h>
#include <cstdint>

namespace ouro {

class arcball
{
public:
	enum class constraint : uint8_t
	{
		none, // classic arcball
		y_up, // Maya-like, prevents non-explicit roll
		z_up, // Max-like, prevents non-explicit roll
	};

	typedef enum class constraint constraint_t;

	enum class type : uint8_t
	{
		none,	 // do no manipulation
		orbit, // move eye position on a virtual sphere around the lookat point
		pan,	 // move in screen space
		dolly, // move in and out of screen space (translate, not zoom)
		look,  // rotate around eye point (not lookat point)
	};

	typedef enum class type type_t;


	// === infrequent state setting ===

	arcball(constraint_t c = constraint::none);

	inline void         constraint(constraint_t c)               { constraint_ = c;    }
	inline constraint_t constraint()                       const { return constraint_; }

	inline void         rotation(const float4& quaternion)       { quat_ = quaternion; }
	inline float4       rotation()                         const { return quat_;       }

	inline void         translation(const float3& tx)            { tx_ = tx;           }
	inline float3       translation()                      const { return tx_;         }

	inline void         lookat(const float3& position)           { lookat_ = position; }
	inline float3       lookat()                           const { return lookat_;     }

	inline type_t       type()                             const { return type_;       }

	void                view(const float4x4& v);
	float4x4            view() const;

	// translate (no rotation change) to fit sphere in view
	void focus(float fovy_radians, const float4& sphere);

	// translate and rotate to center the at-point
	void focus(const float3& eye, const float3& at);


	// === per-frame update ===

	// pass in type of manipulation and the change in mouse coordinates scaled 
	// by some value that 'feels right'. Ensure that when manipulation is finished
	// either one last update with type::none is called, or this update is called
	// every frame even with type::none. This returns the prior state.
	type_t update(const type_t& type, const float3& screen_space_delta);

	// moves eye and lookat relative to the current orientation
	void walk(const float3& delta);

private:
	float4       quat_;				// current rotation (i.e. as part of the view-to-world matrix)
	float3       euler_;      // current rotation as 3-axes values (applied in zyx order)
	float3       tx_;					// current eye position (i.e. as part of the view-to-world matrix)
	float3       lookat_;			// the center around which orbiting occurs
	float        polarity_;		// a value used to ensure controls make sense even when upside down
	constraint_t constraint_;	// ctor-time setting to control orbiting behavior
	type_t       type_;       // tracks current type, mainly to identify changes in type
};

}
