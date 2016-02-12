=== The Ouroboros Libraries ===

The libraries contains fundamental interfaces for typical efforts in advanced 
video and 3D graphics as well as high throughput and multi-threaded programming. 
This code is in active development so there are bugs, APIs and semantics may 
change and extensive documentation is still on the yet-to-do list.

My understanding is all 3rd-party source code distributed with Ouroboros code 
are legally allowed to be distributed. The intent of distribution is primarily 
to ensure a complete and compatible build project. If a reader finds anything 
inappropriate with the usage or license of external software please contact the 
author at arciuolo@gmail.com. All comments, feedback and recommendations on 
better implementations are also welcome.

Enjoy!


=== Library Overview ===

Dependency Graph:

           oArch
          /  ^  \             
     oCore oMath oMemory 
        ^    ^     ^   \
        |    | oString oConcurrency
        |    |     ^    /
     ------ oBase ------
    /     ^     ^       \
oSurface  |   oMesh  oCompute
          |     ^
        oGPU oSystem
          \  /  ^
          oGfx oGUI

oArch:        has cpu architecture and compiler abstractions; extremely basic.
oCore:        very basic functions and language simplifications.
oMemory:      base memcpy, bit/byte swizzles, hashes and allocators.
oMath:        rendering-related math functions: mostly linear algebra and geometry.
oString:      string encoding, csv/ini/json/xml readers, robust uri/path parsing.
oConcurrency: work-stealing future, sync and concurrent containers. This also
              defines some stub functions and simple implementations for them that
              can be overridden with more robust implementations. See oCore.
oBase:        C++ language enhancements and data types with no platform dependencies.
oSurface:     texture/advanced image utils (resize, convert) and file codecs.
oMesh:        3D model definition and manipulation utils as well as primitives.
oCompute:     relatively-pure math functions, many of which are C++/HLSL compatible.
oSystem:      platform-specific, OS constructs. This includes concurrency 
			  implementations using lightweight stubs and the robust TBB scheduler.
oGPU:         abstraction for GPU-related resources and concepts.
oGUI:         A rough, simple layer on top of Win32 that is currently unapologetically
              platform-specific.
oGfx:         A set of graphics-related policies and definitions built on top of oGPU.


=== Thin dependencies ===

These libs are close to being stand-alone.
oConcurrency <- oMemory b/c: concurrent_object_pool std_allocator allocate
oString      <- oMemory b/c: fnv1a


=== Math Notes ===


== Planes ==

A plane defined as Ax + By + Cz + D = 0. Primarily it means positive D 
values are on the side of the normal and negative are on the opposite side.


== Matrices ==

Matrices are column-major - don't be fooled because matrices are a list of 
column vectors. In the debugger it may seem to be row-major but it is 
column-major. (The same as the Bullet math library.)

Matrix multiplication is done in-order from left to right.
A typical SRT composition looks like this: 
	float4x4 transform = ScaleMatrix * RotationMatrix * TransformMatrix
A typical rendering transform concatenation looks like this:
	float4x4 WorldViewProjection = WorldMatrix * ViewMatrix * ProjectionMatrix

All multiplication against vectors occur in matrix * vector order.
Unimplemented operator* for vector * matrix enforces this.


=== Refactor Notes ===

Coding Convention
Things should be moving to a boost/std style coding convention. Anything that
is not like that is legacy code.

Error Reporting/Exceptions
Exceptions are the reporting mechanism for exceptional error cases, not expected
try-and-recover failures. Anything using error codes extensively will be replaced 
with exceptions.
