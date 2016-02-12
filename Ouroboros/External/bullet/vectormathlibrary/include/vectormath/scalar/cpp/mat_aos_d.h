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

#ifndef _VECTORMATH_MAT_AOS_D_CPP_H
#define _VECTORMATH_MAT_AOS_D_CPP_H

namespace Vectormath {
namespace Aos {

//-----------------------------------------------------------------------------
// Constants

#define _VECTORMATH_PI_OVER_2_DOUBLE 1.5707963267948966192313216916398

//-----------------------------------------------------------------------------
// Definitions

inline Matrix3d::Matrix3d( const Matrix3d & mat )
{
    mCol0 = mat.mCol0;
    mCol1 = mat.mCol1;
    mCol2 = mat.mCol2;
}

inline Matrix3d::Matrix3d( double scalar )
{
    mCol0 = Vector3d( scalar );
    mCol1 = Vector3d( scalar );
    mCol2 = Vector3d( scalar );
}

inline Matrix3d::Matrix3d( const Quatd & unitQuat )
{
    double qx, qy, qz, qw, qx2, qy2, qz2, qxqx2, qyqy2, qzqz2, qxqy2, qyqz2, qzqw2, qxqz2, qyqw2, qxqw2;
    qx = unitQuat.getX();
    qy = unitQuat.getY();
    qz = unitQuat.getZ();
    qw = unitQuat.getW();
    qx2 = ( qx + qx );
    qy2 = ( qy + qy );
    qz2 = ( qz + qz );
    qxqx2 = ( qx * qx2 );
    qxqy2 = ( qx * qy2 );
    qxqz2 = ( qx * qz2 );
    qxqw2 = ( qw * qx2 );
    qyqy2 = ( qy * qy2 );
    qyqz2 = ( qy * qz2 );
    qyqw2 = ( qw * qy2 );
    qzqz2 = ( qz * qz2 );
    qzqw2 = ( qw * qz2 );
    mCol0 = Vector3d( ( ( 1.0 - qyqy2 ) - qzqz2 ), ( qxqy2 + qzqw2 ), ( qxqz2 - qyqw2 ) );
    mCol1 = Vector3d( ( qxqy2 - qzqw2 ), ( ( 1.0 - qxqx2 ) - qzqz2 ), ( qyqz2 + qxqw2 ) );
    mCol2 = Vector3d( ( qxqz2 + qyqw2 ), ( qyqz2 - qxqw2 ), ( ( 1.0 - qxqx2 ) - qyqy2 ) );
}

inline Matrix3d::Matrix3d( const Vector3d & _col0, const Vector3d & _col1, const Vector3d & _col2 )
{
    mCol0 = _col0;
    mCol1 = _col1;
    mCol2 = _col2;
}

inline Matrix3d & Matrix3d::setCol0( const Vector3d & _col0 )
{
    mCol0 = _col0;
    return *this;
}

inline Matrix3d & Matrix3d::setCol1( const Vector3d & _col1 )
{
    mCol1 = _col1;
    return *this;
}

inline Matrix3d & Matrix3d::setCol2( const Vector3d & _col2 )
{
    mCol2 = _col2;
    return *this;
}

inline Matrix3d & Matrix3d::setCol( int col, const Vector3d & vec )
{
    *(&mCol0 + col) = vec;
    return *this;
}

inline Matrix3d & Matrix3d::setRow( int row, const Vector3d & vec )
{
    mCol0.setElem( row, vec.getElem( 0 ) );
    mCol1.setElem( row, vec.getElem( 1 ) );
    mCol2.setElem( row, vec.getElem( 2 ) );
    return *this;
}

inline Matrix3d & Matrix3d::setElem( int col, int row, double val )
{
    Vector3d tmpV3_0;
    tmpV3_0 = this->getCol( col );
    tmpV3_0.setElem( row, val );
    this->setCol( col, tmpV3_0 );
    return *this;
}

inline double Matrix3d::getElem( int col, int row ) const
{
    return this->getCol( col ).getElem( row );
}

inline const Vector3d Matrix3d::getCol0( ) const
{
    return mCol0;
}

inline const Vector3d Matrix3d::getCol1( ) const
{
    return mCol1;
}

inline const Vector3d Matrix3d::getCol2( ) const
{
    return mCol2;
}

inline const Vector3d Matrix3d::getCol( int col ) const
{
    return *(&mCol0 + col);
}

inline const Vector3d Matrix3d::getRow( int row ) const
{
    return Vector3d( mCol0.getElem( row ), mCol1.getElem( row ), mCol2.getElem( row ) );
}

inline Vector3d & Matrix3d::operator []( int col )
{
    return *(&mCol0 + col);
}

inline const Vector3d Matrix3d::operator []( int col ) const
{
    return *(&mCol0 + col);
}

inline Matrix3d & Matrix3d::operator =( const Matrix3d & mat )
{
    mCol0 = mat.mCol0;
    mCol1 = mat.mCol1;
    mCol2 = mat.mCol2;
    return *this;
}

inline const Matrix3d transpose( const Matrix3d & mat )
{
    return Matrix3d(
        Vector3d( mat.getCol0().getX(), mat.getCol1().getX(), mat.getCol2().getX() ),
        Vector3d( mat.getCol0().getY(), mat.getCol1().getY(), mat.getCol2().getY() ),
        Vector3d( mat.getCol0().getZ(), mat.getCol1().getZ(), mat.getCol2().getZ() )
    );
}

inline const Matrix3d inverse( const Matrix3d & mat )
{
    Vector3d tmp0, tmp1, tmp2;
    double detinv;
    tmp0 = cross( mat.getCol1(), mat.getCol2() );
    tmp1 = cross( mat.getCol2(), mat.getCol0() );
    tmp2 = cross( mat.getCol0(), mat.getCol1() );
    detinv = ( 1.0 / dot( mat.getCol2(), tmp2 ) );
    return Matrix3d(
        Vector3d( ( tmp0.getX() * detinv ), ( tmp1.getX() * detinv ), ( tmp2.getX() * detinv ) ),
        Vector3d( ( tmp0.getY() * detinv ), ( tmp1.getY() * detinv ), ( tmp2.getY() * detinv ) ),
        Vector3d( ( tmp0.getZ() * detinv ), ( tmp1.getZ() * detinv ), ( tmp2.getZ() * detinv ) )
    );
}

inline double determinant( const Matrix3d & mat )
{
    return dot( mat.getCol2(), cross( mat.getCol0(), mat.getCol1() ) );
}

inline const Matrix3d Matrix3d::operator +( const Matrix3d & mat ) const
{
    return Matrix3d(
        ( mCol0 + mat.mCol0 ),
        ( mCol1 + mat.mCol1 ),
        ( mCol2 + mat.mCol2 )
    );
}

inline const Matrix3d Matrix3d::operator -( const Matrix3d & mat ) const
{
    return Matrix3d(
        ( mCol0 - mat.mCol0 ),
        ( mCol1 - mat.mCol1 ),
        ( mCol2 - mat.mCol2 )
    );
}

inline Matrix3d & Matrix3d::operator +=( const Matrix3d & mat )
{
    *this = *this + mat;
    return *this;
}

inline Matrix3d & Matrix3d::operator -=( const Matrix3d & mat )
{
    *this = *this - mat;
    return *this;
}

inline const Matrix3d Matrix3d::operator -( ) const
{
    return Matrix3d(
        ( -mCol0 ),
        ( -mCol1 ),
        ( -mCol2 )
    );
}

inline const Matrix3d absPerElem( const Matrix3d & mat )
{
    return Matrix3d(
        absPerElem( mat.getCol0() ),
        absPerElem( mat.getCol1() ),
        absPerElem( mat.getCol2() )
    );
}

inline const Matrix3d Matrix3d::operator *( double scalar ) const
{
    return Matrix3d(
        ( mCol0 * scalar ),
        ( mCol1 * scalar ),
        ( mCol2 * scalar )
    );
}

inline Matrix3d & Matrix3d::operator *=( double scalar )
{
    *this = *this * scalar;
    return *this;
}

inline const Matrix3d operator *( double scalar, const Matrix3d & mat )
{
    return mat * scalar;
}

inline const Vector3d Matrix3d::operator *( const Vector3d & vec ) const
{
    return Vector3d(
        ( ( ( mCol0.getX() * vec.getX() ) + ( mCol1.getX() * vec.getY() ) ) + ( mCol2.getX() * vec.getZ() ) ),
        ( ( ( mCol0.getY() * vec.getX() ) + ( mCol1.getY() * vec.getY() ) ) + ( mCol2.getY() * vec.getZ() ) ),
        ( ( ( mCol0.getZ() * vec.getX() ) + ( mCol1.getZ() * vec.getY() ) ) + ( mCol2.getZ() * vec.getZ() ) )
    );
}

inline const Matrix3d Matrix3d::operator *( const Matrix3d & mat ) const
{
    return Matrix3d(
        ( *this * mat.mCol0 ),
        ( *this * mat.mCol1 ),
        ( *this * mat.mCol2 )
    );
}

inline Matrix3d & Matrix3d::operator *=( const Matrix3d & mat )
{
    *this = *this * mat;
    return *this;
}

inline const Matrix3d mulPerElem( const Matrix3d & mat0, const Matrix3d & mat1 )
{
    return Matrix3d(
        mulPerElem( mat0.getCol0(), mat1.getCol0() ),
        mulPerElem( mat0.getCol1(), mat1.getCol1() ),
        mulPerElem( mat0.getCol2(), mat1.getCol2() )
    );
}

inline const Matrix3d Matrix3d::identity( )
{
    return Matrix3d(
        Vector3d::xAxis( ),
        Vector3d::yAxis( ),
        Vector3d::zAxis( )
    );
}

inline const Matrix3d Matrix3d::rotationX( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix3d(
        Vector3d::xAxis( ),
        Vector3d( 0.0, c, s ),
        Vector3d( 0.0, -s, c )
    );
}

inline const Matrix3d Matrix3d::rotationY( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix3d(
        Vector3d( c, 0.0, -s ),
        Vector3d::yAxis( ),
        Vector3d( s, 0.0, c )
    );
}

inline const Matrix3d Matrix3d::rotationZ( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix3d(
        Vector3d( c, s, 0.0 ),
        Vector3d( -s, c, 0.0 ),
        Vector3d::zAxis( )
    );
}

inline const Matrix3d Matrix3d::rotationZYX( const Vector3d & radiansXYZ )
{
    double sX, cX, sY, cY, sZ, cZ, tmp0, tmp1;
    sX = sin( radiansXYZ.getX() );
    cX = cos( radiansXYZ.getX() );
    sY = sin( radiansXYZ.getY() );
    cY = cos( radiansXYZ.getY() );
    sZ = sin( radiansXYZ.getZ() );
    cZ = cos( radiansXYZ.getZ() );
    tmp0 = ( cZ * sY );
    tmp1 = ( sZ * sY );
    return Matrix3d(
        Vector3d( ( cZ * cY ), ( sZ * cY ), -sY ),
        Vector3d( ( ( tmp0 * sX ) - ( sZ * cX ) ), ( ( tmp1 * sX ) + ( cZ * cX ) ), ( cY * sX ) ),
        Vector3d( ( ( tmp0 * cX ) + ( sZ * sX ) ), ( ( tmp1 * cX ) - ( cZ * sX ) ), ( cY * cX ) )
    );
}

inline const Matrix3d Matrix3d::rotation( double radians, const Vector3d & unitVec )
{
    double x, y, z, s, c, oneMinusC, xy, yz, zx;
    s = sin( radians );
    c = cos( radians );
    x = unitVec.getX();
    y = unitVec.getY();
    z = unitVec.getZ();
    xy = ( x * y );
    yz = ( y * z );
    zx = ( z * x );
    oneMinusC = ( 1.0 - c );
    return Matrix3d(
        Vector3d( ( ( ( x * x ) * oneMinusC ) + c ), ( ( xy * oneMinusC ) + ( z * s ) ), ( ( zx * oneMinusC ) - ( y * s ) ) ),
        Vector3d( ( ( xy * oneMinusC ) - ( z * s ) ), ( ( ( y * y ) * oneMinusC ) + c ), ( ( yz * oneMinusC ) + ( x * s ) ) ),
        Vector3d( ( ( zx * oneMinusC ) + ( y * s ) ), ( ( yz * oneMinusC ) - ( x * s ) ), ( ( ( z * z ) * oneMinusC ) + c ) )
    );
}

inline const Matrix3d Matrix3d::rotation( const Quatd & unitQuat )
{
    return Matrix3d( unitQuat );
}

inline const Matrix3d Matrix3d::scale( const Vector3d & scaleVec )
{
    return Matrix3d(
        Vector3d( scaleVec.getX(), 0.0, 0.0 ),
        Vector3d( 0.0, scaleVec.getY(), 0.0 ),
        Vector3d( 0.0, 0.0, scaleVec.getZ() )
    );
}

inline const Matrix3d appendScale( const Matrix3d & mat, const Vector3d & scaleVec )
{
    return Matrix3d(
        ( mat.getCol0() * scaleVec.getX( ) ),
        ( mat.getCol1() * scaleVec.getY( ) ),
        ( mat.getCol2() * scaleVec.getZ( ) )
    );
}

inline const Matrix3d prependScale( const Vector3d & scaleVec, const Matrix3d & mat )
{
    return Matrix3d(
        mulPerElem( mat.getCol0(), scaleVec ),
        mulPerElem( mat.getCol1(), scaleVec ),
        mulPerElem( mat.getCol2(), scaleVec )
    );
}

inline const Matrix3d select( const Matrix3d & mat0, const Matrix3d & mat1, bool select1 )
{
    return Matrix3d(
        select( mat0.getCol0(), mat1.getCol0(), select1 ),
        select( mat0.getCol1(), mat1.getCol1(), select1 ),
        select( mat0.getCol2(), mat1.getCol2(), select1 )
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Matrix3d & mat )
{
    print( mat.getRow( 0 ) );
    print( mat.getRow( 1 ) );
    print( mat.getRow( 2 ) );
}

inline void print( const Matrix3d & mat, const char * name )
{
    printf("%s:\n", name);
    print( mat );
}

#endif

inline Matrix4d::Matrix4d( const Matrix4d & mat )
{
    mCol0 = mat.mCol0;
    mCol1 = mat.mCol1;
    mCol2 = mat.mCol2;
    mCol3 = mat.mCol3;
}

inline Matrix4d::Matrix4d( double scalar )
{
    mCol0 = Vector4d( scalar );
    mCol1 = Vector4d( scalar );
    mCol2 = Vector4d( scalar );
    mCol3 = Vector4d( scalar );
}

inline Matrix4d::Matrix4d( const Transform3d & mat )
{
    mCol0 = Vector4d( mat.getCol0(), 0.0 );
    mCol1 = Vector4d( mat.getCol1(), 0.0 );
    mCol2 = Vector4d( mat.getCol2(), 0.0 );
    mCol3 = Vector4d( mat.getCol3(), 1.0 );
}

inline Matrix4d::Matrix4d( const Vector4d & _col0, const Vector4d & _col1, const Vector4d & _col2, const Vector4d & _col3 )
{
    mCol0 = _col0;
    mCol1 = _col1;
    mCol2 = _col2;
    mCol3 = _col3;
}

inline Matrix4d::Matrix4d( const Matrix3d & mat, const Vector3d & translateVec )
{
    mCol0 = Vector4d( mat.getCol0(), 0.0 );
    mCol1 = Vector4d( mat.getCol1(), 0.0 );
    mCol2 = Vector4d( mat.getCol2(), 0.0 );
    mCol3 = Vector4d( translateVec, 1.0 );
}

inline Matrix4d::Matrix4d( const Quatd & unitQuat, const Vector3d & translateVec )
{
    Matrix3d mat;
    mat = Matrix3d( unitQuat );
    mCol0 = Vector4d( mat.getCol0(), 0.0 );
    mCol1 = Vector4d( mat.getCol1(), 0.0 );
    mCol2 = Vector4d( mat.getCol2(), 0.0 );
    mCol3 = Vector4d( translateVec, 1.0 );
}

inline Matrix4d & Matrix4d::setCol0( const Vector4d & _col0 )
{
    mCol0 = _col0;
    return *this;
}

inline Matrix4d & Matrix4d::setCol1( const Vector4d & _col1 )
{
    mCol1 = _col1;
    return *this;
}

inline Matrix4d & Matrix4d::setCol2( const Vector4d & _col2 )
{
    mCol2 = _col2;
    return *this;
}

inline Matrix4d & Matrix4d::setCol3( const Vector4d & _col3 )
{
    mCol3 = _col3;
    return *this;
}

inline Matrix4d & Matrix4d::setCol( int col, const Vector4d & vec )
{
    *(&mCol0 + col) = vec;
    return *this;
}

inline Matrix4d & Matrix4d::setRow( int row, const Vector4d & vec )
{
    mCol0.setElem( row, vec.getElem( 0 ) );
    mCol1.setElem( row, vec.getElem( 1 ) );
    mCol2.setElem( row, vec.getElem( 2 ) );
    mCol3.setElem( row, vec.getElem( 3 ) );
    return *this;
}

inline Matrix4d & Matrix4d::setElem( int col, int row, double val )
{
    Vector4d tmpV3_0;
    tmpV3_0 = this->getCol( col );
    tmpV3_0.setElem( row, val );
    this->setCol( col, tmpV3_0 );
    return *this;
}

inline double Matrix4d::getElem( int col, int row ) const
{
    return this->getCol( col ).getElem( row );
}

inline const Vector4d Matrix4d::getCol0( ) const
{
    return mCol0;
}

inline const Vector4d Matrix4d::getCol1( ) const
{
    return mCol1;
}

inline const Vector4d Matrix4d::getCol2( ) const
{
    return mCol2;
}

inline const Vector4d Matrix4d::getCol3( ) const
{
    return mCol3;
}

inline const Vector4d Matrix4d::getCol( int col ) const
{
    return *(&mCol0 + col);
}

inline const Vector4d Matrix4d::getRow( int row ) const
{
    return Vector4d( mCol0.getElem( row ), mCol1.getElem( row ), mCol2.getElem( row ), mCol3.getElem( row ) );
}

inline Vector4d & Matrix4d::operator []( int col )
{
    return *(&mCol0 + col);
}

inline const Vector4d Matrix4d::operator []( int col ) const
{
    return *(&mCol0 + col);
}

inline Matrix4d & Matrix4d::operator =( const Matrix4d & mat )
{
    mCol0 = mat.mCol0;
    mCol1 = mat.mCol1;
    mCol2 = mat.mCol2;
    mCol3 = mat.mCol3;
    return *this;
}

inline const Matrix4d transpose( const Matrix4d & mat )
{
    return Matrix4d(
        Vector4d( mat.getCol0().getX(), mat.getCol1().getX(), mat.getCol2().getX(), mat.getCol3().getX() ),
        Vector4d( mat.getCol0().getY(), mat.getCol1().getY(), mat.getCol2().getY(), mat.getCol3().getY() ),
        Vector4d( mat.getCol0().getZ(), mat.getCol1().getZ(), mat.getCol2().getZ(), mat.getCol3().getZ() ),
        Vector4d( mat.getCol0().getW(), mat.getCol1().getW(), mat.getCol2().getW(), mat.getCol3().getW() )
    );
}

inline const Matrix4d inverse( const Matrix4d & mat )
{
    Vector4d res0, res1, res2, res3;
    double mA, mB, mC, mD, mE, mF, mG, mH, mI, mJ, mK, mL, mM, mN, mO, mP, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, detInv;
    mA = mat.getCol0().getX();
    mB = mat.getCol0().getY();
    mC = mat.getCol0().getZ();
    mD = mat.getCol0().getW();
    mE = mat.getCol1().getX();
    mF = mat.getCol1().getY();
    mG = mat.getCol1().getZ();
    mH = mat.getCol1().getW();
    mI = mat.getCol2().getX();
    mJ = mat.getCol2().getY();
    mK = mat.getCol2().getZ();
    mL = mat.getCol2().getW();
    mM = mat.getCol3().getX();
    mN = mat.getCol3().getY();
    mO = mat.getCol3().getZ();
    mP = mat.getCol3().getW();
    tmp0 = ( ( mK * mD ) - ( mC * mL ) );
    tmp1 = ( ( mO * mH ) - ( mG * mP ) );
    tmp2 = ( ( mB * mK ) - ( mJ * mC ) );
    tmp3 = ( ( mF * mO ) - ( mN * mG ) );
    tmp4 = ( ( mJ * mD ) - ( mB * mL ) );
    tmp5 = ( ( mN * mH ) - ( mF * mP ) );
    res0.setX( ( ( ( mJ * tmp1 ) - ( mL * tmp3 ) ) - ( mK * tmp5 ) ) );
    res0.setY( ( ( ( mN * tmp0 ) - ( mP * tmp2 ) ) - ( mO * tmp4 ) ) );
    res0.setZ( ( ( ( mD * tmp3 ) + ( mC * tmp5 ) ) - ( mB * tmp1 ) ) );
    res0.setW( ( ( ( mH * tmp2 ) + ( mG * tmp4 ) ) - ( mF * tmp0 ) ) );
    detInv = ( 1.0 / ( ( ( ( mA * res0.getX() ) + ( mE * res0.getY() ) ) + ( mI * res0.getZ() ) ) + ( mM * res0.getW() ) ) );
    res1.setX( ( mI * tmp1 ) );
    res1.setY( ( mM * tmp0 ) );
    res1.setZ( ( mA * tmp1 ) );
    res1.setW( ( mE * tmp0 ) );
    res3.setX( ( mI * tmp3 ) );
    res3.setY( ( mM * tmp2 ) );
    res3.setZ( ( mA * tmp3 ) );
    res3.setW( ( mE * tmp2 ) );
    res2.setX( ( mI * tmp5 ) );
    res2.setY( ( mM * tmp4 ) );
    res2.setZ( ( mA * tmp5 ) );
    res2.setW( ( mE * tmp4 ) );
    tmp0 = ( ( mI * mB ) - ( mA * mJ ) );
    tmp1 = ( ( mM * mF ) - ( mE * mN ) );
    tmp2 = ( ( mI * mD ) - ( mA * mL ) );
    tmp3 = ( ( mM * mH ) - ( mE * mP ) );
    tmp4 = ( ( mI * mC ) - ( mA * mK ) );
    tmp5 = ( ( mM * mG ) - ( mE * mO ) );
    res2.setX( ( ( ( mL * tmp1 ) - ( mJ * tmp3 ) ) + res2.getX() ) );
    res2.setY( ( ( ( mP * tmp0 ) - ( mN * tmp2 ) ) + res2.getY() ) );
    res2.setZ( ( ( ( mB * tmp3 ) - ( mD * tmp1 ) ) - res2.getZ() ) );
    res2.setW( ( ( ( mF * tmp2 ) - ( mH * tmp0 ) ) - res2.getW() ) );
    res3.setX( ( ( ( mJ * tmp5 ) - ( mK * tmp1 ) ) + res3.getX() ) );
    res3.setY( ( ( ( mN * tmp4 ) - ( mO * tmp0 ) ) + res3.getY() ) );
    res3.setZ( ( ( ( mC * tmp1 ) - ( mB * tmp5 ) ) - res3.getZ() ) );
    res3.setW( ( ( ( mG * tmp0 ) - ( mF * tmp4 ) ) - res3.getW() ) );
    res1.setX( ( ( ( mK * tmp3 ) - ( mL * tmp5 ) ) - res1.getX() ) );
    res1.setY( ( ( ( mO * tmp2 ) - ( mP * tmp4 ) ) - res1.getY() ) );
    res1.setZ( ( ( ( mD * tmp5 ) - ( mC * tmp3 ) ) + res1.getZ() ) );
    res1.setW( ( ( ( mH * tmp4 ) - ( mG * tmp2 ) ) + res1.getW() ) );
    return Matrix4d(
        ( res0 * detInv ),
        ( res1 * detInv ),
        ( res2 * detInv ),
        ( res3 * detInv )
    );
}

inline const Matrix4d affineInverse( const Matrix4d & mat )
{
    Transform3d affineMat;
    affineMat.setCol0( mat.getCol0().getXYZ( ) );
    affineMat.setCol1( mat.getCol1().getXYZ( ) );
    affineMat.setCol2( mat.getCol2().getXYZ( ) );
    affineMat.setCol3( mat.getCol3().getXYZ( ) );
    return Matrix4d( inverse( affineMat ) );
}

inline const Matrix4d orthoInverse( const Matrix4d & mat )
{
    Transform3d affineMat;
    affineMat.setCol0( mat.getCol0().getXYZ( ) );
    affineMat.setCol1( mat.getCol1().getXYZ( ) );
    affineMat.setCol2( mat.getCol2().getXYZ( ) );
    affineMat.setCol3( mat.getCol3().getXYZ( ) );
    return Matrix4d( orthoInverse( affineMat ) );
}

inline double determinant( const Matrix4d & mat )
{
    double dx, dy, dz, dw, mA, mB, mC, mD, mE, mF, mG, mH, mI, mJ, mK, mL, mM, mN, mO, mP, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
    mA = mat.getCol0().getX();
    mB = mat.getCol0().getY();
    mC = mat.getCol0().getZ();
    mD = mat.getCol0().getW();
    mE = mat.getCol1().getX();
    mF = mat.getCol1().getY();
    mG = mat.getCol1().getZ();
    mH = mat.getCol1().getW();
    mI = mat.getCol2().getX();
    mJ = mat.getCol2().getY();
    mK = mat.getCol2().getZ();
    mL = mat.getCol2().getW();
    mM = mat.getCol3().getX();
    mN = mat.getCol3().getY();
    mO = mat.getCol3().getZ();
    mP = mat.getCol3().getW();
    tmp0 = ( ( mK * mD ) - ( mC * mL ) );
    tmp1 = ( ( mO * mH ) - ( mG * mP ) );
    tmp2 = ( ( mB * mK ) - ( mJ * mC ) );
    tmp3 = ( ( mF * mO ) - ( mN * mG ) );
    tmp4 = ( ( mJ * mD ) - ( mB * mL ) );
    tmp5 = ( ( mN * mH ) - ( mF * mP ) );
    dx = ( ( ( mJ * tmp1 ) - ( mL * tmp3 ) ) - ( mK * tmp5 ) );
    dy = ( ( ( mN * tmp0 ) - ( mP * tmp2 ) ) - ( mO * tmp4 ) );
    dz = ( ( ( mD * tmp3 ) + ( mC * tmp5 ) ) - ( mB * tmp1 ) );
    dw = ( ( ( mH * tmp2 ) + ( mG * tmp4 ) ) - ( mF * tmp0 ) );
    return ( ( ( ( mA * dx ) + ( mE * dy ) ) + ( mI * dz ) ) + ( mM * dw ) );
}

inline const Matrix4d Matrix4d::operator +( const Matrix4d & mat ) const
{
    return Matrix4d(
        ( mCol0 + mat.mCol0 ),
        ( mCol1 + mat.mCol1 ),
        ( mCol2 + mat.mCol2 ),
        ( mCol3 + mat.mCol3 )
    );
}

inline const Matrix4d Matrix4d::operator -( const Matrix4d & mat ) const
{
    return Matrix4d(
        ( mCol0 - mat.mCol0 ),
        ( mCol1 - mat.mCol1 ),
        ( mCol2 - mat.mCol2 ),
        ( mCol3 - mat.mCol3 )
    );
}

inline Matrix4d & Matrix4d::operator +=( const Matrix4d & mat )
{
    *this = *this + mat;
    return *this;
}

inline Matrix4d & Matrix4d::operator -=( const Matrix4d & mat )
{
    *this = *this - mat;
    return *this;
}

inline const Matrix4d Matrix4d::operator -( ) const
{
    return Matrix4d(
        ( -mCol0 ),
        ( -mCol1 ),
        ( -mCol2 ),
        ( -mCol3 )
    );
}

inline const Matrix4d absPerElem( const Matrix4d & mat )
{
    return Matrix4d(
        absPerElem( mat.getCol0() ),
        absPerElem( mat.getCol1() ),
        absPerElem( mat.getCol2() ),
        absPerElem( mat.getCol3() )
    );
}

inline const Matrix4d Matrix4d::operator *( double scalar ) const
{
    return Matrix4d(
        ( mCol0 * scalar ),
        ( mCol1 * scalar ),
        ( mCol2 * scalar ),
        ( mCol3 * scalar )
    );
}

inline Matrix4d & Matrix4d::operator *=( double scalar )
{
    *this = *this * scalar;
    return *this;
}

inline const Matrix4d operator *( double scalar, const Matrix4d & mat )
{
    return mat * scalar;
}

inline const Vector4d Matrix4d::operator *( const Vector4d & vec ) const
{
    return Vector4d(
        ( ( ( ( mCol0.getX() * vec.getX() ) + ( mCol1.getX() * vec.getY() ) ) + ( mCol2.getX() * vec.getZ() ) ) + ( mCol3.getX() * vec.getW() ) ),
        ( ( ( ( mCol0.getY() * vec.getX() ) + ( mCol1.getY() * vec.getY() ) ) + ( mCol2.getY() * vec.getZ() ) ) + ( mCol3.getY() * vec.getW() ) ),
        ( ( ( ( mCol0.getZ() * vec.getX() ) + ( mCol1.getZ() * vec.getY() ) ) + ( mCol2.getZ() * vec.getZ() ) ) + ( mCol3.getZ() * vec.getW() ) ),
        ( ( ( ( mCol0.getW() * vec.getX() ) + ( mCol1.getW() * vec.getY() ) ) + ( mCol2.getW() * vec.getZ() ) ) + ( mCol3.getW() * vec.getW() ) )
    );
}

inline const Vector4d Matrix4d::operator *( const Vector3d & vec ) const
{
    return Vector4d(
        ( ( ( mCol0.getX() * vec.getX() ) + ( mCol1.getX() * vec.getY() ) ) + ( mCol2.getX() * vec.getZ() ) ),
        ( ( ( mCol0.getY() * vec.getX() ) + ( mCol1.getY() * vec.getY() ) ) + ( mCol2.getY() * vec.getZ() ) ),
        ( ( ( mCol0.getZ() * vec.getX() ) + ( mCol1.getZ() * vec.getY() ) ) + ( mCol2.getZ() * vec.getZ() ) ),
        ( ( ( mCol0.getW() * vec.getX() ) + ( mCol1.getW() * vec.getY() ) ) + ( mCol2.getW() * vec.getZ() ) )
    );
}

inline const Vector4d Matrix4d::operator *( const Point3d & pnt ) const
{
    return Vector4d(
        ( ( ( ( mCol0.getX() * pnt.getX() ) + ( mCol1.getX() * pnt.getY() ) ) + ( mCol2.getX() * pnt.getZ() ) ) + mCol3.getX() ),
        ( ( ( ( mCol0.getY() * pnt.getX() ) + ( mCol1.getY() * pnt.getY() ) ) + ( mCol2.getY() * pnt.getZ() ) ) + mCol3.getY() ),
        ( ( ( ( mCol0.getZ() * pnt.getX() ) + ( mCol1.getZ() * pnt.getY() ) ) + ( mCol2.getZ() * pnt.getZ() ) ) + mCol3.getZ() ),
        ( ( ( ( mCol0.getW() * pnt.getX() ) + ( mCol1.getW() * pnt.getY() ) ) + ( mCol2.getW() * pnt.getZ() ) ) + mCol3.getW() )
    );
}

inline const Matrix4d Matrix4d::operator *( const Matrix4d & mat ) const
{
    return Matrix4d(
        ( *this * mat.mCol0 ),
        ( *this * mat.mCol1 ),
        ( *this * mat.mCol2 ),
        ( *this * mat.mCol3 )
    );
}

inline Matrix4d & Matrix4d::operator *=( const Matrix4d & mat )
{
    *this = *this * mat;
    return *this;
}

inline const Matrix4d Matrix4d::operator *( const Transform3d & tfrm ) const
{
    return Matrix4d(
        ( *this * tfrm.getCol0() ),
        ( *this * tfrm.getCol1() ),
        ( *this * tfrm.getCol2() ),
        ( *this * Point3d( tfrm.getCol3() ) )
    );
}

inline Matrix4d & Matrix4d::operator *=( const Transform3d & tfrm )
{
    *this = *this * tfrm;
    return *this;
}

inline const Matrix4d mulPerElem( const Matrix4d & mat0, const Matrix4d & mat1 )
{
    return Matrix4d(
        mulPerElem( mat0.getCol0(), mat1.getCol0() ),
        mulPerElem( mat0.getCol1(), mat1.getCol1() ),
        mulPerElem( mat0.getCol2(), mat1.getCol2() ),
        mulPerElem( mat0.getCol3(), mat1.getCol3() )
    );
}

inline const Matrix4d Matrix4d::identity( )
{
    return Matrix4d(
        Vector4d::xAxis( ),
        Vector4d::yAxis( ),
        Vector4d::zAxis( ),
        Vector4d::wAxis( )
    );
}

inline Matrix4d & Matrix4d::setUpper3x3( const Matrix3d & mat3 )
{
    mCol0.setXYZ( mat3.getCol0() );
    mCol1.setXYZ( mat3.getCol1() );
    mCol2.setXYZ( mat3.getCol2() );
    return *this;
}

inline const Matrix3d Matrix4d::getUpper3x3( ) const
{
    return Matrix3d(
        mCol0.getXYZ( ),
        mCol1.getXYZ( ),
        mCol2.getXYZ( )
    );
}

inline Matrix4d & Matrix4d::setTranslation( const Vector3d & translateVec )
{
    mCol3.setXYZ( translateVec );
    return *this;
}

inline const Vector3d Matrix4d::getTranslation( ) const
{
    return mCol3.getXYZ( );
}

inline const Matrix4d Matrix4d::rotationX( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix4d(
        Vector4d::xAxis( ),
        Vector4d( 0.0, c, s, 0.0 ),
        Vector4d( 0.0, -s, c, 0.0 ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d Matrix4d::rotationY( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix4d(
        Vector4d( c, 0.0, -s, 0.0 ),
        Vector4d::yAxis( ),
        Vector4d( s, 0.0, c, 0.0 ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d Matrix4d::rotationZ( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Matrix4d(
        Vector4d( c, s, 0.0, 0.0 ),
        Vector4d( -s, c, 0.0, 0.0 ),
        Vector4d::zAxis( ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d Matrix4d::rotationZYX( const Vector3d & radiansXYZ )
{
    double sX, cX, sY, cY, sZ, cZ, tmp0, tmp1;
    sX = sin( radiansXYZ.getX() );
    cX = cos( radiansXYZ.getX() );
    sY = sin( radiansXYZ.getY() );
    cY = cos( radiansXYZ.getY() );
    sZ = sin( radiansXYZ.getZ() );
    cZ = cos( radiansXYZ.getZ() );
    tmp0 = ( cZ * sY );
    tmp1 = ( sZ * sY );
    return Matrix4d(
        Vector4d( ( cZ * cY ), ( sZ * cY ), -sY, 0.0 ),
        Vector4d( ( ( tmp0 * sX ) - ( sZ * cX ) ), ( ( tmp1 * sX ) + ( cZ * cX ) ), ( cY * sX ), 0.0 ),
        Vector4d( ( ( tmp0 * cX ) + ( sZ * sX ) ), ( ( tmp1 * cX ) - ( cZ * sX ) ), ( cY * cX ), 0.0 ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d Matrix4d::rotation( double radians, const Vector3d & unitVec )
{
    double x, y, z, s, c, oneMinusC, xy, yz, zx;
    s = sin( radians );
    c = cos( radians );
    x = unitVec.getX();
    y = unitVec.getY();
    z = unitVec.getZ();
    xy = ( x * y );
    yz = ( y * z );
    zx = ( z * x );
    oneMinusC = ( 1.0 - c );
    return Matrix4d(
        Vector4d( ( ( ( x * x ) * oneMinusC ) + c ), ( ( xy * oneMinusC ) + ( z * s ) ), ( ( zx * oneMinusC ) - ( y * s ) ), 0.0 ),
        Vector4d( ( ( xy * oneMinusC ) - ( z * s ) ), ( ( ( y * y ) * oneMinusC ) + c ), ( ( yz * oneMinusC ) + ( x * s ) ), 0.0 ),
        Vector4d( ( ( zx * oneMinusC ) + ( y * s ) ), ( ( yz * oneMinusC ) - ( x * s ) ), ( ( ( z * z ) * oneMinusC ) + c ), 0.0 ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d Matrix4d::rotation( const Quatd & unitQuat )
{
    return Matrix4d( Transform3d::rotation( unitQuat ) );
}

inline const Matrix4d Matrix4d::scale( const Vector3d & scaleVec )
{
    return Matrix4d(
        Vector4d( scaleVec.getX(), 0.0, 0.0, 0.0 ),
        Vector4d( 0.0, scaleVec.getY(), 0.0, 0.0 ),
        Vector4d( 0.0, 0.0, scaleVec.getZ(), 0.0 ),
        Vector4d::wAxis( )
    );
}

inline const Matrix4d appendScale( const Matrix4d & mat, const Vector3d & scaleVec )
{
    return Matrix4d(
        ( mat.getCol0() * scaleVec.getX( ) ),
        ( mat.getCol1() * scaleVec.getY( ) ),
        ( mat.getCol2() * scaleVec.getZ( ) ),
        mat.getCol3()
    );
}

inline const Matrix4d prependScale( const Vector3d & scaleVec, const Matrix4d & mat )
{
    Vector4d scale4;
    scale4 = Vector4d( scaleVec, 1.0 );
    return Matrix4d(
        mulPerElem( mat.getCol0(), scale4 ),
        mulPerElem( mat.getCol1(), scale4 ),
        mulPerElem( mat.getCol2(), scale4 ),
        mulPerElem( mat.getCol3(), scale4 )
    );
}

inline const Matrix4d Matrix4d::translation( const Vector3d & translateVec )
{
    return Matrix4d(
        Vector4d::xAxis( ),
        Vector4d::yAxis( ),
        Vector4d::zAxis( ),
        Vector4d( translateVec, 1.0 )
    );
}

inline const Matrix4d Matrix4d::lookAt( const Point3d & eyePos, const Point3d & lookAtPos, const Vector3d & upVec )
{
    Matrix4d m4EyeFrame;
    Vector3d v3X, v3Y, v3Z;
    v3Y = normalize( upVec );
    v3Z = normalize( ( eyePos - lookAtPos ) );
    v3X = normalize( cross( v3Y, v3Z ) );
    v3Y = cross( v3Z, v3X );
    m4EyeFrame = Matrix4d( Vector4d( v3X ), Vector4d( v3Y ), Vector4d( v3Z ), Vector4d( eyePos ) );
    return orthoInverse( m4EyeFrame );
}

inline const Matrix4d Matrix4d::perspective( double fovyRadians, double aspect, double zNear, double zFar )
{
    double f, rangeInv;
    f = tan( ( (double)( _VECTORMATH_PI_OVER_2_DOUBLE ) - ( 0.5 * fovyRadians ) ) );
    rangeInv = ( 1.0 / ( zNear - zFar ) );
    return Matrix4d(
        Vector4d( ( f / aspect ), 0.0, 0.0, 0.0 ),
        Vector4d( 0.0, f, 0.0, 0.0 ),
        Vector4d( 0.0, 0.0, ( ( zNear + zFar ) * rangeInv ), -1.0 ),
        Vector4d( 0.0, 0.0, ( ( ( zNear * zFar ) * rangeInv ) * 2.0 ), 0.0 )
    );
}

inline const Matrix4d Matrix4d::frustum( double left, double right, double bottom, double top, double zNear, double zFar )
{
    double sum_rl, sum_tb, sum_nf, inv_rl, inv_tb, inv_nf, n2;
    sum_rl = ( right + left );
    sum_tb = ( top + bottom );
    sum_nf = ( zNear + zFar );
    inv_rl = ( 1.0 / ( right - left ) );
    inv_tb = ( 1.0 / ( top - bottom ) );
    inv_nf = ( 1.0 / ( zNear - zFar ) );
    n2 = ( zNear + zNear );
    return Matrix4d(
        Vector4d( ( n2 * inv_rl ), 0.0, 0.0, 0.0 ),
        Vector4d( 0.0, ( n2 * inv_tb ), 0.0, 0.0 ),
        Vector4d( ( sum_rl * inv_rl ), ( sum_tb * inv_tb ), ( sum_nf * inv_nf ), -1.0 ),
        Vector4d( 0.0, 0.0, ( ( n2 * inv_nf ) * zFar ), 0.0 )
    );
}

inline const Matrix4d Matrix4d::orthographic( double left, double right, double bottom, double top, double zNear, double zFar )
{
    double sum_rl, sum_tb, sum_nf, inv_rl, inv_tb, inv_nf;
    sum_rl = ( right + left );
    sum_tb = ( top + bottom );
    sum_nf = ( zNear + zFar );
    inv_rl = ( 1.0 / ( right - left ) );
    inv_tb = ( 1.0 / ( top - bottom ) );
    inv_nf = ( 1.0 / ( zNear - zFar ) );
    return Matrix4d(
        Vector4d( ( inv_rl + inv_rl ), 0.0, 0.0, 0.0 ),
        Vector4d( 0.0, ( inv_tb + inv_tb ), 0.0, 0.0 ),
        Vector4d( 0.0, 0.0, ( inv_nf + inv_nf ), 0.0 ),
        Vector4d( ( -sum_rl * inv_rl ), ( -sum_tb * inv_tb ), ( sum_nf * inv_nf ), 1.0 )
    );
}

inline const Matrix4d select( const Matrix4d & mat0, const Matrix4d & mat1, bool select1 )
{
    return Matrix4d(
        select( mat0.getCol0(), mat1.getCol0(), select1 ),
        select( mat0.getCol1(), mat1.getCol1(), select1 ),
        select( mat0.getCol2(), mat1.getCol2(), select1 ),
        select( mat0.getCol3(), mat1.getCol3(), select1 )
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Matrix4d & mat )
{
    print( mat.getRow( 0 ) );
    print( mat.getRow( 1 ) );
    print( mat.getRow( 2 ) );
    print( mat.getRow( 3 ) );
}

inline void print( const Matrix4d & mat, const char * name )
{
    printf("%s:\n", name);
    print( mat );
}

#endif

inline Transform3d::Transform3d( const Transform3d & tfrm )
{
    mCol0 = tfrm.mCol0;
    mCol1 = tfrm.mCol1;
    mCol2 = tfrm.mCol2;
    mCol3 = tfrm.mCol3;
}

inline Transform3d::Transform3d( double scalar )
{
    mCol0 = Vector3d( scalar );
    mCol1 = Vector3d( scalar );
    mCol2 = Vector3d( scalar );
    mCol3 = Vector3d( scalar );
}

inline Transform3d::Transform3d( const Vector3d & _col0, const Vector3d & _col1, const Vector3d & _col2, const Vector3d & _col3 )
{
    mCol0 = _col0;
    mCol1 = _col1;
    mCol2 = _col2;
    mCol3 = _col3;
}

inline Transform3d::Transform3d( const Matrix3d & tfrm, const Vector3d & translateVec )
{
    this->setUpper3x3( tfrm );
    this->setTranslation( translateVec );
}

inline Transform3d::Transform3d( const Quatd & unitQuat, const Vector3d & translateVec )
{
    this->setUpper3x3( Matrix3d( unitQuat ) );
    this->setTranslation( translateVec );
}

inline Transform3d & Transform3d::setCol0( const Vector3d & _col0 )
{
    mCol0 = _col0;
    return *this;
}

inline Transform3d & Transform3d::setCol1( const Vector3d & _col1 )
{
    mCol1 = _col1;
    return *this;
}

inline Transform3d & Transform3d::setCol2( const Vector3d & _col2 )
{
    mCol2 = _col2;
    return *this;
}

inline Transform3d & Transform3d::setCol3( const Vector3d & _col3 )
{
    mCol3 = _col3;
    return *this;
}

inline Transform3d & Transform3d::setCol( int col, const Vector3d & vec )
{
    *(&mCol0 + col) = vec;
    return *this;
}

inline Transform3d & Transform3d::setRow( int row, const Vector4d & vec )
{
    mCol0.setElem( row, vec.getElem( 0 ) );
    mCol1.setElem( row, vec.getElem( 1 ) );
    mCol2.setElem( row, vec.getElem( 2 ) );
    mCol3.setElem( row, vec.getElem( 3 ) );
    return *this;
}

inline Transform3d & Transform3d::setElem( int col, int row, double val )
{
    Vector3d tmpV3_0;
    tmpV3_0 = this->getCol( col );
    tmpV3_0.setElem( row, val );
    this->setCol( col, tmpV3_0 );
    return *this;
}

inline double Transform3d::getElem( int col, int row ) const
{
    return this->getCol( col ).getElem( row );
}

inline const Vector3d Transform3d::getCol0( ) const
{
    return mCol0;
}

inline const Vector3d Transform3d::getCol1( ) const
{
    return mCol1;
}

inline const Vector3d Transform3d::getCol2( ) const
{
    return mCol2;
}

inline const Vector3d Transform3d::getCol3( ) const
{
    return mCol3;
}

inline const Vector3d Transform3d::getCol( int col ) const
{
    return *(&mCol0 + col);
}

inline const Vector4d Transform3d::getRow( int row ) const
{
    return Vector4d( mCol0.getElem( row ), mCol1.getElem( row ), mCol2.getElem( row ), mCol3.getElem( row ) );
}

inline Vector3d & Transform3d::operator []( int col )
{
    return *(&mCol0 + col);
}

inline const Vector3d Transform3d::operator []( int col ) const
{
    return *(&mCol0 + col);
}

inline Transform3d & Transform3d::operator =( const Transform3d & tfrm )
{
    mCol0 = tfrm.mCol0;
    mCol1 = tfrm.mCol1;
    mCol2 = tfrm.mCol2;
    mCol3 = tfrm.mCol3;
    return *this;
}

inline const Transform3d inverse( const Transform3d & tfrm )
{
    Vector3d tmp0, tmp1, tmp2, inv0, inv1, inv2;
    double detinv;
    tmp0 = cross( tfrm.getCol1(), tfrm.getCol2() );
    tmp1 = cross( tfrm.getCol2(), tfrm.getCol0() );
    tmp2 = cross( tfrm.getCol0(), tfrm.getCol1() );
    detinv = ( 1.0 / dot( tfrm.getCol2(), tmp2 ) );
    inv0 = Vector3d( ( tmp0.getX() * detinv ), ( tmp1.getX() * detinv ), ( tmp2.getX() * detinv ) );
    inv1 = Vector3d( ( tmp0.getY() * detinv ), ( tmp1.getY() * detinv ), ( tmp2.getY() * detinv ) );
    inv2 = Vector3d( ( tmp0.getZ() * detinv ), ( tmp1.getZ() * detinv ), ( tmp2.getZ() * detinv ) );
    return Transform3d(
        inv0,
        inv1,
        inv2,
        Vector3d( ( -( ( inv0 * tfrm.getCol3().getX() ) + ( ( inv1 * tfrm.getCol3().getY() ) + ( inv2 * tfrm.getCol3().getZ() ) ) ) ) )
    );
}

inline const Transform3d orthoInverse( const Transform3d & tfrm )
{
    Vector3d inv0, inv1, inv2;
    inv0 = Vector3d( tfrm.getCol0().getX(), tfrm.getCol1().getX(), tfrm.getCol2().getX() );
    inv1 = Vector3d( tfrm.getCol0().getY(), tfrm.getCol1().getY(), tfrm.getCol2().getY() );
    inv2 = Vector3d( tfrm.getCol0().getZ(), tfrm.getCol1().getZ(), tfrm.getCol2().getZ() );
    return Transform3d(
        inv0,
        inv1,
        inv2,
        Vector3d( ( -( ( inv0 * tfrm.getCol3().getX() ) + ( ( inv1 * tfrm.getCol3().getY() ) + ( inv2 * tfrm.getCol3().getZ() ) ) ) ) )
    );
}

inline const Transform3d absPerElem( const Transform3d & tfrm )
{
    return Transform3d(
        absPerElem( tfrm.getCol0() ),
        absPerElem( tfrm.getCol1() ),
        absPerElem( tfrm.getCol2() ),
        absPerElem( tfrm.getCol3() )
    );
}

inline const Vector3d Transform3d::operator *( const Vector3d & vec ) const
{
    return Vector3d(
        ( ( ( mCol0.getX() * vec.getX() ) + ( mCol1.getX() * vec.getY() ) ) + ( mCol2.getX() * vec.getZ() ) ),
        ( ( ( mCol0.getY() * vec.getX() ) + ( mCol1.getY() * vec.getY() ) ) + ( mCol2.getY() * vec.getZ() ) ),
        ( ( ( mCol0.getZ() * vec.getX() ) + ( mCol1.getZ() * vec.getY() ) ) + ( mCol2.getZ() * vec.getZ() ) )
    );
}

inline const Point3d Transform3d::operator *( const Point3d & pnt ) const
{
    return Point3d(
        ( ( ( ( mCol0.getX() * pnt.getX() ) + ( mCol1.getX() * pnt.getY() ) ) + ( mCol2.getX() * pnt.getZ() ) ) + mCol3.getX() ),
        ( ( ( ( mCol0.getY() * pnt.getX() ) + ( mCol1.getY() * pnt.getY() ) ) + ( mCol2.getY() * pnt.getZ() ) ) + mCol3.getY() ),
        ( ( ( ( mCol0.getZ() * pnt.getX() ) + ( mCol1.getZ() * pnt.getY() ) ) + ( mCol2.getZ() * pnt.getZ() ) ) + mCol3.getZ() )
    );
}

inline const Transform3d Transform3d::operator *( const Transform3d & tfrm ) const
{
    return Transform3d(
        ( *this * tfrm.mCol0 ),
        ( *this * tfrm.mCol1 ),
        ( *this * tfrm.mCol2 ),
        Vector3d( ( *this * Point3d( tfrm.mCol3 ) ) )
    );
}

inline Transform3d & Transform3d::operator *=( const Transform3d & tfrm )
{
    *this = *this * tfrm;
    return *this;
}

inline const Transform3d mulPerElem( const Transform3d & tfrm0, const Transform3d & tfrm1 )
{
    return Transform3d(
        mulPerElem( tfrm0.getCol0(), tfrm1.getCol0() ),
        mulPerElem( tfrm0.getCol1(), tfrm1.getCol1() ),
        mulPerElem( tfrm0.getCol2(), tfrm1.getCol2() ),
        mulPerElem( tfrm0.getCol3(), tfrm1.getCol3() )
    );
}

inline const Transform3d Transform3d::identity( )
{
    return Transform3d(
        Vector3d::xAxis( ),
        Vector3d::yAxis( ),
        Vector3d::zAxis( ),
        Vector3d( 0.0 )
    );
}

inline Transform3d & Transform3d::setUpper3x3( const Matrix3d & tfrm )
{
    mCol0 = tfrm.getCol0();
    mCol1 = tfrm.getCol1();
    mCol2 = tfrm.getCol2();
    return *this;
}

inline const Matrix3d Transform3d::getUpper3x3( ) const
{
    return Matrix3d( mCol0, mCol1, mCol2 );
}

inline Transform3d & Transform3d::setTranslation( const Vector3d & translateVec )
{
    mCol3 = translateVec;
    return *this;
}

inline const Vector3d Transform3d::getTranslation( ) const
{
    return mCol3;
}

inline const Transform3d Transform3d::rotationX( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Transform3d(
        Vector3d::xAxis( ),
        Vector3d( 0.0, c, s ),
        Vector3d( 0.0, -s, c ),
        Vector3d( 0.0 )
    );
}

inline const Transform3d Transform3d::rotationY( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Transform3d(
        Vector3d( c, 0.0, -s ),
        Vector3d::yAxis( ),
        Vector3d( s, 0.0, c ),
        Vector3d( 0.0 )
    );
}

inline const Transform3d Transform3d::rotationZ( double radians )
{
    double s, c;
    s = sin( radians );
    c = cos( radians );
    return Transform3d(
        Vector3d( c, s, 0.0 ),
        Vector3d( -s, c, 0.0 ),
        Vector3d::zAxis( ),
        Vector3d( 0.0 )
    );
}

inline const Transform3d Transform3d::rotationZYX( const Vector3d & radiansXYZ )
{
    double sX, cX, sY, cY, sZ, cZ, tmp0, tmp1;
    sX = sin( radiansXYZ.getX() );
    cX = cos( radiansXYZ.getX() );
    sY = sin( radiansXYZ.getY() );
    cY = cos( radiansXYZ.getY() );
    sZ = sin( radiansXYZ.getZ() );
    cZ = cos( radiansXYZ.getZ() );
    tmp0 = ( cZ * sY );
    tmp1 = ( sZ * sY );
    return Transform3d(
        Vector3d( ( cZ * cY ), ( sZ * cY ), -sY ),
        Vector3d( ( ( tmp0 * sX ) - ( sZ * cX ) ), ( ( tmp1 * sX ) + ( cZ * cX ) ), ( cY * sX ) ),
        Vector3d( ( ( tmp0 * cX ) + ( sZ * sX ) ), ( ( tmp1 * cX ) - ( cZ * sX ) ), ( cY * cX ) ),
        Vector3d( 0.0 )
    );
}

inline const Transform3d Transform3d::rotation( double radians, const Vector3d & unitVec )
{
    return Transform3d( Matrix3d::rotation( radians, unitVec ), Vector3d( 0.0 ) );
}

inline const Transform3d Transform3d::rotation( const Quatd & unitQuat )
{
    return Transform3d( Matrix3d( unitQuat ), Vector3d( 0.0 ) );
}

inline const Transform3d Transform3d::scale( const Vector3d & scaleVec )
{
    return Transform3d(
        Vector3d( scaleVec.getX(), 0.0, 0.0 ),
        Vector3d( 0.0, scaleVec.getY(), 0.0 ),
        Vector3d( 0.0, 0.0, scaleVec.getZ() ),
        Vector3d( 0.0 )
    );
}

inline const Transform3d appendScale( const Transform3d & tfrm, const Vector3d & scaleVec )
{
    return Transform3d(
        ( tfrm.getCol0() * scaleVec.getX( ) ),
        ( tfrm.getCol1() * scaleVec.getY( ) ),
        ( tfrm.getCol2() * scaleVec.getZ( ) ),
        tfrm.getCol3()
    );
}

inline const Transform3d prependScale( const Vector3d & scaleVec, const Transform3d & tfrm )
{
    return Transform3d(
        mulPerElem( tfrm.getCol0(), scaleVec ),
        mulPerElem( tfrm.getCol1(), scaleVec ),
        mulPerElem( tfrm.getCol2(), scaleVec ),
        mulPerElem( tfrm.getCol3(), scaleVec )
    );
}

inline const Transform3d Transform3d::translation( const Vector3d & translateVec )
{
    return Transform3d(
        Vector3d::xAxis( ),
        Vector3d::yAxis( ),
        Vector3d::zAxis( ),
        translateVec
    );
}

inline const Transform3d select( const Transform3d & tfrm0, const Transform3d & tfrm1, bool select1 )
{
    return Transform3d(
        select( tfrm0.getCol0(), tfrm1.getCol0(), select1 ),
        select( tfrm0.getCol1(), tfrm1.getCol1(), select1 ),
        select( tfrm0.getCol2(), tfrm1.getCol2(), select1 ),
        select( tfrm0.getCol3(), tfrm1.getCol3(), select1 )
    );
}

#ifdef _VECTORMATH_DEBUG

inline void print( const Transform3d & tfrm )
{
    print( tfrm.getRow( 0 ) );
    print( tfrm.getRow( 1 ) );
    print( tfrm.getRow( 2 ) );
}

inline void print( const Transform3d & tfrm, const char * name )
{
    printf("%s:\n", name);
    print( tfrm );
}

#endif

inline Quatd::Quatd( const Matrix3d & tfrm )
{
    double trace, radicand, scale, xx, yx, zx, xy, yy, zy, xz, yz, zz, tmpx, tmpy, tmpz, tmpw, qx, qy, qz, qw;
    int negTrace, ZgtX, ZgtY, YgtX;
    int largestXorY, largestYorZ, largestZorX;

    xx = tfrm.getCol0().getX();
    yx = tfrm.getCol0().getY();
    zx = tfrm.getCol0().getZ();
    xy = tfrm.getCol1().getX();
    yy = tfrm.getCol1().getY();
    zy = tfrm.getCol1().getZ();
    xz = tfrm.getCol2().getX();
    yz = tfrm.getCol2().getY();
    zz = tfrm.getCol2().getZ();

    trace = ( ( xx + yy ) + zz );

    negTrace = ( trace < 0.0 );
    ZgtX = zz > xx;
    ZgtY = zz > yy;
    YgtX = yy > xx;
    largestXorY = ( !ZgtX || !ZgtY ) && negTrace;
    largestYorZ = ( YgtX || ZgtX ) && negTrace;
    largestZorX = ( ZgtY || !YgtX ) && negTrace;
    
    if ( largestXorY )
    {
        zz = -zz;
        xy = -xy;
    }
    if ( largestYorZ )
    {
        xx = -xx;
        yz = -yz;
    }
    if ( largestZorX )
    {
        yy = -yy;
        zx = -zx;
    }

    radicand = ( ( ( xx + yy ) + zz ) + 1.0 );
    scale = ( 0.5 * ( 1.0 / sqrt( radicand ) ) );

    tmpx = ( ( zy - yz ) * scale );
    tmpy = ( ( xz - zx ) * scale );
    tmpz = ( ( yx - xy ) * scale );
    tmpw = ( radicand * scale );
    qx = tmpx;
    qy = tmpy;
    qz = tmpz;
    qw = tmpw;

    if ( largestXorY )
    {
        qx = tmpw;
        qy = tmpz;
        qz = tmpy;
        qw = tmpx;
    }
    if ( largestYorZ )
    {
        tmpx = qx;
        tmpz = qz;
        qx = qy;
        qy = tmpx;
        qz = qw;
        qw = tmpz;
    }

    mX = qx;
    mY = qy;
    mZ = qz;
    mW = qw;
}

inline const Matrix3d outer( const Vector3d & tfrm0, const Vector3d & tfrm1 )
{
    return Matrix3d(
        ( tfrm0 * tfrm1.getX( ) ),
        ( tfrm0 * tfrm1.getY( ) ),
        ( tfrm0 * tfrm1.getZ( ) )
    );
}

inline const Matrix4d outer( const Vector4d & tfrm0, const Vector4d & tfrm1 )
{
    return Matrix4d(
        ( tfrm0 * tfrm1.getX( ) ),
        ( tfrm0 * tfrm1.getY( ) ),
        ( tfrm0 * tfrm1.getZ( ) ),
        ( tfrm0 * tfrm1.getW( ) )
    );
}

inline const Vector3d rowMul( const Vector3d & vec, const Matrix3d & mat )
{
    return Vector3d(
        ( ( ( vec.getX() * mat.getCol0().getX() ) + ( vec.getY() * mat.getCol0().getY() ) ) + ( vec.getZ() * mat.getCol0().getZ() ) ),
        ( ( ( vec.getX() * mat.getCol1().getX() ) + ( vec.getY() * mat.getCol1().getY() ) ) + ( vec.getZ() * mat.getCol1().getZ() ) ),
        ( ( ( vec.getX() * mat.getCol2().getX() ) + ( vec.getY() * mat.getCol2().getY() ) ) + ( vec.getZ() * mat.getCol2().getZ() ) )
    );
}

inline const Matrix3d crossMatrix( const Vector3d & vec )
{
    return Matrix3d(
        Vector3d( 0.0, vec.getZ(), -vec.getY() ),
        Vector3d( -vec.getZ(), 0.0, vec.getX() ),
        Vector3d( vec.getY(), -vec.getX(), 0.0 )
    );
}

inline const Matrix3d crossMatrixMul( const Vector3d & vec, const Matrix3d & mat )
{
    return Matrix3d( cross( vec, mat.getCol0() ), cross( vec, mat.getCol1() ), cross( vec, mat.getCol2() ) );
}

} // namespace Aos
} // namespace Vectormath

#endif
