/*
   Copyright (C) 2006, 2007 Sony Computer Entertainment Inc.
   All rights reserved.

   Redistribution and use in source and binary forms,
   with or without modification, are permitted provided that the
   following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Sony Computer Entertainment Inc nor the names
      of its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

// @tony: Modified to use doubles

#ifndef _VECTORMATH_QUAT_AOS_D_CPP_H
#define _VECTORMATH_QUAT_AOS_D_CPP_H
//-----------------------------------------------------------------------------
// Definitions

#ifndef _VECTORMATH_INTERNAL_FUNCTIONS_D
#define _VECTORMATH_INTERNAL_FUNCTIONS_D

#endif

namespace Vectormath {
namespace Aos {

inline Quatd::Quatd( const Quatd & quat )
{
    mX = quat.mX;
    mY = quat.mY;
    mZ = quat.mZ;
    mW = quat.mW;
}

inline Quatd::Quatd( double _x, double _y, double _z, double _w )
{
    mX = _x;
    mY = _y;
    mZ = _z;
    mW = _w;
}

inline Quatd::Quatd( const Vector3d & xyz, double _w )
{
    this->setXYZ( xyz );
    this->setW( _w );
}

inline Quatd::Quatd( const Vector4d & vec )
{
    mX = vec.getX();
    mY = vec.getY();
    mZ = vec.getZ();
    mW = vec.getW();
}

inline Quatd::Quatd( double scalar )
{
    mX = scalar;
    mY = scalar;
    mZ = scalar;
    mW = scalar;
}

inline const Quatd Quatd::identity( )
{
    return Quatd( 0.0, 0.0, 0.0, 1.0 );
}

inline const Quatd lerp( double t, const Quatd & quat0, const Quatd & quat1 )
{
    return ( quat0 + ( ( quat1 - quat0 ) * t ) );
}

inline const Quatd slerp( double t, const Quatd & unitQuat0, const Quatd & unitQuat1 )
{
    Quatd start;
    double recipSinAngle, scale0, scale1, cosAngle, angle;
    cosAngle = dot( unitQuat0, unitQuat1 );
    if ( cosAngle < 0.0 ) {
        cosAngle = -cosAngle;
        start = ( -unitQuat0 );
    } else {
        start = unitQuat0;
    }
    if ( cosAngle < _VECTORMATH_SLERP_TOL ) {
        angle = acos( cosAngle );
        recipSinAngle = ( 1.0 / sin( angle ) );
        scale0 = ( sin( ( ( 1.0 - t ) * angle ) ) * recipSinAngle );
        scale1 = ( sin( ( t * angle ) ) * recipSinAngle );
    } else {
        scale0 = ( 1.0 - t );
        scale1 = t;
    }
    return ( ( start * scale0 ) + ( unitQuat1 * scale1 ) );
}

inline const Quatd squad( double t, const Quatd & unitQuat0, const Quatd & unitQuat1, const Quatd & unitQuat2, const Quatd & unitQuat3 )
{
    Quatd tmp0, tmp1;
    tmp0 = slerp( t, unitQuat0, unitQuat3 );
    tmp1 = slerp( t, unitQuat1, unitQuat2 );
    return slerp( ( ( 2.0 * t ) * ( 1.0 - t ) ), tmp0, tmp1 );
}

inline Quatd & Quatd::operator =( const Quatd & quat )
{
    mX = quat.mX;
    mY = quat.mY;
    mZ = quat.mZ;
    mW = quat.mW;
    return *this;
}

inline Quatd & Quatd::setXYZ( const Vector3d & vec )
{
    mX = vec.getX();
    mY = vec.getY();
    mZ = vec.getZ();
    return *this;
}

inline const Vector3d Quatd::getXYZ( ) const
{
    return Vector3d( mX, mY, mZ );
}

inline Quatd & Quatd::setX( double _x )
{
    mX = _x;
    return *this;
}

inline double Quatd::getX( ) const
{
    return mX;
}

inline Quatd & Quatd::setY( double _y )
{
    mY = _y;
    return *this;
}

inline double Quatd::getY( ) const
{
    return mY;
}

inline Quatd & Quatd::setZ( double _z )
{
    mZ = _z;
    return *this;
}

inline double Quatd::getZ( ) const
{
    return mZ;
}

inline Quatd & Quatd::setW( double _w )
{
    mW = _w;
    return *this;
}

inline double Quatd::getW( ) const
{
    return mW;
}

inline Quatd & Quatd::setElem( int idx, double value )
{
    *(&mX + idx) = value;
    return *this;
}

inline double Quatd::getElem( int idx ) const
{
    return *(&mX + idx);
}

inline double & Quatd::operator []( int idx )
{
    return *(&mX + idx);
}

inline double Quatd::operator []( int idx ) const
{
    return *(&mX + idx);
}

inline const Quatd Quatd::operator +( const Quatd & quat ) const
{
    return Quatd(
        ( mX + quat.mX ),
        ( mY + quat.mY ),
        ( mZ + quat.mZ ),
        ( mW + quat.mW )
    );
}

inline const Quatd Quatd::operator -( const Quatd & quat ) const
{
    return Quatd(
        ( mX - quat.mX ),
        ( mY - quat.mY ),
        ( mZ - quat.mZ ),
        ( mW - quat.mW )
    );
}

inline const Quatd Quatd::operator *( double scalar ) const
{
    return Quatd(
        ( mX * scalar ),
        ( mY * scalar ),
        ( mZ * scalar ),
        ( mW * scalar )
    );
}

inline Quatd & Quatd::operator +=( const Quatd & quat )
{
    *this = *this + quat;
    return *this;
}

inline Quatd & Quatd::operator -=( const Quatd & quat )
{
    *this = *this - quat;
    return *this;
}

inline Quatd & Quatd::operator *=( double scalar )
{
    *this = *this * scalar;
    return *this;
}

inline const Quatd Quatd::operator /( double scalar ) const
{
    return Quatd(
        ( mX / scalar ),
        ( mY / scalar ),
        ( mZ / scalar ),
        ( mW / scalar )
    );
}

inline Quatd & Quatd::operator /=( double scalar )
{
    *this = *this / scalar;
    return *this;
}

inline const Quatd Quatd::operator -( ) const
{
    return Quatd(
        -mX,
        -mY,
        -mZ,
        -mW
    );
}

inline const Quatd operator *( double scalar, const Quatd & quat )
{
    return quat * scalar;
}

inline double dot( const Quatd & quat0, const Quatd & quat1 )
{
    double result;
    result = ( quat0.getX() * quat1.getX() );
    result = ( result + ( quat0.getY() * quat1.getY() ) );
    result = ( result + ( quat0.getZ() * quat1.getZ() ) );
    result = ( result + ( quat0.getW() * quat1.getW() ) );
    return result;
}

inline double norm( const Quatd & quat )
{
    double result;
    result = ( quat.getX() * quat.getX() );
    result = ( result + ( quat.getY() * quat.getY() ) );
    result = ( result + ( quat.getZ() * quat.getZ() ) );
    result = ( result + ( quat.getW() * quat.getW() ) );
    return result;
}

inline double length( const Quatd & quat )
{
    return sqrt( norm( quat ) );
}

inline const Quatd normalize( const Quatd & quat )
{
    double lenSqr, lenInv;
    lenSqr = norm( quat );
    lenInv = ( 1.0 / sqrt( lenSqr ) );
    return Quatd(
        ( quat.getX() * lenInv ),
        ( quat.getY() * lenInv ),
        ( quat.getZ() * lenInv ),
        ( quat.getW() * lenInv )
    );
}

inline const Quatd Quatd::rotation( const Vector3d & unitVec0, const Vector3d & unitVec1 )
{
    double cosHalfAngleX2, recipCosHalfAngleX2;
    cosHalfAngleX2 = sqrt( ( 2.0 * ( 1.0 + dot( unitVec0, unitVec1 ) ) ) );
    recipCosHalfAngleX2 = ( 1.0 / cosHalfAngleX2 );
    return Quatd( ( cross( unitVec0, unitVec1 ) * recipCosHalfAngleX2 ), ( cosHalfAngleX2 * 0.5 ) );
}

inline const Quatd Quatd::rotation( double radians, const Vector3d & unitVec )
{
    double s, c, angle;
    angle = ( radians * 0.5 );
    s = sin( angle );
    c = cos( angle );
    return Quatd( ( unitVec * s ), c );
}

inline const Quatd Quatd::rotationX( double radians )
{
    double s, c, angle;
    angle = ( radians * 0.5 );
    s = sin( angle );
    c = cos( angle );
    return Quatd( s, 0.0, 0.0, c );
}

inline const Quatd Quatd::rotationY( double radians )
{
    double s, c, angle;
    angle = ( radians * 0.5 );
    s = sin( angle );
    c = cos( angle );
    return Quatd( 0.0, s, 0.0, c );
}

inline const Quatd Quatd::rotationZ( double radians )
{
    double s, c, angle;
    angle = ( radians * 0.5 );
    s = sin( angle );
    c = cos( angle );
    return Quatd( 0.0, 0.0, s, c );
}

inline const Quatd Quatd::operator *( const Quatd & quat ) const
{
    return Quatd(
        ( ( ( ( mW * quat.mX ) + ( mX * quat.mW ) ) + ( mY * quat.mZ ) ) - ( mZ * quat.mY ) ),
        ( ( ( ( mW * quat.mY ) + ( mY * quat.mW ) ) + ( mZ * quat.mX ) ) - ( mX * quat.mZ ) ),
        ( ( ( ( mW * quat.mZ ) + ( mZ * quat.mW ) ) + ( mX * quat.mY ) ) - ( mY * quat.mX ) ),
        ( ( ( ( mW * quat.mW ) - ( mX * quat.mX ) ) - ( mY * quat.mY ) ) - ( mZ * quat.mZ ) )
    );
}

inline Quatd & Quatd::operator *=( const Quatd & quat )
{
    *this = *this * quat;
    return *this;
}

inline const Vector3d rotate( const Quatd & quat, const Vector3d & vec )
{
    double tmpX, tmpY, tmpZ, tmpW;
    tmpX = ( ( ( quat.getW() * vec.getX() ) + ( quat.getY() * vec.getZ() ) ) - ( quat.getZ() * vec.getY() ) );
    tmpY = ( ( ( quat.getW() * vec.getY() ) + ( quat.getZ() * vec.getX() ) ) - ( quat.getX() * vec.getZ() ) );
    tmpZ = ( ( ( quat.getW() * vec.getZ() ) + ( quat.getX() * vec.getY() ) ) - ( quat.getY() * vec.getX() ) );
    tmpW = ( ( ( quat.getX() * vec.getX() ) + ( quat.getY() * vec.getY() ) ) + ( quat.getZ() * vec.getZ() ) );
    return Vector3d(
        ( ( ( ( tmpW * quat.getX() ) + ( tmpX * quat.getW() ) ) - ( tmpY * quat.getZ() ) ) + ( tmpZ * quat.getY() ) ),
        ( ( ( ( tmpW * quat.getY() ) + ( tmpY * quat.getW() ) ) - ( tmpZ * quat.getX() ) ) + ( tmpX * quat.getZ() ) ),
        ( ( ( ( tmpW * quat.getZ() ) + ( tmpZ * quat.getW() ) ) - ( tmpX * quat.getY() ) ) + ( tmpY * quat.getX() ) )
    );
}

inline const Quatd conj( const Quatd & quat )
{
    return Quatd( -quat.getX(), -quat.getY(), -quat.getZ(), quat.getW() );
}

inline const Quatd select( const Quatd & quat0, const Quatd & quat1, bool select1 )
{
    return Quatd(
        ( select1 )? quat1.getX() : quat0.getX(),
        ( select1 )? quat1.getY() : quat0.getY(),
        ( select1 )? quat1.getZ() : quat0.getZ(),
        ( select1 )? quat1.getW() : quat0.getW()
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Quatd & quat )
{
    printf( "( %f %f %f %f )\n", quat.getX(), quat.getY(), quat.getZ(), quat.getW() );
}

inline void print( const Quatd & quat, const char * name )
{
    printf( "%s: ( %f %f %f %f )\n", name, quat.getX(), quat.getY(), quat.getZ(), quat.getW() );
}

#endif

} // namespace Aos
} // namespace Vectormath

#endif
