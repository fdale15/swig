/* -----------------------------------------------------------------------------
 * javahead.swg
 *
 * Java support code
 * ----------------------------------------------------------------------------- */


/* JNI function calls require different calling conventions for C and C++. These JCALL macros are used so 
 * that the same typemaps can be used for generating code for both C and C++. The SWIG preprocessor can expand
 * the macros thereby generating the correct calling convention. It is thus essential that all typemaps that
 * use the macros are not within %{ %} brackets as they won't be run through the SWIG preprocessor. */
#ifdef __cplusplus
#   define JCALL0(func, jenv) jenv->func()
#   define JCALL1(func, jenv, ar1) jenv->func(ar1)
#   define JCALL2(func, jenv, ar1, ar2) jenv->func(ar1, ar2)
#   define JCALL3(func, jenv, ar1, ar2, ar3) jenv->func(ar1, ar2, ar3)
#   define JCALL4(func, jenv, ar1, ar2, ar3, ar4) jenv->func(ar1, ar2, ar3, ar4)
#   define JCALL5(func, jenv, ar1, ar2, ar3, ar4, ar5) jenv->func(ar1, ar2, ar3, ar4, ar5)
#   define JCALL6(func, jenv, ar1, ar2, ar3, ar4, ar5, ar6) jenv->func(ar1, ar2, ar3, ar4, ar5, ar6)
#   define JCALL7(func, jenv, ar1, ar2, ar3, ar4, ar5, ar6, ar7) jenv->func(ar1, ar2, ar3, ar4, ar5, ar6, ar7)
#else
#   define JCALL0(func, jenv) (*jenv)->func(jenv)
#   define JCALL1(func, jenv, ar1) (*jenv)->func(jenv, ar1)
#   define JCALL2(func, jenv, ar1, ar2) (*jenv)->func(jenv, ar1, ar2)
#   define JCALL3(func, jenv, ar1, ar2, ar3) (*jenv)->func(jenv, ar1, ar2, ar3)
#   define JCALL4(func, jenv, ar1, ar2, ar3, ar4) (*jenv)->func(jenv, ar1, ar2, ar3, ar4)
#   define JCALL5(func, jenv, ar1, ar2, ar3, ar4, ar5) (*jenv)->func(jenv, ar1, ar2, ar3, ar4, ar5)
#   define JCALL6(func, jenv, ar1, ar2, ar3, ar4, ar5, ar6) (*jenv)->func(jenv, ar1, ar2, ar3, ar4, ar5, ar6)
#   define JCALL7(func, jenv, ar1, ar2, ar3, ar4, ar5, ar6, ar7) (*jenv)->func(jenv, ar1, ar2, ar3, ar4, ar5, ar6, ar7)
#endif

 %insert(runtime) %{
#include <stdlib.h>
#include <string.h>
%}
