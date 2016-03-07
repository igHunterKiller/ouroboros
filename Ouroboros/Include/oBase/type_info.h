// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// extension to C++'s type_info

// note: vtable() calls the type's default constructor, so beware related costs.

#pragma once
#include <oCore/stringize.h>
#include <oBase/fundamental.h>
#include <oMemory/fnv1a.h>
#include <type_traits>

namespace ouro {

namespace detail
{
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
}

typedef void        (*type_info_default_constructor_fn)(void* instance);
typedef void        (*type_info_copy_constructor_fn)   (void* instance, const void* src);
typedef void        (*type_info_destructor_fn)         (void* instance);
typedef size_t      (*type_info_to_string_fn)          (char* dst, size_t dst_size, const void* src);
typedef const char* (*type_info_as_string_fn)          (const void* src);

template<typename T> struct type_info
{
	enum type_trait_flag
	{
		trait_is_void                            = 1<<0,
		trait_is_integral                        = 1<<1,
		trait_is_floating_point                  = 1<<2,
		trait_is_array                           = 1<<3,
		trait_is_pointer                         = 1<<4,
		trait_is_reference                       = 1<<5,
		trait_is_member_object_pointer           = 1<<6,
		trait_is_member_function_pointer         = 1<<7,
		trait_is_enum                            = 1<<8,
		trait_is_union                           = 1<<9,
		trait_is_class                           = 1<<10,
		trait_is_function                        = 1<<11,
		trait_is_arithmetic                      = 1<<12,
		trait_is_fundamental                     = 1<<13,
		trait_is_object                          = 1<<14,
		trait_is_scalar                          = 1<<15,
		trait_is_compound                        = 1<<16,
		trait_is_member_pointer                  = 1<<17,
		trait_is_const                           = 1<<18,
		trait_is_volatile                        = 1<<19,
		trait_is_pod                             = 1<<20,
		trait_is_empty                           = 1<<21,
		trait_is_polymorphic                     = 1<<22,
		trait_is_abstract                        = 1<<23,
		trait_is_trivially_default_constructible = 1<<24,
		trait_is_trivially_copy_constructible    = 1<<25,
		trait_is_trivially_copy_assignable       = 1<<26,
		trait_is_trivially_destructible          = 1<<27,
		trait_has_virtual_destructor             = 1<<28,
		trait_is_signed                          = 1<<29,
		trait_is_unsigned                        = 1<<30,
		// unused                                = 1<<31,
	};

	typedef T type;

	operator const ::type_info&() const { return typeid(T); }

	// returns the regular RTTI name for this type
	static const char* name() { return typeid(T).name(); }

	// returns the RTTI name without "union ", "class " or "enum " prefixes.
	static const char* simple_name()
	{
		const char* type_info_name = name();
		if (std::is_union<T>::value) type_info_name += 5;
		if (std::is_class<T>::value) type_info_name += 5;
		if (std::is_enum <T>::value) type_info_name += 4;

		// move past whitespace
		const char* nn = type_info_name;
		while (*nn && *nn++ != ' ');
		return *nn ? nn : type_info_name;
	}

	// total size of the type
	static const size_t size = detail::sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::size;

	// if the type is a fixed array, return the size of one element of that array
	static const size_t element_size = detail::sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::element_size;
		
	// if the type is a fixed array, return the number of elements in that array
	static const size_t num_elements = detail::sizeof_void<std::integral_constant<bool, std::is_void<std::remove_cv<T>::type>::value>, T>::num_elements;

	// creates a value that is either an oTYPE_ID or a hash of the RTTI name for
	// enums, classes, and unions
	static unsigned int id() { return is_fundamental<T>::value ? (unsigned int)ouro::fundamental_type<T>::value : ouro::fnv1a<unsigned int>(simple_name()); }

	// assumes single inheritance and basest class is virtual
	static const void* vtable()
	{
		const void* vtbl = nullptr;
		if (std::is_polymorphic<T>::value)
		{
			struct vtable { void* pointer; };
			T instance; // default construct, careful
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

	static const char* as_string(const void* src) { return ouro::as_string(*(const T*)src); }

	static size_t to_string(char* dst, size_t dst_size, const void* src) { return ouro::to_string(dst, dst_size, *(const T*)src); }

	// store traits for runtime access
	static const unsigned int traits = 
		((std::is_void                            <T>::value&0x1)<<0 )|
		((std::is_integral                        <T>::value&0x1)<<1 )|
		((std::is_floating_point                  <T>::value&0x1)<<2 )|
		((std::is_array                           <T>::value&0x1)<<3 )|
		((std::is_pointer                         <T>::value&0x1)<<4 )|
		((std::is_reference                       <T>::value&0x1)<<5 )|
		((std::is_member_object_pointer           <T>::value&0x1)<<6 )|
		((std::is_member_function_pointer         <T>::value&0x1)<<7 )|
		((std::is_enum                            <T>::value&0x1)<<8 )|
		((std::is_union                           <T>::value&0x1)<<9 )|
		((std::is_class                           <T>::value&0x1)<<10)|
		((std::is_function                        <T>::value&0x1)<<11)|
		((std::is_arithmetic                      <T>::value&0x1)<<12)|
		((std::is_fundamental                     <T>::value&0x1)<<13)|
		((std::is_object                          <T>::value&0x1)<<14)|
		((std::is_scalar                          <T>::value&0x1)<<15)|
		((std::is_compound                        <T>::value&0x1)<<16)|
		((std::is_member_pointer                  <T>::value&0x1)<<17)|
		((std::is_const                           <T>::value&0x1)<<18)|
		((std::is_volatile                        <T>::value&0x1)<<19)|
		((std::is_pod                             <T>::value&0x1)<<20)|
		((std::is_empty                           <T>::value&0x1)<<21)|
		((std::is_polymorphic                     <T>::value&0x1)<<22)|
		((std::is_abstract                        <T>::value&0x1)<<23)|
		((std::is_trivially_default_constructible <T>::value&0x1)<<24)|
		((std::is_trivially_copy_constructible    <T>::value&0x1)<<25)|
		((std::is_trivially_copy_assignable       <T>::value&0x1)<<26)|
		((std::is_trivially_destructible          <T>::value&0x1)<<27)|
		((std::has_virtual_destructor             <T>::value&0x1)<<28)|
		((std::is_signed                          <T>::value&0x1)<<29)|
		((std::is_unsigned                        <T>::value&0x1)<<30);
};

// Indents a field=value type format, using to_string to convert the specified data.
                                  size_t field_snprintf(char* dst, size_t dst_size, const char* label, size_t indent, type_info_to_string_fn to_string, const void* data);
template<typename T>              size_t field_snprintf(char* dst, size_t dst_size, const char* label, size_t indent, const T& data) { return field_snprintf(dst, dst_size, label, indent, type_info<T>::to_string, &data); }
template<typename T, size_t size> size_t field_snprintf(char (&dst)[size],          const char* label, size_t indent, const T& data) { return field_snprintf(dst, size,     label, indent, type_info<T>::to_string, &data); }

template<typename T, typename U> struct type_info_cast_pointer_to_void                     { typedef U     type; };
template<typename U>             struct type_info_cast_pointer_to_void<std::true_type,  U> { typedef void* type; };

// _ACC form assumes 'offset' and this will increment that value as it appends member strings
#define STRUCTF_BEGIN(dst__, dst_size__, indent__, label__, ptr__) snprintf(dst__, dst_size__, "%.*s%s 0x%p\n{\n", 2*__min(8, indent__), "                ", (label__) ? (label__) : "?", ptr__);
#define STRUCTF_BEGIN_ACC(dst__, dst_size__, indent__, label__, ptr__) offset += STRUCTF_BEGIN((dst__) + offset, (dst_size__) - offset, indent__, label__, ptr__)
#define STRUCTF_END(dst__, dst_size__, indent__, label__, ptr__) snprintf(dst__, dst_size__, "%.*s}\n", 2*__min(8, indent__), "                ")
#define STRUCTF_END_ACC(dst__, dst_size__, indent__, label__, ptr__) offset += STRUCTF_END((dst__) + offset, (dst_size__) - offset, indent__, label__, ptr__)
#define FIELDF(dst__, dst_size__, indent__, member__) field_snprintf(dst__, dst_size__, #member__, indent__, ouro::type_info<type_info_cast_pointer_to_void<std::is_pointer<std::remove_reference<decltype(member__)>::type>::type, std::remove_reference<decltype(member__)>::type>::type>::to_string, &(member__))
#define FIELDF_ACC(dst__, dst_size__, indent__, member__) offset += FIELDF((dst__) + offset, (dst_size__) - offset, indent__, member__)

}
