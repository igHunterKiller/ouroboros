// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/arcball.h>
#include <oMath/hlslx.h>
#include <oMath/matrix.h>
#include <oMath/quat.h>

namespace ouro {

arcball::arcball(constraint_t c)
	: quat_(kIdentityQuat)
	, euler_(kZero3)
	, tx_(kZero3)
	, lookat_(kZero3)
	, polarity_(1.0f)
	, constraint_(c)
	, type_(type::none)
{}

void arcball::view(const float4x4& v)
{
	float4x4 view_inv = invert(v);
	euler_ = ouro::rotation(view_inv);
	quat_  = qrotate(euler_);
	tx_    = view_inv[3].xyz();
}

float4x4 arcball::view() const
{
	return invert(qrotatetranslate(quat_, tx_));
}

void arcball::focus(float fovy_radians, const float4& sphere)
{
	float4x4 v = translate_view_to_fit(view(), fovy_radians, sphere);
	lookat(sphere.xyz());
	view(v);
}

void arcball::focus(const float3& eye, const float3& at)
{
	float3 up = kZero3;
	up[constraint_ == constraint::z_up ? 2 : 1] = 1.0f;
	view(lookat_lh(eye, at, up));
}

arcball::type_t arcball::update(const type_t& type, const float3& screen_space_delta)
{
	switch (type)
	{
		case type::pan:
		{
			float3 delta = qmul(quat_, float3(-screen_space_delta.x, screen_space_delta.y, 0.0f));
			tx_         += delta;
			lookat_     += delta;
			break;
		}

		case type::dolly:
		{
			tx_ += qmul(quat_, float3(0.0f, 0.0f, screen_space_delta.x + screen_space_delta.y));
			break;
		}

		case type::orbit:
		{
			if (type_ != type::orbit) // if first orbit update
			{
				// @tony: note: there is a bug for z_up, not sure where.
				// the dot-product seems rotated 90 degrees.

				float4x4 rotation = ouro::rotate(quat_);
				
				// define the relevant axis from an identity matrix
				float3 identity_axis(kZero3);
				identity_axis[(int)constraint_] = 1.0f;
				
				// if the current basis is aligned away from default up, it means that left & right
				// will be reversed, so include a multiplier to fix later calcuations
				bool upside_down = dot(rotation[(int)constraint_].xyz(), identity_axis) < 0.0f;

				// if rotation is upside down, reverse the ideas of left & right
				polarity_ = upside_down ? -1.0f : 1.0f;
			}

			else if (!equal(screen_space_delta, float3(0.0f, 0.0f, 0.0f)))
			{
				// define the vector going from the origin of the arcball to the eye
				float3 orig_vec = qmul(invert(quat_), tx_ - lookat_);

				// apply rotations considering the reversal of left & right when the
				// view is upside down
				switch (constraint_)
				{
					case constraint::y_up:
						quat_ = qmul(quat_, qrotate(screen_space_delta.y, kXAxis));
						quat_ = qmul(qrotate(polarity_ * screen_space_delta.x, kYAxis), quat_);
						break;
					case constraint::z_up:
						quat_ = qmul(quat_, qrotate(screen_space_delta.y, kXAxis));
						quat_ = qmul(qrotate(polarity_ * screen_space_delta.x, kZAxis), quat_);
						break;
					case constraint::none:
						quat_ = qmul(quat_, qrotate(float3(screen_space_delta.yx(), 0.0f)));
						break;
				}

				// rotate the original vector and offset it from the center of the arcball
				tx_    = qmul(normalize(quat_), orig_vec) + lookat_;
				euler_ = quattoeuler(quat_);
			}

			break;
		}

		case type::look:
		{
			// using the orbit logic here without altering tx_ causes the 'up' of the
			// system to deteriorate. This is semi-happening in orbit, so perhaps 
			// try this method with orbit instead to preserve upness more.
			float3 LSeuler = float3(screen_space_delta.yx(), 0.0f);
			euler_ += LSeuler;
			quat_   = qrotate(euler_);
			lookat_ = qmul(qrotate(LSeuler), lookat_);
			break;
		}
	}

	auto prior = type_;
	type_      = type;
	return prior;
}

void arcball::walk(const float3& delta)
{
	float3 tx = qmul(quat_, delta);
	tx_      += tx;
	lookat_  += tx;
}

}