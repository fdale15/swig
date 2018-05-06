/* -----------------------------------------------------------------------------
 * enumtypesafe.swg
 *
 * Include this file in order for C/C++ enums to be wrapped by the so called
 * typesafe enum pattern. Each enum has an equivalent Java class named after the
 * enum and each enum item is a static instance of this class.
 * ----------------------------------------------------------------------------- */

// const enum SWIGTYPE & typemaps
%typemap(jni) const enum SWIGTYPE & "int"
%typemap(jtype) const enum SWIGTYPE & "int"
%typemap(jstype) const enum SWIGTYPE & "$*javaclassname"

%typemap(in) const enum SWIGTYPE & ($*1_ltype temp)
%{ temp = ($*1_ltype)$input; 
   $1 = &temp; %}
%typemap(out) const enum SWIGTYPE &  %{ $result = (int)*$1; %}

%typemap(directorout,warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const enum SWIGTYPE &
%{ static $*1_ltype temp = ($*1_ltype)$input; 
   $result = &temp; %}
%typemap(directorin, descriptor="L$packagepath/$*javaclassname;") const enum SWIGTYPE & "$input = (int)$1;"
%typemap(javadirectorin) const enum SWIGTYPE & "$*javaclassname.swigToEnum($jniinput)"
%typemap(javadirectorout) const enum SWIGTYPE & "($javacall).swigValue()"

%typecheck(SWIG_TYPECHECK_POINTER) const enum SWIGTYPE & ""

%typemap(throws) const enum SWIGTYPE &
%{ (void)$1;
   SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, "C++ $1_type exception thrown"); %}

%typemap(javain) const enum SWIGTYPE & "$javainput.swigValue()"
%typemap(javaout) const enum SWIGTYPE & {
    return $*javaclassname.swigToEnum($jnicall);
  }

// enum SWIGTYPE typemaps
%typemap(jni) enum SWIGTYPE "int"
%typemap(jtype) enum SWIGTYPE "int"
%typemap(jstype) enum SWIGTYPE "$javaclassname"

%typemap(in) enum SWIGTYPE  %{ $1 = ($1_ltype)$input; %}
%typemap(out) enum SWIGTYPE  %{ $result = (int)$1; %}

%typemap(directorout) enum SWIGTYPE  %{ $result = ($1_ltype)$input; %}
%typemap(directorin, descriptor="L$packagepath/$javaclassname;") enum SWIGTYPE "$input = (int) $1;"
%typemap(javadirectorin) enum SWIGTYPE "$javaclassname.swigToEnum($jniinput)"
%typemap(javadirectorout) enum SWIGTYPE "($javacall).swigValue()"

%typecheck(SWIG_TYPECHECK_POINTER) enum SWIGTYPE ""

%typemap(throws) enum SWIGTYPE
%{ (void)$1;
   SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, "C++ $1_type exception thrown"); %}

%typemap(javain) enum SWIGTYPE "$javainput.swigValue()"
%typemap(javaout) enum SWIGTYPE {
    return $javaclassname.swigToEnum($jnicall);
  }

// '$static' will be replaced with either 'static' or nothing depending on whether the enum is an inner Java class or not
%typemap(javaclassmodifiers) enum SWIGTYPE "public class"
%typemap(javabase)           enum SWIGTYPE ""
%typemap(javacode)           enum SWIGTYPE ""
%typemap(javaimports)        enum SWIGTYPE ""
%typemap(javainterfaces)     enum SWIGTYPE ""

/*
 * The swigToEnum method is used to find the Java enum from a C++ enum integer value. The default one here takes 
 * advantage of the fact that most enums do not have initial values specified, so the lookup is fast. If initial
 * values are specified then a lengthy linear search through all possible enums might occur. Specific typemaps could be
 * written to possibly optimise this lookup by taking advantage of characteristics peculiar to the targeted enum.
 * The special variable, $enumvalues, is replaced with a comma separated list of all the enum values.
 */
%typemap(javabody) enum SWIGTYPE %{
  public final func swigEnumValue() -> Int32 {
    return swigValue
  }

  public func toString() -> String {
    return swigName
  }

  public static func swigToEnum(swigValue: Int32) -> $javaclassname {
    if (swigValue < swigValues.count && swigValue >= 0 && $javaclassname.swigValues[Int(swigValue)].swigValue == swigValue) {
      return $javaclassname.swigValues[Int(swigValue)]
    }
    for i in 0...(swigValues.count - 1) {
      if ($javaclassname.swigValues[i].swigValue == swigValue) {
        return $javaclassname.swigValues[i]
      }
    }
    //TODO: Exception handling.
    //throw new IllegalArgumentException("No enum " + $javaclassname.class + " with value " + swigValue);
  }

  private init(_ swigName: String) {
    self.swigName = swigName
    self.swigValue = $javaclassname.swigNext
    $javaclassname.swigNext = $javaclassname.swigNext + 1
  }

  private init(_ swigName: String, _ swigValue: Int32) {
    self.swigName = swigName
    self.swigValue = swigValue
    $javaclassname.swigNext = swigValue+1
  }

  private init(_ swigName: String, _ swigEnum: $javaclassname) {
    self.swigName = swigName
    self.swigValue = swigEnum.swigValue
    $javaclassname.swigNext = self.swigValue+1
  }

  private static let swigValues: [$javaclassname] = [ $enumvalues ];
  private static var swigNext: Int32 = 0;
  private final  var swigValue: Int32;
  private final  var swigName: String;
%}

%javaenum(typesafe);

