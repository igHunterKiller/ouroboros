// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Extension to C++'s type_info that includes basic, generic built-in 
// construction. Note vtable() can be somewhat expensive since it must call the 
// default constructor to have C++ populate the vtable correctly. If the default 
// constructor is complex, then that call will reflect such complexity. If the 
// destructor is not properly implemented, then resource issues could arise too. 
// Use with care.

#pragma once
#include <oBase/fundamental.h>
#include <oMemory/fnv1a.h>
#include <type_traits>

namespace ouro {

namespace type_trait_flag { enum flag {

	is_voidf                    = 1<<0,
	is_integralf                = 1<<1,
	is_floating_pointf          = 1<<2,
	is_arrayf                   = 1<<3,
	is_pointerf                 = 1<<4,
	is_referencef               = 1<<5,
	is_member_object_pointerf   = 1<<6,
	is_member_function_pointerf = 1<<7,
	is_enumf                    = 1<<8,
	is_unionf                   = 1<<9,
	is_classf                   = 1<<10,
	is_functionf                = 1<<11,
	is_arithmeticf              = 1<<12,
	is_fundamentalf             = 1<<13,
	is_objectf                  = 1<<14,
	is_scalarf                  = 1<<15,
	is_compoundf                = 1<<16,
	is_member_pointerf          = 1<<17,
	is_constf                   = 1<<18,
	is_volatilef                = 1<<19,
	is_podf                     = 1<<20,
	is_emptyf                   = 1<<21,
	is_polymorphicf             = 1<<22,
	is_abstractf                = 1<<23,
	has_trivial_constructorf    = 1<<24,
	has_trivial_copyf           = 1<<25,
	has_trivial_assignf         = 1<<26,
	has_trivial_destructorf     = 1<<27,
	has_virtual_destructorf     = 1<<28,
	is_signedf                  = 1<<29,
	is_unsignedf                = 1<<30,
	// unused                   = 1<<31,

};}

typedef void (*type_info_default_constructor)(void* instance);
typedef void (*type_info_copy_constructor)(void* instance, const void* src);
typedef void (*type_info_destructor)(void* instance);

template<typename T, typename U> struct sizeof_void
{
	static const size_t size = sizeof(U);
	static const size_t element_size = sizeof(std::remove_extent<U>::type);
	static const size_t num_elements = sizeof(U) / sizeof(std::remove_extent<U>::type);
};

template<typename U> struct sizeof_void<std::true_type, U>
{
	static const size_t size = 0;
	static const size_t element_size = 0;
	static const size_t num_elements = 0;
};

template<typename T> struct type_info
{
	typedef T type;

	operator const type_info&() const { return typeid(T); }

	// returns the regular RTTI name for this type
	static const char* name() { return typeid(T).name(); }

	// returns the RTTI name without "union ", "class " or "enum " prefixes.
	static const char* simple_name()
	{
		const char* n = name();
		if (std::is_union<T>::value) n += 5;
		if (std::is_class<T>::value) n += 5;
		if (std::is_enum <T>::value) n += 4;

		// move past whitespace
		const char* nn = n;
		while (*nn && *nn++ != ' ');
		return *nn ? nn : n;
	}

	// total size of the type
	static const size_t size = sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::size;

	// if the type is a fixed array, return the size of one element of that array
	static const size_t element_size = sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::element_size;
		
	// if the type is a fixed array, return the number of elements in that array
	static const size_t num_elements = sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::num_elements;

	// creates a value that is either an oTYPE_ID or a hash of the RTTI name for
	// enums, classes, and unions
	static unsigned int id() { return is_fundamental<T>::value ? (unsigned int)ouro::fundamental_type<T>::value : ouro::fnv1a<unsigned int>(simple_name()); }

	// returns the vtable pointer for the specified class (nullptr if the class is 
	// not polymorphic) NOTE: this isn't general, it assumes the basest class is 
	// virtual or none of the class is.
	static const void* vtable()
	{
		const void* vtbl = nullptr;
		if (std::is_polymorphic<T>::value)
		{
			struct vtable { void* pointer; };
			T instance; // default construct
			#ifdef _MSC_VER
				vtbl = *(void**)((vtable*)&instance)->pointer;
			#else
				#error Unsupported platform
			#endif 
		}
		return vtbl;
	}

	// calls this type's default constructor on the specified memory (similar to
	// placement new)
	static void default_construct(void* instance)
	{
		#pragma warning(disable:4345) // behavior change: an object of POD type constructed with an initializer of the form () 
		::new (instance) T();
		#pragma warning(default:4345)
	}

	// calls this type's copy constructor copying from src to instance
	static void copy_construct(void* instance, const void* src) { return src ? ::new (instance) T(*static_cast<T*>(const_cast<void*>(src))) : default_construct(instance); }

	// calls this type's destructor
	static void destroy(void* instance) { static_cast<T*>(instance)->~T(); }

	// store traits for runtime access
	static const unsigned int traits = 
		((std::is_void                   <T>::value&0x1)<<0)|
		((std::is_integral               <T>::value&0x1)<<1)|
		((std::is_floating_point         <T>::value&0x1)<<2)|
		((std::is_array                  <T>::value&0x1)<<3)|
		((std::is_pointer                <T>::value&0x1)<<4)|
		((std::is_reference              <T>::value&0x1)<<5)|
		((std::is_member_object_pointer  <T>::value&0x1)<<6)|
		((std::is_member_function_pointer<T>::value&0x1)<<7)|
		((std::is_enum                   <T>::value&0x1)<<8)|
		((std::is_union                  <T>::value&0x1)<<9)|
		((std::is_class                  <T>::value&0x1)<<10)|
		((std::is_function               <T>::value&0x1)<<11)|
		((std::is_arithmetic             <T>::value&0x1)<<12)|
		((std::is_fundamental            <T>::value&0x1)<<13)|
		((std::is_object                 <T>::value&0x1)<<14)|
		((std::is_scalar                 <T>::value&0x1)<<15)|
		((std::is_compound               <T>::value&0x1)<<16)|
		((std::is_member_pointer         <T>::value&0x1)<<17)|
		((std::is_const                  <T>::value&0x1)<<18)|
		((std::is_volatile               <T>::value&0x1)<<19)|
		((std::is_pod                    <T>::value&0x1)<<20)|
		((std::is_empty                  <T>::value&0x1)<<21)|
		((std::is_polymorphic            <T>::value&0x1)<<22)|
		((std::is_abstract               <T>::value&0x1)<<23)|
		((std::has_trivial_constructor   <T>::value&0x1)<<24)|
		((std::has_trivial_copy          <T>::value&0x1)<<25)|
		((std::has_trivial_assign        <T>::value&0x1)<<26)|
		((std::has_trivial_destructor    <T>::value&0x1)<<27)|
		((std::has_virtual_destructor    <T>::value&0x1)<<28)|
		((std::is_signed                 <T>::value&0x1)<<29)|
		((std::is_unsigned               <T>::value&0x1)<<30);
};

}
