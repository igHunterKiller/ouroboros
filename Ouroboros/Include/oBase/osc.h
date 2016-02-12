// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// NOTE: This API tries to conform to the vocabulary used in the definition of 
// both OSC 1.0 and OSC 1.1 specifications.
// http://opensoundcontrol.org/spec-1_0
// http://cnmat.berkeley.edu/system/files/attachments/Nime09OSCfinal.pdf
// IN ADDITION:
// This implementation supports the following type tags:
// [1-9]: A single digit will be interpreted as a fixed char array in multiples 
//        of 64 chars. This way, small fixed-size strings can be inlined. The 
//        buffer will be treated as a string, so serialization to a buffer is no 
//        different than with 's', but this allows some avoidance of char 
//        pointers in relatively simple cases such as small names and paths. The
//        field_size param will still be strlen()+1.

#pragma once
#include <oBase/date.h>
#include <functional>

namespace ouro { namespace osc {

static const int fixed_string_max_length = 9*64;

// type: an OSC Type Tag identifier. field: points to the buffer containing the 
// data, i.e. it will point to an intrinsic type for intrinsics or directly to a 
// string or blob. field_size represents the size of the buffer pointed at by 
// field. For strings, this will just be the sizeof(char*) as field must be 
// dereferenced to get to the actual ASCII string. For blobs this is the size of 
// the blob. For char the size is sizeof(char). No OSC padding requirements are 
// considered in data preparation, only use of OSC type tags to describe traversal 
// of a C++ struct.
typedef std::function<void(int type, void* field, size_t field_size)> visitor_fn;
typedef std::function<void(int type, const void* field, size_t field_size)> visitor_const_fn;

// This function correctly traverses fields in a specified struct that is 
// trivial, meaning:
// o Struct does not inherit from any base struct, so has no end-of-struct 
//   alignment issues
// o Struct does not have a vtable, either inherited or direct
// o Struct does not compile under any user-specialized pragma packs (though this may work, it's just not been tested at this time)
//
// Visiting fields is done according to specified the OSC 1.1 compliant TypeTags 
// string. Support for the optional common types described in the OSC 1.0
// specification under "OSC Type Tag String" EXCEPT for MIDI message m because 
// currently I don't time/a way for testing proper receipt of MIDI.
//
// Fields such as '[' or 'T' are also traversed with a nullptr value for field 
// and 0 value for field_size.
//
// Important Reminders from the OSC 1.1 spec:
// o All blob data must be described in the struct by an int followed by a void*
// o All ints and longlongs are considered signed - there is no guarantee of 
//   unsigned support, though most operations are just size-based pass-through 
//   operations/copies to client code.
bool visit_struct_fields(const char* typetags, void* _struct, size_t struct_size, const visitor_fn& visitor);
bool visit_struct_fields(const char* typetags, const void* _struct, size_t struct_size, const visitor_const_fn& visitor);
template<typename T> inline bool visit_struct_fields(const char* typetags, const T* _struct, const visitor_fn& visitor) { return visit_struct_fields(typetags, _struct, sizeof(T), _Visitor); }

// This function traverses an OSC 1.1 compliant message's arguments list 
// according to the specified type tags and calls the specified visitor on each
// field.
bool visit_message_type_tags(const char* typetags, void* msg_args, const visitor_fn& visitor);
bool visit_message_type_tags(const char* typetags, const void* msg_args, const visitor_const_fn& visitor);

// Returns the number of fields described by the type tags (ignores commands,
// array delimiters, counts arrays as one field etc.)
size_t calc_num_fields(const char* typetags);

// Given the specified type tags and struct (need to traverse string lengths and
// blob sizes) return the size of a buffer fit for use in allocating the 
// arguments portion of an OSC message. This takes into consideration the extra
// bytes imposed by OSC alignment requirements such as extra nuls on the end of
// a string to ensure 32-bit alignment.
size_t calc_args_data_size(const char* typetags, const void* _struct, size_t struct_size);
template<typename T> inline size_t calc_args_data_size(const char* typetags, const T& _struct) { return calc_args_data_size(typetags, &_struct, sizeof(T)); }

// Calculate the size of a buffer required to store the specified message. 
// _ArgumentsDataSize is the result of the calc_args_data_size()
// function. This takes into consideration the extra bytes imposed by OSC 
// alignment requirements such as extra nuls on the end of a string to ensure 
// 32-bit alignment.
size_t calc_msg_size(const char* address, const char* typetags, size_t _ArgumentsDataSize);

// Calculate the size of a buffer required to store the specified bundles. 
// The value for _SumOfAllSubbundleSizes should be the sum of various calls to
// calc_msg_size() and calc_bundle_size() for recursive 
// bundles.
size_t calc_bundle_size(size_t _NumSubbundles, size_t _SumOfAllSubbundleSizes);

// Calculate the size of the struct needed to hold the deserialized messgage.
// This does not include the memory needed to hold strings or blobs, just
// the size of the pointers and sizes.
size_t calc_deserialized_struct_size(const char* typetags);

// Returns the message's address so it can be redirected as appropriate. This
// will return nullptr if the message is not well formed (address doesn't begin
// with a '/')
const char* get_msg_address(const void* msg);

// Returns the message's type tags. This will return a nullptr if the specified
// message does not have a valid address or if the type tags is not specified/
// valid (does not begin with ',').
const char* get_msg_type_tags(const void* msg);

// Takes the contents of the specified struct and creates an OSC 1.1 compliant
// message in msg. The typetags are used to interpret the data in
// struct. When T or F is encountered in the type tags the type tags in the
// message are patched to represent either true or false based on the underlying
// data in the structure. size of the entire message written is returned.
size_t serialize_struct_to_msg(const char* address, const char* typetags
	, const void* _struct, size_t struct_size, void* msg, size_t msg_size);
template<typename T> inline size_t serialize_struct_to_msg(const char* address, const char* typetags
	, const T& _struct, void* msg, size_t msg_size) { return serialize_struct_to_msg(address, typetags, &_struct, sizeof(T), msg, msg_size); }

// Interpret the specified buffer as an OSC 1.1 compliant message and fill the 
// specified buffer with its contents. struct is required to be
// 4-byte aligned. Any strings and or blobs will point back into the original message.
bool deserialize_msg_to_struct(const void* msg, void* _struct, size_t struct_size);
template<typename T> inline bool deserialize_msg_to_struct(const void* msg, T* _struct) { return deserialize_msg_to_struct(msg, _struct, sizeof(T)); }

// Prefer using this to verify an expected type tag versus a messages type tags
// over a string compare because we bake bool values into the type tag itself
// using T/F since there is no bool type in OSC, so T==F F==T T==T F==F in this
// compare.
bool type_tags_match(const char* typetags0, const char* typetags1);

// Returns true if the specified packet is an OSC message as defined in the spec.
inline bool is_msg(const void* osc_packet) { return osc_packet && *(const char*)osc_packet == '/'; }

// Returns true if the specified packet is an OSC bundle as defined in the spec.
inline bool is_bundle(const void* osc_packet) { return osc_packet && *(const char*)osc_packet == '#'; }

// Returns the timestamp in the specified bundle.
ouro::ntp_timestamp get_bundle_timestamp(const void* osc_bundle);

// Like strtok, this iterates through all subbundles found inside an OSC packet
// that IS a bundle. Per the spec, size must be provided by either the packet or
// the underlying transit protocol. This function is not threadsafe. If 
// iteration exits before the last subbundle is reached, then close_tokenize must 
// be called explicitly, otherwise it is called automatically.
// NOTE: Because bundles can contain other bundles, tokenize should be used to 
// set up each one, and each one should have its own context.
const void* tokenize(const void* osc_packet, size_t osc_packet_size, void** out_ctx);

// This is only required if iteration using tokenize is exited early (error?). 
// A context is automatically closed when tokenize returns nullptr due to being
// finished iterating.
void close_tokenize(void** out_ctx);

}}
