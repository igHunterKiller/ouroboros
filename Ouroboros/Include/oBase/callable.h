// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Pre-variadic templates we need to just type out templates with different 
// counts of parameters, so here it is. This facilitates the std::thread pattern 
// of wrapping the need for std::bind inside the calls to functions that take 
// functors.

#pragma once
#include <functional>

#define oCALLABLE_CONCAT(x, y) x##y

// Callable pattern: a workaround for not having variadic templates. This 
// pattern seems to be emerging from C++11 APIs, so support it ahead of compiler
// support in this way.
#ifndef oHAS_VARIADIC_TEMPLATES

	#define oARG_TYPENAMES0
	#define oARG_TYPENAMES1 typename Arg0
	#define oARG_TYPENAMES2 oARG_TYPENAMES1, typename Arg1
	#define oARG_TYPENAMES3 oARG_TYPENAMES2, typename Arg2
	#define oARG_TYPENAMES4 oARG_TYPENAMES3, typename Arg3
	#define oARG_TYPENAMES5 oARG_TYPENAMES4, typename Arg4
	#define oARG_TYPENAMES6 oARG_TYPENAMES5, typename Arg5
	#define oARG_TYPENAMES7 oARG_TYPENAMES6, typename Arg6
	#define oARG_TYPENAMES8 oARG_TYPENAMES7, typename Arg7
	#define oARG_TYPENAMES9 oARG_TYPENAMES8, typename Arg8
	#define oARG_TYPENAMES10 oARG_TYPENAMES9, typename Arg9

	#define oARG_COMMA_TYPENAMES0
	#define oARG_COMMA_TYPENAMES1 , typename Arg0
	#define oARG_COMMA_TYPENAMES2 oARG_COMMA_TYPENAMES1, typename Arg1
	#define oARG_COMMA_TYPENAMES3 oARG_COMMA_TYPENAMES2, typename Arg2
	#define oARG_COMMA_TYPENAMES4 oARG_COMMA_TYPENAMES3, typename Arg3
	#define oARG_COMMA_TYPENAMES5 oARG_COMMA_TYPENAMES4, typename Arg4
	#define oARG_COMMA_TYPENAMES6 oARG_COMMA_TYPENAMES5, typename Arg5
	#define oARG_COMMA_TYPENAMES7 oARG_COMMA_TYPENAMES6, typename Arg6
	#define oARG_COMMA_TYPENAMES8 oARG_COMMA_TYPENAMES7, typename Arg7
	#define oARG_COMMA_TYPENAMES9 oARG_COMMA_TYPENAMES8, typename Arg8
	#define oARG_COMMA_TYPENAMES10 oARG_COMMA_TYPENAMES9, typename Arg9
	
	#define oARG_DECL0
	#define oARG_DECL1 Arg0 Arg0__
	#define oARG_DECL2 oARG_DECL1, Arg1 Arg1__
	#define oARG_DECL3 oARG_DECL2, Arg2 Arg2__
	#define oARG_DECL4 oARG_DECL3, Arg3 Arg3__
	#define oARG_DECL5 oARG_DECL4, Arg4 Arg4__
	#define oARG_DECL6 oARG_DECL5, Arg5 Arg5__
	#define oARG_DECL7 oARG_DECL6, Arg6 Arg6__
	#define oARG_DECL8 oARG_DECL7, Arg7 Arg7__
	#define oARG_DECL9 oARG_DECL8, Arg8 Arg8__
	#define oARG_DECL10 oARG_DECL9, Arg9 Arg9__

	#define oARG_PASS0
	#define oARG_PASS1 Arg0__
	#define oARG_PASS2 oARG_PASS1, Arg1__
	#define oARG_PASS3 oARG_PASS2, Arg2__
	#define oARG_PASS4 oARG_PASS3, Arg3__
	#define oARG_PASS5 oARG_PASS4, Arg4__
	#define oARG_PASS6 oARG_PASS5, Arg5__
	#define oARG_PASS7 oARG_PASS6, Arg6__
	#define oARG_PASS8 oARG_PASS7, Arg7__
	#define oARG_PASS9 oARG_PASS8, Arg8__
	#define oARG_PASS10 oARG_PASS9, Arg9__
	
	#define oARG_COMMA_PASS0
	#define oARG_COMMA_PASS1 , Arg0__
	#define oARG_COMMA_PASS2 oARG_COMMA_PASS1, Arg1__
	#define oARG_COMMA_PASS3 oARG_COMMA_PASS2, Arg2__
	#define oARG_COMMA_PASS4 oARG_COMMA_PASS3, Arg3__
	#define oARG_COMMA_PASS5 oARG_COMMA_PASS4, Arg4__
	#define oARG_COMMA_PASS6 oARG_COMMA_PASS5, Arg5__
	#define oARG_COMMA_PASS7 oARG_COMMA_PASS6, Arg6__
	#define oARG_COMMA_PASS8 oARG_COMMA_PASS7, Arg7__
	#define oARG_COMMA_PASS9 oARG_COMMA_PASS8, Arg8__
	#define oARG_COMMA_PASS10 oARG_COMMA_PASS9, Arg9__

	#define oARG_PARTIAL_TYPENAMES0 
	#define oARG_PARTIAL_TYPENAMES1 Arg0
	#define oARG_PARTIAL_TYPENAMES2 oARG_PARTIAL_TYPENAMES1, Arg1
	#define oARG_PARTIAL_TYPENAMES3 oARG_PARTIAL_TYPENAMES2, Arg2
	#define oARG_PARTIAL_TYPENAMES4 oARG_PARTIAL_TYPENAMES3, Arg3
	#define oARG_PARTIAL_TYPENAMES5 oARG_PARTIAL_TYPENAMES4, Arg4
	#define oARG_PARTIAL_TYPENAMES6 oARG_PARTIAL_TYPENAMES5, Arg5
	#define oARG_PARTIAL_TYPENAMES7 oARG_PARTIAL_TYPENAMES6, Arg6
	#define oARG_PARTIAL_TYPENAMES8 oARG_PARTIAL_TYPENAMES7, Arg7
	#define oARG_PARTIAL_TYPENAMES9 oARG_PARTIAL_TYPENAMES8, Arg8
	#define oARG_PARTIAL_TYPENAMES10 oARG_PARTIAL_TYPENAMES9, Arg9

	#define oCALLABLE_TEMPLATE0 template<oCALLABLE_ARG_TYPENAMES0>
	#define oCALLABLE_TEMPLATE1 template<oCALLABLE_ARG_TYPENAMES1>
	#define oCALLABLE_TEMPLATE2 template<oCALLABLE_ARG_TYPENAMES2>
	#define oCALLABLE_TEMPLATE3 template<oCALLABLE_ARG_TYPENAMES3>
	#define oCALLABLE_TEMPLATE4 template<oCALLABLE_ARG_TYPENAMES4>
	#define oCALLABLE_TEMPLATE5 template<oCALLABLE_ARG_TYPENAMES5>
	#define oCALLABLE_TEMPLATE6 template<oCALLABLE_ARG_TYPENAMES6>
	#define oCALLABLE_TEMPLATE7 template<oCALLABLE_ARG_TYPENAMES7>
	#define oCALLABLE_TEMPLATE8 template<oCALLABLE_ARG_TYPENAMES8>
	#define oCALLABLE_TEMPLATE9 template<oCALLABLE_ARG_TYPENAMES9>
	#define oCALLABLE_TEMPLATE10 template<oCALLABLE_ARG_TYPENAMES10>

	#define oCALLABLE_ARG_TYPENAMES0 typename Callable
	#define oCALLABLE_ARG_TYPENAMES1 typename Callable, oARG_TYPENAMES1
	#define oCALLABLE_ARG_TYPENAMES2 typename Callable, oARG_TYPENAMES2
	#define oCALLABLE_ARG_TYPENAMES3 typename Callable, oARG_TYPENAMES3
	#define oCALLABLE_ARG_TYPENAMES4 typename Callable, oARG_TYPENAMES4
	#define oCALLABLE_ARG_TYPENAMES5 typename Callable, oARG_TYPENAMES5
	#define oCALLABLE_ARG_TYPENAMES6 typename Callable, oARG_TYPENAMES6
	#define oCALLABLE_ARG_TYPENAMES7 typename Callable, oARG_TYPENAMES7
	#define oCALLABLE_ARG_TYPENAMES8 typename Callable, oARG_TYPENAMES8
	#define oCALLABLE_ARG_TYPENAMES9 typename Callable, oARG_TYPENAMES9
	#define oCALLABLE_ARG_TYPENAMES10 typename Callable, oARG_TYPENAMES10

	#define oCALLABLE_ARG_TYPENAMES_PASS0 Callable
	#define oCALLABLE_ARG_TYPENAMES_PASS1 Callable, Arg0
	#define oCALLABLE_ARG_TYPENAMES_PASS2 oCALLABLE_ARG_TYPENAMES_PASS1, Arg1
	#define oCALLABLE_ARG_TYPENAMES_PASS3 oCALLABLE_ARG_TYPENAMES_PASS2, Arg2
	#define oCALLABLE_ARG_TYPENAMES_PASS4 oCALLABLE_ARG_TYPENAMES_PASS3, Arg3
	#define oCALLABLE_ARG_TYPENAMES_PASS5 oCALLABLE_ARG_TYPENAMES_PASS4, Arg4
	#define oCALLABLE_ARG_TYPENAMES_PASS6 oCALLABLE_ARG_TYPENAMES_PASS5, Arg5
	#define oCALLABLE_ARG_TYPENAMES_PASS7 oCALLABLE_ARG_TYPENAMES_PASS6, Arg6
	#define oCALLABLE_ARG_TYPENAMES_PASS8 oCALLABLE_ARG_TYPENAMES_PASS7, Arg7
	#define oCALLABLE_ARG_TYPENAMES_PASS9 oCALLABLE_ARG_TYPENAMES_PASS8, Arg8
	#define oCALLABLE_ARG_TYPENAMES_PASS10 oCALLABLE_ARG_TYPENAMES_PASS9, Arg9

	#define oCALLABLE_PARAMS_FN Callable Function__
	#define oCALLABLE_PARAMS0 oCALLABLE_PARAMS_FN
	#define oCALLABLE_PARAMS1 oCALLABLE_PARAMS_FN, oARG_DECL1
	#define oCALLABLE_PARAMS2 oCALLABLE_PARAMS_FN, oARG_DECL2
	#define oCALLABLE_PARAMS3 oCALLABLE_PARAMS_FN, oARG_DECL3
	#define oCALLABLE_PARAMS4 oCALLABLE_PARAMS_FN, oARG_DECL4
	#define oCALLABLE_PARAMS5 oCALLABLE_PARAMS_FN, oARG_DECL5
	#define oCALLABLE_PARAMS6 oCALLABLE_PARAMS_FN, oARG_DECL6
	#define oCALLABLE_PARAMS7 oCALLABLE_PARAMS_FN, oARG_DECL7
	#define oCALLABLE_PARAMS8 oCALLABLE_PARAMS_FN, oARG_DECL8
	#define oCALLABLE_PARAMS9 oCALLABLE_PARAMS_FN, oARG_DECL9
	#define oCALLABLE_PARAMS10 oCALLABLE_PARAMS_FN, oARG_DECL10

	#define oCONST_REF_CALLABLE_PARAMS_FN const Callable& Function__
	#define oCONST_REF_CALLABLE_PARAMS0 oCONST_REF_CALLABLE_PARAMS_FN
	#define oCONST_REF_CALLABLE_PARAMS1 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL1
	#define oCONST_REF_CALLABLE_PARAMS2 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL2
	#define oCONST_REF_CALLABLE_PARAMS3 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL3
	#define oCONST_REF_CALLABLE_PARAMS4 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL4
	#define oCONST_REF_CALLABLE_PARAMS5 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL5
	#define oCONST_REF_CALLABLE_PARAMS6 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL6
	#define oCONST_REF_CALLABLE_PARAMS7 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL7
	#define oCONST_REF_CALLABLE_PARAMS8 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL8
	#define oCONST_REF_CALLABLE_PARAMS9 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL9
	#define oCONST_REF_CALLABLE_PARAMS10 oCONST_REF_CALLABLE_PARAMS_FN, oARG_DECL10

	#define oMOVABLE_CALLABLE_PARAMS_FN Callable&& Function__
	#define oMOVABLE_CALLABLE_PARAMS0 oMOVABLE_CALLABLE_PARAMS_FN
	#define oMOVABLE_CALLABLE_PARAMS1 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL1
	#define oMOVABLE_CALLABLE_PARAMS2 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL2
	#define oMOVABLE_CALLABLE_PARAMS3 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL3
	#define oMOVABLE_CALLABLE_PARAMS4 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL4
	#define oMOVABLE_CALLABLE_PARAMS5 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL5
	#define oMOVABLE_CALLABLE_PARAMS6 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL6
	#define oMOVABLE_CALLABLE_PARAMS7 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL7
	#define oMOVABLE_CALLABLE_PARAMS8 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL8
	#define oMOVABLE_CALLABLE_PARAMS9 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL9
	#define oMOVABLE_CALLABLE_PARAMS10 oMOVABLE_CALLABLE_PARAMS_FN, oARG_DECL10

	#define oCALLABLE_PASS0 Function__
	#define oCALLABLE_PASS1 Function__, oARG_PASS1
	#define oCALLABLE_PASS2 Function__, oARG_PASS2
	#define oCALLABLE_PASS3 Function__, oARG_PASS3
	#define oCALLABLE_PASS4 Function__, oARG_PASS4
	#define oCALLABLE_PASS5 Function__, oARG_PASS5
	#define oCALLABLE_PASS6 Function__, oARG_PASS6
	#define oCALLABLE_PASS7 Function__, oARG_PASS7
	#define oCALLABLE_PASS8 Function__, oARG_PASS8
	#define oCALLABLE_PASS9 Function__, oARG_PASS9
	#define oCALLABLE_PASS10 Function__, oARG_PASS10

	#define oCALLABLE_BIND0 Function__
	#define oCALLABLE_BIND1 std::bind(Function__, oARG_PASS1)
	#define oCALLABLE_BIND2 std::bind(Function__, oARG_PASS2)
	#define oCALLABLE_BIND3 std::bind(Function__, oARG_PASS3)
	#define oCALLABLE_BIND4 std::bind(Function__, oARG_PASS4)
	#define oCALLABLE_BIND5 std::bind(Function__, oARG_PASS5)
	#define oCALLABLE_BIND6 std::bind(Function__, oARG_PASS6)
	#define oCALLABLE_BIND7 std::bind(Function__, oARG_PASS7)
	#define oCALLABLE_BIND8 std::bind(Function__, oARG_PASS8)
	#define oCALLABLE_BIND9 std::bind(Function__, oARG_PASS9)
	#define oCALLABLE_BIND10 std::bind(Function__, oARG_PASS10)

	#define oCALLABLE_CALL0 Function__(oARG_PASS0)
	#define oCALLABLE_CALL1 Function__(oARG_PASS1)
	#define oCALLABLE_CALL2 Function__(oARG_PASS2)
	#define oCALLABLE_CALL3 Function__(oARG_PASS3)
	#define oCALLABLE_CALL4 Function__(oARG_PASS4)
	#define oCALLABLE_CALL5 Function__(oARG_PASS5)
	#define oCALLABLE_CALL6 Function__(oARG_PASS6)
	#define oCALLABLE_CALL7 Function__(oARG_PASS7)
	#define oCALLABLE_CALL8 Function__(oARG_PASS8)
	#define oCALLABLE_CALL9 Function__(oARG_PASS9)
	#define oCALLABLE_CALL10 Function__(oARG_PASS10)

	#define oCALLABLE_RETURN_TYPE0 typename std::result_of<Callable()>::type
	#define oCALLABLE_RETURN_TYPE1 typename std::result_of<Callable(Arg0)>::type
	#define oCALLABLE_RETURN_TYPE2 typename std::result_of<Callable(Arg0,Arg1)>::type
	#define oCALLABLE_RETURN_TYPE3 typename std::result_of<Callable(Arg0,Arg1,Arg2)>::type
	#define oCALLABLE_RETURN_TYPE4 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3)>::type
	#define oCALLABLE_RETURN_TYPE5 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4)>::type
	#define oCALLABLE_RETURN_TYPE6 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4,Arg5)>::type
	#define oCALLABLE_RETURN_TYPE7 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6)>::type
	#define oCALLABLE_RETURN_TYPE8 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7)>::type
	#define oCALLABLE_RETURN_TYPE9 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7,Arg8)>::type
	#define oCALLABLE_RETURN_TYPE10 typename std::result_of<Callable(Arg0,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7,Arg8,Arg9)>::type

	#define oCALLABLE_PROPAGATE_SKIP0(_Macro) \
		_Macro(1) _Macro(2) _Macro(3) _Macro(4) _Macro(5) _Macro(6) _Macro(7) _Macro(8) _Macro(9) _Macro(10)

	#define oCALLABLE_PROPAGATE(_Macro) _Macro(0) oCALLABLE_PROPAGATE_SKIP0(_Macro)

	#define oDEFINE_CALLABLE_RETURN_WRAPPER(_Index, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) oCALLABLE_CONCAT(oCALLABLE_TEMPLATE, _Index) inline _PublicMethodReturnValue _PublicMethod(oCALLABLE_CONCAT(oCALLABLE_PARAMS, _Index)) _PublicMethodThreadsafety { return _ImplementationMethod(oCALLABLE_CONCAT(oCALLABLE_BIND, _Index)); }
	#define oDEFINE_CALLABLE_WRAPPER(_Index, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) oCALLABLE_CONCAT(oCALLABLE_TEMPLATE, _Index) _Static inline void _PublicMethod(oCALLABLE_CONCAT(oCALLABLE_PARAMS, _Index)) _PublicMethodThreadsafety { _ImplementationMethod(oCALLABLE_CONCAT(oCALLABLE_BIND, _Index)); }
	#define oDEFINE_CALLABLE_CTOR_WRAPPER(_Index, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) oCALLABLE_CONCAT(oCALLABLE_TEMPLATE, _Index) _PublicCtorExplicit _PublicCtor(oCALLABLE_CONCAT(oCALLABLE_PARAMS, _Index)) { _ImplementationMethod(oCALLABLE_CONCAT(oCALLABLE_BIND, _Index)); }

	#define oDEFINE_CALLABLE_RETURN_WRAPPERS(_PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(1, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(2, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(3, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(4, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(5, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(6, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(7, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(8, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(9, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_RETURN_WRAPPER(10, _PublicMethodReturnValue, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod)

	#define oDEFINE_CALLABLE_WRAPPERS(_Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(1, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(2, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(3, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(4, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(5, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(6, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(7, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(8, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(9, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod) \
		oDEFINE_CALLABLE_WRAPPER(10, _Static, _PublicMethod, _PublicMethodThreadsafety, _ImplementationMethod)

	#define oDEFINE_CALLABLE_CTOR_WRAPPERS(_PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(0, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(1, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(2, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(3, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(4, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(5, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(6, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(7, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(8, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(9, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod) \
		oDEFINE_CALLABLE_CTOR_WRAPPER(10, _PublicCtorExplicit, _PublicCtor, _ImplementationMethod)

#else
	#error TODO: Implement all uses of the Callable pattern with true variadic macros.
#endif
