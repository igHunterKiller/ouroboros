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

#ifndef _VECTORMATH_AOS_D_CPP_SCALAR_H
#define _VECTORMATH_AOS_D_CPP_SCALAR_H

#include <math.h>

#ifdef _VECTORMATH_DEBUG
#include <stdio.h>
#endif

namespace Vectormath {

namespace Aos {

//-----------------------------------------------------------------------------
// Forward Declarations
//

class Vector3d;
class Vector4d;
class Point3d;
class Quatd;
class Matrix3d;
class Matrix4d;
class Transform3d;

// A 3-D vector in array-of-structures format
//
class Vector3d
{
    double mX;
    double mY;
    double mZ;
#ifndef __GNUC__
    double d;
#endif

public:
    // Default constructor; does no initialization
    // 
    inline Vector3d( ) { };

    // Copy a 3-D vector
    // 
    inline Vector3d( const Vector3d & vec );

    // Construct a 3-D vector from x, y, and z elements
    // 
    inline Vector3d( double x, double y, double z );

    // Copy elements from a 3-D point into a 3-D vector
    // 
    explicit inline Vector3d( const Point3d & pnt );

    // Set all elements of a 3-D vector to the same scalar value
    // 
    explicit inline Vector3d( double scalar );

    // Assign one 3-D vector to another
    // 
    inline Vector3d & operator =( const Vector3d & vec );

    // Set the x element of a 3-D vector
    // 
    inline Vector3d & setX( double x );

    // Set the y element of a 3-D vector
    // 
    inline Vector3d & setY( double y );

    // Set the z element of a 3-D vector
    // 
    inline Vector3d & setZ( double z );

    // Get the x element of a 3-D vector
    // 
    inline double getX( ) const;

    // Get the y element of a 3-D vector
    // 
    inline double getY( ) const;

    // Get the z element of a 3-D vector
    // 
    inline double getZ( ) const;

    // Set an x, y, or z element of a 3-D vector by index
    // 
    inline Vector3d & setElem( int idx, double value );

    // Get an x, y, or z element of a 3-D vector by index
    // 
    inline double getElem( int idx ) const;

    // Subscripting operator to set or get an element
    // 
    inline double & operator []( int idx );

    // Subscripting operator to get an element
    // 
    inline double operator []( int idx ) const;

    // Add two 3-D vectors
    // 
    inline const Vector3d operator +( const Vector3d & vec ) const;

    // Subtract a 3-D vector from another 3-D vector
    // 
    inline const Vector3d operator -( const Vector3d & vec ) const;

    // Add a 3-D vector to a 3-D point
    // 
    inline const Point3d operator +( const Point3d & pnt ) const;

    // Multiply a 3-D vector by a scalar
    // 
    inline const Vector3d operator *( double scalar ) const;

    // Divide a 3-D vector by a scalar
    // 
    inline const Vector3d operator /( double scalar ) const;

    // Perform compound assignment and addition with a 3-D vector
    // 
    inline Vector3d & operator +=( const Vector3d & vec );

    // Perform compound assignment and subtraction by a 3-D vector
    // 
    inline Vector3d & operator -=( const Vector3d & vec );

    // Perform compound assignment and multiplication by a scalar
    // 
    inline Vector3d & operator *=( double scalar );

    // Perform compound assignment and division by a scalar
    // 
    inline Vector3d & operator /=( double scalar );

    // Negate all elements of a 3-D vector
    // 
    inline const Vector3d operator -( ) const;

    // Construct x axis
    // 
    static inline const Vector3d xAxis( );

    // Construct y axis
    // 
    static inline const Vector3d yAxis( );

    // Construct z axis
    // 
    static inline const Vector3d zAxis( );

}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;

// Multiply a 3-D vector by a scalar
// 
inline const Vector3d operator *( double scalar, const Vector3d & vec );

// Multiply two 3-D vectors per element
// 
inline const Vector3d mulPerElem( const Vector3d & vec0, const Vector3d & vec1 );

// Divide two 3-D vectors per element
// NOTE: 
// Floating-point behavior matches standard library function divf4.
// 
inline const Vector3d divPerElem( const Vector3d & vec0, const Vector3d & vec1 );

// Compute the reciprocal of a 3-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function recipf4.
// 
inline const Vector3d recipPerElem( const Vector3d & vec );

// Compute the square root of a 3-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function sqrt4.
// 
inline const Vector3d sqrtPerElem( const Vector3d & vec );

// Compute the reciprocal square root of a 3-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function rsqrt4.
// 
inline const Vector3d rsqrtPerElem( const Vector3d & vec );

// Compute the absolute value of a 3-D vector per element
// 
inline const Vector3d absPerElem( const Vector3d & vec );

// Copy sign from one 3-D vector to another, per element
// 
inline const Vector3d copySignPerElem( const Vector3d & vec0, const Vector3d & vec1 );

// Maximum of two 3-D vectors per element
// 
inline const Vector3d maxPerElem( const Vector3d & vec0, const Vector3d & vec1 );

// Minimum of two 3-D vectors per element
// 
inline const Vector3d minPerElem( const Vector3d & vec0, const Vector3d & vec1 );

// Maximum element of a 3-D vector
// 
inline double maxElem( const Vector3d & vec );

// Minimum element of a 3-D vector
// 
inline double minElem( const Vector3d & vec );

// Compute the sum of all elements of a 3-D vector
// 
inline double sum( const Vector3d & vec );

// Compute the dot product of two 3-D vectors
// 
inline double dot( const Vector3d & vec0, const Vector3d & vec1 );

// Compute the square of the length of a 3-D vector
// 
inline double lengthSqr( const Vector3d & vec );

// Compute the length of a 3-D vector
// 
inline double length( const Vector3d & vec );

// Normalize a 3-D vector
// NOTE: 
// The result is unpredictable when all elements of vec are at or near zero.
// 
inline const Vector3d normalize( const Vector3d & vec );

// Compute cross product of two 3-D vectors
// 
inline const Vector3d cross( const Vector3d & vec0, const Vector3d & vec1 );

// Outer product of two 3-D vectors
// 
inline const Matrix3d outer( const Vector3d & vec0, const Vector3d & vec1 );

// Pre-multiply a row vector by a 3x3 matrix
// 
inline const Vector3d rowMul( const Vector3d & vec, const Matrix3d & mat );

// Cross-product matrix of a 3-D vector
// 
inline const Matrix3d crossMatrix( const Vector3d & vec );

// Create cross-product matrix and multiply
// NOTE: 
// Faster than separately creating a cross-product matrix and multiplying.
// 
inline const Matrix3d crossMatrixMul( const Vector3d & vec, const Matrix3d & mat );

// Linear interpolation between two 3-D vectors
// NOTE: 
// Does not clamp t between 0 and 1.
// 
inline const Vector3d lerp( double t, const Vector3d & vec0, const Vector3d & vec1 );

// Spherical linear interpolation between two 3-D vectors
// NOTE: 
// The result is unpredictable if the vectors point in opposite directions.
// Does not clamp t between 0 and 1.
// 
inline const Vector3d slerp( double t, const Vector3d & unitVec0, const Vector3d & unitVec1 );

// Conditionally select between two 3-D vectors
// 
inline const Vector3d select( const Vector3d & vec0, const Vector3d & vec1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 3-D vector
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Vector3d & vec );

// Print a 3-D vector and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Vector3d & vec, const char * name );

#endif

// A 4-D vector in array-of-structures format
//
class Vector4d
{
    double mX;
    double mY;
    double mZ;
    double mW;

public:
    // Default constructor; does no initialization
    // 
    inline Vector4d( ) { };

    // Copy a 4-D vector
    // 
    inline Vector4d( const Vector4d & vec );

    // Construct a 4-D vector from x, y, z, and w elements
    // 
    inline Vector4d( double x, double y, double z, double w );

    // Construct a 4-D vector from a 3-D vector and a scalar
    // 
    inline Vector4d( const Vector3d & xyz, double w );

    // Copy x, y, and z from a 3-D vector into a 4-D vector, and set w to 0
    // 
    explicit inline Vector4d( const Vector3d & vec );

    // Copy x, y, and z from a 3-D point into a 4-D vector, and set w to 1
    // 
    explicit inline Vector4d( const Point3d & pnt );

    // Copy elements from a quaternion into a 4-D vector
    // 
    explicit inline Vector4d( const Quatd & quat );

    // Set all elements of a 4-D vector to the same scalar value
    // 
    explicit inline Vector4d( double scalar );

    // Assign one 4-D vector to another
    // 
    inline Vector4d & operator =( const Vector4d & vec );

    // Set the x, y, and z elements of a 4-D vector
    // NOTE: 
    // This function does not change the w element.
    // 
    inline Vector4d & setXYZ( const Vector3d & vec );

    // Get the x, y, and z elements of a 4-D vector
    // 
    inline const Vector3d getXYZ( ) const;

    // Set the x element of a 4-D vector
    // 
    inline Vector4d & setX( double x );

    // Set the y element of a 4-D vector
    // 
    inline Vector4d & setY( double y );

    // Set the z element of a 4-D vector
    // 
    inline Vector4d & setZ( double z );

    // Set the w element of a 4-D vector
    // 
    inline Vector4d & setW( double w );

    // Get the x element of a 4-D vector
    // 
    inline double getX( ) const;

    // Get the y element of a 4-D vector
    // 
    inline double getY( ) const;

    // Get the z element of a 4-D vector
    // 
    inline double getZ( ) const;

    // Get the w element of a 4-D vector
    // 
    inline double getW( ) const;

    // Set an x, y, z, or w element of a 4-D vector by index
    // 
    inline Vector4d & setElem( int idx, double value );

    // Get an x, y, z, or w element of a 4-D vector by index
    // 
    inline double getElem( int idx ) const;

    // Subscripting operator to set or get an element
    // 
    inline double & operator []( int idx );

    // Subscripting operator to get an element
    // 
    inline double operator []( int idx ) const;

    // Add two 4-D vectors
    // 
    inline const Vector4d operator +( const Vector4d & vec ) const;

    // Subtract a 4-D vector from another 4-D vector
    // 
    inline const Vector4d operator -( const Vector4d & vec ) const;

    // Multiply a 4-D vector by a scalar
    // 
    inline const Vector4d operator *( double scalar ) const;

    // Divide a 4-D vector by a scalar
    // 
    inline const Vector4d operator /( double scalar ) const;

    // Perform compound assignment and addition with a 4-D vector
    // 
    inline Vector4d & operator +=( const Vector4d & vec );

    // Perform compound assignment and subtraction by a 4-D vector
    // 
    inline Vector4d & operator -=( const Vector4d & vec );

    // Perform compound assignment and multiplication by a scalar
    // 
    inline Vector4d & operator *=( double scalar );

    // Perform compound assignment and division by a scalar
    // 
    inline Vector4d & operator /=( double scalar );

    // Negate all elements of a 4-D vector
    // 
    inline const Vector4d operator -( ) const;

    // Construct x axis
    // 
    static inline const Vector4d xAxis( );

    // Construct y axis
    // 
    static inline const Vector4d yAxis( );

    // Construct z axis
    // 
    static inline const Vector4d zAxis( );

    // Construct w axis
    // 
    static inline const Vector4d wAxis( );

}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;

// Multiply a 4-D vector by a scalar
// 
inline const Vector4d operator *( double scalar, const Vector4d & vec );

// Multiply two 4-D vectors per element
// 
inline const Vector4d mulPerElem( const Vector4d & vec0, const Vector4d & vec1 );

// Divide two 4-D vectors per element
// NOTE: 
// Floating-point behavior matches standard library function divf4.
// 
inline const Vector4d divPerElem( const Vector4d & vec0, const Vector4d & vec1 );

// Compute the reciprocal of a 4-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function recipf4.
// 
inline const Vector4d recipPerElem( const Vector4d & vec );

// Compute the square root of a 4-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function sqrt4.
// 
inline const Vector4d sqrtPerElem( const Vector4d & vec );

// Compute the reciprocal square root of a 4-D vector per element
// NOTE: 
// Floating-point behavior matches standard library function rsqrt4.
// 
inline const Vector4d rsqrtPerElem( const Vector4d & vec );

// Compute the absolute value of a 4-D vector per element
// 
inline const Vector4d absPerElem( const Vector4d & vec );

// Copy sign from one 4-D vector to another, per element
// 
inline const Vector4d copySignPerElem( const Vector4d & vec0, const Vector4d & vec1 );

// Maximum of two 4-D vectors per element
// 
inline const Vector4d maxPerElem( const Vector4d & vec0, const Vector4d & vec1 );

// Minimum of two 4-D vectors per element
// 
inline const Vector4d minPerElem( const Vector4d & vec0, const Vector4d & vec1 );

// Maximum element of a 4-D vector
// 
inline double maxElem( const Vector4d & vec );

// Minimum element of a 4-D vector
// 
inline double minElem( const Vector4d & vec );

// Compute the sum of all elements of a 4-D vector
// 
inline double sum( const Vector4d & vec );

// Compute the dot product of two 4-D vectors
// 
inline double dot( const Vector4d & vec0, const Vector4d & vec1 );

// Compute the square of the length of a 4-D vector
// 
inline double lengthSqr( const Vector4d & vec );

// Compute the length of a 4-D vector
// 
inline double length( const Vector4d & vec );

// Normalize a 4-D vector
// NOTE: 
// The result is unpredictable when all elements of vec are at or near zero.
// 
inline const Vector4d normalize( const Vector4d & vec );

// Outer product of two 4-D vectors
// 
inline const Matrix4d outer( const Vector4d & vec0, const Vector4d & vec1 );

// Linear interpolation between two 4-D vectors
// NOTE: 
// Does not clamp t between 0 and 1.
// 
inline const Vector4d lerp( double t, const Vector4d & vec0, const Vector4d & vec1 );

// Spherical linear interpolation between two 4-D vectors
// NOTE: 
// The result is unpredictable if the vectors point in opposite directions.
// Does not clamp t between 0 and 1.
// 
inline const Vector4d slerp( double t, const Vector4d & unitVec0, const Vector4d & unitVec1 );

// Conditionally select between two 4-D vectors
// 
inline const Vector4d select( const Vector4d & vec0, const Vector4d & vec1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 4-D vector
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Vector4d & vec );

// Print a 4-D vector and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Vector4d & vec, const char * name );

#endif

// A 3-D point in array-of-structures format
//
class Point3d
{
    double mX;
    double mY;
    double mZ;
#ifndef __GNUC__
    double d;
#endif

public:
    // Default constructor; does no initialization
    // 
    inline Point3d( ) { };

    // Copy a 3-D point
    // 
    inline Point3d( const Point3d & pnt );

    // Construct a 3-D point from x, y, and z elements
    // 
    inline Point3d( double x, double y, double z );

    // Copy elements from a 3-D vector into a 3-D point
    // 
    explicit inline Point3d( const Vector3d & vec );

    // Set all elements of a 3-D point to the same scalar value
    // 
    explicit inline Point3d( double scalar );

    // Assign one 3-D point to another
    // 
    inline Point3d & operator =( const Point3d & pnt );

    // Set the x element of a 3-D point
    // 
    inline Point3d & setX( double x );

    // Set the y element of a 3-D point
    // 
    inline Point3d & setY( double y );

    // Set the z element of a 3-D point
    // 
    inline Point3d & setZ( double z );

    // Get the x element of a 3-D point
    // 
    inline double getX( ) const;

    // Get the y element of a 3-D point
    // 
    inline double getY( ) const;

    // Get the z element of a 3-D point
    // 
    inline double getZ( ) const;

    // Set an x, y, or z element of a 3-D point by index
    // 
    inline Point3d & setElem( int idx, double value );

    // Get an x, y, or z element of a 3-D point by index
    // 
    inline double getElem( int idx ) const;

    // Subscripting operator to set or get an element
    // 
    inline double & operator []( int idx );

    // Subscripting operator to get an element
    // 
    inline double operator []( int idx ) const;

    // Subtract a 3-D point from another 3-D point
    // 
    inline const Vector3d operator -( const Point3d & pnt ) const;

    // Add a 3-D point to a 3-D vector
    // 
    inline const Point3d operator +( const Vector3d & vec ) const;

    // Subtract a 3-D vector from a 3-D point
    // 
    inline const Point3d operator -( const Vector3d & vec ) const;

    // Perform compound assignment and addition with a 3-D vector
    // 
    inline Point3d & operator +=( const Vector3d & vec );

    // Perform compound assignment and subtraction by a 3-D vector
    // 
    inline Point3d & operator -=( const Vector3d & vec );

}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;

// Multiply two 3-D points per element
// 
inline const Point3d mulPerElem( const Point3d & pnt0, const Point3d & pnt1 );

// Divide two 3-D points per element
// NOTE: 
// Floating-point behavior matches standard library function divf4.
// 
inline const Point3d divPerElem( const Point3d & pnt0, const Point3d & pnt1 );

// Compute the reciprocal of a 3-D point per element
// NOTE: 
// Floating-point behavior matches standard library function recipf4.
// 
inline const Point3d recipPerElem( const Point3d & pnt );

// Compute the square root of a 3-D point per element
// NOTE: 
// Floating-point behavior matches standard library function sqrt4.
// 
inline const Point3d sqrtPerElem( const Point3d & pnt );

// Compute the reciprocal square root of a 3-D point per element
// NOTE: 
// Floating-point behavior matches standard library function rsqrt4.
// 
inline const Point3d rsqrtPerElem( const Point3d & pnt );

// Compute the absolute value of a 3-D point per element
// 
inline const Point3d absPerElem( const Point3d & pnt );

// Copy sign from one 3-D point to another, per element
// 
inline const Point3d copySignPerElem( const Point3d & pnt0, const Point3d & pnt1 );

// Maximum of two 3-D points per element
// 
inline const Point3d maxPerElem( const Point3d & pnt0, const Point3d & pnt1 );

// Minimum of two 3-D points per element
// 
inline const Point3d minPerElem( const Point3d & pnt0, const Point3d & pnt1 );

// Maximum element of a 3-D point
// 
inline double maxElem( const Point3d & pnt );

// Minimum element of a 3-D point
// 
inline double minElem( const Point3d & pnt );

// Compute the sum of all elements of a 3-D point
// 
inline double sum( const Point3d & pnt );

// Apply uniform scale to a 3-D point
// 
inline const Point3d scale( const Point3d & pnt, double scaleVal );

// Apply non-uniform scale to a 3-D point
// 
inline const Point3d scale( const Point3d & pnt, const Vector3d & scaleVec );

// Scalar projection of a 3-D point on a unit-length 3-D vector
// 
inline double projection( const Point3d & pnt, const Vector3d & unitVec );

// Compute the square of the distance of a 3-D point from the coordinate-system origin
// 
inline double distSqrFromOrigin( const Point3d & pnt );

// Compute the distance of a 3-D point from the coordinate-system origin
// 
inline double distFromOrigin( const Point3d & pnt );

// Compute the square of the distance between two 3-D points
// 
inline double distSqr( const Point3d & pnt0, const Point3d & pnt1 );

// Compute the distance between two 3-D points
// 
inline double dist( const Point3d & pnt0, const Point3d & pnt1 );

// Linear interpolation between two 3-D points
// NOTE: 
// Does not clamp t between 0 and 1.
// 
inline const Point3d lerp( double t, const Point3d & pnt0, const Point3d & pnt1 );

// Conditionally select between two 3-D points
// 
inline const Point3d select( const Point3d & pnt0, const Point3d & pnt1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 3-D point
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Point3d & pnt );

// Print a 3-D point and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Point3d & pnt, const char * name );

#endif

// A quaternion in array-of-structures format
//
class Quatd
{
    double mX;
    double mY;
    double mZ;
    double mW;

public:
    // Default constructor; does no initialization
    // 
    inline Quatd( ) { };

    // Copy a quaternion
    // 
    inline Quatd( const Quatd & quat );

    // Construct a quaternion from x, y, z, and w elements
    // 
    inline Quatd( double x, double y, double z, double w );

    // Construct a quaternion from a 3-D vector and a scalar
    // 
    inline Quatd( const Vector3d & xyz, double w );

    // Copy elements from a 4-D vector into a quaternion
    // 
    explicit inline Quatd( const Vector4d & vec );

    // Convert a rotation matrix to a unit-length quaternion
    // 
    explicit inline Quatd( const Matrix3d & rotMat );

    // Set all elements of a quaternion to the same scalar value
    // 
    explicit inline Quatd( double scalar );

    // Assign one quaternion to another
    // 
    inline Quatd & operator =( const Quatd & quat );

    // Set the x, y, and z elements of a quaternion
    // NOTE: 
    // This function does not change the w element.
    // 
    inline Quatd & setXYZ( const Vector3d & vec );

    // Get the x, y, and z elements of a quaternion
    // 
    inline const Vector3d getXYZ( ) const;

    // Set the x element of a quaternion
    // 
    inline Quatd & setX( double x );

    // Set the y element of a quaternion
    // 
    inline Quatd & setY( double y );

    // Set the z element of a quaternion
    // 
    inline Quatd & setZ( double z );

    // Set the w element of a quaternion
    // 
    inline Quatd & setW( double w );

    // Get the x element of a quaternion
    // 
    inline double getX( ) const;

    // Get the y element of a quaternion
    // 
    inline double getY( ) const;

    // Get the z element of a quaternion
    // 
    inline double getZ( ) const;

    // Get the w element of a quaternion
    // 
    inline double getW( ) const;

    // Set an x, y, z, or w element of a quaternion by index
    // 
    inline Quatd & setElem( int idx, double value );

    // Get an x, y, z, or w element of a quaternion by index
    // 
    inline double getElem( int idx ) const;

    // Subscripting operator to set or get an element
    // 
    inline double & operator []( int idx );

    // Subscripting operator to get an element
    // 
    inline double operator []( int idx ) const;

    // Add two quaternions
    // 
    inline const Quatd operator +( const Quatd & quat ) const;

    // Subtract a quaternion from another quaternion
    // 
    inline const Quatd operator -( const Quatd & quat ) const;

    // Multiply two quaternions
    // 
    inline const Quatd operator *( const Quatd & quat ) const;

    // Multiply a quaternion by a scalar
    // 
    inline const Quatd operator *( double scalar ) const;

    // Divide a quaternion by a scalar
    // 
    inline const Quatd operator /( double scalar ) const;

    // Perform compound assignment and addition with a quaternion
    // 
    inline Quatd & operator +=( const Quatd & quat );

    // Perform compound assignment and subtraction by a quaternion
    // 
    inline Quatd & operator -=( const Quatd & quat );

    // Perform compound assignment and multiplication by a quaternion
    // 
    inline Quatd & operator *=( const Quatd & quat );

    // Perform compound assignment and multiplication by a scalar
    // 
    inline Quatd & operator *=( double scalar );

    // Perform compound assignment and division by a scalar
    // 
    inline Quatd & operator /=( double scalar );

    // Negate all elements of a quaternion
    // 
    inline const Quatd operator -( ) const;

    // Construct an identity quaternion
    // 
    static inline const Quatd identity( );

    // Construct a quaternion to rotate between two unit-length 3-D vectors
    // NOTE: 
    // The result is unpredictable if unitVec0 and unitVec1 point in opposite directions.
    // 
    static inline const Quatd rotation( const Vector3d & unitVec0, const Vector3d & unitVec1 );

    // Construct a quaternion to rotate around a unit-length 3-D vector
    // 
    static inline const Quatd rotation( double radians, const Vector3d & unitVec );

    // Construct a quaternion to rotate around the x axis
    // 
    static inline const Quatd rotationX( double radians );

    // Construct a quaternion to rotate around the y axis
    // 
    static inline const Quatd rotationY( double radians );

    // Construct a quaternion to rotate around the z axis
    // 
    static inline const Quatd rotationZ( double radians );

}
#ifdef __GNUC__
__attribute__ ((aligned(16)))
#endif
;

// Multiply a quaternion by a scalar
// 
inline const Quatd operator *( double scalar, const Quatd & quat );

// Compute the conjugate of a quaternion
// 
inline const Quatd conj( const Quatd & quat );

// Use a unit-length quaternion to rotate a 3-D vector
// 
inline const Vector3d rotate( const Quatd & unitQuat, const Vector3d & vec );

// Compute the dot product of two quaternions
// 
inline double dot( const Quatd & quat0, const Quatd & quat1 );

// Compute the norm of a quaternion
// 
inline double norm( const Quatd & quat );

// Compute the length of a quaternion
// 
inline double length( const Quatd & quat );

// Normalize a quaternion
// NOTE: 
// The result is unpredictable when all elements of quat are at or near zero.
// 
inline const Quatd normalize( const Quatd & quat );

// Linear interpolation between two quaternions
// NOTE: 
// Does not clamp t between 0 and 1.
// 
inline const Quatd lerp( double t, const Quatd & quat0, const Quatd & quat1 );

// Spherical linear interpolation between two quaternions
// NOTE: 
// Interpolates along the shortest path between orientations.
// Does not clamp t between 0 and 1.
// 
inline const Quatd slerp( double t, const Quatd & unitQuat0, const Quatd & unitQuat1 );

// Spherical quadrangle interpolation
// 
inline const Quatd squad( double t, const Quatd & unitQuat0, const Quatd & unitQuat1, const Quatd & unitQuat2, const Quatd & unitQuat3 );

// Conditionally select between two quaternions
// 
inline const Quatd select( const Quatd & quat0, const Quatd & quat1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a quaternion
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Quatd & quat );

// Print a quaternion and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Quatd & quat, const char * name );

#endif

// A 3x3 matrix in array-of-structures format
//
class Matrix3d
{
    Vector3d mCol0;
    Vector3d mCol1;
    Vector3d mCol2;

public:
    // Default constructor; does no initialization
    // 
    inline Matrix3d( ) { };

    // Copy a 3x3 matrix
    // 
    inline Matrix3d( const Matrix3d & mat );

    // Construct a 3x3 matrix containing the specified columns
    // 
    inline Matrix3d( const Vector3d & col0, const Vector3d & col1, const Vector3d & col2 );

    // Construct a 3x3 rotation matrix from a unit-length quaternion
    // 
    explicit inline Matrix3d( const Quatd & unitQuat );

    // Set all elements of a 3x3 matrix to the same scalar value
    // 
    explicit inline Matrix3d( double scalar );

    // Assign one 3x3 matrix to another
    // 
    inline Matrix3d & operator =( const Matrix3d & mat );

    // Set column 0 of a 3x3 matrix
    // 
    inline Matrix3d & setCol0( const Vector3d & col0 );

    // Set column 1 of a 3x3 matrix
    // 
    inline Matrix3d & setCol1( const Vector3d & col1 );

    // Set column 2 of a 3x3 matrix
    // 
    inline Matrix3d & setCol2( const Vector3d & col2 );

    // Get column 0 of a 3x3 matrix
    // 
    inline const Vector3d getCol0( ) const;

    // Get column 1 of a 3x3 matrix
    // 
    inline const Vector3d getCol1( ) const;

    // Get column 2 of a 3x3 matrix
    // 
    inline const Vector3d getCol2( ) const;

    // Set the column of a 3x3 matrix referred to by the specified index
    // 
    inline Matrix3d & setCol( int col, const Vector3d & vec );

    // Set the row of a 3x3 matrix referred to by the specified index
    // 
    inline Matrix3d & setRow( int row, const Vector3d & vec );

    // Get the column of a 3x3 matrix referred to by the specified index
    // 
    inline const Vector3d getCol( int col ) const;

    // Get the row of a 3x3 matrix referred to by the specified index
    // 
    inline const Vector3d getRow( int row ) const;

    // Subscripting operator to set or get a column
    // 
    inline Vector3d & operator []( int col );

    // Subscripting operator to get a column
    // 
    inline const Vector3d operator []( int col ) const;

    // Set the element of a 3x3 matrix referred to by column and row indices
    // 
    inline Matrix3d & setElem( int col, int row, double val );

    // Get the element of a 3x3 matrix referred to by column and row indices
    // 
    inline double getElem( int col, int row ) const;

    // Add two 3x3 matrices
    // 
    inline const Matrix3d operator +( const Matrix3d & mat ) const;

    // Subtract a 3x3 matrix from another 3x3 matrix
    // 
    inline const Matrix3d operator -( const Matrix3d & mat ) const;

    // Negate all elements of a 3x3 matrix
    // 
    inline const Matrix3d operator -( ) const;

    // Multiply a 3x3 matrix by a scalar
    // 
    inline const Matrix3d operator *( double scalar ) const;

    // Multiply a 3x3 matrix by a 3-D vector
    // 
    inline const Vector3d operator *( const Vector3d & vec ) const;

    // Multiply two 3x3 matrices
    // 
    inline const Matrix3d operator *( const Matrix3d & mat ) const;

    // Perform compound assignment and addition with a 3x3 matrix
    // 
    inline Matrix3d & operator +=( const Matrix3d & mat );

    // Perform compound assignment and subtraction by a 3x3 matrix
    // 
    inline Matrix3d & operator -=( const Matrix3d & mat );

    // Perform compound assignment and multiplication by a scalar
    // 
    inline Matrix3d & operator *=( double scalar );

    // Perform compound assignment and multiplication by a 3x3 matrix
    // 
    inline Matrix3d & operator *=( const Matrix3d & mat );

    // Construct an identity 3x3 matrix
    // 
    static inline const Matrix3d identity( );

    // Construct a 3x3 matrix to rotate around the x axis
    // 
    static inline const Matrix3d rotationX( double radians );

    // Construct a 3x3 matrix to rotate around the y axis
    // 
    static inline const Matrix3d rotationY( double radians );

    // Construct a 3x3 matrix to rotate around the z axis
    // 
    static inline const Matrix3d rotationZ( double radians );

    // Construct a 3x3 matrix to rotate around the x, y, and z axes
    // 
    static inline const Matrix3d rotationZYX( const Vector3d & radiansXYZ );

    // Construct a 3x3 matrix to rotate around a unit-length 3-D vector
    // 
    static inline const Matrix3d rotation( double radians, const Vector3d & unitVec );

    // Construct a rotation matrix from a unit-length quaternion
    // 
    static inline const Matrix3d rotation( const Quatd & unitQuat );

    // Construct a 3x3 matrix to perform scaling
    // 
    static inline const Matrix3d scale( const Vector3d & scaleVec );

};
// Multiply a 3x3 matrix by a scalar
// 
inline const Matrix3d operator *( double scalar, const Matrix3d & mat );

// Append (post-multiply) a scale transformation to a 3x3 matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Matrix3d appendScale( const Matrix3d & mat, const Vector3d & scaleVec );

// Prepend (pre-multiply) a scale transformation to a 3x3 matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Matrix3d prependScale( const Vector3d & scaleVec, const Matrix3d & mat );

// Multiply two 3x3 matrices per element
// 
inline const Matrix3d mulPerElem( const Matrix3d & mat0, const Matrix3d & mat1 );

// Compute the absolute value of a 3x3 matrix per element
// 
inline const Matrix3d absPerElem( const Matrix3d & mat );

// Transpose of a 3x3 matrix
// 
inline const Matrix3d transpose( const Matrix3d & mat );

// Compute the inverse of a 3x3 matrix
// NOTE: 
// Result is unpredictable when the determinant of mat is equal to or near 0.
// 
inline const Matrix3d inverse( const Matrix3d & mat );

// Determinant of a 3x3 matrix
// 
inline double determinant( const Matrix3d & mat );

// Conditionally select between two 3x3 matrices
// 
inline const Matrix3d select( const Matrix3d & mat0, const Matrix3d & mat1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 3x3 matrix
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Matrix3d & mat );

// Print a 3x3 matrix and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Matrix3d & mat, const char * name );

#endif

// A 4x4 matrix in array-of-structures format
//
class Matrix4d
{
    Vector4d mCol0;
    Vector4d mCol1;
    Vector4d mCol2;
    Vector4d mCol3;

public:
    // Default constructor; does no initialization
    // 
    inline Matrix4d( ) { };

    // Copy a 4x4 matrix
    // 
    inline Matrix4d( const Matrix4d & mat );

    // Construct a 4x4 matrix containing the specified columns
    // 
    inline Matrix4d( const Vector4d & col0, const Vector4d & col1, const Vector4d & col2, const Vector4d & col3 );

    // Construct a 4x4 matrix from a 3x4 transformation matrix
    // 
    explicit inline Matrix4d( const Transform3d & mat );

    // Construct a 4x4 matrix from a 3x3 matrix and a 3-D vector
    // 
    inline Matrix4d( const Matrix3d & mat, const Vector3d & translateVec );

    // Construct a 4x4 matrix from a unit-length quaternion and a 3-D vector
    // 
    inline Matrix4d( const Quatd & unitQuat, const Vector3d & translateVec );

    // Set all elements of a 4x4 matrix to the same scalar value
    // 
    explicit inline Matrix4d( double scalar );

    // Assign one 4x4 matrix to another
    // 
    inline Matrix4d & operator =( const Matrix4d & mat );

    // Set the upper-left 3x3 submatrix
    // NOTE: 
    // This function does not change the bottom row elements.
    // 
    inline Matrix4d & setUpper3x3( const Matrix3d & mat3 );

    // Get the upper-left 3x3 submatrix of a 4x4 matrix
    // 
    inline const Matrix3d getUpper3x3( ) const;

    // Set translation component
    // NOTE: 
    // This function does not change the bottom row elements.
    // 
    inline Matrix4d & setTranslation( const Vector3d & translateVec );

    // Get the translation component of a 4x4 matrix
    // 
    inline const Vector3d getTranslation( ) const;

    // Set column 0 of a 4x4 matrix
    // 
    inline Matrix4d & setCol0( const Vector4d & col0 );

    // Set column 1 of a 4x4 matrix
    // 
    inline Matrix4d & setCol1( const Vector4d & col1 );

    // Set column 2 of a 4x4 matrix
    // 
    inline Matrix4d & setCol2( const Vector4d & col2 );

    // Set column 3 of a 4x4 matrix
    // 
    inline Matrix4d & setCol3( const Vector4d & col3 );

    // Get column 0 of a 4x4 matrix
    // 
    inline const Vector4d getCol0( ) const;

    // Get column 1 of a 4x4 matrix
    // 
    inline const Vector4d getCol1( ) const;

    // Get column 2 of a 4x4 matrix
    // 
    inline const Vector4d getCol2( ) const;

    // Get column 3 of a 4x4 matrix
    // 
    inline const Vector4d getCol3( ) const;

    // Set the column of a 4x4 matrix referred to by the specified index
    // 
    inline Matrix4d & setCol( int col, const Vector4d & vec );

    // Set the row of a 4x4 matrix referred to by the specified index
    // 
    inline Matrix4d & setRow( int row, const Vector4d & vec );

    // Get the column of a 4x4 matrix referred to by the specified index
    // 
    inline const Vector4d getCol( int col ) const;

    // Get the row of a 4x4 matrix referred to by the specified index
    // 
    inline const Vector4d getRow( int row ) const;

    // Subscripting operator to set or get a column
    // 
    inline Vector4d & operator []( int col );

    // Subscripting operator to get a column
    // 
    inline const Vector4d operator []( int col ) const;

    // Set the element of a 4x4 matrix referred to by column and row indices
    // 
    inline Matrix4d & setElem( int col, int row, double val );

    // Get the element of a 4x4 matrix referred to by column and row indices
    // 
    inline double getElem( int col, int row ) const;

    // Add two 4x4 matrices
    // 
    inline const Matrix4d operator +( const Matrix4d & mat ) const;

    // Subtract a 4x4 matrix from another 4x4 matrix
    // 
    inline const Matrix4d operator -( const Matrix4d & mat ) const;

    // Negate all elements of a 4x4 matrix
    // 
    inline const Matrix4d operator -( ) const;

    // Multiply a 4x4 matrix by a scalar
    // 
    inline const Matrix4d operator *( double scalar ) const;

    // Multiply a 4x4 matrix by a 4-D vector
    // 
    inline const Vector4d operator *( const Vector4d & vec ) const;

    // Multiply a 4x4 matrix by a 3-D vector
    // 
    inline const Vector4d operator *( const Vector3d & vec ) const;

    // Multiply a 4x4 matrix by a 3-D point
    // 
    inline const Vector4d operator *( const Point3d & pnt ) const;

    // Multiply two 4x4 matrices
    // 
    inline const Matrix4d operator *( const Matrix4d & mat ) const;

    // Multiply a 4x4 matrix by a 3x4 transformation matrix
    // 
    inline const Matrix4d operator *( const Transform3d & tfrm ) const;

    // Perform compound assignment and addition with a 4x4 matrix
    // 
    inline Matrix4d & operator +=( const Matrix4d & mat );

    // Perform compound assignment and subtraction by a 4x4 matrix
    // 
    inline Matrix4d & operator -=( const Matrix4d & mat );

    // Perform compound assignment and multiplication by a scalar
    // 
    inline Matrix4d & operator *=( double scalar );

    // Perform compound assignment and multiplication by a 4x4 matrix
    // 
    inline Matrix4d & operator *=( const Matrix4d & mat );

    // Perform compound assignment and multiplication by a 3x4 transformation matrix
    // 
    inline Matrix4d & operator *=( const Transform3d & tfrm );

    // Construct an identity 4x4 matrix
    // 
    static inline const Matrix4d identity( );

    // Construct a 4x4 matrix to rotate around the x axis
    // 
    static inline const Matrix4d rotationX( double radians );

    // Construct a 4x4 matrix to rotate around the y axis
    // 
    static inline const Matrix4d rotationY( double radians );

    // Construct a 4x4 matrix to rotate around the z axis
    // 
    static inline const Matrix4d rotationZ( double radians );

    // Construct a 4x4 matrix to rotate around the x, y, and z axes
    // 
    static inline const Matrix4d rotationZYX( const Vector3d & radiansXYZ );

    // Construct a 4x4 matrix to rotate around a unit-length 3-D vector
    // 
    static inline const Matrix4d rotation( double radians, const Vector3d & unitVec );

    // Construct a rotation matrix from a unit-length quaternion
    // 
    static inline const Matrix4d rotation( const Quatd & unitQuat );

    // Construct a 4x4 matrix to perform scaling
    // 
    static inline const Matrix4d scale( const Vector3d & scaleVec );

    // Construct a 4x4 matrix to perform translation
    // 
    static inline const Matrix4d translation( const Vector3d & translateVec );

    // Construct viewing matrix based on eye position, position looked at, and up direction
    // 
    static inline const Matrix4d lookAt( const Point3d & eyePos, const Point3d & lookAtPos, const Vector3d & upVec );

    // Construct a perspective projection matrix
    // 
    static inline const Matrix4d perspective( double fovyRadians, double aspect, double zNear, double zFar );

    // Construct a perspective projection matrix based on frustum
    // 
    static inline const Matrix4d frustum( double left, double right, double bottom, double top, double zNear, double zFar );

    // Construct an orthographic projection matrix
    // 
    static inline const Matrix4d orthographic( double left, double right, double bottom, double top, double zNear, double zFar );

};
// Multiply a 4x4 matrix by a scalar
// 
inline const Matrix4d operator *( double scalar, const Matrix4d & mat );

// Append (post-multiply) a scale transformation to a 4x4 matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Matrix4d appendScale( const Matrix4d & mat, const Vector3d & scaleVec );

// Prepend (pre-multiply) a scale transformation to a 4x4 matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Matrix4d prependScale( const Vector3d & scaleVec, const Matrix4d & mat );

// Multiply two 4x4 matrices per element
// 
inline const Matrix4d mulPerElem( const Matrix4d & mat0, const Matrix4d & mat1 );

// Compute the absolute value of a 4x4 matrix per element
// 
inline const Matrix4d absPerElem( const Matrix4d & mat );

// Transpose of a 4x4 matrix
// 
inline const Matrix4d transpose( const Matrix4d & mat );

// Compute the inverse of a 4x4 matrix
// NOTE: 
// Result is unpredictable when the determinant of mat is equal to or near 0.
// 
inline const Matrix4d inverse( const Matrix4d & mat );

// Compute the inverse of a 4x4 matrix, which is expected to be an affine matrix
// NOTE: 
// This can be used to achieve better performance than a general inverse when the specified 4x4 matrix meets the given restrictions.  The result is unpredictable when the determinant of mat is equal to or near 0.
// 
inline const Matrix4d affineInverse( const Matrix4d & mat );

// Compute the inverse of a 4x4 matrix, which is expected to be an affine matrix with an orthogonal upper-left 3x3 submatrix
// NOTE: 
// This can be used to achieve better performance than a general inverse when the specified 4x4 matrix meets the given restrictions.
// 
inline const Matrix4d orthoInverse( const Matrix4d & mat );

// Determinant of a 4x4 matrix
// 
inline double determinant( const Matrix4d & mat );

// Conditionally select between two 4x4 matrices
// 
inline const Matrix4d select( const Matrix4d & mat0, const Matrix4d & mat1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 4x4 matrix
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Matrix4d & mat );

// Print a 4x4 matrix and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Matrix4d & mat, const char * name );

#endif

// A 3x4 transformation matrix in array-of-structures format
//
class Transform3d
{
    Vector3d mCol0;
    Vector3d mCol1;
    Vector3d mCol2;
    Vector3d mCol3;

public:
    // Default constructor; does no initialization
    // 
    inline Transform3d( ) { };

    // Copy a 3x4 transformation matrix
    // 
    inline Transform3d( const Transform3d & tfrm );

    // Construct a 3x4 transformation matrix containing the specified columns
    // 
    inline Transform3d( const Vector3d & col0, const Vector3d & col1, const Vector3d & col2, const Vector3d & col3 );

    // Construct a 3x4 transformation matrix from a 3x3 matrix and a 3-D vector
    // 
    inline Transform3d( const Matrix3d & tfrm, const Vector3d & translateVec );

    // Construct a 3x4 transformation matrix from a unit-length quaternion and a 3-D vector
    // 
    inline Transform3d( const Quatd & unitQuat, const Vector3d & translateVec );

    // Set all elements of a 3x4 transformation matrix to the same scalar value
    // 
    explicit inline Transform3d( double scalar );

    // Assign one 3x4 transformation matrix to another
    // 
    inline Transform3d & operator =( const Transform3d & tfrm );

    // Set the upper-left 3x3 submatrix
    // 
    inline Transform3d & setUpper3x3( const Matrix3d & mat3 );

    // Get the upper-left 3x3 submatrix of a 3x4 transformation matrix
    // 
    inline const Matrix3d getUpper3x3( ) const;

    // Set translation component
    // 
    inline Transform3d & setTranslation( const Vector3d & translateVec );

    // Get the translation component of a 3x4 transformation matrix
    // 
    inline const Vector3d getTranslation( ) const;

    // Set column 0 of a 3x4 transformation matrix
    // 
    inline Transform3d & setCol0( const Vector3d & col0 );

    // Set column 1 of a 3x4 transformation matrix
    // 
    inline Transform3d & setCol1( const Vector3d & col1 );

    // Set column 2 of a 3x4 transformation matrix
    // 
    inline Transform3d & setCol2( const Vector3d & col2 );

    // Set column 3 of a 3x4 transformation matrix
    // 
    inline Transform3d & setCol3( const Vector3d & col3 );

    // Get column 0 of a 3x4 transformation matrix
    // 
    inline const Vector3d getCol0( ) const;

    // Get column 1 of a 3x4 transformation matrix
    // 
    inline const Vector3d getCol1( ) const;

    // Get column 2 of a 3x4 transformation matrix
    // 
    inline const Vector3d getCol2( ) const;

    // Get column 3 of a 3x4 transformation matrix
    // 
    inline const Vector3d getCol3( ) const;

    // Set the column of a 3x4 transformation matrix referred to by the specified index
    // 
    inline Transform3d & setCol( int col, const Vector3d & vec );

    // Set the row of a 3x4 transformation matrix referred to by the specified index
    // 
    inline Transform3d & setRow( int row, const Vector4d & vec );

    // Get the column of a 3x4 transformation matrix referred to by the specified index
    // 
    inline const Vector3d getCol( int col ) const;

    // Get the row of a 3x4 transformation matrix referred to by the specified index
    // 
    inline const Vector4d getRow( int row ) const;

    // Subscripting operator to set or get a column
    // 
    inline Vector3d & operator []( int col );

    // Subscripting operator to get a column
    // 
    inline const Vector3d operator []( int col ) const;

    // Set the element of a 3x4 transformation matrix referred to by column and row indices
    // 
    inline Transform3d & setElem( int col, int row, double val );

    // Get the element of a 3x4 transformation matrix referred to by column and row indices
    // 
    inline double getElem( int col, int row ) const;

    // Multiply a 3x4 transformation matrix by a 3-D vector
    // 
    inline const Vector3d operator *( const Vector3d & vec ) const;

    // Multiply a 3x4 transformation matrix by a 3-D point
    // 
    inline const Point3d operator *( const Point3d & pnt ) const;

    // Multiply two 3x4 transformation matrices
    // 
    inline const Transform3d operator *( const Transform3d & tfrm ) const;

    // Perform compound assignment and multiplication by a 3x4 transformation matrix
    // 
    inline Transform3d & operator *=( const Transform3d & tfrm );

    // Construct an identity 3x4 transformation matrix
    // 
    static inline const Transform3d identity( );

    // Construct a 3x4 transformation matrix to rotate around the x axis
    // 
    static inline const Transform3d rotationX( double radians );

    // Construct a 3x4 transformation matrix to rotate around the y axis
    // 
    static inline const Transform3d rotationY( double radians );

    // Construct a 3x4 transformation matrix to rotate around the z axis
    // 
    static inline const Transform3d rotationZ( double radians );

    // Construct a 3x4 transformation matrix to rotate around the x, y, and z axes
    // 
    static inline const Transform3d rotationZYX( const Vector3d & radiansXYZ );

    // Construct a 3x4 transformation matrix to rotate around a unit-length 3-D vector
    // 
    static inline const Transform3d rotation( double radians, const Vector3d & unitVec );

    // Construct a rotation matrix from a unit-length quaternion
    // 
    static inline const Transform3d rotation( const Quatd & unitQuat );

    // Construct a 3x4 transformation matrix to perform scaling
    // 
    static inline const Transform3d scale( const Vector3d & scaleVec );

    // Construct a 3x4 transformation matrix to perform translation
    // 
    static inline const Transform3d translation( const Vector3d & translateVec );

};
// Append (post-multiply) a scale transformation to a 3x4 transformation matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Transform3d appendScale( const Transform3d & tfrm, const Vector3d & scaleVec );

// Prepend (pre-multiply) a scale transformation to a 3x4 transformation matrix
// NOTE: 
// Faster than creating and multiplying a scale transformation matrix.
// 
inline const Transform3d prependScale( const Vector3d & scaleVec, const Transform3d & tfrm );

// Multiply two 3x4 transformation matrices per element
// 
inline const Transform3d mulPerElem( const Transform3d & tfrm0, const Transform3d & tfrm1 );

// Compute the absolute value of a 3x4 transformation matrix per element
// 
inline const Transform3d absPerElem( const Transform3d & tfrm );

// Inverse of a 3x4 transformation matrix
// NOTE: 
// Result is unpredictable when the determinant of the left 3x3 submatrix is equal to or near 0.
// 
inline const Transform3d inverse( const Transform3d & tfrm );

// Compute the inverse of a 3x4 transformation matrix, expected to have an orthogonal upper-left 3x3 submatrix
// NOTE: 
// This can be used to achieve better performance than a general inverse when the specified 3x4 transformation matrix meets the given restrictions.
// 
inline const Transform3d orthoInverse( const Transform3d & tfrm );

// Conditionally select between two 3x4 transformation matrices
// 
inline const Transform3d select( const Transform3d & tfrm0, const Transform3d & tfrm1, bool select1 );

#ifdef _VECTORMATH_DEBUG

// Print a 3x4 transformation matrix
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Transform3d & tfrm );

// Print a 3x4 transformation matrix and an associated string identifier
// NOTE: 
// Function is only defined when _VECTORMATH_DEBUG is defined.
// 
inline void print( const Transform3d & tfrm, const char * name );

#endif

} // namespace Aos
} // namespace Vectormath

#include "vec_aos_d.h"
#include "quat_aos_d.h"
#include "mat_aos_d.h"

#endif
