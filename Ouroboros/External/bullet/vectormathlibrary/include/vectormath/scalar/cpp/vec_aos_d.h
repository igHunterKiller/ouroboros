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

#ifndef _VECTORMATH_VEC_AOS_D_CPP_H
#define _VECTORMATH_VEC_AOS_D_CPP_H
//-----------------------------------------------------------------------------
// Constants

#define _VECTORMATH_SLERP_TOL_DOUBLE 0.999

//-----------------------------------------------------------------------------
// Definitions

#ifndef _VECTORMATH_INTERNAL_FUNCTIONS
#define _VECTORMATH_INTERNAL_FUNCTIONS

#endif

namespace Vectormath {
namespace Aos {

inline Vector3d::Vector3d( const Vector3d & vec )
{
    mX = vec.mX;
    mY = vec.mY;
    mZ = vec.mZ;
}

inline Vector3d::Vector3d( double _x, double _y, double _z )
{
    mX = _x;
    mY = _y;
    mZ = _z;
}

inline Vector3d::Vector3d( const Point3d & pnt )
{
    mX = pnt.getX();
    mY = pnt.getY();
    mZ = pnt.getZ();
}

inline Vector3d::Vector3d( double scalar )
{
    mX = scalar;
    mY = scalar;
    mZ = scalar;
}

inline const Vector3d Vector3d::xAxis( )
{
    return Vector3d( 1.0, 0.0, 0.0 );
}

inline const Vector3d Vector3d::yAxis( )
{
    return Vector3d( 0.0, 1.0, 0.0 );
}

inline const Vector3d Vector3d::zAxis( )
{
    return Vector3d( 0.0, 0.0, 1.0 );
}

inline const Vector3d lerp( double t, const Vector3d & vec0, const Vector3d & vec1 )
{
    return ( vec0 + ( ( vec1 - vec0 ) * t ) );
}

inline const Vector3d slerp( double t, const Vector3d & unitVec0, const Vector3d & unitVec1 )
{
    double recipSinAngle, scale0, scale1, cosAngle, angle;
    cosAngle = dot( unitVec0, unitVec1 );
    if ( cosAngle < _VECTORMATH_SLERP_TOL_DOUBLE ) {
        angle = acos( cosAngle );
        recipSinAngle = ( 1.0 / sin( angle ) );
        scale0 = ( sin( ( ( 1.0 - t ) * angle ) ) * recipSinAngle );
        scale1 = ( sin( ( t * angle ) ) * recipSinAngle );
    } else {
        scale0 = ( 1.0 - t );
        scale1 = t;
    }
    return ( ( unitVec0 * scale0 ) + ( unitVec1 * scale1 ) );
}

inline Vector3d & Vector3d::operator =( const Vector3d & vec )
{
    mX = vec.mX;
    mY = vec.mY;
    mZ = vec.mZ;
    return *this;
}

inline Vector3d & Vector3d::setX( double _x )
{
    mX = _x;
    return *this;
}

inline double Vector3d::getX( ) const
{
    return mX;
}

inline Vector3d & Vector3d::setY( double _y )
{
    mY = _y;
    return *this;
}

inline double Vector3d::getY( ) const
{
    return mY;
}

inline Vector3d & Vector3d::setZ( double _z )
{
    mZ = _z;
    return *this;
}

inline double Vector3d::getZ( ) const
{
    return mZ;
}

inline Vector3d & Vector3d::setElem( int idx, double value )
{
    *(&mX + idx) = value;
    return *this;
}

inline double Vector3d::getElem( int idx ) const
{
    return *(&mX + idx);
}

inline double & Vector3d::operator []( int idx )
{
    return *(&mX + idx);
}

inline double Vector3d::operator []( int idx ) const
{
    return *(&mX + idx);
}

inline const Vector3d Vector3d::operator +( const Vector3d & vec ) const
{
    return Vector3d(
        ( mX + vec.mX ),
        ( mY + vec.mY ),
        ( mZ + vec.mZ )
    );
}

inline const Vector3d Vector3d::operator -( const Vector3d & vec ) const
{
    return Vector3d(
        ( mX - vec.mX ),
        ( mY - vec.mY ),
        ( mZ - vec.mZ )
    );
}

inline const Point3d Vector3d::operator +( const Point3d & pnt ) const
{
    return Point3d(
        ( mX + pnt.getX() ),
        ( mY + pnt.getY() ),
        ( mZ + pnt.getZ() )
    );
}

inline const Vector3d Vector3d::operator *( double scalar ) const
{
    return Vector3d(
        ( mX * scalar ),
        ( mY * scalar ),
        ( mZ * scalar )
    );
}

inline Vector3d & Vector3d::operator +=( const Vector3d & vec )
{
    *this = *this + vec;
    return *this;
}

inline Vector3d & Vector3d::operator -=( const Vector3d & vec )
{
    *this = *this - vec;
    return *this;
}

inline Vector3d & Vector3d::operator *=( double scalar )
{
    *this = *this * scalar;
    return *this;
}

inline const Vector3d Vector3d::operator /( double scalar ) const
{
    return Vector3d(
        ( mX / scalar ),
        ( mY / scalar ),
        ( mZ / scalar )
    );
}

inline Vector3d & Vector3d::operator /=( double scalar )
{
    *this = *this / scalar;
    return *this;
}

inline const Vector3d Vector3d::operator -( ) const
{
    return Vector3d(
        -mX,
        -mY,
        -mZ
    );
}

inline const Vector3d operator *( double scalar, const Vector3d & vec )
{
    return vec * scalar;
}

inline const Vector3d mulPerElem( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        ( vec0.getX() * vec1.getX() ),
        ( vec0.getY() * vec1.getY() ),
        ( vec0.getZ() * vec1.getZ() )
    );
}

inline const Vector3d divPerElem( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        ( vec0.getX() / vec1.getX() ),
        ( vec0.getY() / vec1.getY() ),
        ( vec0.getZ() / vec1.getZ() )
    );
}

inline const Vector3d recipPerElem( const Vector3d & vec )
{
    return Vector3d(
        ( 1.0 / vec.getX() ),
        ( 1.0 / vec.getY() ),
        ( 1.0 / vec.getZ() )
    );
}

inline const Vector3d sqrtPerElem( const Vector3d & vec )
{
    return Vector3d(
        sqrt( vec.getX() ),
        sqrt( vec.getY() ),
        sqrt( vec.getZ() )
    );
}

inline const Vector3d rsqrtPerElem( const Vector3d & vec )
{
    return Vector3d(
        ( 1.0 / sqrt( vec.getX() ) ),
        ( 1.0 / sqrt( vec.getY() ) ),
        ( 1.0 / sqrt( vec.getZ() ) )
    );
}

inline const Vector3d absPerElem( const Vector3d & vec )
{
    return Vector3d(
        fabs( vec.getX() ),
        fabs( vec.getY() ),
        fabs( vec.getZ() )
    );
}

inline const Vector3d copySignPerElem( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        ( vec1.getX() < 0.0 )? -fabs( vec0.getX() ) : fabs( vec0.getX() ),
        ( vec1.getY() < 0.0 )? -fabs( vec0.getY() ) : fabs( vec0.getY() ),
        ( vec1.getZ() < 0.0 )? -fabs( vec0.getZ() ) : fabs( vec0.getZ() )
    );
}

inline const Vector3d maxPerElem( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        (vec0.getX() > vec1.getX())? vec0.getX() : vec1.getX(),
        (vec0.getY() > vec1.getY())? vec0.getY() : vec1.getY(),
        (vec0.getZ() > vec1.getZ())? vec0.getZ() : vec1.getZ()
    );
}

inline double maxElem( const Vector3d & vec )
{
    double result;
    result = (vec.getX() > vec.getY())? vec.getX() : vec.getY();
    result = (vec.getZ() > result)? vec.getZ() : result;
    return result;
}

inline const Vector3d minPerElem( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        (vec0.getX() < vec1.getX())? vec0.getX() : vec1.getX(),
        (vec0.getY() < vec1.getY())? vec0.getY() : vec1.getY(),
        (vec0.getZ() < vec1.getZ())? vec0.getZ() : vec1.getZ()
    );
}

inline double minElem( const Vector3d & vec )
{
    double result;
    result = (vec.getX() < vec.getY())? vec.getX() : vec.getY();
    result = (vec.getZ() < result)? vec.getZ() : result;
    return result;
}

inline double sum( const Vector3d & vec )
{
    double result;
    result = ( vec.getX() + vec.getY() );
    result = ( result + vec.getZ() );
    return result;
}

inline double dot( const Vector3d & vec0, const Vector3d & vec1 )
{
    double result;
    result = ( vec0.getX() * vec1.getX() );
    result = ( result + ( vec0.getY() * vec1.getY() ) );
    result = ( result + ( vec0.getZ() * vec1.getZ() ) );
    return result;
}

inline double lengthSqr( const Vector3d & vec )
{
    double result;
    result = ( vec.getX() * vec.getX() );
    result = ( result + ( vec.getY() * vec.getY() ) );
    result = ( result + ( vec.getZ() * vec.getZ() ) );
    return result;
}

inline double length( const Vector3d & vec )
{
    return sqrt( lengthSqr( vec ) );
}

inline const Vector3d normalize( const Vector3d & vec )
{
    double lenSqr, lenInv;
    lenSqr = lengthSqr( vec );
    lenInv = ( 1.0 / sqrt( lenSqr ) );
    return Vector3d(
        ( vec.getX() * lenInv ),
        ( vec.getY() * lenInv ),
        ( vec.getZ() * lenInv )
    );
}

inline const Vector3d cross( const Vector3d & vec0, const Vector3d & vec1 )
{
    return Vector3d(
        ( ( vec0.getY() * vec1.getZ() ) - ( vec0.getZ() * vec1.getY() ) ),
        ( ( vec0.getZ() * vec1.getX() ) - ( vec0.getX() * vec1.getZ() ) ),
        ( ( vec0.getX() * vec1.getY() ) - ( vec0.getY() * vec1.getX() ) )
    );
}

inline const Vector3d select( const Vector3d & vec0, const Vector3d & vec1, bool select1 )
{
    return Vector3d(
        ( select1 )? vec1.getX() : vec0.getX(),
        ( select1 )? vec1.getY() : vec0.getY(),
        ( select1 )? vec1.getZ() : vec0.getZ()
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Vector3d & vec )
{
    printf( "( %f %f %f )\n", vec.getX(), vec.getY(), vec.getZ() );
}

inline void print( const Vector3d & vec, const char * name )
{
    printf( "%s: ( %f %f %f )\n", name, vec.getX(), vec.getY(), vec.getZ() );
}

#endif

inline Vector4d::Vector4d( const Vector4d & vec )
{
    mX = vec.mX;
    mY = vec.mY;
    mZ = vec.mZ;
    mW = vec.mW;
}

inline Vector4d::Vector4d( double _x, double _y, double _z, double _w )
{
    mX = _x;
    mY = _y;
    mZ = _z;
    mW = _w;
}

inline Vector4d::Vector4d( const Vector3d & xyz, double _w )
{
    this->setXYZ( xyz );
    this->setW( _w );
}

inline Vector4d::Vector4d( const Vector3d & vec )
{
    mX = vec.getX();
    mY = vec.getY();
    mZ = vec.getZ();
    mW = 0.0;
}

inline Vector4d::Vector4d( const Point3d & pnt )
{
    mX = pnt.getX();
    mY = pnt.getY();
    mZ = pnt.getZ();
    mW = 1.0;
}

inline Vector4d::Vector4d( const Quatd & quat )
{
    mX = quat.getX();
    mY = quat.getY();
    mZ = quat.getZ();
    mW = quat.getW();
}

inline Vector4d::Vector4d( double scalar )
{
    mX = scalar;
    mY = scalar;
    mZ = scalar;
    mW = scalar;
}

inline const Vector4d Vector4d::xAxis( )
{
    return Vector4d( 1.0, 0.0, 0.0, 0.0 );
}

inline const Vector4d Vector4d::yAxis( )
{
    return Vector4d( 0.0, 1.0, 0.0, 0.0 );
}

inline const Vector4d Vector4d::zAxis( )
{
    return Vector4d( 0.0, 0.0, 1.0, 0.0 );
}

inline const Vector4d Vector4d::wAxis( )
{
    return Vector4d( 0.0, 0.0, 0.0, 1.0 );
}

inline const Vector4d lerp( double t, const Vector4d & vec0, const Vector4d & vec1 )
{
    return ( vec0 + ( ( vec1 - vec0 ) * t ) );
}

inline const Vector4d slerp( double t, const Vector4d & unitVec0, const Vector4d & unitVec1 )
{
    double recipSinAngle, scale0, scale1, cosAngle, angle;
    cosAngle = dot( unitVec0, unitVec1 );
    if ( cosAngle < _VECTORMATH_SLERP_TOL_DOUBLE ) {
        angle = acos( cosAngle );
        recipSinAngle = ( 1.0 / sin( angle ) );
        scale0 = ( sin( ( ( 1.0 - t ) * angle ) ) * recipSinAngle );
        scale1 = ( sin( ( t * angle ) ) * recipSinAngle );
    } else {
        scale0 = ( 1.0 - t );
        scale1 = t;
    }
    return ( ( unitVec0 * scale0 ) + ( unitVec1 * scale1 ) );
}

inline Vector4d & Vector4d::operator =( const Vector4d & vec )
{
    mX = vec.mX;
    mY = vec.mY;
    mZ = vec.mZ;
    mW = vec.mW;
    return *this;
}

inline Vector4d & Vector4d::setXYZ( const Vector3d & vec )
{
    mX = vec.getX();
    mY = vec.getY();
    mZ = vec.getZ();
    return *this;
}

inline const Vector3d Vector4d::getXYZ( ) const
{
    return Vector3d( mX, mY, mZ );
}

inline Vector4d & Vector4d::setX( double _x )
{
    mX = _x;
    return *this;
}

inline double Vector4d::getX( ) const
{
    return mX;
}

inline Vector4d & Vector4d::setY( double _y )
{
    mY = _y;
    return *this;
}

inline double Vector4d::getY( ) const
{
    return mY;
}

inline Vector4d & Vector4d::setZ( double _z )
{
    mZ = _z;
    return *this;
}

inline double Vector4d::getZ( ) const
{
    return mZ;
}

inline Vector4d & Vector4d::setW( double _w )
{
    mW = _w;
    return *this;
}

inline double Vector4d::getW( ) const
{
    return mW;
}

inline Vector4d & Vector4d::setElem( int idx, double value )
{
    *(&mX + idx) = value;
    return *this;
}

inline double Vector4d::getElem( int idx ) const
{
    return *(&mX + idx);
}

inline double & Vector4d::operator []( int idx )
{
    return *(&mX + idx);
}

inline double Vector4d::operator []( int idx ) const
{
    return *(&mX + idx);
}

inline const Vector4d Vector4d::operator +( const Vector4d & vec ) const
{
    return Vector4d(
        ( mX + vec.mX ),
        ( mY + vec.mY ),
        ( mZ + vec.mZ ),
        ( mW + vec.mW )
    );
}

inline const Vector4d Vector4d::operator -( const Vector4d & vec ) const
{
    return Vector4d(
        ( mX - vec.mX ),
        ( mY - vec.mY ),
        ( mZ - vec.mZ ),
        ( mW - vec.mW )
    );
}

inline const Vector4d Vector4d::operator *( double scalar ) const
{
    return Vector4d(
        ( mX * scalar ),
        ( mY * scalar ),
        ( mZ * scalar ),
        ( mW * scalar )
    );
}

inline Vector4d & Vector4d::operator +=( const Vector4d & vec )
{
    *this = *this + vec;
    return *this;
}

inline Vector4d & Vector4d::operator -=( const Vector4d & vec )
{
    *this = *this - vec;
    return *this;
}

inline Vector4d & Vector4d::operator *=( double scalar )
{
    *this = *this * scalar;
    return *this;
}

inline const Vector4d Vector4d::operator /( double scalar ) const
{
    return Vector4d(
        ( mX / scalar ),
        ( mY / scalar ),
        ( mZ / scalar ),
        ( mW / scalar )
    );
}

inline Vector4d & Vector4d::operator /=( double scalar )
{
    *this = *this / scalar;
    return *this;
}

inline const Vector4d Vector4d::operator -( ) const
{
    return Vector4d(
        -mX,
        -mY,
        -mZ,
        -mW
    );
}

inline const Vector4d operator *( double scalar, const Vector4d & vec )
{
    return vec * scalar;
}

inline const Vector4d mulPerElem( const Vector4d & vec0, const Vector4d & vec1 )
{
    return Vector4d(
        ( vec0.getX() * vec1.getX() ),
        ( vec0.getY() * vec1.getY() ),
        ( vec0.getZ() * vec1.getZ() ),
        ( vec0.getW() * vec1.getW() )
    );
}

inline const Vector4d divPerElem( const Vector4d & vec0, const Vector4d & vec1 )
{
    return Vector4d(
        ( vec0.getX() / vec1.getX() ),
        ( vec0.getY() / vec1.getY() ),
        ( vec0.getZ() / vec1.getZ() ),
        ( vec0.getW() / vec1.getW() )
    );
}

inline const Vector4d recipPerElem( const Vector4d & vec )
{
    return Vector4d(
        ( 1.0 / vec.getX() ),
        ( 1.0 / vec.getY() ),
        ( 1.0 / vec.getZ() ),
        ( 1.0 / vec.getW() )
    );
}

inline const Vector4d sqrtPerElem( const Vector4d & vec )
{
    return Vector4d(
        sqrt( vec.getX() ),
        sqrt( vec.getY() ),
        sqrt( vec.getZ() ),
        sqrt( vec.getW() )
    );
}

inline const Vector4d rsqrtPerElem( const Vector4d & vec )
{
    return Vector4d(
        ( 1.0 / sqrt( vec.getX() ) ),
        ( 1.0 / sqrt( vec.getY() ) ),
        ( 1.0 / sqrt( vec.getZ() ) ),
        ( 1.0 / sqrt( vec.getW() ) )
    );
}

inline const Vector4d absPerElem( const Vector4d & vec )
{
    return Vector4d(
        fabs( vec.getX() ),
        fabs( vec.getY() ),
        fabs( vec.getZ() ),
        fabs( vec.getW() )
    );
}

inline const Vector4d copySignPerElem( const Vector4d & vec0, const Vector4d & vec1 )
{
    return Vector4d(
        ( vec1.getX() < 0.0 )? -fabs( vec0.getX() ) : fabs( vec0.getX() ),
        ( vec1.getY() < 0.0 )? -fabs( vec0.getY() ) : fabs( vec0.getY() ),
        ( vec1.getZ() < 0.0 )? -fabs( vec0.getZ() ) : fabs( vec0.getZ() ),
        ( vec1.getW() < 0.0 )? -fabs( vec0.getW() ) : fabs( vec0.getW() )
    );
}

inline const Vector4d maxPerElem( const Vector4d & vec0, const Vector4d & vec1 )
{
    return Vector4d(
        (vec0.getX() > vec1.getX())? vec0.getX() : vec1.getX(),
        (vec0.getY() > vec1.getY())? vec0.getY() : vec1.getY(),
        (vec0.getZ() > vec1.getZ())? vec0.getZ() : vec1.getZ(),
        (vec0.getW() > vec1.getW())? vec0.getW() : vec1.getW()
    );
}

inline double maxElem( const Vector4d & vec )
{
    double result;
    result = (vec.getX() > vec.getY())? vec.getX() : vec.getY();
    result = (vec.getZ() > result)? vec.getZ() : result;
    result = (vec.getW() > result)? vec.getW() : result;
    return result;
}

inline const Vector4d minPerElem( const Vector4d & vec0, const Vector4d & vec1 )
{
    return Vector4d(
        (vec0.getX() < vec1.getX())? vec0.getX() : vec1.getX(),
        (vec0.getY() < vec1.getY())? vec0.getY() : vec1.getY(),
        (vec0.getZ() < vec1.getZ())? vec0.getZ() : vec1.getZ(),
        (vec0.getW() < vec1.getW())? vec0.getW() : vec1.getW()
    );
}

inline double minElem( const Vector4d & vec )
{
    double result;
    result = (vec.getX() < vec.getY())? vec.getX() : vec.getY();
    result = (vec.getZ() < result)? vec.getZ() : result;
    result = (vec.getW() < result)? vec.getW() : result;
    return result;
}

inline double sum( const Vector4d & vec )
{
    double result;
    result = ( vec.getX() + vec.getY() );
    result = ( result + vec.getZ() );
    result = ( result + vec.getW() );
    return result;
}

inline double dot( const Vector4d & vec0, const Vector4d & vec1 )
{
    double result;
    result = ( vec0.getX() * vec1.getX() );
    result = ( result + ( vec0.getY() * vec1.getY() ) );
    result = ( result + ( vec0.getZ() * vec1.getZ() ) );
    result = ( result + ( vec0.getW() * vec1.getW() ) );
    return result;
}

inline double lengthSqr( const Vector4d & vec )
{
    double result;
    result = ( vec.getX() * vec.getX() );
    result = ( result + ( vec.getY() * vec.getY() ) );
    result = ( result + ( vec.getZ() * vec.getZ() ) );
    result = ( result + ( vec.getW() * vec.getW() ) );
    return result;
}

inline double length( const Vector4d & vec )
{
    return sqrt( lengthSqr( vec ) );
}

inline const Vector4d normalize( const Vector4d & vec )
{
    double lenSqr, lenInv;
    lenSqr = lengthSqr( vec );
    lenInv = ( 1.0 / sqrt( lenSqr ) );
    return Vector4d(
        ( vec.getX() * lenInv ),
        ( vec.getY() * lenInv ),
        ( vec.getZ() * lenInv ),
        ( vec.getW() * lenInv )
    );
}

inline const Vector4d select( const Vector4d & vec0, const Vector4d & vec1, bool select1 )
{
    return Vector4d(
        ( select1 )? vec1.getX() : vec0.getX(),
        ( select1 )? vec1.getY() : vec0.getY(),
        ( select1 )? vec1.getZ() : vec0.getZ(),
        ( select1 )? vec1.getW() : vec0.getW()
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Vector4d & vec )
{
    printf( "( %f %f %f %f )\n", vec.getX(), vec.getY(), vec.getZ(), vec.getW() );
}

inline void print( const Vector4d & vec, const char * name )
{
    printf( "%s: ( %f %f %f %f )\n", name, vec.getX(), vec.getY(), vec.getZ(), vec.getW() );
}

#endif

inline Point3d::Point3d( const Point3d & pnt )
{
    mX = pnt.mX;
    mY = pnt.mY;
    mZ = pnt.mZ;
}

inline Point3d::Point3d( double _x, double _y, double _z )
{
    mX = _x;
    mY = _y;
    mZ = _z;
}

inline Point3d::Point3d( const Vector3d & vec )
{
    mX = vec.getX();
    mY = vec.getY();
    mZ = vec.getZ();
}

inline Point3d::Point3d( double scalar )
{
    mX = scalar;
    mY = scalar;
    mZ = scalar;
}

inline const Point3d lerp( double t, const Point3d & pnt0, const Point3d & pnt1 )
{
    return ( pnt0 + ( ( pnt1 - pnt0 ) * t ) );
}

inline Point3d & Point3d::operator =( const Point3d & pnt )
{
    mX = pnt.mX;
    mY = pnt.mY;
    mZ = pnt.mZ;
    return *this;
}

inline Point3d & Point3d::setX( double _x )
{
    mX = _x;
    return *this;
}

inline double Point3d::getX( ) const
{
    return mX;
}

inline Point3d & Point3d::setY( double _y )
{
    mY = _y;
    return *this;
}

inline double Point3d::getY( ) const
{
    return mY;
}

inline Point3d & Point3d::setZ( double _z )
{
    mZ = _z;
    return *this;
}

inline double Point3d::getZ( ) const
{
    return mZ;
}

inline Point3d & Point3d::setElem( int idx, double value )
{
    *(&mX + idx) = value;
    return *this;
}

inline double Point3d::getElem( int idx ) const
{
    return *(&mX + idx);
}

inline double & Point3d::operator []( int idx )
{
    return *(&mX + idx);
}

inline double Point3d::operator []( int idx ) const
{
    return *(&mX + idx);
}

inline const Vector3d Point3d::operator -( const Point3d & pnt ) const
{
    return Vector3d(
        ( mX - pnt.mX ),
        ( mY - pnt.mY ),
        ( mZ - pnt.mZ )
    );
}

inline const Point3d Point3d::operator +( const Vector3d & vec ) const
{
    return Point3d(
        ( mX + vec.getX() ),
        ( mY + vec.getY() ),
        ( mZ + vec.getZ() )
    );
}

inline const Point3d Point3d::operator -( const Vector3d & vec ) const
{
    return Point3d(
        ( mX - vec.getX() ),
        ( mY - vec.getY() ),
        ( mZ - vec.getZ() )
    );
}

inline Point3d & Point3d::operator +=( const Vector3d & vec )
{
    *this = *this + vec;
    return *this;
}

inline Point3d & Point3d::operator -=( const Vector3d & vec )
{
    *this = *this - vec;
    return *this;
}

inline const Point3d mulPerElem( const Point3d & pnt0, const Point3d & pnt1 )
{
    return Point3d(
        ( pnt0.getX() * pnt1.getX() ),
        ( pnt0.getY() * pnt1.getY() ),
        ( pnt0.getZ() * pnt1.getZ() )
    );
}

inline const Point3d divPerElem( const Point3d & pnt0, const Point3d & pnt1 )
{
    return Point3d(
        ( pnt0.getX() / pnt1.getX() ),
        ( pnt0.getY() / pnt1.getY() ),
        ( pnt0.getZ() / pnt1.getZ() )
    );
}

inline const Point3d recipPerElem( const Point3d & pnt )
{
    return Point3d(
        ( 1.0 / pnt.getX() ),
        ( 1.0 / pnt.getY() ),
        ( 1.0 / pnt.getZ() )
    );
}

inline const Point3d sqrtPerElem( const Point3d & pnt )
{
    return Point3d(
        sqrt( pnt.getX() ),
        sqrt( pnt.getY() ),
        sqrt( pnt.getZ() )
    );
}

inline const Point3d rsqrtPerElem( const Point3d & pnt )
{
    return Point3d(
        ( 1.0 / sqrt( pnt.getX() ) ),
        ( 1.0 / sqrt( pnt.getY() ) ),
        ( 1.0 / sqrt( pnt.getZ() ) )
    );
}

inline const Point3d absPerElem( const Point3d & pnt )
{
    return Point3d(
        fabs( pnt.getX() ),
        fabs( pnt.getY() ),
        fabs( pnt.getZ() )
    );
}

inline const Point3d copySignPerElem( const Point3d & pnt0, const Point3d & pnt1 )
{
    return Point3d(
        ( pnt1.getX() < 0.0 )? -fabs( pnt0.getX() ) : fabs( pnt0.getX() ),
        ( pnt1.getY() < 0.0 )? -fabs( pnt0.getY() ) : fabs( pnt0.getY() ),
        ( pnt1.getZ() < 0.0 )? -fabs( pnt0.getZ() ) : fabs( pnt0.getZ() )
    );
}

inline const Point3d maxPerElem( const Point3d & pnt0, const Point3d & pnt1 )
{
    return Point3d(
        (pnt0.getX() > pnt1.getX())? pnt0.getX() : pnt1.getX(),
        (pnt0.getY() > pnt1.getY())? pnt0.getY() : pnt1.getY(),
        (pnt0.getZ() > pnt1.getZ())? pnt0.getZ() : pnt1.getZ()
    );
}

inline double maxElem( const Point3d & pnt )
{
    double result;
    result = (pnt.getX() > pnt.getY())? pnt.getX() : pnt.getY();
    result = (pnt.getZ() > result)? pnt.getZ() : result;
    return result;
}

inline const Point3d minPerElem( const Point3d & pnt0, const Point3d & pnt1 )
{
    return Point3d(
        (pnt0.getX() < pnt1.getX())? pnt0.getX() : pnt1.getX(),
        (pnt0.getY() < pnt1.getY())? pnt0.getY() : pnt1.getY(),
        (pnt0.getZ() < pnt1.getZ())? pnt0.getZ() : pnt1.getZ()
    );
}

inline double minElem( const Point3d & pnt )
{
    double result;
    result = (pnt.getX() < pnt.getY())? pnt.getX() : pnt.getY();
    result = (pnt.getZ() < result)? pnt.getZ() : result;
    return result;
}

inline double sum( const Point3d & pnt )
{
    double result;
    result = ( pnt.getX() + pnt.getY() );
    result = ( result + pnt.getZ() );
    return result;
}

inline const Point3d scale( const Point3d & pnt, double scaleVal )
{
    return mulPerElem( pnt, Point3d( scaleVal ) );
}

inline const Point3d scale( const Point3d & pnt, const Vector3d & scaleVec )
{
    return mulPerElem( pnt, Point3d( scaleVec ) );
}

inline double projection( const Point3d & pnt, const Vector3d & unitVec )
{
    double result;
    result = ( pnt.getX() * unitVec.getX() );
    result = ( result + ( pnt.getY() * unitVec.getY() ) );
    result = ( result + ( pnt.getZ() * unitVec.getZ() ) );
    return result;
}

inline double distSqrFromOrigin( const Point3d & pnt )
{
    return lengthSqr( Vector3d( pnt ) );
}

inline double distFromOrigin( const Point3d & pnt )
{
    return length( Vector3d( pnt ) );
}

inline double distSqr( const Point3d & pnt0, const Point3d & pnt1 )
{
    return lengthSqr( ( pnt1 - pnt0 ) );
}

inline double dist( const Point3d & pnt0, const Point3d & pnt1 )
{
    return length( ( pnt1 - pnt0 ) );
}

inline const Point3d select( const Point3d & pnt0, const Point3d & pnt1, bool select1 )
{
    return Point3d(
        ( select1 )? pnt1.getX() : pnt0.getX(),
        ( select1 )? pnt1.getY() : pnt0.getY(),
        ( select1 )? pnt1.getZ() : pnt0.getZ()
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Point3d & pnt )
{
    printf( "( %f %f %f )\n", pnt.getX(), pnt.getY(), pnt.getZ() );
}

inline void print( const Point3d & pnt, const char * name )
{
    printf( "%s: ( %f %f %f )\n", name, pnt.getX(), pnt.getY(), pnt.getZ() );
}

#endif

} // namespace Aos
} // namespace Vectormath

#endif
