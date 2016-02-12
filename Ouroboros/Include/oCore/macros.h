// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Collection of primitive macros useful in many system-level cases

#pragma once
#include <cstddef>

// _____________________________________________________________________________
// Preprocessor macros

// Creates a single symbol from the two specified symbols
#define oCONCAT(x, y) x##y

// Safely converts the specified value into a string at pre-processor time
#define oINTERNAL_STRINGIZE__(x) #x
#define oSTRINGIZE(x) oINTERNAL_STRINGIZE__(x)

// _____________________________________________________________________________
// String check macros

// Wrappers that should be used to protect against null pointers to strings
#define oSAFESTR(str)  ((str) ? (str) : "")
#define oSAFESTRN(str) ((str) ? (str) : "(null)")

// It is often used to test for a null or empty string, so encapsulate the 
// pattern in a more self-documenting macro.
#define oSTRVALID(str) ((str) && (str)[0] != '\0')

// _____________________________________________________________________________
// Constant/parameter macros

// For signed (int) values, here is an outrageously negative number. Use this as 
// a special value to indicate 'default'.
#define oDEFAULT 0x80000000

// _____________________________________________________________________________
// Handle macros

// Encapsulate the pattern of declaring typed handles by defining a typed pointer
#define oDECLARE_HANDLE(handle_name)                                   typedef struct handle_name##__tag {}* handle_name;
#define oDECLARE_DERIVED_HANDLE(base_handle_name, derived_handle_name) typedef struct derived_handle_name##__tag : public base_handle_name##__tag {}* derived_handle_name;
