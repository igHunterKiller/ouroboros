// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Vertex/interpolant semantics defined so the same struct compiles in C++/HLSL.

#ifndef oHLSL
#pragma once
	#define oGPU_SVPOSITION
	#define oGPU_SVINSTANCEID
	#define oGPU_POSITION0
	#define oGPU_POSITION1
	#define oGPU_POSITION2
	#define oGPU_POSITION3
	#define oGPU_POSITION4
	#define oGPU_POSITION5
	#define oGPU_POSITION6
	#define oGPU_POSITION7
	#define oGPU_NORMAL0
	#define oGPU_NORMAL1
	#define oGPU_NORMAL2
	#define oGPU_NORMAL3
	#define oGPU_NORMAL4
	#define oGPU_NORMAL5
	#define oGPU_NORMAL6
	#define oGPU_NORMAL7
	#define oGPU_TANGENT0
	#define oGPU_TANGENT1
	#define oGPU_TANGENT2
	#define oGPU_TANGENT3
	#define oGPU_TANGENT4
	#define oGPU_TANGENT5
	#define oGPU_TANGENT6
	#define oGPU_TANGENT7
	#define oGPU_TEXCOORD0
	#define oGPU_TEXCOORD1
	#define oGPU_TEXCOORD2
	#define oGPU_TEXCOORD3
	#define oGPU_TEXCOORD4
	#define oGPU_TEXCOORD5
	#define oGPU_TEXCOORD6
	#define oGPU_TEXCOORD7
	#define oGPU_COLOR0
	#define oGPU_COLOR1
	#define oGPU_COLOR2
	#define oGPU_COLOR3
	#define oGPU_COLOR4
	#define oGPU_COLOR5
	#define oGPU_COLOR6
	#define oGPU_COLOR7
	#define oGPU_MISC0
	#define oGPU_MISC1
	#define oGPU_MISC2
	#define oGPU_MISC3
	#define oGPU_MISC4
	#define oGPU_MISC5
  #define oGPU_MISC6
  #define oGPU_MISC7
#else
	#define oGPU_SVPOSITION   : SV_Position
	#define oGPU_SVINSTANCEID : SV_InstanceID
  #define oGPU_POSITION0	  : POSITION0
  #define oGPU_POSITION1	  : POSITION1
  #define oGPU_POSITION2	  : POSITION2
  #define oGPU_POSITION3	  : POSITION3
  #define oGPU_POSITION4	  : POSITION4
  #define oGPU_POSITION5	  : POSITION5
  #define oGPU_POSITION6	  : POSITION6
  #define oGPU_POSITION7	  : POSITION7
  #define oGPU_NORMAL0		  : NORMAL0
  #define oGPU_NORMAL1		  : NORMAL1
  #define oGPU_NORMAL2		  : NORMAL2
  #define oGPU_NORMAL3		  : NORMAL3
  #define oGPU_NORMAL4		  : NORMAL4
  #define oGPU_NORMAL5		  : NORMAL5
  #define oGPU_NORMAL6		  : NORMAL6
  #define oGPU_NORMAL7		  : NORMAL7
  #define oGPU_TANGENT0		  : TANGENT0
  #define oGPU_TANGENT1		  : TANGENT1
  #define oGPU_TANGENT2		  : TANGENT2
  #define oGPU_TANGENT3		  : TANGENT3
  #define oGPU_TANGENT4		  : TANGENT4
  #define oGPU_TANGENT5		  : TANGENT5
  #define oGPU_TANGENT6		  : TANGENT6
  #define oGPU_TANGENT7		  : TANGENT7
  #define oGPU_TEXCOORD0	  : TEXCOORD0
  #define oGPU_TEXCOORD1	  : TEXCOORD1
  #define oGPU_TEXCOORD2	  : TEXCOORD2
  #define oGPU_TEXCOORD3	  : TEXCOORD3
  #define oGPU_TEXCOORD4	  : TEXCOORD4
  #define oGPU_TEXCOORD5	  : TEXCOORD5
  #define oGPU_TEXCOORD6	  : TEXCOORD6
  #define oGPU_TEXCOORD7	  : TEXCOORD7
  #define oGPU_COLOR0		    : COLOR0
  #define oGPU_COLOR1		    : COLOR1
  #define oGPU_COLOR2		    : COLOR2
  #define oGPU_COLOR3		    : COLOR3
  #define oGPU_COLOR4		    : COLOR4
  #define oGPU_COLOR5		    : COLOR5
  #define oGPU_COLOR6		    : COLOR6
  #define oGPU_COLOR7		    : COLOR7
  #define oGPU_MISC0		    : MISC0
  #define oGPU_MISC1		    : MISC1
  #define oGPU_MISC2		    : MISC2
  #define oGPU_MISC3		    : MISC3
  #define oGPU_MISC4		    : MISC4
  #define oGPU_MISC5		    : MISC5
  #define oGPU_MISC6		    : MISC6
  #define oGPU_MISC7		    : MISC7
#endif

#define oGPU_POSITION  oGPU_POSITION0
#define oGPU_NORMAL    oGPU_NORMAL0
#define oGPU_TANGENT   oGPU_TANGENT0
#define oGPU_BITANGENT oGPU_TANGENT1
#define oGPU_TEXCOORD  oGPU_TEXCOORD0
#define oGPU_COLOR     oGPU_COLOR0
#define oGPU_MISC      oGPU_MISC0
