=== The Ouroboros Libraries ===

The libraries contains fundamental interfaces for typical efforts in advanced 
video and 3D graphics as well as high throughput and multi-threaded programming. 
This code is in active development so there are bugs, APIs and semantics may 
change and extensive documentation is still on the yet-to-do list.

My understanding is all 3rd-party source code distributed with Ouroboros code 
are legally allowed to be distributed. The intent of distribution is primarily 
to ensure a compatible build project and access to the particular version of the 
library use is always available. If a reader finds anything inappropriate with 
the usage or license of external software please contact the author at 
arciuolo@gmail.com. All comments, feedback and recommendations on better 
implementations are also welcome.

Enjoy!

=== Library Overview ===

Dependency Graph:

    oMemory
       ^   \
    oString oConcurrency
       ^
     oBase   oHLSL
       ^       ^  \
   oSurface  oMesh  oCompute
       ^   \   ^
		 oCore   oGPU
			 \    /  ^
		    oGUI  oGfx

oMemory: base memcpy, bit/byte swizzles, hashes and allocators.
oString: string encoding, csv/ini/json/xml readers, robust uri/path parsing.
oConcurrency: work-stealing future, sync and concurrent containers. This also
              defines some stub functions and simple implementations for them
							that can be overriden with more robust implementations. See oCore.
oBase: C++ language enhancements and data types with no platform dependencies.
oHLSL: C++ implementation of the HLSL shader language with some helper macros 
       intended to enable cross-language implementation of shader routines.
oSurface: texture/advanced image utils (resize, convert) and file codecs 
oMesh: 3D model definition and manipulation utils as well as primitives
oCompute: relatively-pure math functions, many of which are cross-language.
oCore: platform-specific, OS constructs. This includes concurrency 
			 implementations using lightweight stubs and the robust TBB scheduler.
oGPU: abstraction for GPU-related resources and concepts.
oGUI: A rough, simple layer on top of Win32 that is currently unapologetically
      platform-specific.
oGfx: A set of graphics-related policies and definitions built on top of oGPU.

LEGACY 
These libraries are scheduled for removal, though likely their contents will 
shift to one of the above libraries

oBasis: Contains objects higher-level than oBase that are relatively free of 
        platform dependencies, but rely on COM-lite style of API that is no 
				longer the standard. The intent is to move most functional pieces to 
				oBase and remove all infrastructural parts.

oPlatform: The old oCore, but socket-related objects have not been migrated yet.


=== Refactor Notes ===

Coding Convention
Ouroboros was started before C++11 seemed like a real possibility, so a 
Microsoft-leaning gamer's coding standard was adopted. Now with the rise of the 
mobile programmer, the ascent of Boost and the swelling of stable C++11 imple-
mentations it seems that more code can be made super-generic. Such elements in 
Ouroboros are being migrated to a more std:: or Boost-looking coding convention.                                                                                                                                       

Error Reporting/Exceptions
Since C++11 now defines a robust set of error values as well as robust 
foundations for building user exceptions, newer code favors throwing rather than    
return-success or set-last-error. There are simply certain C++ standard APIs                                                    
that make avoiding exception handling impossible (std::future) so Ouroboros 
embraces exceptions and the primary error handling mechanism. Older code may not 
yet be migrated, but is expected to be.

Reference Counting
Ouroboros is migrating away from an intrusive ref-counting monolithic object 
model. oInterface is deprecated because it asks too much policy to be adopted. 

Reflection
Some coding conventions such as no virtual destructor on the base class for a 
true-pure interface and the use of QueryInterface as a substitution for multiple 
inheritance were in preparation for a compiler-based implementation of 
introspects/reflection. Project needs resulted in the execution of oRTTI, a 
preprocessor/manual version, but it seems sufficient for now and AAA titles have 
shipped with such a system, so other coding conventions may be relaxed for more 
objects since the design constraints have also been relaxed.
