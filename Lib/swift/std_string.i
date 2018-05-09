/* -----------------------------------------------------------------------------
 * std_string.i
 *
 * Typemaps for std::string and const std::string&
 * These are mapped to a Java String and are passed around by value.
 *
 * To use non-const std::string references use the following %apply.  Note 
 * that they are passed by value.
 * %apply const std::string & {std::string &};
 * ----------------------------------------------------------------------------- */

%{
#include <string>
%}

namespace std {

%naturalvar string;

class string;

// string
%typemap(jni) string "char *"
%typemap(jni) string & "char *"
%typemap(jni) string const "const char *"

%typemap(jtype) string       "const char *"
%typemap(jtype) string const "const char *"
%typemap(jtype) string &      "const char *"

%typemap(jstype) string "String"
%typemap(jstype) string * "String"
%typemap(jstype) string & "String"

%typemap(javadirectorin) string "$jniinput"
%typemap(javadirectorout) string "$javacall"

%typemap(in) string 
%{  if(!$input) {
       assert(false);
     return $null;
    }

    if (!$input) return $null;
    $1.assign($input);

    //I think swift will take care of this.
    // jenv->ReleaseStringUTFChars($input, $1_pstr);
%}


%typemap(out) string 
%{ 
    $result = new char[result.size()+1];
    std::strcpy(const_cast<char*>($result), result.c_str());
%}

%typemap(javain) string "$javainput"

%typemap(javaout) string {
    return String.init(cString: $jnicall)
}

%typemap(javaout) string *{
    return String.init(cString: $jnicall)
}

%typemap(javaout) string &{
    return String.init(cString: $jnicall)
}

%typemap(javaout) string const {
    //Maybe we should do all strings this way and hinge the delete behavior based on references.
    let ptr = $jnicall
    return String.init(bytesNoCopy: UnsafeMutableRawPointer(mutating: ptr!), length: strlen(ptr), encoding: String.Encoding.utf8, freeWhenDone: true)!
}

%typemap(typecheck) string = char *;

%typemap(throws) string
%{ //SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

// const string &
%typemap(jni) const string & "const char *"
%typemap(jtype) const string & "const char *"
%typemap(jstype) const string & "String"
%typemap(javadirectorin) const string & "$jniinput"

%typemap(javadirectorout) const string & "$javacall"

%typemap(in) const string &
%{ if(!$input) {
     assert(false);
     return $null;
   }

   $*1_ltype $1_str(p_$1);
   $1 = &$1_str;
%}

%typemap(out) const string & 
%{ $result = $1->c_str(); %}

//Convert the Swift string back to a C-style string.
%typemap(javain) string %{$javainput%}
%typemap(javain) string & %{$javainput)%}
%typemap(javain) string * %{$javainput%}
%typemap(javain) const string & %{$javainput%}

%typemap(typecheck) const string & = char *;

%typemap(throws) const string &
%{ //SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

}

