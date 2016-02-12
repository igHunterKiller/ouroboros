// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// encapsulation of all the details for setting up projection of world-space 
// objects.

#pragma once
#include <oMath/hlsl.h>
#include <oMath/hlslx.h>
#include <oMath/matrix.h>
#include <oMath/floats.h>
#include <array>

#define oFAR_VEC_TOP_LEFT     0
#define oFAR_VEC_BOTTOM_LEFT  1
#define oFAR_VEC_TOP_RIGHT    2
#define oFAR_VEC_BOTTOM_RIGHT 3

namespace ouro {

struct viewport_t
{
	// A typical full-screen render should have a position of 0,0 and 
	// dimensions of the width and height of the render target in pixels
	// and min and max should be 0 and 1 respectively unless some specific
	// technique is being employed.

	viewport_t(const float2& dim = float2(0.0f, 0.0f))                                                 : position(0.0f, 0.0f), dimensions(dim),                        min_depth(0.0f),       max_depth(1.0f)       {}
	viewport_t(const uint2& dim)                                                                       : position(0.0f, 0.0f), dimensions((float)dim.x, (float)dim.y), min_depth(0.0f),       max_depth(1.0f)       {}
	viewport_t(const float2& pos, const float2& dim, float min_depth_ = 0.0f, float max_depth_ = 1.0f) : position(pos),        dimensions(dim),                        min_depth(min_depth_), max_depth(max_depth_) {}

	float2 position;
	float2 dimensions;
	float min_depth;
	float max_depth;

	float aspect_ratio() const                              { return dimensions.x / dimensions.y; }
	float2 viewport_to_texcoord(const float2& vp_pos) const { return (vp_pos - position) / ( dimensions - 1.0f); }
	float2 viewport_to_ndc(const float2& vp_pos)      const { return viewport_to_texcoord(vp_pos) * 2.0f - 1.0f; } 
};

class pov_t
{
public:
	pov_t(const uint2& viewport_dimensions = uint2(0, 0)
		, float fovx = oRECOMMENDED_PC_FOVX_RADIANSf
		, float near = oDEFAULT_NEAR_CLIPf
		, float far = oDEFAULT_FAR_CLIPf);

	// ___________________________________________________________________________________________________________________________________
	// view elements
	
	void view(const float4x4& view)                                            { view_ = view; update();                                 }
	const float4x4& view()                                               const { return view_;                                           }
	const float4x4& view_inverse()                                       const { return view_inverse_;                                   }
	float3 right()                                                       const { return view_inverse_[0].xyz();                          }
	float3 up()                                                          const { return view_inverse_[1].xyz();                          }
	float3 forward()                                                     const { return view_inverse_[2].xyz();                          }
	float3 position()                                                    const { return view_inverse_[3].xyz();                          }
	float3 euler_rotation() /* aka pitch,yaw,roll */                     const { return rotation2(view_inverse_);                        }

	// moves the pov to focus on the specified sphere such that it fits the view frustum
	void focus(const float4& sphere);


	// ___________________________________________________________________________________________________________________________________
	// projection elements. Modification of projection is done through fov, near, far settings all other values are derived from those

	const float4x4& projection()                                         const { return projection_;                                     }
	const float4x4& projection_inverse()                                 const { return projection_inverse_;                             }
	const float4x4& view_projection()                                    const { return view_projection_;                                }
	const float4x4& view_projection_inverse()                            const { return view_projection_inverse_;                        }
	const float2& fov()                                                  const { return fov_;                                            }
	const float2& ratio() /* tan(fov() * 0.5f) */                        const { return ratio_;                                          }
	float near_dist()                                                    const { return near_;                                           }
	float far_dist()                                                     const { return far_;                                            }

	void projection(float fovx, float near, float far);

	// ___________________________________________________________________________________________________________________________________
	// viewport elements (call viewport(new_dimensions) on PC in a window resize handler)

	void viewport(const viewport_t& viewport);
	void viewport(const uint2& new_dimensions)                                 { viewport(viewport_t(new_dimensions));                   }
	const viewport_t& viewport()                                         const { return viewport_;                                       }
	const float4& viewport_to_clip_scale_bias()                          const { return viewport_to_clip_scale_bias_;                    }
	float aspect_ratio()                                                 const { return viewport_.aspect_ratio();                        }


	// ___________________________________________________________________________________________________________________________________
	// geometry derived from the view & projection matrices

	const float3* corners()                                              const { return corners_.data();                                 }
	const float3& corner(int c)                                          const { return corners_[c];                                     }
	const float4* planes()                                               const { return planes_.data();                                  }
	const float4& plane(int p)                                           const { return planes_[p];                                      }
	const float3* view_far_corner_vectors()     /* unnormalized */       const { return vs_corner_vectors_.data();                       }
	const float3& view_far_corner_vector(int c) /* unnormalized */       const { return vs_corner_vectors_[c];                           }


	// ___________________________________________________________________________________________________________________________________
	// space conversions | clip space: xy on [-w,w], z on [0,w] | ndc space: xy on [-1,1], z on [0,1], w same as clip space

	float3 world_to_view    (const float3& ws_pos)     const { return mul(view_, float4(ws_pos, 1.0f)).xyz();                            }
	float4 world_to_clip    (const float3& ws_pos)     const { return mul(view_projection_, float4(ws_pos, 1.0f));                       }
	float4 world_to_ndc     (const float3& ws_pos)     const { return clip_to_ndc(world_to_clip(ws_pos));                                }
	float4 world_to_viewport(const float3& ws_pos)     const { return ndc_to_viewport(world_to_ndc(ws_pos));                             }
	float3 view_to_world    (const float3& vs_pos)     const { return mul(view_inverse_, float4(vs_pos, 1.0f)).xyz();                    }
	float4 view_to_clip     (const float3& vs_pos)     const { return mul(projection_, float4(vs_pos, 1.0f));                            }
	float4 view_to_ndc      (const float3& vs_pos)     const { return clip_to_ndc(view_to_clip(vs_pos));                                 }
 	float4 view_to_viewport (const float3& vs_pos)     const { return clip_to_ndc(view_to_ndc(vs_pos));                                  }
	float3 clip_to_world    (const float4& clip_pos)   const { float4 p = mul(view_projection_inverse_, clip_pos); return p.xyz() / p.w; }
	float3 clip_to_view     (const float4& clip_pos)   const { float4 p = mul(projection_inverse_,      clip_pos); return p.xyz() / p.w; }
	float4 clip_to_ndc      (const float4& clip_pos)   const { return float4(clip_pos.xyz() / clip_pos.w, clip_pos.w);                   }
	float4 clip_to_viewport (const float4& clip_pos)   const { return ndc_to_viewport(clip_to_ndc(clip_pos));                            }
	float3 ndc_to_world     (const float4& ndc_pos)    const { return clip_to_world(ndc_to_clip(ndc_pos));                               }
	float3 ndc_to_view      (const float4& ndc_pos)    const { return clip_to_view(ndc_to_clip(ndc_pos));                                }
	float4 ndc_to_clip      (const float4& ndc_pos)    const { return float4(ndc_pos.xyz() * ndc_pos.w, ndc_pos.w);                      }
	float4 ndc_to_viewport  (const float4& ndc_pos)    const { return ndc_pos * viewport_scale_ + viewport_bias_;                        }
	float3 viewport_to_world(const float4& vp_pos)     const { return ndc_to_world(viewport_to_ndc(vp_pos));                             }
	float3 viewport_to_view (const float4& vp_pos)     const { return ndc_to_view(viewport_to_ndc(vp_pos));                              }
	float4 viewport_to_clip (const float4& vp_pos)     const { return ndc_to_clip(viewport_to_ndc(vp_pos));                              }
	float4 viewport_to_ndc  (const float4& vp_pos)     const { return vp_pos * viewport_scale_inverse_ + viewport_bias_inverse_;         }


	// ___________________________________________________________________________________________________________________________________
	// unprojection: screen-space point with view-space depth to world-space coordinates. Optionally VSdepth 
	// can be cached using pointer_depth() since that is often obtained very differently from how the screen 
	// XY are obtained. To flag this as not a useful hint, assign and test for a negative value.

	void  pointer_depth(float VSdepth) /* view-space depth */            { pointer_depth_ = VSdepth;                                     }
	float pointer_depth()              /* view-space depth */      const { return pointer_depth_;                                        }

	float3 unproject(const float2& screen_position, float VSdepth) const;
	float3 unproject        (int x, int y, float VSdepth)          const { return unproject(float2((float)x, (float)y), VSdepth);        }
	float3 unproject_near   (int x, int y)                         const { return unproject(x, y, near_dist());                          }
	float3 unproject_far    (int x, int y)                         const { return unproject(x, y, far_dist());                           }
	float3 unproject_pointer(int x, int y)                         const { return unproject(x, y, pointer_depth());                      }
	float3 unproject_near   (const float2& screen_position)        const { return unproject(screen_position, near_dist());               }
	float3 unproject_far    (const float2& screen_position)        const { return unproject(screen_position, far_dist());                }
	float3 unproject_pointer(const float2& screen_position)        const { return unproject(screen_position, pointer_depth());           }

	// returns the delta of the specified screen positions adjusted by pointer_depth() so the delta rate 
	// remains constant for any object depth. Use this when panning.
	float2 unprojected_delta(const float2& old_screen_position, const float2& new_screen_position) const;
	float2 unprojected_delta(int old_x, int old_y, int new_x, int new_y) const { return unprojected_delta(float2((float)old_x, (float)old_y), float2((float)new_x, (float)new_y)); }

	// ___________________________________________________________________________________________________________________________________
private:
	// Note: The following are user-defined/set directly:
	// projection_
	// view_
	// viewport_
	// pointer_depth_
	// All other fields are derived and calculated and cached using update_derived().

	void update();

	float4x4 view_;
	float4x4 view_inverse_;
	float4x4 projection_;
	float4x4 projection_inverse_;
	float4x4 view_projection_;
	float4x4 view_projection_inverse_;
	
	viewport_t viewport_;
	float4 viewport_scale_;
	float4 viewport_bias_;
	float4 viewport_scale_inverse_;
	float4 viewport_bias_inverse_;

	float4 viewport_to_clip_scale_bias_;
	float4 screen_to_view_scale_bias_;

	float2 fov_;
	float2 ratio_;
	float near_;
	float far_;
	float pointer_depth_; // system pointer (i.e. mouse)
	
	std::array<float4, 6> planes_;
	std::array<float3, 8> corners_;
	std::array<float3, 4> vs_corner_vectors_;
};

}
