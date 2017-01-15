/*
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#ifndef GEOMETRY_INCLUDED
#define GEOMETRY_INCLUDED

#include <stdio.h>
#include <math.h>
#include <vector>
#include <stdlib.h>
#include <unordered_map>
enum BoundaryType
{
	BOUNDARY_FREE = 0,
	BOUNDARY_DIRICHLET = 1,
	BOUNDARY_NEUMANN = 2,
	BOUNDARY_COUNT = 3
};
template<class Real>
Real Random(void);

template< class Real >
struct Point3D
{
	Real coords[3];
	Point3D( void ) { coords[0] = coords[1] = coords[2] = Real(0); }
	Point3D( Real v ) { coords[0] = coords[1] = coords[2] = v; }
	template< class _Real > Point3D( _Real v0 , _Real v1 , _Real v2 ){ coords[0] = Real(v0) , coords[1] = Real(v1) , coords[2] = Real(v2); }
	template< class _Real > Point3D( const Point3D< _Real >& p ){ coords[0] = Real( p[0] ) , coords[1] = Real( p[1] ) , coords[2] = Real( p[2] ); }
	inline       Real& operator[] ( int i )       { return coords[i]; }
	inline const Real& operator[] ( int i ) const { return coords[i]; }
	inline Point3D  operator - ( void ) const { Point3D q ; q.coords[0] = -coords[0] , q.coords[1] = -coords[1] , q.coords[2] = -coords[2] ; return q; }

	template< class _Real > inline Point3D& operator += ( Point3D< _Real > p ){ coords[0] += Real(p.coords[0]) , coords[1] += Real(p.coords[1]) , coords[2] += Real(p.coords[2]) ; return *this; }
	template< class _Real > inline Point3D  operator +  ( Point3D< _Real > p ) const { Point3D q ; q.coords[0] = coords[0] + Real(p.coords[0]) , q.coords[1] = coords[1] + Real(p.coords[1]) , q.coords[2] = coords[2] + Real(p.coords[2]) ; return q; }
	template< class _Real > inline Point3D& operator *= ( _Real r ) { coords[0] *= Real(r) , coords[1] *= Real(r) , coords[2] *= Real(r) ; return *this; }
	template< class _Real > inline Point3D  operator *  ( _Real r ) const { Point3D q ; q.coords[0] = coords[0] * Real(r) , q.coords[1] = coords[1] * Real(r) , q.coords[2] = coords[2] * Real(r) ; return q; }

	template< class _Real > inline Point3D& operator -= ( Point3D< _Real > p ){ return ( (*this)+=(-p) ); }
	template< class _Real > inline Point3D  operator -  ( Point3D< _Real > p ) const { return (*this)+(-p); }
	template< class _Real > inline Point3D& operator /= ( _Real r ){ return ( (*this)*=Real(1./r) ); }
	template< class _Real > inline Point3D  operator /  ( _Real r ) const { return (*this) * ( Real(1.)/r ); }

	static Real Dot( const Point3D< Real >& p1 , const Point3D< Real >& p2 ){ return p1.coords[0]*p2.coords[0] + p1.coords[1]*p2.coords[1] + p1.coords[2]*p2.coords[2]; }
	template< class Real1 , class Real2 >
	static Real Dot( const Point3D< Real1 >& p1 , const Point3D< Real2 >& p2 ){ return Real( p1.coords[0]*p2.coords[0] + p1.coords[1]*p2.coords[1] + p1.coords[2]*p2.coords[2] ); }
};

template< class Real >
struct XForm3x3
{
	Real coords[3][3];
	XForm3x3( void ) { for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ )  coords[i][j] = Real(0.); }
	static XForm3x3 Identity( void )
	{
		XForm3x3 xForm;
		xForm(0,0) = xForm(1,1) = xForm(2,2) = Real(1.);
		return xForm;
	}
	Real& operator() ( int i , int j ){ return coords[i][j]; }
	const Real& operator() ( int i , int j ) const { return coords[i][j]; }
	template< class _Real > Point3D< _Real > operator * ( const Point3D< _Real >& p ) const
	{
		Point3D< _Real > q;
		for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) q[i] += _Real( coords[j][i] * p[j] );
		return q;
	}
	XForm3x3 operator * ( const XForm3x3& m ) const
	{
		XForm3x3 n;
		for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) for( int k=0 ; k<3 ; k++ ) n.coords[i][j] += m.coords[i][k]*coords[k][j];
		return n;
	}
	XForm3x3 transpose( void ) const
	{
		XForm3x3 xForm;
		for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ; j++ ) xForm( i , j ) = coords[j][i];
		return xForm;
	}
	Real subDeterminant( int i , int j ) const
	{
		int i1 = (i+1)%3 , i2 = (i+2)%3;
		int j1 = (j+1)%3 , j2 = (j+2)%3;
		return coords[i1][j1] * coords[i2][j2] - coords[i1][j2] * coords[i2][j1];
	}
	Real determinant( void ) const { return coords[0][0] * subDeterminant( 0 , 0 ) + coords[1][0] * subDeterminant( 1 , 0 ) + coords[2][0] * subDeterminant( 2 , 0 ); }
	XForm3x3 inverse( void ) const
	{
		XForm3x3 xForm;
		Real d = determinant();
		for( int i=0 ; i<3 ; i++ ) for( int j=0 ; j<3 ;j++ ) xForm.coords[j][i] =  subDeterminant( i , j ) / d;
		return xForm;
	}
};

template< class Real >
struct XForm4x4
{
	Real coords[4][4];
	XForm4x4( void ) { for( int i=0 ; i<4 ; i++ ) for( int j=0 ; j<4 ; j++ )  coords[i][j] = Real(0.); }
	static XForm4x4 Identity( void )
	{
		XForm4x4 xForm;
		xForm(0,0) = xForm(1,1) = xForm(2,2) = xForm(3,3) = Real(1.);
		return xForm;
	}
	Real& operator() ( int i , int j ){ return coords[i][j]; }
	const Real& operator() ( int i , int j ) const { return coords[i][j]; }
	template< class _Real > Point3D< _Real > operator * ( const Point3D< _Real >& p ) const
	{
		Point3D< _Real > q;
		for( int i=0 ; i<3 ; i++ )
		{
			for( int j=0 ; j<3 ; j++ ) q[i] += (_Real)( coords[j][i] * p[j] );
			q[i] += (_Real)coords[3][i];
		}
		return q;
	}
	XForm4x4 operator * ( const XForm4x4& m ) const
	{
		XForm4x4 n;
		for( int i=0 ; i<4 ; i++ ) for( int j=0 ; j<4 ; j++ ) for( int k=0 ; k<4 ; k++ ) n.coords[i][j] += m.coords[i][k]*coords[k][j];
		return n;
	}
	XForm4x4 transpose( void ) const
	{
		XForm4x4 xForm;
		for( int i=0 ; i<4 ; i++ ) for( int j=0 ; j<4 ; j++ ) xForm( i , j ) = coords[j][i];
		return xForm;
	}
	Real subDeterminant( int i , int j ) const
	{
		XForm3x3< Real > xForm;
		int ii[] = { (i+1)%4 , (i+2)%4 , (i+3)%4 } , jj[] = { (j+1)%4 , (j+2)%4 , (j+3)%4 };
		for( int _i=0 ; _i<3 ; _i++ ) for( int _j=0 ; _j<3 ; _j++ ) xForm( _i , _j ) = coords[ ii[_i] ][ jj[_j] ];
		return xForm.determinant();
	}
	Real determinant( void ) const { return coords[0][0] * subDeterminant( 0 , 0 ) - coords[1][0] * subDeterminant( 1 , 0 ) + coords[2][0] * subDeterminant( 2 , 0 ) - coords[3][0] * subDeterminant( 3 , 0 ); }
	XForm4x4 inverse( void ) const
	{
		XForm4x4 xForm;
		Real d = determinant();
		for( int i=0 ; i<4 ; i++ ) for( int j=0 ; j<4 ;j++ )
			if( (i+j)%2==0 ) xForm.coords[j][i] =  subDeterminant( i , j ) / d;
			else             xForm.coords[j][i] = -subDeterminant( i , j ) / d;
		return xForm;
	}
};

template< class Real >
struct OrientedPoint3D
{
	Point3D< Real > p , n;
	OrientedPoint3D( Point3D< Real > pp=Point3D< Real >() , Point3D< Real > nn=Point3D< Real >() ) : p(pp) , n(nn) { ; }
	template< class _Real > OrientedPoint3D( const OrientedPoint3D< _Real >& p ) : OrientedPoint3D( Point3D< Real >( p.p ) , Point3D< Real >( p.n ) ){ ; }

	template< class _Real > inline OrientedPoint3D& operator += ( OrientedPoint3D< _Real > _p ){ p += _p.p , n += _p.n ; return *this; }
	template< class _Real > inline OrientedPoint3D  operator +  ( OrientedPoint3D< _Real > _p ) const { return OrientedPoint3D< Real >( p+_p.p , n+_p.n ); }
	template< class _Real > inline OrientedPoint3D& operator *= ( _Real r ) { p *= r , n *= r ; return *this; }
	template< class _Real > inline OrientedPoint3D  operator *  ( _Real r ) const { return OrientedPoint3D< Real >( p*r , n*r ); }

	template< class _Real > inline OrientedPoint3D& operator -= ( OrientedPoint3D< _Real > p ){ return ( (*this)+=(-p) ); }
	template< class _Real > inline OrientedPoint3D  operator -  ( OrientedPoint3D< _Real > p ) const { return (*this)+(-p); }
	template< class _Real > inline OrientedPoint3D& operator /= ( _Real r ){ return ( (*this)*=Real(1./r) ); }
	template< class _Real > inline OrientedPoint3D  operator /  ( _Real r ) const { return (*this) * ( Real(1.)/r ); }
};

template< class Data , class Real >
struct ProjectiveData
{
	Data data;
	Real weight;
	ProjectiveData( Data d=Data(0) , Real w=Real(0) ) : data(d) , weight(w) { ; }
	operator Data (){ return weight!=0 ? data/weight : data*weight; }
	ProjectiveData& operator += ( const ProjectiveData& p ){ data += p.data , weight += p.weight ; return *this; }
	ProjectiveData& operator -= ( const ProjectiveData& p ){ data -= p.data , weight -= p.weight ; return *this; }
	ProjectiveData& operator *= ( Real s ){ data *= s , weight *= s ; return *this; }
	ProjectiveData& operator /= ( Real s ){ data /= s , weight /= s ; return *this; }
	ProjectiveData  operator +  ( const ProjectiveData& p ) const { return ProjectiveData( data+p.data , weight+p.weight ); }
	ProjectiveData  operator -  ( const ProjectiveData& p ) const { return ProjectiveData( data-p.data , weight-p.weight ); }
	ProjectiveData  operator *  ( Real s ) const { return ProjectiveData( data*s , weight*s ); }
	ProjectiveData  operator /  ( Real s ) const { return ProjectiveData( data/s , weight/s ); }
};

template< class Real >
class PlyVertex
{
public:
	typedef PlyVertex Wrapper;


	Point3D< Real > point;

	PlyVertex(void) { ; }
	PlyVertex(Point3D< Real > p) { point = p; }
	PlyVertex operator + (PlyVertex p) const { return PlyVertex(point + p.point); }
	PlyVertex operator - (PlyVertex p) const { return PlyVertex(point - p.point); }
	template< class _Real > PlyVertex operator * (_Real s) const { return PlyVertex(point*s); }
	template< class _Real > PlyVertex operator / (_Real s) const { return PlyVertex(point / s); }
	PlyVertex& operator += (PlyVertex p) { point += p.point; return *this; }
	PlyVertex& operator -= (PlyVertex p) { point -= p.point; return *this; }
	template< class _Real > PlyVertex& operator *= (_Real s) { point *= s; return *this; }
	template< class _Real > PlyVertex& operator /= (_Real s) { point /= s; return *this; }
};
template< class Real, class _Real > PlyVertex< Real > operator * (XForm4x4< _Real > xForm, PlyVertex< Real > v) { return PlyVertex< Real >(xForm * v.point); }

template< class Real >
class PlyValueVertex
{
public:
	typedef PlyValueVertex Wrapper;
	Point3D<Real> point;
	Real value;

	PlyValueVertex(void) : value(Real(0)) { ; }
	PlyValueVertex(Point3D< Real > p, Real v) : point(p), value(v) { ; }
	PlyValueVertex operator + (PlyValueVertex p) const { return PlyValueVertex(point + p.point, value + p.value); }
	PlyValueVertex operator - (PlyValueVertex p) const { return PlyValueVertex(point - p.value, value - p.value); }
	template< class _Real > PlyValueVertex operator * (_Real s) const { return PlyValueVertex(point*s, Real(value*s)); }
	template< class _Real > PlyValueVertex operator / (_Real s) const { return PlyValueVertex(point / s, Real(value / s)); }
	PlyValueVertex& operator += (PlyValueVertex p) { point += p.point, value += p.value; return *this; }
	PlyValueVertex& operator -= (PlyValueVertex p) { point -= p.point, value -= p.value; return *this; }
	template< class _Real > PlyValueVertex& operator *= (_Real s) { point *= s, value *= Real(s); return *this; }
	template< class _Real > PlyValueVertex& operator /= (_Real s) { point /= s, value /= Real(s); return *this; }
};
template< class Real, class _Real > PlyValueVertex< Real > operator * (XForm4x4< _Real > xForm, PlyValueVertex< Real > v) { return PlyValueVertex< Real >(xForm * v.point, v.value); }

template< class Real >
class PlyOrientedVertex
{
public:
	typedef PlyOrientedVertex Wrapper;
	Point3D<Real> point, normal;

	PlyOrientedVertex(void) { ; }
	PlyOrientedVertex(Point3D< Real > p, Point3D< Real > n) : point(p), normal(n) { ; }
	PlyOrientedVertex operator + (PlyOrientedVertex p) const { return PlyOrientedVertex(point + p.point, normal + p.normal); }
	PlyOrientedVertex operator - (PlyOrientedVertex p) const { return PlyOrientedVertex(point - p.point, normal - p.normal); }
	template< class _Real > PlyOrientedVertex operator * (_Real s) const { return PlyOrientedVertex(point*s, normal*s); }
	template< class _Real > PlyOrientedVertex operator / (_Real s) const { return PlyOrientedVertex(point / s, normal / s); }
	PlyOrientedVertex& operator += (PlyOrientedVertex p) { point += p.point, normal += p.normal; return *this; }
	PlyOrientedVertex& operator -= (PlyOrientedVertex p) { point -= p.point, normal -= p.normal; return *this; }
	template< class _Real > PlyOrientedVertex& operator *= (_Real s) { point *= s, normal *= s; return *this; }
	template< class _Real > PlyOrientedVertex& operator /= (_Real s) { point /= s, normal /= s; return *this; }
};
template< class Real, class _Real > PlyOrientedVertex< Real > operator * (XForm4x4< _Real > xForm, PlyOrientedVertex< Real > v) { return PlyOrientedVertex< Real >(xForm * v.point, xForm.inverse().transpose() * v.normal); }
template< class Real >
class PlyColorVertex
{
public:
	struct _PlyColorVertex
	{
		Point3D< Real > point, color;
		_PlyColorVertex(void) { ; }
		_PlyColorVertex(Point3D< Real > p, Point3D< Real > c) : point(p), color(c) { ; }
		_PlyColorVertex(PlyColorVertex< Real > p) { point = p.point; for (int c = 0; c<3; c++) color[c] = (Real)p.color[c]; }
		operator PlyColorVertex< Real >()
		{
			PlyColorVertex< Real > p;
			p.point = point;
			for (int c = 0; c<3; c++) p.color[c] = (unsigned char)std::max< int >(0, std::min< int >(255, (int)(color[c] + 0.5)));
			return p;
		}

		_PlyColorVertex operator + (_PlyColorVertex p) const { return _PlyColorVertex(point + p.point, color + p.color); }
		_PlyColorVertex operator - (_PlyColorVertex p) const { return _PlyColorVertex(point - p.point, color - p.color); }
		template< class _Real > _PlyColorVertex operator * (_Real s) const { return _PlyColorVertex(point*s, color*s); }
		template< class _Real > _PlyColorVertex operator / (_Real s) const { return _PlyColorVertex(point / s, color / s); }
		_PlyColorVertex& operator += (_PlyColorVertex p) { point += p.point, color += p.color; return *this; }
		_PlyColorVertex& operator -= (_PlyColorVertex p) { point -= p.point, color -= p.color; return *this; }
		template< class _Real > _PlyColorVertex& operator *= (_Real s) { point *= s, color *= s; return *this; }
		template< class _Real > _PlyColorVertex& operator /= (_Real s) { point /= s, color /= s; return *this; }
	};

	typedef _PlyColorVertex Wrapper;
	Point3D< Real > point;
	unsigned char color[3];

	operator Point3D< Real >& () { return point; }
	operator const Point3D< Real >& () const { return point; }
	PlyColorVertex(void) { point.coords[0] = point.coords[1] = point.coords[2] = 0, color[0] = color[1] = color[2] = 0; }
	PlyColorVertex(const Point3D<Real>& p) { point = p; }
	PlyColorVertex(const Point3D< Real >& p, const unsigned char c[3]) { point = p, color[0] = c[0], color[1] = c[1], color[2] = c[2]; }
};
template< class Real, class _Real > PlyColorVertex< Real > operator * (XForm4x4< _Real > xForm, PlyColorVertex< Real > v) { return PlyColorVertex< Real >(xForm * v.point, v.color); }

template< class Real >
class PlyColorAndValueVertex
{
public:
	struct _PlyColorAndValueVertex
	{
		Point3D< Real > point, color;
		Real value;
		_PlyColorAndValueVertex(void) : value(0) { ; }
		_PlyColorAndValueVertex(Point3D< Real > p, Point3D< Real > c, Real v) : point(p), color(c), value(v) { ; }
		_PlyColorAndValueVertex(PlyColorAndValueVertex< Real > p) { point = p.point; for (int c = 0; c<3; c++) color[c] = (Real)p.color[c]; value = p.value; }
		operator PlyColorAndValueVertex< Real >()
		{
			PlyColorAndValueVertex< Real > p;
			p.point = point;
			for (int c = 0; c<3; c++) p.color[c] = (unsigned char)std::max< int >(0, std::min< int >(255, (int)(color[c] + 0.5)));
			p.value = value;
			return p;
		}

		_PlyColorAndValueVertex operator + (_PlyColorAndValueVertex p) const { return _PlyColorAndValueVertex(point + p.point, color + p.color, value + p.value); }
		_PlyColorAndValueVertex operator - (_PlyColorAndValueVertex p) const { return _PlyColorAndValueVertex(point - p.value, color - p.color, value + p.value); }
		template< class _Real > _PlyColorAndValueVertex operator * (_Real s) const { return _PlyColorAndValueVertex(point*s, color*s, value*s); }
		template< class _Real > _PlyColorAndValueVertex operator / (_Real s) const { return _PlyColorAndValueVertex(point / s, color / s, value / s); }
		_PlyColorAndValueVertex& operator += (_PlyColorAndValueVertex p) { point += p.point, color += p.color, value += p.value; return *this; }
		_PlyColorAndValueVertex& operator -= (_PlyColorAndValueVertex p) { point -= p.point, color -= p.color, value -= p.value; return *this; }
		template< class _Real > _PlyColorAndValueVertex& operator *= (_Real s) { point *= s, color *= s, value *= (Real)s; return *this; }
		template< class _Real > _PlyColorAndValueVertex& operator /= (_Real s) { point /= s, color /= s, value /= (Real)s; return *this; }
	};

	typedef _PlyColorAndValueVertex Wrapper;

	Point3D< Real > point;
	unsigned char color[3];
	Real value;

	operator Point3D< Real >& () { return point; }
	operator const Point3D< Real >& () const { return point; }
	PlyColorAndValueVertex(void) { point.coords[0] = point.coords[1] = point.coords[2] = (Real)0, color[0] = color[1] = color[2] = 0, value = (Real)0; }
	PlyColorAndValueVertex(const Point3D< Real >& p) { point = p; }
	PlyColorAndValueVertex(const Point3D< Real >& p, const unsigned char c[3], Real v) { point = p, color[0] = c[0], color[1] = c[1], color[2] = c[2], value = v; }
};
template< class Real, class _Real > PlyColorAndValueVertex< Real > operator * (XForm4x4< _Real > xForm, PlyColorAndValueVertex< Real > v) { return PlyColorAndValueVertex< Real >(xForm * v.point, v.color, v.value); }


template<class Real>
Point3D<Real> RandomBallPoint(void);

template<class Real>
Point3D<Real> RandomSpherePoint(void);

template<class Real>
double Length(const Point3D<Real>& p);

template<class Real>
double SquareLength(const Point3D<Real>& p);

template<class Real>
double Distance(const Point3D<Real>& p1,const Point3D<Real>& p2);

template<class Real>
double SquareDistance(const Point3D<Real>& p1,const Point3D<Real>& p2);

template <class Real>
void CrossProduct(const Point3D<Real>& p1,const Point3D<Real>& p2,Point3D<Real>& p);


class Edge{
public:
	double p[2][2];
	double Length(void) const{
		double d[2];
		d[0]=p[0][0]-p[1][0];
		d[1]=p[0][1]-p[1][1];

		return sqrt(d[0]*d[0]+d[1]*d[1]);
	}
};
class Triangle{
public:
	double p[3][3];
	double Area(void) const{
		double v1[3] , v2[3] , v[3];
		for( int d=0 ; d<3 ; d++ )
		{
			v1[d] = p[1][d] - p[0][d];
			v2[d] = p[2][d] - p[0][d];
		}
		v[0] =  v1[1]*v2[2] - v1[2]*v2[1];
		v[1] = -v1[0]*v2[2] + v1[2]*v2[0];
		v[2] =  v1[0]*v2[1] - v1[1]*v2[0];
		return sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] ) / 2;
	}
	double AspectRatio(void) const{
		double d=0;
		int i,j;
		for(i=0;i<3;i++){
	  for(i=0;i<3;i++)
			for(j=0;j<3;j++){d+=(p[(i+1)%3][j]-p[i][j])*(p[(i+1)%3][j]-p[i][j]);}
		}
		return Area()/d;
	}
	
};
class CoredPointIndex
{
public:
	int index;
	char inCore;

	int operator == (const CoredPointIndex& cpi) const {return (index==cpi.index) && (inCore==cpi.inCore);};
	int operator != (const CoredPointIndex& cpi) const {return (index!=cpi.index) || (inCore!=cpi.inCore);};
};
class EdgeIndex{
public:
	int idx[2];
};
class CoredEdgeIndex
{
public:
	CoredPointIndex idx[2];
};
class TriangleIndex{
public:
	int idx[3];
};

class TriangulationEdge
{
public:
	TriangulationEdge(void);
	int pIndex[2];
	int tIndex[2];
};

class TriangulationTriangle
{
public:
	TriangulationTriangle(void);
	int eIndex[3];
};

template<class Real>
class Triangulation
{
public:

	std::vector<Point3D<Real> >		points;
	std::vector<TriangulationEdge>				edges;
	std::vector<TriangulationTriangle>			triangles;

	int factor( int tIndex,int& p1,int& p2,int& p3);
	double area(void);
	double area( int tIndex );
	double area( int p1 , int p2 , int p3 );
	int flipMinimize( int eIndex);
	int addTriangle( int p1 , int p2 , int p3 );

protected:
	std::unordered_map<long long, int> edgeMap;
	static long long EdgeIndex( int p1 , int p2 );
	double area(const Triangle& t);
};


template<class Real>
void EdgeCollapse(const Real& edgeRatio,std::vector<TriangleIndex>& triangles,std::vector< Point3D<Real> >& positions,std::vector<Point3D<Real> >* normals);
template<class Real>
void TriangleCollapse(const Real& edgeRatio,std::vector<TriangleIndex>& triangles,std::vector<Point3D<Real> >& positions,std::vector<Point3D<Real> >* normals);

struct CoredVertexIndex
{
	int idx;
	bool inCore;
};
template< class Vertex >
class CoredMeshData
{
public:
	std::vector< Vertex > inCorePoints;
	virtual void resetIterator( void ) = 0;

	virtual int addOutOfCorePoint( const Vertex& p ) = 0;
	virtual int addOutOfCorePoint_s( const Vertex& p ) = 0;
	virtual int addPolygon_s( const std::vector< CoredVertexIndex >& vertices ) = 0;
	virtual int addPolygon_s( const std::vector< int >& vertices ) = 0;

	virtual int nextOutOfCorePoint( Vertex& p )=0;
	virtual int nextPolygon( std::vector< CoredVertexIndex >& vertices ) = 0;

	virtual int outOfCorePointCount(void)=0;
	virtual int polygonCount( void ) = 0;
};

template< class Vertex >
class CoredVectorMeshData : public CoredMeshData< Vertex >
{
	std::vector< Vertex > oocPoints;
	std::vector< std::vector< int > > polygons;
	int polygonIndex;
	int oocPointIndex;
public:
	CoredVectorMeshData(void);

	void resetIterator(void);

	int addOutOfCorePoint( const Vertex& p );
	int addOutOfCorePoint_s( const Vertex& p );
	int addPolygon_s( const std::vector< CoredVertexIndex >& vertices );
	int addPolygon_s( const std::vector< int >& vertices );

	int nextOutOfCorePoint( Vertex& p );
	int nextPolygon( std::vector< CoredVertexIndex >& vertices );

	int outOfCorePointCount(void);
	int polygonCount( void );
};
class BufferedReadWriteFile
{
	bool tempFile;
	FILE* _fp;
	char *_buffer , _fileName[1024];
	size_t _bufferIndex , _bufferSize;
public:
	BufferedReadWriteFile( char* fileName=NULL , int bufferSize=(1<<20) );
	~BufferedReadWriteFile( void );
	bool write( const void* data , size_t size );
	bool read ( void* data , size_t size );
	void reset( void );
};
template< class Vertex >
class CoredFileMeshData : public CoredMeshData< Vertex >
{
	char pointFileName[1024] , polygonFileName[1024];
	BufferedReadWriteFile *oocPointFile , *polygonFile;
	int oocPoints , polygons;
public:
	CoredFileMeshData( void );
	~CoredFileMeshData( void );

	void resetIterator( void );

	int addOutOfCorePoint( const Vertex& p );
	int addOutOfCorePoint_s( const Vertex& p );
	int addPolygon_s( const std::vector< CoredVertexIndex >& vertices );
	int addPolygon_s( const std::vector< int >& vertices );

	int nextOutOfCorePoint( Vertex& p );
	int nextPolygon( std::vector< CoredVertexIndex >& vertices );

	int outOfCorePointCount( void );
	int polygonCount( void );
};
#include "Geometry.hpp"

#endif // GEOMETRY_INCLUDED
