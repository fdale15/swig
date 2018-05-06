/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3 
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * java.cxx
 *
 * Java language module for SWIG.
 * ----------------------------------------------------------------------------- */

#include "swigmod.h"
#include <limits.h>		// for INT_MAX
#include "cparse.h"
#include <ctype.h>

/* Hash type used for upcalls from C/C++ */
typedef DOH UpcallData;

class SWIFT:public Language {
    static const char *usage;
    const String *empty_string;
    const String *public_string;
    const String *protected_string;

    Hash *swig_types_hash;
    File *f_begin;
    File *f_runtime;
    File *f_runtime_h;
    File *f_header;
    File *f_wrappers;
    File *f_init;
    File *f_directors;
    File *f_directors_h;
    List *filenames_list;

    bool proxy_flag;		// Flag for generating proxy classes
    bool nopgcpp_flag;		// Flag for suppressing the premature garbage collection prevention parameter
    bool native_function_flag;	// Flag for when wrapping a native function
    bool enum_constant_flag;	// Flag for when wrapping an enum or constant
    bool static_flag;		// Flag for when wrapping a static functions or member variables
    bool variable_wrapper_flag;	// Flag for when wrapping a nonstatic member variable
    bool wrapping_member_flag;	// Flag for when wrapping a member variable/enum/const
    bool global_variable_flag;	// Flag for when wrapping a global variable
    bool old_variable_names;	// Flag for old style variable names in the intermediary class
    bool member_func_flag;	// flag set when wrapping a member function

    String *imclass_name;		// intermediary class name
    String *module_class_name;	// module class name
    String *constants_interface_name;	// constants interface name
    String *imclass_class_code;	// intermediary class code
    String *proxy_class_def;
    String *proxy_class_code;
    String *interface_class_code; // if %feature("interface") was declared for a class, here goes the interface declaration
    String *module_class_code;
    String *proxy_class_name;	// proxy class name
    String *full_proxy_class_name;// fully qualified proxy class name when using nspace feature, otherwise same as proxy_class_name
    String *full_imclass_name;	// fully qualified intermediary class name when using nspace feature, otherwise same as imclass_name
    String *variable_name;	//Name of a variable being wrapped
    String *proxy_class_constants_code;
    String *module_class_constants_code;
    String *enum_code;
    String *package;		// Optional package name
    String *jnipackage;		// Package name used in the JNI code
    String *package_path;		// Package name used internally by JNI (slashes)
    String *imclass_imports;	//intermediary class imports from %pragma
    String *module_imports;	//module imports from %pragma
    String *imclass_baseclass;	//inheritance for intermediary class class from %pragma
    String *imclass_package;	//package in which to generate the intermediary class
    String *module_baseclass;	//inheritance for module class from %pragma
    String *imclass_interfaces;	//interfaces for intermediary class class from %pragma
    String *module_interfaces;	//interfaces for module class from %pragma
    String *imclass_class_modifiers;	//class modifiers for intermediary class overriden by %pragma
    String *module_class_modifiers;	//class modifiers for module class overriden by %pragma
    String *upcasts_code;		//C++ casts for inheritance hierarchies C++ code
    String *imclass_cppcasts_code;	//C++ casts up inheritance hierarchies intermediary class code
    String *imclass_directors;	// Intermediate class director code
    String *destructor_call;	//C++ destructor call if any
    String *destructor_throws_clause;	//C++ destructor throws clause if any

    // Director method stuff:
    List *dmethods_seq;
    Hash *dmethods_table;
    int n_dmethods;
    int n_directors;
    int first_class_dmethod;
    int curr_class_dmethod;
    int nesting_depth;

    enum EnumFeature { SimpleEnum, TypeunsafeEnum, TypesafeEnum, ProperEnum };

public:

    /* -----------------------------------------------------------------------------
   * SWIFT()
   * ----------------------------------------------------------------------------- */

    SWIFT():empty_string(NewString("")),
           public_string(NewString("public")),
           protected_string(NewString("protected")),
           swig_types_hash(NULL),
           f_begin(NULL),
           f_runtime(NULL),
           f_runtime_h(NULL),
           f_header(NULL),
           f_wrappers(NULL),
           f_init(NULL),
           f_directors(NULL),
           f_directors_h(NULL),
           filenames_list(NULL),
           proxy_flag(true),
           nopgcpp_flag(false),
           native_function_flag(false),
           enum_constant_flag(false),
           static_flag(false),
           variable_wrapper_flag(false),
           wrapping_member_flag(false),
           global_variable_flag(false),
           old_variable_names(false),
           member_func_flag(false),
           imclass_name(NULL),
           module_class_name(NULL),
           constants_interface_name(NULL),
           imclass_class_code(NULL),
           proxy_class_def(NULL),
           proxy_class_code(NULL),
           interface_class_code(NULL),
           module_class_code(NULL),
           proxy_class_name(NULL),
           full_proxy_class_name(NULL),
           full_imclass_name(NULL),
           variable_name(NULL),
           proxy_class_constants_code(NULL),
           module_class_constants_code(NULL),
           enum_code(NULL),
           package(NULL),
           jnipackage(NULL),
           package_path(NULL),
           imclass_imports(NULL),
           module_imports(NULL),
           imclass_baseclass(NULL),
           imclass_package(NULL),
           module_baseclass(NULL),
           imclass_interfaces(NULL),
           module_interfaces(NULL),
           imclass_class_modifiers(NULL),
           module_class_modifiers(NULL),
           upcasts_code(NULL),
           imclass_cppcasts_code(NULL),
           imclass_directors(NULL),
           destructor_call(NULL),
           destructor_throws_clause(NULL),
           dmethods_seq(NULL),
           dmethods_table(NULL),
           n_dmethods(0),
           n_directors(0),
           first_class_dmethod(0),
           curr_class_dmethod(0),
           nesting_depth(0){
        /* for now, multiple inheritance in directors is disabled, this
       should be easy to implement though */
        director_multiple_inheritance = 0;
        director_language = 1;
    }

    /* -----------------------------------------------------------------------------
   * constructIntermediateClassName()
   *
   * Construct the fully qualified name of the intermidiate class and set
   * the full_imclass_name attribute accordingly.
   * ----------------------------------------------------------------------------- */
    void constructIntermediateClassName(Node *n) {
        String *nspace = Getattr(n, "sym:nspace");

        if (imclass_package && package)
            full_imclass_name = NewStringf("%s.%s.%s", package, imclass_package, imclass_name);
        else if (package && nspace)
            full_imclass_name = NewStringf("%s.%s", package, imclass_name);
        else if (imclass_package)
            full_imclass_name = NewStringf("%s.%s", imclass_package, imclass_name);
        else
            full_imclass_name = NewStringf("%s", imclass_name);

        if (nspace && !package) {
            String *name = Getattr(n, "name") ? Getattr(n, "name") : NewString("<unnamed>");
            Swig_warning(WARN_JAVA_NSPACE_WITHOUT_PACKAGE, Getfile(n), Getline(n),
                         "The nspace feature is used on '%s' without -package. "
                                 "The generated code may not compile as Java does not support types declared in a named package accessing types declared in an unnamed package.\n", name);
        }
    }

    /* -----------------------------------------------------------------------------
   * getProxyName()
   *
   * Test to see if a type corresponds to something wrapped with a proxy class.
   * Return NULL if not otherwise the proxy class name, fully qualified with
   * package name if the nspace feature is used, unless jnidescriptor is true as
   * the package name is handled differently (unfortunately for legacy reasons).
   * ----------------------------------------------------------------------------- */

    String *getProxyName(SwigType *t, bool jnidescriptor = false) {
        String *proxyname = NULL;
        if (proxy_flag) {
            Node *n = classLookup(t);
            if (n) {
                proxyname = Getattr(n, "proxyname");
                if (!proxyname || jnidescriptor) {
                    String *nspace = Getattr(n, "sym:nspace");
                    String *symname = Copy(Getattr(n, "sym:name"));
                    if (symname && !GetFlag(n, "feature:flatnested")) {
                        for (Node *outer_class = Getattr(n, "nested:outer"); outer_class; outer_class = Getattr(outer_class, "nested:outer")) {
                            if (String* name = Getattr(outer_class, "sym:name")) {
                                Push(symname, jnidescriptor ? "$" : ".");
                                Push(symname, name);
                            }
                            else
                                return NULL;
                        }
                    }
                    if (nspace) {
                        if (package && !jnidescriptor)
                            proxyname = NewStringf("%s.%s.%s", package, nspace, symname);
                        else
                            proxyname = NewStringf("%s.%s", nspace, symname);
                    } else {
                        proxyname = Copy(symname);
                    }
                    if (!jnidescriptor) {
                        Setattr(n, "proxyname", proxyname); // Cache it
                        Delete(proxyname);
                    }
                    Delete(symname);
                }
            }
        }
        return proxyname;
    }

    /* -----------------------------------------------------------------------------
   * makeValidJniName()
   * ----------------------------------------------------------------------------- */

    String *makeValidJniName(const String *name) {
        String *valid_jni_name = NewString(name);
        Replaceall(valid_jni_name, "_", "_1");
        return valid_jni_name;
    }

    /* -----------------------------------------------------------------------------
    * makeValidIntermediateName()
    * ----------------------------------------------------------------------------- */

    String *makeValidIntermediateName(const String *name) {
        String *swift_prefix = NewString("Swift_");

        String *valid_jni_name = makeValidJniName(name);
        Printf(swift_prefix, "%sSwiftInterface_%s", module_class_name, valid_jni_name);
        Delete(valid_jni_name);

        return swift_prefix;
    }

    /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

    virtual void main(int argc, char *argv[]) {

        SWIG_library_directory("swift");

        // Look for certain command line options
        for (int i = 1; i < argc; i++) {
            if (argv[i]) {
                if (strcmp(argv[i], "-package") == 0) {
                    if (argv[i + 1]) {
                        package = NewString("");
                        Printf(package, argv[i + 1]);
                        if (Len(package) == 0) {
                            Delete(package);
                            package = 0;
                        }
                        Swig_mark_arg(i);
                        Swig_mark_arg(i + 1);
                        i++;
                    } else {
                        Swig_arg_error();
                    }
                } else if ((strcmp(argv[i], "-shadow") == 0) || ((strcmp(argv[i], "-proxy") == 0))) {
                    Printf(stderr, "Deprecated command line option: %s. Proxy classes are now generated by default.\n", argv[i]);
                    Swig_mark_arg(i);
                    proxy_flag = true;
                } else if ((strcmp(argv[i], "-noproxy") == 0)) {
                    Swig_mark_arg(i);
                    proxy_flag = false;
                } else if (strcmp(argv[i], "-nopgcpp") == 0) {
                    Swig_mark_arg(i);
                    nopgcpp_flag = true;
                } else if (strcmp(argv[i], "-oldvarnames") == 0) {
                    Swig_mark_arg(i);
                    old_variable_names = true;
                } else if (strcmp(argv[i], "-jnic") == 0) {
                    Swig_mark_arg(i);
                    Printf(stderr, "Deprecated command line option: -jnic. C JNI calling convention now used when -c++ not specified.\n");
                } else if (strcmp(argv[i], "-nofinalize") == 0) {
                    Swig_mark_arg(i);
                    Printf(stderr, "Deprecated command line option: -nofinalize. Use the new javafinalize typemap instead.\n");
                } else if (strcmp(argv[i], "-jnicpp") == 0) {
                    Swig_mark_arg(i);
                    Printf(stderr, "Deprecated command line option: -jnicpp. C++ JNI calling convention now used when -c++ specified.\n");
                } else if (strcmp(argv[i], "-help") == 0) {
                    Printf(stdout, "%s\n", usage);
                }
            }
        }

        // Add a symbol to the parser for conditional compilation
        Preprocessor_define("SWIGSWIFT 1", 0);

        // Add typemap definitions
        SWIG_typemap_lang("swift");
        SWIG_config_file("swift.swg");

        allow_overloading();
        Swig_interface_feature_enable();
    }

    /* ---------------------------------------------------------------------
   * top()
   * --------------------------------------------------------------------- */

    virtual int top(Node *n) {

        // Get any options set in the module directive
        Node *optionsnode = Getattr(Getattr(n, "module"), "options");

        if (optionsnode) {
            if (Getattr(optionsnode, "jniclassname"))
                imclass_name = Copy(Getattr(optionsnode, "jniclassname"));
            /* check if directors are enabled for this module.  note: this 
       * is a "master" switch, without which no director code will be
       * emitted.  %feature("director") statements are also required
       * to enable directors for individual classes or methods.
       *
       * use %module(directors="1") modulename at the start of the 
       * interface file to enable director generation.
       */
            if (Getattr(optionsnode, "directors")) {
                allow_directors();
            }
            if (Getattr(optionsnode, "dirprot")) {
                allow_dirprot();
            }
            allow_allprotected(GetFlag(optionsnode, "allprotected"));
        }

        /* Initialize all of the output files */
        String *outfile = Getattr(n, "outfile");
        String *outfile_h = Getattr(n, "outfile_h");

        if (!outfile) {
            Printf(stderr, "Unable to determine outfile\n");
            SWIG_exit(EXIT_FAILURE);
        }

        f_begin = NewFile(outfile, "w", SWIG_output_files());
        if (!f_begin) {
            FileErrorDisplay(outfile);
            SWIG_exit(EXIT_FAILURE);
        }

        if (directorsEnabled()) {
            if (!outfile_h) {
                Printf(stderr, "Unable to determine outfile_h\n");
                SWIG_exit(EXIT_FAILURE);
            }
            f_runtime_h = NewFile(outfile_h, "w", SWIG_output_files());
            if (!f_runtime_h) {
                FileErrorDisplay(outfile_h);
                SWIG_exit(EXIT_FAILURE);
            }
        }

        f_runtime = NewString("");
        f_init = NewString("");
        f_header = NewString("");
        f_wrappers = NewString("");
        f_directors_h = NewString("");
        f_directors = NewString("");

        /* Register file targets with the SWIG file handler */
        Swig_register_filebyname("begin", f_begin);
        Swig_register_filebyname("header", f_header);
        Swig_register_filebyname("wrapper", f_wrappers);
        Swig_register_filebyname("runtime", f_runtime);
        Swig_register_filebyname("init", f_init);
        Swig_register_filebyname("director", f_directors);
        Swig_register_filebyname("director_h", f_directors_h);

        swig_types_hash = NewHash();
        filenames_list = NewList();

        // Make the intermediary class and module class names. The intermediary class name can be set in the module directive.
        if (!imclass_name) {
            imclass_name = NewStringf("%sSwiftInterface", Getattr(n, "name"));
            module_class_name = Copy(Getattr(n, "name"));
        } else {
            // Rename the module name if it is the same as intermediary class name - a backwards compatibility solution
            if (Cmp(imclass_name, Getattr(n, "name")) == 0)
                module_class_name = NewStringf("%sModule", Getattr(n, "name"));
            else
                module_class_name = Copy(Getattr(n, "name"));
        }
        constants_interface_name = NewStringf("%sConstants", module_class_name);

        // module class and intermediary classes are always created
        if (!addSymbol(imclass_name, n))
            return SWIG_ERROR;
        if (!addSymbol(module_class_name, n))
            return SWIG_ERROR;

        imclass_class_code = NewString("");
        proxy_class_def = NewString("");
        proxy_class_code = NewString("");
        module_class_constants_code = NewString("");
        imclass_baseclass = NewString("");
        imclass_package = NULL;
        imclass_interfaces = NewString("");
        imclass_class_modifiers = NewString("");
        module_class_code = NewString("");
        module_baseclass = NewString("");
        module_interfaces = NewString("");
        module_imports = NewString("");
        module_class_modifiers = NewString("");
        imclass_imports = NewString("");
        imclass_cppcasts_code = NewString("");
        imclass_directors = NewString("");
        upcasts_code = NewString("");
        dmethods_seq = NewList();
        dmethods_table = NewHash();
        n_dmethods = 0;
        n_directors = 0;
        jnipackage = NewString("");
        package_path = NewString("");

        Swig_banner(f_begin);

        Printf(f_runtime, "\n\n#ifndef SWIGSWIFT\n#define SWIGSWIFT\n#endif\n\n");

        Printf(f_runtime, "\n");

        String *wrapper_name = NewString("");

        if (package) {
            String *jniname = makeValidJniName(package);
            Printv(jnipackage, jniname, NIL);
            Delete(jniname);
            Replaceall(jnipackage, ".", "_");
            Append(jnipackage, "_");
            Printv(package_path, package, NIL);
            Replaceall(package_path, ".", "/");
        }
        String *jniname = makeValidIntermediateName(package);
        Printf(wrapper_name, "%s%%f", package);
        Delete(jniname);

        Swig_name_register("wrapper", Char(wrapper_name));
        if (old_variable_names) {
            Swig_name_register("set", "set_%n%v");
            Swig_name_register("get", "get_%n%v");
        }

        Delete(wrapper_name);

        Printf(f_wrappers, "\n#ifdef __cplusplus\n");
        Printf(f_wrappers, "extern \"C\" {\n");
        Printf(f_wrappers, "#endif\n\n");

        /* Emit code */
        Language::top(n);

        if (directorsEnabled()) {
            // Insert director runtime into the f_runtime file (make it occur before %header section)
            Swig_insert_file("director_common.swg", f_runtime);
            Swig_insert_file("director.swg", f_runtime);
        }
        // Generate the intermediary class
        {
            String *filen = NewStringf("%s%s.h", outputDirectory(imclass_package), imclass_name);
            File *f_im = NewFile(filen, "w", SWIG_output_files());
            if (!f_im) {
                FileErrorDisplay(filen);
                SWIG_exit(EXIT_FAILURE);
            }
            Append(filenames_list, Copy(filen));
            Delete(filen);
            filen = NULL;

            // Start writing out the intermediary class file
            emitBanner(f_im);

// A C header is all we need, no classes.
//            if (imclass_package && package)
//                Printf(f_im, "package %s.%s;", package, imclass_package);
//            else if (imclass_package)
//                Printf(f_im, "package %s;", imclass_package);
//            else if (package)
//                Printf(f_im, "package %s;\n", package);
//
//            if (imclass_imports)
//                Printf(f_im, "%s\n", imclass_imports);
//
//            if (Len(imclass_class_modifiers) > 0)
//                Printf(f_im, "%s ", imclass_class_modifiers);
//            Printf(f_im, "%s ", imclass_name);
//
//            if (imclass_baseclass && *Char(imclass_baseclass))
//                Printf(f_im, ": %s ", imclass_baseclass);
//            if (Len(imclass_interfaces) > 0)
//                Printv(f_im, ": ", imclass_interfaces, " ", NIL);
//            Printf(f_im, "{\n");

            // Add the intermediary class methods
            Replaceall(imclass_class_code, "$module", module_class_name);
            Replaceall(imclass_class_code, "$imclassname", imclass_name);
            Printv(f_im, imclass_class_code, NIL);
            Printv(f_im, imclass_cppcasts_code, NIL);
            if (Len(imclass_directors) > 0)
                Printv(f_im, "\n", imclass_directors, NIL);

            if (n_dmethods > 0) {
                Putc('\n', f_im);
                Printf(f_im, "  fileprivate static void swig_module_init();\n");

                Printf(f_im, "  static let run: Void = {\n");
                Printf(f_im, "    swig_module_init();\n");
                Printf(f_im, "    return ()\n" );
                Printf(f_im, "  }()\n");
            }
            // Finish off the class
//            Printf(f_im, "}\n");
            Delete(f_im);
        }

        // Generate the Swift module class
        {
            String *filen = NewStringf("%s%s.swift", SWIG_output_directory(), module_class_name);
            File *f_module = NewFile(filen, "w", SWIG_output_files());
            if (!f_module) {
                FileErrorDisplay(filen);
                SWIG_exit(EXIT_FAILURE);
            }
            Append(filenames_list, Copy(filen));
            Delete(filen);
            filen = NULL;

            // Start writing out the module class file
            emitBanner(f_module);

            if (package)
                Printf(f_module, "package %s;\n", package);

            if (module_imports)
                Printf(f_module, "%s\n", module_imports);

            if (Len(module_class_modifiers) > 0)
                Printf(f_module, "%s ", module_class_modifiers);
            Printf(f_module, "%s ", module_class_name);

            if (module_baseclass && *Char(module_baseclass))
                Printf(f_module, ": %s ", module_baseclass);
            if (Len(module_interfaces) > 0) {
                if (Len(module_class_constants_code) != 0)
                    Printv(f_module, ": ", constants_interface_name, ", ", module_interfaces, " ", NIL);
                else
                    Printv(f_module, ": ", module_interfaces, " ", NIL);
            } else {
                if (Len(module_class_constants_code) != 0)
                    Printv(f_module, ": ", constants_interface_name, " ", NIL);
            }
            Printf(f_module, "{\n");

            Replaceall(module_class_code, "$module", module_class_name);
            Replaceall(module_class_constants_code, "$module", module_class_name);

            Replaceall(module_class_code, "$imclassname", imclass_name);
            Replaceall(module_class_constants_code, "$imclassname", imclass_name);

            // Add the wrapper methods
            Printv(f_module, module_class_code, NIL);

            // Finish off the class
            Printf(f_module, "}\n");
            Delete(f_module);
        }

        // Generate the Java constants interface
        if (Len(module_class_constants_code) != 0) {
            String *filen = NewStringf("%s%s.swift", SWIG_output_directory(), constants_interface_name);
            File *f_module = NewFile(filen, "w", SWIG_output_files());
            if (!f_module) {
                FileErrorDisplay(filen);
                SWIG_exit(EXIT_FAILURE);
            }
            Append(filenames_list, Copy(filen));
            Delete(filen);
            filen = NULL;

            // Start writing out the Java constants interface file
            emitBanner(f_module);

            if (package)
                Printf(f_module, "package %s;\n", package);

            if (module_imports)
                Printf(f_module, "%s\n", module_imports);

            Printf(f_module, "public interface %s {\n", constants_interface_name);

            // Write out all the global constants
            Printv(f_module, module_class_constants_code, NIL);

            // Finish off the Java interface
            Printf(f_module, "}\n");
            Delete(f_module);
        }

        if (upcasts_code)
            Printv(f_wrappers, upcasts_code, NIL);
// FIXME: remove
//        emitDirectorUpcalls();

        Printf(f_wrappers, "#ifdef __cplusplus\n");
        Printf(f_wrappers, "}\n");
        Printf(f_wrappers, "#endif\n");

        // Output a Java type wrapper class for each SWIG type
        for (Iterator swig_type = First(swig_types_hash); swig_type.key; swig_type = Next(swig_type)) {
            emitTypeWrapperClass(swig_type.key, swig_type.item);
        }

        // Check for overwriting file problems on filesystems that are case insensitive
        Iterator it1;
        Iterator it2;
        for (it1 = First(filenames_list); it1.item; it1 = Next(it1)) {
            String *item1_lower = Swig_string_lower(it1.item);
            for (it2 = Next(it1); it2.item; it2 = Next(it2)) {
                String *item2_lower = Swig_string_lower(it2.item);
                if (it1.item && it2.item) {
                    if (Strcmp(item1_lower, item2_lower) == 0) {
                        Swig_warning(WARN_LANG_PORTABILITY_FILENAME, input_file, line_number,
                                     "Portability warning: File %s will be overwritten by %s on case insensitive filesystems such as "
                                             "Windows' FAT32 and NTFS unless the class/module name is renamed\n", it1.item, it2.item);
                    }
                }
                Delete(item2_lower);
            }
            Delete(item1_lower);
        }

        Delete(swig_types_hash);
        swig_types_hash = NULL;
        Delete(filenames_list);
        filenames_list = NULL;
        Delete(imclass_name);
        imclass_name = NULL;
        Delete(imclass_class_code);
        imclass_class_code = NULL;
        Delete(proxy_class_def);
        proxy_class_def = NULL;
        Delete(proxy_class_code);
        proxy_class_code = NULL;
        Delete(module_class_constants_code);
        module_class_constants_code = NULL;
        Delete(imclass_baseclass);
        imclass_baseclass = NULL;
        Delete(imclass_package);
        imclass_package = NULL;
        Delete(imclass_interfaces);
        imclass_interfaces = NULL;
        Delete(imclass_class_modifiers);
        imclass_class_modifiers = NULL;
        Delete(module_class_name);
        module_class_name = NULL;
        Delete(constants_interface_name);
        constants_interface_name = NULL;
        Delete(module_class_code);
        module_class_code = NULL;
        Delete(module_baseclass);
        module_baseclass = NULL;
        Delete(module_interfaces);
        module_interfaces = NULL;
        Delete(module_imports);
        module_imports = NULL;
        Delete(module_class_modifiers);
        module_class_modifiers = NULL;
        Delete(imclass_imports);
        imclass_imports = NULL;
        Delete(imclass_cppcasts_code);
        imclass_cppcasts_code = NULL;
        Delete(imclass_directors);
        imclass_directors = NULL;
        Delete(upcasts_code);
        upcasts_code = NULL;
        Delete(package);
        package = NULL;
        Delete(jnipackage);
        jnipackage = NULL;
        Delete(package_path);
        package_path = NULL;
        Delete(dmethods_seq);
        dmethods_seq = NULL;
        Delete(dmethods_table);
        dmethods_table = NULL;
        n_dmethods = 0;

        /* Close all of the files */
        Dump(f_header, f_runtime);

        if (directorsEnabled()) {
            Dump(f_directors, f_runtime);
            Dump(f_directors_h, f_runtime_h);

            Printf(f_runtime_h, "\n");
            Printf(f_runtime_h, "#endif\n");

            Delete(f_runtime_h);
            f_runtime_h = NULL;
            Delete(f_directors);
            f_directors = NULL;
            Delete(f_directors_h);
            f_directors_h = NULL;
        }

        Dump(f_wrappers, f_runtime);
        Wrapper_pretty_print(f_init, f_runtime);
        Delete(f_header);
        Delete(f_wrappers);
        Delete(f_init);
        Dump(f_runtime, f_begin);
        Delete(f_runtime);
        Delete(f_begin);
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------------
   * emitBanner()
   * ----------------------------------------------------------------------------- */

    void emitBanner(File *f) {
        Printf(f, "/* ----------------------------------------------------------------------------\n");
        Swig_banner_target_lang(f, " *");
        Printf(f, " * ----------------------------------------------------------------------------- */\n\n");
    }

    /*-----------------------------------------------------------------------
   * Add new director upcall signature
   *----------------------------------------------------------------------*/

    UpcallData *addUpcallMethod(String *imclass_method, String *class_method, String *imclass_desc, String *class_desc, String *decl) {
        String *key = NewStringf("%s|%s", imclass_method, decl);

        ++curr_class_dmethod;

        String *imclass_methodidx = NewStringf("%d", n_dmethods);
        String *class_methodidx = NewStringf("%d", n_dmethods - first_class_dmethod);
        n_dmethods++;

        Hash *new_udata = NewHash();
        Append(dmethods_seq, new_udata);
        Setattr(dmethods_table, key, new_udata);

        Setattr(new_udata, "method", Copy(class_method));
        Setattr(new_udata, "fdesc", Copy(class_desc));
        Setattr(new_udata, "imclass_method", Copy(imclass_method));
        Setattr(new_udata, "imclass_fdesc", Copy(imclass_desc));
        Setattr(new_udata, "imclass_methodidx", imclass_methodidx);
        Setattr(new_udata, "class_methodidx", class_methodidx);
        Setattr(new_udata, "decl", Copy(decl));

        Delete(key);
        return new_udata;
    }

    /*-----------------------------------------------------------------------
   * Get director upcall signature
   *----------------------------------------------------------------------*/

    UpcallData *getUpcallMethodData(String *director_class, String *decl) {
        String *key = NewStringf("%s|%s", director_class, decl);
        UpcallData *udata = Getattr(dmethods_table, key);

        Delete(key);
        return udata;
    }

    /* ----------------------------------------------------------------------
   * nativeWrapper()
   * ---------------------------------------------------------------------- */

    virtual int nativeWrapper(Node *n) {
        String *wrapname = Getattr(n, "wrap:name");

        if (!addSymbol(wrapname, n, imclass_name))
            return SWIG_ERROR;

        if (Getattr(n, "type")) {
            Swig_save("nativeWrapper", n, "name", NIL);
            Setattr(n, "name", wrapname);
            native_function_flag = true;
            functionWrapper(n);
            Swig_restore(n);
            native_function_flag = false;
        } else {
            Swig_error(input_file, line_number, "No return type for %%native method %s.\n", Getattr(n, "wrap:name"));
        }

        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * functionWrapper()
   * ---------------------------------------------------------------------- */

    virtual int functionWrapper(Node *n) {
        String *symname = Getattr(n, "sym:name");
        SwigType *t = Getattr(n, "type");
        ParmList *l = Getattr(n, "parms");
        String *tm;
        Parm *p;
        int i;
        String *c_return_type = NewString("");
        String *im_return_type = NewString("");
        String *cleanup = NewString("");
        String *outarg = NewString("");
        String *body = NewString("");
        int num_arguments = 0;
        int gencomma = 0;
        bool is_void_return;
        String *overloaded_name = getOverloadedName(n);
        String *nondir_args = NewString("");
        bool is_destructor = (Cmp(Getattr(n, "nodeType"), "destructor") == 0);

        if (!Getattr(n, "sym:overloaded")) {
            if (!addSymbol(symname, n, imclass_name))
                return SWIG_ERROR;
        }

        /*
       The rest of this function deals with generating the intermediary class wrapper function (that wraps
       a c/c++ function) and generating the JNI c code. Each Java wrapper function has a 
       matching JNI c function call.
     */

        // A new wrapper function object
        Wrapper *f = NewWrapper();

        // Make a wrapper name for this function
        String *jniname = makeValidJniName(overloaded_name);
        String *wname = Swig_name_wrapper(jniname);
        Delete(jniname);

        /* Attach the non-standard typemaps to the parameter list. */
        Swig_typemap_attach_parms("jni", l, f);
        Swig_typemap_attach_parms("jtype", l, f);
        Swig_typemap_attach_parms("jstype", l, f);

        /* Get return types */
        if ((tm = Swig_typemap_lookup("jni", n, "", 0))) {
            Printf(c_return_type, "%s", tm);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, "No jni typemap defined for %s\n", SwigType_str(t, 0));
        }

        if ((tm = Swig_typemap_lookup("jni", n, "", 0))) {
            Printf(im_return_type, "%s", tm);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, "No jtype typemap defined for %s\n", SwigType_str(t, 0));
        }

        is_void_return = (Cmp(c_return_type, "void") == 0);
        if (!is_void_return)
            Wrapper_add_localv(f, "void*", c_return_type, "_cresult = 0", NIL);

        String *valid_im_name = makeValidIntermediateName(overloaded_name);
        Printv(f->def, c_return_type, " ", valid_im_name, "(", NIL);
        Delete(valid_im_name);

        // Usually these function parameters are unused - The code below ensures
        // that compilers do not issue such a warning if configured to do so.

// I don't think we need these.
//        Printv(f->code, "    (void)jenv;\n", NIL);
//        Printv(f->code, "    (void)jcls;\n", NIL);

        // Emit all of the local variables for holding arguments.
        emit_parameter_variables(l, f);

        /* Attach the standard typemaps */
        emit_attach_parmmaps(l, f);

        // Parameter overloading
        Setattr(n, "wrap:parms", l);
        Setattr(n, "wrap:name", wname);

        // Wrappers not wanted for some methods where the parameters cannot be overloaded in Java
        if (Getattr(n, "sym:overloaded")) {
            // Emit warnings for the few cases that can't be overloaded in Java and give up on generating wrapper
            Swig_overload_check(n);
            if (Getattr(n, "overload:ignore")) {
                DelWrapper(f);
                return SWIG_OK;
            }
        }

        String *imjni_name = makeValidIntermediateName(overloaded_name);
        Printf(imclass_class_code, " %s %s(", im_return_type, imjni_name);
        Delete(imjni_name);
//        Printf(wrapper_name, "%s%s_%%f", jnipackage, jniname);


        num_arguments = emit_num_arguments(l);

        // Now walk the function parameter list and generate code to get arguments
        for (i = 0, p = l; i < num_arguments; i++) {

            while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
            }

            SwigType *pt = Getattr(p, "type");
            String *ln = Getattr(p, "lname");
            String *im_param_type = NewString("");
            String *c_param_type = NewString("");
            String *arg = NewString("");

            Printf(arg, "%s", ln);

            /* Get the JNI C types of the parameter */
            if ((tm = Getattr(p, "tmap:jni"))) {
                Printv(c_param_type, tm, NIL);
            } else {
                Swig_warning(WARN_JAVA_TYPEMAP_JNI_UNDEF, input_file, line_number, "No jni typemap defined for %s\n", SwigType_str(pt, 0));
            }

            /* Get the intermediary class parameter types of the parameter */
            if ((tm = Getattr(p, "tmap:jtype"))) {
                Printv(im_param_type, tm, NIL);
            } else {
                Swig_warning(WARN_JAVA_TYPEMAP_JTYPE_UNDEF, input_file, line_number, "No jtype typemap defined for %s\n", SwigType_str(pt, 0));
            }

            /* Add parameter to intermediary class method */
            if (gencomma) {
                Printf(imclass_class_code, ", ");
                Printf(f->def, ", ");
            }
            Printf(imclass_class_code, "%s p_%s", im_param_type, arg);

            // Add parameter to C function
            Printv(f->def, c_param_type, " p_", arg, NIL);

            ++gencomma;

            //TODO: Find out if this is necessary.
            // Premature garbage collection prevention parameter
//            if (!is_destructor) {
//                String *pgc_parameter = prematureGarbageCollectionPreventionParameter(pt, p);
//                if (pgc_parameter) {
//                    Printf(imclass_class_code, ", %s %s_", pgc_parameter, arg);
//                    Printf(f->def, ", jobject %s_", arg);
//                    Printf(f->code, "    (void)%s_;\n", arg);
//                }
//            }
            // Get typemap for this argument
            if ((tm = Getattr(p, "tmap:in"))) {
                addThrows(n, "tmap:in", p);
                Replaceall(tm, "$source", arg);	/* deprecated */
                Replaceall(tm, "$target", ln);	/* deprecated */
                Replaceall(tm, "$arg", arg);	/* deprecated? */
                Replaceall(tm, "$input", arg);
                Setattr(p, "emit:input", arg);

                Printf(nondir_args, "%s\n", tm);

                p = Getattr(p, "tmap:in:next");
            } else {
                Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number, "Unable to use type %s as a function argument.\n", SwigType_str(pt, 0));
                p = nextSibling(p);
            }

            Delete(im_param_type);
            Delete(c_param_type);
            Delete(arg);
        }

        Printv(f->code, nondir_args, NIL);
        Delete(nondir_args);

        /* Insert constraint checking code */
        for (p = l; p;) {
            if ((tm = Getattr(p, "tmap:check"))) {
                addThrows(n, "tmap:check", p);
                Replaceall(tm, "$target", Getattr(p, "lname"));	/* deprecated */
                Replaceall(tm, "$arg", Getattr(p, "emit:input"));	/* deprecated? */
                Replaceall(tm, "$input", Getattr(p, "emit:input"));
                Printv(f->code, tm, "\n", NIL);
                p = Getattr(p, "tmap:check:next");
            } else {
                p = nextSibling(p);
            }
        }

        /* Insert cleanup code */
        for (p = l; p;) {
            if ((tm = Getattr(p, "tmap:freearg"))) {
                addThrows(n, "tmap:freearg", p);
                Replaceall(tm, "$source", Getattr(p, "emit:input"));	/* deprecated */
                Replaceall(tm, "$arg", Getattr(p, "emit:input"));	/* deprecated? */
                Replaceall(tm, "$input", Getattr(p, "emit:input"));
                Printv(cleanup, tm, "\n", NIL);
                p = Getattr(p, "tmap:freearg:next");
            } else {
                p = nextSibling(p);
            }
        }

        /* Insert argument output code */
        for (p = l; p;) {
            if ((tm = Getattr(p, "tmap:argout"))) {
                addThrows(n, "tmap:argout", p);
                Replaceall(tm, "$source", Getattr(p, "emit:input"));	/* deprecated */
                Replaceall(tm, "$target", Getattr(p, "lname"));	/* deprecated */
                Replaceall(tm, "$arg", Getattr(p, "emit:input"));	/* deprecated? */
                Replaceall(tm, "$result", "result");
                Replaceall(tm, "$input", Getattr(p, "emit:input"));
                Printv(outarg, tm, "\n", NIL);
                p = Getattr(p, "tmap:argout:next");
            } else {
                p = nextSibling(p);
            }
        }

        // Get any Java exception classes in the throws typemap
        ParmList *throw_parm_list = NULL;
        if ((throw_parm_list = Getattr(n, "catchlist"))) {
            Swig_typemap_attach_parms("throws", throw_parm_list, f);
            for (p = throw_parm_list; p; p = nextSibling(p)) {
                if (Getattr(p, "tmap:throws")) {
                    addThrows(n, "tmap:throws", p);
                }
            }
        }

        // Now write code to make the function call
        if (!native_function_flag) {

            Swig_director_emit_dynamic_cast(n, f);
            String *actioncode = emit_action(n);

            // Handle exception classes specified in the "except" feature's "throws" attribute
            addThrows(n, "feature:except", n);

            /* Return value if necessary  */
            if ((tm = Swig_typemap_lookup_out("out", n, Swig_cresult_name(), f, actioncode))) {
                addThrows(n, "tmap:out", n);
                Replaceall(tm, "$source", Swig_cresult_name());	/* deprecated */
                Replaceall(tm, "$target", "_cresult");	/* deprecated */
                Replaceall(tm, "$result", "_cresult");

                if (GetFlag(n, "feature:new"))
                    Replaceall(tm, "$owner", "1");
                else
                    Replaceall(tm, "$owner", "0");

                Printf(f->code, "%s", tm);
                if (Len(tm))
                    Printf(f->code, "\n");
            } else {
                Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, "Unable to use return type %s in function %s.\n", SwigType_str(t, 0), Getattr(n, "name"));
            }
            emit_return_variable(n, t, f);
        }

        /* Output argument output code */
        Printv(f->code, outarg, NIL);

        /* Output cleanup code */
        Printv(f->code, cleanup, NIL);

        /* Look to see if there is any newfree cleanup code */
        if (GetFlag(n, "feature:new")) {
            if ((tm = Swig_typemap_lookup("newfree", n, Swig_cresult_name(), 0))) {
                addThrows(n, "tmap:newfree", n);
                Replaceall(tm, "$source", Swig_cresult_name());	/* deprecated */
                Printf(f->code, "%s\n", tm);
            }
        }

        /* See if there is any return cleanup code */
        if (!native_function_flag) {
            if ((tm = Swig_typemap_lookup("ret", n, Swig_cresult_name(), 0))) {
                addThrows(n, "tmap:ret", n);
                Replaceall(tm, "$source", Swig_cresult_name());	/* deprecated */
                Printf(f->code, "%s\n", tm);
            }
        }

        /* Finish C function and intermediary class function definitions */
        Printf(imclass_class_code, ")");
        generateThrowsClause(n, imclass_class_code);
        Printf(imclass_class_code, ";\n");

        Printf(f->def, ") {");

        if (!is_void_return)
            Printv(f->code, "    return _cresult;\n", NIL);
        Printf(f->code, "}\n");

        /* Substitute the cleanup code */
        Replaceall(f->code, "$cleanup", cleanup);

        /* Substitute the function name */
        Replaceall(f->code, "$symname", symname);

        /* Contract macro modification */
        Replaceall(f->code, "SWIG_contract_assert(", "SWIG_contract_assert($null, ");

        if (!is_void_return)
            Replaceall(f->code, "$null", "0");
        else
            Replaceall(f->code, "$null", "");

        /* Dump the function out */
        if (!native_function_flag)
            Wrapper_print(f, f_wrappers);

        if (!(proxy_flag && is_wrapping_class()) && !enum_constant_flag) {
            moduleClassFunctionHandler(n);
        }

        /* 
     * Generate the proxy class getters/setters for public member variables.
     * Not for enums and constants.
     */
        if (proxy_flag && wrapping_member_flag && !enum_constant_flag) {
            // Capitalize the first letter in the variable to create a JavaBean type getter/setter function name
            bool getter_flag = Cmp(symname, Swig_name_set(getNSpace(), Swig_name_member(0, getClassPrefix(), variable_name))) != 0;

            String *getter_setter_name = NewString("");
            if (!getter_flag)
                Printf(getter_setter_name, "set");
            else
                Printf(getter_setter_name, "get");
            Putc(toupper((int) *Char(variable_name)), getter_setter_name);
            Printf(getter_setter_name, "%s", Char(variable_name) + 1);

            Setattr(n, "proxyfuncname", getter_setter_name);
            Setattr(n, "imfuncname", symname);

            proxyClassFunctionHandler(n);
            Delete(getter_setter_name);
        }

        Delete(c_return_type);
        Delete(im_return_type);
        Delete(cleanup);
        Delete(outarg);
        Delete(body);
        Delete(overloaded_name);
        DelWrapper(f);
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------
   * variableWrapper()
   * ----------------------------------------------------------------------- */

    virtual int variableWrapper(Node *n) {
        variable_wrapper_flag = true;
        Language::variableWrapper(n);	/* Default to functions */
        variable_wrapper_flag = false;
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------
   * globalvariableHandler()
   * ------------------------------------------------------------------------ */

    virtual int globalvariableHandler(Node *n) {

        variable_name = Getattr(n, "sym:name");
        global_variable_flag = true;
        int ret = Language::globalvariableHandler(n);
        global_variable_flag = false;
        return ret;
    }

    String *getCurrentScopeName(String *nspace) {
        String *scope = 0;
        if (nspace || getCurrentClass()) {
            scope = NewString("");
            if (nspace)
                Printf(scope, "%s", nspace);
            if (Node* cls = getCurrentClass()) {
                if (Node *outer = Getattr(cls, "nested:outer")) {
                    String *outerClassesPrefix = Copy(Getattr(outer, "sym:name"));
                    for (outer = Getattr(outer, "nested:outer"); outer != 0; outer = Getattr(outer, "nested:outer")) {
                        Push(outerClassesPrefix, ".");
                        Push(outerClassesPrefix, Getattr(outer, "sym:name"));
                    }
                    Printv(scope, nspace ? "." : "", outerClassesPrefix, ".", proxy_class_name, NIL);
                    Delete(outerClassesPrefix);
                } else
                    Printv(scope, nspace ? "." : "", proxy_class_name, NIL);
            }
        }
        return scope;
    }

    /* ----------------------------------------------------------------------
   * enumDeclaration()
   *
   * C/C++ enums can be mapped in one of 4 ways, depending on the java:enum feature specified:
   * 1) Simple enums - simple constant within the proxy class or module class
   * 2) Typeunsafe enums - simple constant in a Java class (class named after the c++ enum name)
   * 3) Typesafe enum - typesafe enum pattern (class named after the c++ enum name)
   * 4) Proper enums - proper Java enum
   * Anonymous enums always default to 1)
   * ---------------------------------------------------------------------- */

    virtual int enumDeclaration(Node *n) {

        if (!ImportMode) {
            if (getCurrentClass() && (cplus_mode != PUBLIC))
                return SWIG_NOWRAP;

            String *nspace = Getattr(n, "sym:nspace"); // NSpace/getNSpace() only works during Language::enumDeclaration call
            if (proxy_flag && !is_wrapping_class()) {
                // Global enums / enums in a namespace
                assert(!full_imclass_name);
                constructIntermediateClassName(n);
            }

            enum_code = NewString("");
            String *symname = Getattr(n, "sym:name");
            String *constants_code = (proxy_flag && is_wrapping_class())? proxy_class_constants_code : module_class_constants_code;
            EnumFeature enum_feature = decodeEnumFeature(n);
            String *typemap_lookup_type = Getattr(n, "name");

            if ((enum_feature != SimpleEnum) && symname && typemap_lookup_type) {
                // Wrap (non-anonymous) C/C++ enum within a typesafe, typeunsafe or proper Java enum

                String *scope = getCurrentScopeName(nspace);
                if (!addSymbol(symname, n, scope))
                    return SWIG_ERROR;

                // Pure Java baseclass and interfaces
                const String *pure_baseclass = typemapLookup(n, "javabase", typemap_lookup_type, WARN_NONE);
                const String *pure_interfaces = typemapLookup(n, "javainterfaces", typemap_lookup_type, WARN_NONE);

                // Emit the enum
                Printv(enum_code, typemapLookup(n, "javaclassmodifiers", typemap_lookup_type, WARN_JAVA_TYPEMAP_CLASSMOD_UNDEF),	// Class modifiers (enum modifiers really)
                       " ", symname, *Char(pure_baseclass) ?	// Bases
                                     " extends " : "", pure_baseclass, *Char(pure_interfaces) ?	// Interfaces
                                                                       " implements " : "", pure_interfaces, " {\n", NIL);
                if (proxy_flag && is_wrapping_class())
                    Replaceall(enum_code, "$static ", "static ");
                else
                    Replaceall(enum_code, "$static ", "");
                Delete(scope);
            } else {
                // Wrap C++ enum with integers - just indicate start of enum with a comment, no comment for anonymous enums of any sort
                if (symname && !Getattr(n, "unnamedinstance"))
                    Printf(constants_code, "  // %s \n", symname);
            }

            // Emit each enum item
            Language::enumDeclaration(n);

            if ((enum_feature != SimpleEnum) && symname && typemap_lookup_type) {
                // Wrap (non-anonymous) C/C++ enum within a typesafe, typeunsafe or proper Java enum
                // Finish the enum declaration
                // Typemaps are used to generate the enum definition in a similar manner to proxy classes.
                Printv(enum_code, (enum_feature == ProperEnum) ? ";\n" : "", typemapLookup(n, "javabody", typemap_lookup_type, WARN_JAVA_TYPEMAP_JAVABODY_UNDEF),	// main body of class
                       typemapLookup(n, "javacode", typemap_lookup_type, WARN_NONE),	// extra Java code
                       "}", NIL);

                Replaceall(enum_code, "$javaclassname", symname);

                // Substitute $enumvalues - intended usage is for typesafe enums
                if (Getattr(n, "enumvalues"))
                    Replaceall(enum_code, "$enumvalues", Getattr(n, "enumvalues"));
                else
                    Replaceall(enum_code, "$enumvalues", "");

                if (proxy_flag && is_wrapping_class()) {
                    // Enums defined within the C++ class are defined within the proxy class

                    // Add extra indentation
                    Replaceall(enum_code, "\n", "\n  ");
                    Replaceall(enum_code, "  \n", "\n");
                    if (GetFlag(getCurrentClass(), "feature:interface"))
                        Printv(interface_class_code, "  ", enum_code, "\n\n", NIL);
                    else
                        Printv(proxy_class_constants_code, "  ", enum_code, "\n\n", NIL);
                } else {
                    // Global enums are defined in their own file
                    String *output_directory = outputDirectory(nspace);
                    String *filen = NewStringf("%s%s.swift", output_directory, symname);
                    File *f_enum = NewFile(filen, "w", SWIG_output_files());
                    if (!f_enum) {
                        FileErrorDisplay(filen);
                        SWIG_exit(EXIT_FAILURE);
                    }
                    Append(filenames_list, Copy(filen));
                    Delete(filen);
                    filen = NULL;

                    // Start writing out the enum file
                    emitBanner(f_enum);

                    if (package || nspace) {
                        Printf(f_enum, "package ");
                        if (package)
                            Printv(f_enum, package, nspace ? "." : "", NIL);
                        if (nspace)
                            Printv(f_enum, nspace, NIL);
                        Printf(f_enum, ";\n");
                    }

                    Printv(f_enum, typemapLookup(n, "javaimports", typemap_lookup_type, WARN_NONE), // Import statements
                           "\n", enum_code, "\n", NIL);

                    Printf(f_enum, "\n");
                    Delete(f_enum);
                    Delete(output_directory);
                }
            } else {
                // Wrap C++ enum with simple constant
                Printf(enum_code, "\n");
                if (proxy_flag && is_wrapping_class())
                    Printv(proxy_class_constants_code, enum_code, NIL);
                else
                    Printv(module_class_constants_code, enum_code, NIL);
            }

            Delete(enum_code);
            enum_code = NULL;

            if (proxy_flag && !is_wrapping_class()) {
                Delete(full_imclass_name);
                full_imclass_name = 0;
            }
        }
        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * enumvalueDeclaration()
   * ---------------------------------------------------------------------- */

    virtual int enumvalueDeclaration(Node *n) {
        if (getCurrentClass() && (cplus_mode != PUBLIC))
            return SWIG_NOWRAP;

        Swig_require("enumvalueDeclaration", n, "*name", "?value", NIL);
        String *symname = Getattr(n, "sym:name");
        String *value = Getattr(n, "value");
        String *name = Getattr(n, "name");
        Node *parent = parentNode(n);
        int unnamedinstance = GetFlag(parent, "unnamedinstance");
        String *parent_name = Getattr(parent, "name");
        String *nspace = getNSpace();
        String *newsymname = 0;
        String *tmpValue;

        // Strange hack from parent method
        if (value)
            tmpValue = NewString(value);
        else
            tmpValue = NewString(name);
        // Note that this is used in enumValue() amongst other places
        Setattr(n, "value", tmpValue);

        // Deal with enum values that are not int
        int swigtype = SwigType_type(Getattr(n, "type"));
        if (swigtype == T_BOOL) {
            const char *val = Equal(Getattr(n, "enumvalue"), "true") ? "1" : "0";
            Setattr(n, "enumvalue", val);
        } else if (swigtype == T_CHAR) {
            String *val = NewStringf("'%(escape)s'", Getattr(n, "enumvalue"));
            Setattr(n, "enumvalue", val);
            Delete(val);
        }

        {
            EnumFeature enum_feature = decodeEnumFeature(parent);

            if ((enum_feature == SimpleEnum) && GetFlag(parent, "scopedenum")) {
                newsymname = Swig_name_member(0, Getattr(parent, "sym:name"), symname);
                symname = newsymname;
            }

            // Add to language symbol table
            String *scope = 0;
            if (unnamedinstance || !parent_name || enum_feature == SimpleEnum) {
                String *enumClassPrefix = getEnumClassPrefix();
                if (enumClassPrefix) {
                    scope = NewString("");
                    if (nspace)
                        Printf(scope, "%s.", nspace);
                    Printf(scope, "%s", enumClassPrefix);
                } else {
                    scope = Copy(constants_interface_name);
                }
            } else {
                scope = getCurrentScopeName(nspace);
                if (!scope)
                    scope = Copy(Getattr(parent, "sym:name"));
                else
                    Printf(scope, ".%s", Getattr(parent, "sym:name"));
            }
            if (!addSymbol(symname, n, scope))
                return SWIG_ERROR;

            if ((enum_feature == ProperEnum) && parent_name && !unnamedinstance) {
                // Wrap (non-anonymous) C/C++ enum with a proper Java enum
                // Emit the enum item.
                if (!GetFlag(n, "firstenumitem"))
                    Printf(enum_code, ",\n");
                Printf(enum_code, "  %s", symname);
                if (Getattr(n, "enumvalue")) {
                    String *value = enumValue(n);
                    Printf(enum_code, "(%s)", value);
                    Delete(value);
                }
            } else {
                // Wrap C/C++ enums with constant integers or use the typesafe enum pattern
                SwigType *typemap_lookup_type = parent_name ? parent_name : NewString("enum ");
                Setattr(n, "type", typemap_lookup_type);
                const String *tm = typemapLookup(n, "jstype", typemap_lookup_type, WARN_JAVA_TYPEMAP_JSTYPE_UNDEF);

                String *return_type = Copy(tm);
                substituteClassname(typemap_lookup_type, return_type);
                const String *methodmods = Getattr(n, "feature:java:methodmodifiers");
                methodmods = methodmods ? methodmods : (is_public(n) ? public_string : protected_string);

                if ((enum_feature == TypesafeEnum) && parent_name && !unnamedinstance) {
                    // Wrap (non-anonymous) enum using the typesafe enum pattern
                    if (Getattr(n, "enumvalue")) {
                        String *value = enumValue(n);
                        Printf(enum_code, "  %s final static %s %s = new %s(\"%s\", %s);\n", methodmods, return_type, symname, return_type, symname, value);
                        Delete(value);
                    } else {
                        Printf(enum_code, "  %s final static %s %s = new %s(\"%s\");\n", methodmods, return_type, symname, return_type, symname);
                    }
                } else {
                    // Simple integer constants
                    // Note these are always generated for anonymous enums, no matter what enum_feature is specified
                    // Code generated is the same for SimpleEnum and TypeunsafeEnum -> the class it is generated into is determined later
                    String *value = enumValue(n);
                    Printf(enum_code, "  %s final static %s %s = %s;\n", methodmods, return_type, symname, value);
                    Delete(value);
                }
                Delete(return_type);
            }

            // Add the enum value to the comma separated list being constructed in the enum declaration.
            String *enumvalues = Getattr(parent, "enumvalues");
            if (!enumvalues)
                Setattr(parent, "enumvalues", Copy(symname));
            else
                Printv(enumvalues, ", ", symname, NIL);
            Delete(scope);
        }

        Delete(newsymname);
        Delete(tmpValue);
        Swig_restore(n);
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------
   * constantWrapper()
   * Used for wrapping constants - #define or %constant.
   * Also for inline initialised const static primitive type member variables (short, int, double, enums etc).
   * Java static final variables are generated for these.
   * If the %javaconst(1) feature is used then the C constant value is used to initialise the Java final variable.
   * If not, a JNI method is generated to get the C constant value for initialisation of the Java final variable.
   * However, if the %javaconstvalue feature is used, it overrides all other ways to generate the initialisation.
   * Also note that this method might be called for wrapping enum items (when the enum is using %javaconst(0)).
   * ------------------------------------------------------------------------ */

    virtual int constantWrapper(Node *n) {
        String *symname = Getattr(n, "sym:name");
        SwigType *t = Getattr(n, "type");
        SwigType *valuetype = Getattr(n, "valuetype");
        ParmList *l = Getattr(n, "parms");
        String *tm;
        String *return_type = NewString("");
        String *constants_code = NewString("");
        Swig_save("constantWrapper", n, "value", NIL);

        bool is_enum_item = (Cmp(nodeType(n), "enumitem") == 0);

        const String *itemname = (proxy_flag && wrapping_member_flag) ? variable_name : symname;
        if (!is_enum_item) {
            String *scope = 0;
            if (proxy_class_name) {
                String *nspace = getNSpace();
                scope = NewString("");
                if (nspace)
                    Printf(scope, "%s.", nspace);
                Printf(scope, "%s", proxy_class_name);
            } else {
                scope = Copy(constants_interface_name);
            }
            if (!addSymbol(itemname, n, scope))
                return SWIG_ERROR;
            Delete(scope);
        }

        // The %javaconst feature determines how the constant value is obtained
        int const_feature_flag = GetFlag(n, "feature:java:const");

        /* Adjust the enum type for the Swig_typemap_lookup.
     * We want the same jstype typemap for all the enum items so we use the enum type (parent node). */
        if (is_enum_item) {
            t = Getattr(parentNode(n), "enumtype");
            Setattr(n, "type", t);
        }

        /* Attach the non-standard typemaps to the parameter list. */
        Swig_typemap_attach_parms("jstype", l, NULL);

        /* Get Java return types */
        bool classname_substituted_flag = false;

        if ((tm = Swig_typemap_lookup("jstype", n, "", 0))) {
            classname_substituted_flag = substituteClassname(t, tm);
            Printf(return_type, "%s", tm);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(t, 0));
        }

        // Add the stripped quotes back in
        String *new_value = NewString("");
        if (SwigType_type(t) == T_STRING) {
            Printf(new_value, "\"%s\"", Copy(Getattr(n, "value")));
            Setattr(n, "value", new_value);
        } else if (SwigType_type(t) == T_CHAR) {
            Printf(new_value, "\'%s\'", Copy(Getattr(n, "value")));
            Setattr(n, "value", new_value);
        }

        const String *methodmods = Getattr(n, "feature:java:methodmodifiers");
        methodmods = methodmods ? methodmods : (is_public(n) ? public_string : protected_string);

        Printf(constants_code, "  %s final static %s %s = ", methodmods, return_type, itemname);

        // Check for the %javaconstvalue feature
        String *value = Getattr(n, "feature:java:constvalue");

        if (value) {
            Printf(constants_code, "%s;\n", value);
        } else if (!const_feature_flag) {
            // Default enum and constant handling will work with any type of C constant and initialises the Java variable from C through a JNI call.

            if (classname_substituted_flag) {
                if (SwigType_isenum(t)) {
                    // This handles wrapping of inline initialised const enum static member variables (not when wrapping enum items - ignored later on)
                    Printf(constants_code, "%s.swigToEnum(%s.%s());\n", return_type, full_imclass_name, Swig_name_get(getNSpace(), symname));
                } else {
                    // This handles function pointers using the %constant directive
                    Printf(constants_code, "new %s(%s.%s(), false);\n", return_type, full_imclass_name ? full_imclass_name : imclass_name, Swig_name_get(getNSpace(), symname));
                }
            } else {
                Printf(constants_code, "%s.%s();\n", full_imclass_name ? full_imclass_name : imclass_name, Swig_name_get(getNSpace(), symname));
            }

            // Each constant and enum value is wrapped with a separate JNI function call
            SetFlag(n, "feature:immutable");
            enum_constant_flag = true;
            variableWrapper(n);
            enum_constant_flag = false;
        } else {
            // Alternative constant handling will use the C syntax to make a true Java constant and hope that it compiles as Java code
            if (Getattr(n, "wrappedasconstant")) {
                if (SwigType_type(valuetype) == T_CHAR)
                    Printf(constants_code, "\'%(escape)s\';\n", Getattr(n, "staticmembervariableHandler:value"));
                else
                    Printf(constants_code, "%s;\n", Getattr(n, "staticmembervariableHandler:value"));
            } else {
                Printf(constants_code, "%s;\n", Getattr(n, "value"));
            }
        }

        // Emit the generated code to appropriate place
        // Enums only emit the intermediate and JNI methods, so no proxy or module class wrapper methods needed
        if (!is_enum_item) {
            if (proxy_flag && wrapping_member_flag)
                Printv(proxy_class_constants_code, constants_code, NIL);
            else
                Printv(module_class_constants_code, constants_code, NIL);
        }
        // Cleanup
        Swig_restore(n);
        Delete(new_value);
        Delete(return_type);
        Delete(constants_code);
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------------
   * insertDirective()
   * ----------------------------------------------------------------------------- */

    virtual int insertDirective(Node *n) {
        int ret = SWIG_OK;
        String *code = Getattr(n, "code");
        String *section = Getattr(n, "section");
        Replaceall(code, "$module", module_class_name);
        Replaceall(code, "$imclassname", imclass_name);

        if (!ImportMode && (Cmp(section, "proxycode") == 0)) {
            if (proxy_class_code) {
                Swig_typemap_replace_embedded_typemap(code, n);
                int offset = Len(code) > 0 && *Char(code) == '\n' ? 1 : 0;
                Printv(proxy_class_code, Char(code) + offset, "\n", NIL);
            }
        } else {
            ret = Language::insertDirective(n);
        }
        return ret;
    }

    /* -----------------------------------------------------------------------------
   * pragmaDirective()
   *
   * Valid Pragmas:
   * jniclassbase            - base (extends) for the intermediary class
   * jniclasspackage         - package in which to generate the intermediary class
   * jniclassclassmodifiers  - class modifiers for the intermediary class
   * jniclasscode            - text (java code) is copied verbatim to the intermediary class
   * jniclassimports         - import statements for the intermediary class
   * jniclassinterfaces      - interface (implements) for the intermediary class
   *
   * modulebase              - base (extends) for the module class
   * moduleclassmodifiers    - class modifiers for the module class
   * modulecode              - text (java code) is copied verbatim to the module class
   * moduleimports           - import statements for the module class
   * moduleinterfaces        - interface (implements) for the module class
   *
   * ----------------------------------------------------------------------------- */

    virtual int pragmaDirective(Node *n) {
        if (!ImportMode) {
            String *lang = Getattr(n, "lang");
            String *code = Getattr(n, "name");
            String *value = Getattr(n, "value");

            if (Strcmp(lang, "java") == 0) {

                String *strvalue = NewString(value);
                Replaceall(strvalue, "\\\"", "\"");

                if (Strcmp(code, "jniclassbase") == 0) {
                    Delete(imclass_baseclass);
                    imclass_baseclass = Copy(strvalue);
                } else if (Strcmp(code, "jniclasspackage") == 0) {
                    Delete(imclass_package);
                    imclass_package = Copy(strvalue);
                    String *imclass_class_package_jniname = makeValidJniName(imclass_package);
                    Printv(jnipackage, imclass_class_package_jniname, NIL);
                    Delete(imclass_class_package_jniname);
                    Replaceall(jnipackage, NSPACE_SEPARATOR, "_");
                    Append(jnipackage, "_");

                    String *wrapper_name = NewString("");
                    String *imclass_class_jniname = makeValidJniName(imclass_name);
                    Printf(wrapper_name, "Java_%s%s_%%f", jnipackage, imclass_class_jniname);
                    Delete(imclass_class_jniname);

                    Swig_name_unregister("wrapper");
                    Swig_name_register("wrapper", Char(wrapper_name));

                    Delete(wrapper_name);
                } else if (Strcmp(code, "jniclassclassmodifiers") == 0) {
                    Delete(imclass_class_modifiers);
                    imclass_class_modifiers = Copy(strvalue);
                } else if (Strcmp(code, "jniclasscode") == 0) {
                    Printf(imclass_class_code, "%s\n", strvalue);
                } else if (Strcmp(code, "jniclassimports") == 0) {
                    Delete(imclass_imports);
                    imclass_imports = Copy(strvalue);
                } else if (Strcmp(code, "jniclassinterfaces") == 0) {
                    Delete(imclass_interfaces);
                    imclass_interfaces = Copy(strvalue);
                } else if (Strcmp(code, "modulebase") == 0) {
                    Delete(module_baseclass);
                    module_baseclass = Copy(strvalue);
                } else if (Strcmp(code, "moduleclassmodifiers") == 0) {
                    Delete(module_class_modifiers);
                    module_class_modifiers = Copy(strvalue);
                } else if (Strcmp(code, "modulecode") == 0) {
                    Printf(module_class_code, "%s\n", strvalue);
                } else if (Strcmp(code, "moduleimports") == 0) {
                    Delete(module_imports);
                    module_imports = Copy(strvalue);
                } else if (Strcmp(code, "moduleinterfaces") == 0) {
                    Delete(module_interfaces);
                    module_interfaces = Copy(strvalue);
                } else if (Strcmp(code, "moduleimport") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use the moduleimports pragma.\n");
                } else if (Strcmp(code, "moduleinterface") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use the moduleinterfaces pragma.\n");
                } else if (Strcmp(code, "modulemethodmodifiers") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%javamethodmodifiers.\n");
                } else if (Strcmp(code, "allshadowimport") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javaimports).\n");
                } else if (Strcmp(code, "allshadowcode") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javacode).\n");
                } else if (Strcmp(code, "allshadowbase") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javabase).\n");
                } else if (Strcmp(code, "allshadowinterface") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javainterfaces).\n");
                } else if (Strcmp(code, "allshadowclassmodifiers") == 0) {
                    Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javaclassmodifiers).\n");
                } else if (proxy_flag) {
                    if (Strcmp(code, "shadowcode") == 0) {
                        Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javacode).\n");
                    } else if (Strcmp(code, "shadowimport") == 0) {
                        Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javaimports).\n");
                    } else if (Strcmp(code, "shadowbase") == 0) {
                        Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javabase).\n");
                    } else if (Strcmp(code, "shadowinterface") == 0) {
                        Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javainterfaces).\n");
                    } else if (Strcmp(code, "shadowclassmodifiers") == 0) {
                        Swig_error(input_file, line_number, "Deprecated pragma. Please use %%typemap(javaclassmodifiers).\n");
                    } else {
                        Swig_error(input_file, line_number, "Unrecognized pragma.\n");
                    }
                } else {
                    Swig_error(input_file, line_number, "Unrecognized pragma.\n");
                }
                Delete(strvalue);
            }
        }
        return Language::pragmaDirective(n);
    }

    /* -----------------------------------------------------------------------------
   * getQualifiedInterfaceName()
   * ----------------------------------------------------------------------------- */

    String *getQualifiedInterfaceName(Node *n) {
        String *ret = Getattr(n, "interface:qname");
        if (!ret) {
            String *nspace = Getattr(n, "sym:nspace");
            String *symname = Getattr(n, "interface:name");
            if (nspace) {
                if (package)
                    ret = NewStringf("%s.%s.%s", package, nspace, symname);
                else
                    ret = NewStringf("%s.%s", nspace, symname);
            } else {
                ret = Copy(symname);
            }
            Setattr(n, "interface:qname", ret);
        }
        return ret;
    }

    /* -----------------------------------------------------------------------------
   * getInterfaceName()
   * ----------------------------------------------------------------------------- */

    String *getInterfaceName(SwigType *t, bool qualified) {
        String *interface_name = NULL;
        if (proxy_flag) {
            Node *n = classLookup(t);
            if (n && Getattr(n, "interface:name"))
                interface_name = qualified ? getQualifiedInterfaceName(n) : Getattr(n, "interface:name");
        }
        return interface_name;
    }

    /* -----------------------------------------------------------------------------
   * addInterfaceNameAndUpcasts()
   * ----------------------------------------------------------------------------- */

    void addInterfaceNameAndUpcasts(SwigType *smart, String *interface_list, String *interface_upcasts, Hash *base_list, String *c_classname) {
        List *keys = Keys(base_list);
        for (Iterator it = First(keys); it.item; it = Next(it)) {
            Node *base = Getattr(base_list, it.item);
            String *c_baseclass = SwigType_namestr(Getattr(base, "name"));
            String *interface_name = Getattr(base, "interface:name");
            if (Len(interface_list))
                Append(interface_list, ", ");
            Append(interface_list, interface_name);

            Node *attributes = NewHash();
            String *interface_code = Copy(typemapLookup(base, "javainterfacecode", Getattr(base, "classtypeobj"), WARN_JAVA_TYPEMAP_INTERFACECODE_UNDEF, attributes));
            String *cptr_method_name = 0;
            if (interface_code) {
                Replaceall(interface_code, "$interfacename", interface_name);
                Printv(interface_upcasts, interface_code, NIL);
                cptr_method_name = Copy(Getattr(attributes, "tmap:javainterfacecode:cptrmethod"));
            }
            if (!cptr_method_name)
                cptr_method_name = NewStringf("%s_GetInterfaceCPtr", interface_name);
            Replaceall(cptr_method_name, ".", "_");
            Replaceall(cptr_method_name, "$interfacename", interface_name);

            String *upcast_method_name = Swig_name_member(getNSpace(), proxy_class_name, cptr_method_name);
            upcastsCode(smart, upcast_method_name, c_classname, c_baseclass);
            Delete(upcast_method_name);
            Delete(cptr_method_name);
            Delete(interface_code);
            Delete(c_baseclass);
        }
        Delete(keys);
    }

    /* -----------------------------------------------------------------------------
   * upcastsCode()
   *
   * Add code for C++ casting to base class
   * ----------------------------------------------------------------------------- */

    void upcastsCode(SwigType *smart, String *upcast_method_name, String *c_classname, String *c_baseclass) {
        String *jniname = makeValidJniName(upcast_method_name);
        String *wname = Swig_name_wrapper(jniname);
        Printf(imclass_cppcasts_code, "  public final static native long %s(long jarg1);\n", upcast_method_name);
        if (smart) {
            SwigType *bsmart = Copy(smart);
            SwigType *rclassname = SwigType_typedef_resolve_all(c_classname);
            SwigType *rbaseclass = SwigType_typedef_resolve_all(c_baseclass);
            Replaceall(bsmart, rclassname, rbaseclass);
            Delete(rclassname);
            Delete(rbaseclass);
            String *smartnamestr = SwigType_namestr(smart);
            String *bsmartnamestr = SwigType_namestr(bsmart);
            Printv(upcasts_code,
                   "SWIGEXPORT jlong JNICALL ", wname, "(JNIEnv *jenv, jclass jcls, jlong jarg1) {\n",
                   "    jlong baseptr = 0;\n"
                           "    ", smartnamestr, " *argp1;\n"
                           "    (void)jenv;\n"
                           "    (void)jcls;\n"
                           "    argp1 = *(", smartnamestr, " **)&jarg1;\n"
                           "    *(", bsmartnamestr, " **)&baseptr = argp1 ? new ", bsmartnamestr, "(*argp1) : 0;\n"
                           "    return baseptr;\n"
                           "}\n", "\n", NIL);
            Delete(bsmartnamestr);
            Delete(smartnamestr);
            Delete(bsmart);
        } else {
            Printv(upcasts_code,
                   "SWIGEXPORT jlong JNICALL ", wname, "(JNIEnv *jenv, jclass jcls, jlong jarg1) {\n",
                   "    jlong baseptr = 0;\n"
                           "    (void)jenv;\n"
                           "    (void)jcls;\n"
                           "    *(", c_baseclass, " **)&baseptr = *(", c_classname, " **)&jarg1;\n"
                           "    return baseptr;\n"
                           "}\n", "\n", NIL);
        }
        Delete(wname);
        Delete(jniname);
    }

    /* -----------------------------------------------------------------------------
   * emitProxyClassDefAndCPPCasts()
   * ----------------------------------------------------------------------------- */

    void emitProxyClassDefAndCPPCasts(Node *n) {
        String *c_classname = SwigType_namestr(Getattr(n, "name"));
        String *c_baseclass = NULL;
        String *baseclass = NULL;
        String *c_baseclassname = NULL;
        String *interface_list = NewStringEmpty();
        String *interface_upcasts = NewStringEmpty();
        SwigType *typemap_lookup_type = Getattr(n, "classtypeobj");
        bool feature_director = Swig_directorclass(n) ? true : false;
        bool has_outerclass = Getattr(n, "nested:outer") != 0 && !GetFlag(n, "feature:flatnested");
        SwigType *smart = Swig_cparse_smartptr(n);

        // Inheritance from pure Java classes
        Node *attributes = NewHash();
        const String *pure_baseclass = typemapLookup(n, "javabase", typemap_lookup_type, WARN_NONE, attributes);
        bool purebase_replace = GetFlag(attributes, "tmap:javabase:replace") ? true : false;
        bool purebase_notderived = GetFlag(attributes, "tmap:javabase:notderived") ? true : false;
        Delete(attributes);

        // C++ inheritance
        if (!purebase_replace) {
            List *baselist = Getattr(n, "bases");
            if (baselist) {
                Iterator base = First(baselist);
                while (base.item) {
                    if (!(GetFlag(base.item, "feature:ignore") || Getattr(base.item, "feature:interface"))) {
                        String *baseclassname = Getattr(base.item, "name");
                        if (!c_baseclassname) {
                            c_baseclassname = baseclassname;
                            baseclass = Copy(getProxyName(baseclassname));
                            if (baseclass)
                                c_baseclass = SwigType_namestr(baseclassname);
                        } else {
                            /* Warn about multiple inheritance for additional base class(es) */
                            String *proxyclassname = Getattr(n, "classtypeobj");
                            Swig_warning(WARN_JAVA_MULTIPLE_INHERITANCE, Getfile(n), Getline(n),
                                         "Warning for %s, base %s ignored. Multiple inheritance is not supported in Java.\n", SwigType_namestr(proxyclassname), SwigType_namestr(baseclassname));
                        }
                    }
                    base = Next(base);
                }
            }
        }

        Hash *interface_bases = Getattr(n, "interface:bases");
        if (interface_bases)
            addInterfaceNameAndUpcasts(smart, interface_list, interface_upcasts, interface_bases, c_classname);

        bool derived = baseclass && getProxyName(c_baseclassname);
        if (derived && purebase_notderived)
            pure_baseclass = empty_string;
        const String *wanted_base = baseclass ? baseclass : pure_baseclass;

        if (purebase_replace) {
            wanted_base = pure_baseclass;
            derived = false;
            Delete(baseclass);
            baseclass = NULL;
            if (purebase_notderived)
                Swig_error(Getfile(n), Getline(n), "The javabase typemap for proxy %s must contain just one of the 'replace' or 'notderived' attributes.\n", typemap_lookup_type);
        } else if (Len(pure_baseclass) > 0 && Len(baseclass) > 0) {
            Swig_warning(WARN_JAVA_MULTIPLE_INHERITANCE, Getfile(n), Getline(n),
                         "Warning for %s, base %s ignored. Multiple inheritance is not supported in Java. "
                                 "Perhaps you need one of the 'replace' or 'notderived' attributes in the javabase typemap?\n", typemap_lookup_type, pure_baseclass);
        }

        // Pure Java interfaces
        const String *pure_interfaces = typemapLookup(n, "javainterfaces", typemap_lookup_type, WARN_NONE);
        if (*Char(interface_list) && *Char(pure_interfaces))
            Append(interface_list, ", ");
        Append(interface_list, pure_interfaces);
        // Start writing the proxy class
        if (!has_outerclass) // Import statements
            Printv(proxy_class_def, typemapLookup(n, "javaimports", typemap_lookup_type, WARN_NONE),"\n", NIL);
        else
            Printv(proxy_class_def, "static ", NIL); // C++ nested classes correspond to static java classes
        Printv(proxy_class_def, typemapLookup(n, "javaclassmodifiers", typemap_lookup_type, WARN_JAVA_TYPEMAP_CLASSMOD_UNDEF),	// Class modifiers
               " $javaclassname",	// Class name and bases
               (*Char(wanted_base)) ? " extends " : "", wanted_base, *Char(interface_list) ?	// Pure Java interfaces
                                                                     " implements " : "", interface_list, " {", derived ? typemapLookup(n, "javabody_derived", typemap_lookup_type, WARN_JAVA_TYPEMAP_JAVABODY_UNDEF) :	// main body of class
                                                                                                                typemapLookup(n, "javabody", typemap_lookup_type, WARN_JAVA_TYPEMAP_JAVABODY_UNDEF),	// main body of class
               NIL);

        // C++ destructor is wrapped by the delete method
        // Note that the method name is specified in a typemap attribute called methodname
        String *destruct = NewString("");
        const String *tm = NULL;
        attributes = NewHash();
        String *destruct_methodname = NULL;
        String *destruct_methodmodifiers = NULL;
        if (derived) {
            tm = typemapLookup(n, "javadestruct_derived", typemap_lookup_type, WARN_NONE, attributes);
            destruct_methodname = Getattr(attributes, "tmap:javadestruct_derived:methodname");
            destruct_methodmodifiers = Getattr(attributes, "tmap:javadestruct_derived:methodmodifiers");
        } else {
            tm = typemapLookup(n, "javadestruct", typemap_lookup_type, WARN_NONE, attributes);
            destruct_methodname = Getattr(attributes, "tmap:javadestruct:methodname");
            destruct_methodmodifiers = Getattr(attributes, "tmap:javadestruct:methodmodifiers");
        }
        if (tm && *Char(tm)) {
            if (!destruct_methodname) {
                Swig_error(Getfile(n), Getline(n), "No methodname attribute defined in javadestruct%s typemap for %s\n", (derived ? "_derived" : ""), proxy_class_name);
            }
            if (!destruct_methodmodifiers) {
                Swig_error(Getfile(n), Getline(n), "No methodmodifiers attribute defined in javadestruct%s typemap for %s.\n", (derived ? "_derived" : ""), proxy_class_name);
            }
        }
        // Emit the finalize and delete methods
        if (tm) {
            // Finalize method
            if (*Char(destructor_call)) {
                Printv(proxy_class_def, typemapLookup(n, "javafinalize", typemap_lookup_type, WARN_NONE), NIL);
            }
            // delete method
            Printv(destruct, tm, NIL);
            if (*Char(destructor_call))
                Replaceall(destruct, "$jnicall", destructor_call);
            else
                Replaceall(destruct, "$jnicall", "throw new UnsupportedOperationException(\"C++ destructor does not have public access\")");
            if (*Char(destruct))
                Printv(proxy_class_def, "\n  ", destruct_methodmodifiers, " func ", destruct_methodname, "() -> Void ", " ", destruct, "\n", NIL);
        }
        if (*Char(interface_upcasts))
            Printv(proxy_class_def, interface_upcasts, NIL);

//FIXME: No directors
        /* Insert directordisconnect typemap, if this class has directors enabled */
        /* Also insert the swigTakeOwnership and swigReleaseOwnership methods */
//        if (feature_director) {
//            String *destruct_jnicall, *release_jnicall, *take_jnicall;
//            String *changeown_method_name = Swig_name_member(getNSpace(), getClassPrefix(), "change_ownership");
//
//            destruct_jnicall = NewStringf("%s()", destruct_methodname);
//            release_jnicall = NewStringf("%s.%s(self, swigCPtr, false)", full_imclass_name, changeown_method_name);
//            take_jnicall = NewStringf("%s.%s(self, swigCPtr, true)", full_imclass_name, changeown_method_name);
//
//            emitCodeTypemap(n, false, typemap_lookup_type, "directordisconnect", "methodname", destruct_jnicall);
//            emitCodeTypemap(n, false, typemap_lookup_type, "directorowner_release", "methodname", release_jnicall);
//            emitCodeTypemap(n, false, typemap_lookup_type, "directorowner_take", "methodname", take_jnicall);
//
//            Delete(destruct_jnicall);
//            Delete(changeown_method_name);
//            Delete(release_jnicall);
//            Delete(take_jnicall);
//        }

        Delete(interface_upcasts);
        Delete(interface_list);
        Delete(attributes);
        Delete(destruct);

        // Emit extra user code
        Printv(proxy_class_def, typemapLookup(n, "javacode", typemap_lookup_type, WARN_NONE),	// extra Java code
               "\n", NIL);

        if (derived) {
            String *upcast_method_name = Swig_name_member(getNSpace(), getClassPrefix(), smart != 0 ? "SWIGSmartPtrUpcast" : "SWIGUpcast");
            upcastsCode(smart, upcast_method_name, c_classname, c_baseclass);
            Delete(upcast_method_name);
        }

        Delete(smart);
        Delete(baseclass);
    }

    /* ----------------------------------------------------------------------
   * emitInterfaceDeclaration()
   * ---------------------------------------------------------------------- */

    void emitInterfaceDeclaration(Node *n, String *interface_name, File *f_interface, String *nspace) {
        if (package || nspace) {
            Printf(f_interface, "package ");
            if (package)
                Printv(f_interface, package, nspace ? "." : "", NIL);
            if (nspace)
                Printv(f_interface, nspace, NIL);
            Printf(f_interface, ";\n");
        }

        Printv(f_interface, typemapLookup(n, "javaimports", Getattr(n, "classtypeobj"), WARN_NONE), "\n", NIL);
        Printf(f_interface, "public interface %s", interface_name);
        if (List *baselist = Getattr(n, "bases")) {
            String *bases = 0;
            for (Iterator base = First(baselist); base.item; base = Next(base)) {
                if (GetFlag(base.item, "feature:ignore") || !Getattr(base.item, "feature:interface"))
                    continue; // TODO: warn about skipped non-interface bases
                String *base_iname = Getattr(base.item, "interface:name");
                if (!bases)
                    bases = Copy(base_iname);
                else {
                    Append(bases, ", ");
                    Append(bases, base_iname);
                }
            }
            if (bases) {
                Printv(f_interface, " extends ", bases, NIL);
                Delete(bases);
            }
        }
        Printf(f_interface, " {\n");

        Node *attributes = NewHash();
        String *interface_code = Copy(typemapLookup(n, "javainterfacecode", Getattr(n, "classtypeobj"), WARN_JAVA_TYPEMAP_INTERFACECODE_UNDEF, attributes));
        if (interface_code) {
            String *interface_declaration = Copy(Getattr(attributes, "tmap:javainterfacecode:declaration"));
            if (interface_declaration) {
                Replaceall(interface_declaration, "$interfacename", interface_name);
                Printv(f_interface, interface_declaration, NIL);
                Delete(interface_declaration);
            }
            Delete(interface_code);
        }
    }

    /* ----------------------------------------------------------------------
  * classDeclaration()
  * ---------------------------------------------------------------------- */

    int classDeclaration(Node *n) {
        return Language::classDeclaration(n);
    }

    /* ----------------------------------------------------------------------
  * classHandler()
  * ---------------------------------------------------------------------- */

    virtual int classHandler(Node *n) {
        File *f_proxy = NULL;
        File *f_interface = NULL;
        String *old_proxy_class_name = proxy_class_name;
        String *old_full_proxy_class_name = full_proxy_class_name;
        String *old_full_imclass_name = full_imclass_name;
        String *old_destructor_call = destructor_call;
        String *old_destructor_throws_clause = destructor_throws_clause;
        String *old_proxy_class_constants_code = proxy_class_constants_code;
        String *old_proxy_class_def = proxy_class_def;
        String *old_proxy_class_code = proxy_class_code;
        bool has_outerclass = Getattr(n, "nested:outer") && !GetFlag(n, "feature:flatnested");
        String *old_interface_class_code = interface_class_code;
        interface_class_code = 0;

        if (proxy_flag) {
            proxy_class_name = NewString(Getattr(n, "sym:name"));
            String *nspace = getNSpace();
            constructIntermediateClassName(n);

            String *outerClassesPrefix = 0;
            if (Node *outer = Getattr(n, "nested:outer")) {
                outerClassesPrefix = Copy(Getattr(outer, "sym:name"));
                for (outer = Getattr(outer, "nested:outer"); outer != 0; outer = Getattr(outer, "nested:outer")) {
                    Push(outerClassesPrefix, ".");
                    Push(outerClassesPrefix, Getattr(outer, "sym:name"));
                }
            }
            if (!nspace) {
                full_proxy_class_name = outerClassesPrefix ? NewStringf("%s.%s", outerClassesPrefix, proxy_class_name) : NewStringf("%s", proxy_class_name);

                if (Cmp(proxy_class_name, imclass_name) == 0) {
                    Printf(stderr, "Class name cannot be equal to intermediary class name: %s\n", proxy_class_name);
                    SWIG_exit(EXIT_FAILURE);
                }

                if (Cmp(proxy_class_name, module_class_name) == 0) {
                    Printf(stderr, "Class name cannot be equal to module class name: %s\n", proxy_class_name);
                    SWIG_exit(EXIT_FAILURE);
                }
            } else {
                if (outerClassesPrefix) {
                    if (package)
                        full_proxy_class_name = NewStringf("%s.%s.%s.%s", package, nspace, outerClassesPrefix, proxy_class_name);
                    else
                        full_proxy_class_name = NewStringf("%s.%s.%s", nspace, outerClassesPrefix, proxy_class_name);
                } else {
                    if (package)
                        full_proxy_class_name = NewStringf("%s.%s.%s", package, nspace, proxy_class_name);
                    else
                        full_proxy_class_name = NewStringf("%s.%s", nspace, proxy_class_name);
                }
            }

            String *interface_name = Getattr(n, "feature:interface") ? Getattr(n, "interface:name") : 0;
            if (outerClassesPrefix) {
                String *fnspace = nspace ? NewStringf("%s.%s", nspace, outerClassesPrefix) : outerClassesPrefix;
                if (!addSymbol(proxy_class_name, n, fnspace))
                    return SWIG_ERROR;
                if (interface_name && !addInterfaceSymbol(interface_name, n, fnspace))
                    return SWIG_ERROR;
                if (nspace)
                    Delete(fnspace);
                Delete(outerClassesPrefix);
            } else {
                if (!addSymbol(proxy_class_name, n, nspace))
                    return SWIG_ERROR;
                if (interface_name && !addInterfaceSymbol(interface_name, n, nspace))
                    return SWIG_ERROR;
            }

            // Each outer proxy class goes into a separate file
            if (!has_outerclass) {
                String *output_directory = outputDirectory(nspace);
                String *filen = NewStringf("%s%s.swift", output_directory, proxy_class_name);
                f_proxy = NewFile(filen, "w", SWIG_output_files());
                if (!f_proxy) {
                    FileErrorDisplay(filen);
                    SWIG_exit(EXIT_FAILURE);
                }
                Append(filenames_list, Copy(filen));
                Delete(filen);
                Delete(output_directory);

                // Start writing out the proxy class file
                emitBanner(f_proxy);

// No packages for swift.
//                if (package || nspace) {
//                    Printf(f_proxy, "package ");
//                    if (package)
//                        Printv(f_proxy, package, nspace ? "." : "", NIL);
//                    if (nspace)
//                        Printv(f_proxy, nspace, NIL);
//                    Printf(f_proxy, ";\n");
//                }
            }
            else
                ++nesting_depth;

            proxy_class_def = NewString("");
            proxy_class_code = NewString("");
            destructor_call = NewString("");
            destructor_throws_clause = NewString("");
            proxy_class_constants_code = NewString("");

            if (Getattr(n, "feature:interface")) {
                interface_class_code = NewString("");
                String *output_directory = outputDirectory(nspace);
                String *filen = NewStringf("%s%s.java", output_directory, interface_name);
                f_interface = NewFile(filen, "w", SWIG_output_files());
                if (!f_interface) {
                    FileErrorDisplay(filen);
                    SWIG_exit(EXIT_FAILURE);
                }
                Append(filenames_list, filen); // file name ownership goes to the list
                emitBanner(f_interface);
                emitInterfaceDeclaration(n, interface_name, interface_class_code, nspace);
                Delete(filen);
                Delete(output_directory);
            }
        }

        Language::classHandler(n);

        if (proxy_flag) {
            emitProxyClassDefAndCPPCasts(n);

            String *javaclazzname = Swig_name_member(getNSpace(), getClassPrefix(), ""); // mangled full proxy class name

            Replaceall(proxy_class_def, "$javaclassname", proxy_class_name);
            Replaceall(proxy_class_code, "$javaclassname", proxy_class_name);
            Replaceall(proxy_class_constants_code, "$javaclassname", proxy_class_name);
            Replaceall(interface_class_code, "$javaclassname", proxy_class_name);

            Replaceall(proxy_class_def, "$javaclazzname", javaclazzname);
            Replaceall(proxy_class_code, "$javaclazzname", javaclazzname);
            Replaceall(proxy_class_constants_code, "$javaclazzname", javaclazzname);
            Replaceall(interface_class_code, "$javaclazzname", javaclazzname);

            Replaceall(proxy_class_def, "$module", module_class_name);
            Replaceall(proxy_class_code, "$module", module_class_name);
            Replaceall(proxy_class_constants_code, "$module", module_class_name);
            Replaceall(interface_class_code, "$module", module_class_name);

            Replaceall(proxy_class_def, "$imclassname", full_imclass_name);
            Replaceall(proxy_class_code, "$imclassname", full_imclass_name);
            Replaceall(proxy_class_constants_code, "$imclassname", full_imclass_name);
            Replaceall(interface_class_code, "$imclassname", full_imclass_name);

            if (!has_outerclass)
                Printv(f_proxy, proxy_class_def, proxy_class_code, NIL);
            else {
                Swig_offset_string(proxy_class_def, nesting_depth);
                Append(old_proxy_class_code, proxy_class_def);
                Swig_offset_string(proxy_class_code, nesting_depth);
                Append(old_proxy_class_code, proxy_class_code);
            }

            // Write out all the constants
            if (Len(proxy_class_constants_code) != 0) {
                if (!has_outerclass)
                    Printv(f_proxy, proxy_class_constants_code, NIL);
                else {
                    Swig_offset_string(proxy_class_constants_code, nesting_depth);
                    Append(old_proxy_class_code, proxy_class_constants_code);
                }
            }

            if (!has_outerclass) {
                Printf(f_proxy, "}\n");
                Delete(f_proxy);
                f_proxy = NULL;
            } else {
                for (int i = 0; i < nesting_depth; ++i)
                    Append(old_proxy_class_code, "  ");
                Append(old_proxy_class_code, "}\n\n");
                --nesting_depth;
            }

            if (f_interface) {
                Printv(f_interface, interface_class_code, "}\n", NIL);
                Delete(f_interface);
                f_interface = 0;
            }

            //FIXME: remove
//            emitDirectorExtraMethods(n);

            Delete(interface_class_code);
            interface_class_code = old_interface_class_code;
            Delete(javaclazzname);
            Delete(proxy_class_name);
            proxy_class_name = old_proxy_class_name;
            Delete(full_proxy_class_name);
            full_proxy_class_name = old_full_proxy_class_name;
            Delete(full_imclass_name);
            full_imclass_name = old_full_imclass_name;
            Delete(destructor_call);
            destructor_call = old_destructor_call;
            Delete(destructor_throws_clause);
            destructor_throws_clause = old_destructor_throws_clause;
            Delete(proxy_class_constants_code);
            proxy_class_constants_code = old_proxy_class_constants_code;
            Delete(proxy_class_def);
            proxy_class_def = old_proxy_class_def;
            Delete(proxy_class_code);
            proxy_class_code = old_proxy_class_code;
        }

        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * memberfunctionHandler()
   * ---------------------------------------------------------------------- */

    virtual int memberfunctionHandler(Node *n) {
        member_func_flag = true;
        Language::memberfunctionHandler(n);

        if (proxy_flag) {
            String *overloaded_name = getOverloadedName(n);
            String *intermediary_function_name = Swig_name_member(getNSpace(), getClassPrefix(), overloaded_name);
            Setattr(n, "proxyfuncname", Getattr(n, "sym:name"));
            Setattr(n, "imfuncname", intermediary_function_name);
            proxyClassFunctionHandler(n);
            Delete(overloaded_name);
        }
        member_func_flag = false;
        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * staticmemberfunctionHandler()
   * ---------------------------------------------------------------------- */

    virtual int staticmemberfunctionHandler(Node *n) {

        static_flag = true;
        member_func_flag = true;
        Language::staticmemberfunctionHandler(n);

        if (proxy_flag) {
            String *overloaded_name = getOverloadedName(n);
            String *intermediary_function_name = Swig_name_member(getNSpace(), getClassPrefix(), overloaded_name);
            Setattr(n, "proxyfuncname", Getattr(n, "sym:name"));
            Setattr(n, "imfuncname", intermediary_function_name);
            proxyClassFunctionHandler(n);
            Delete(overloaded_name);
        }
        static_flag = false;
        member_func_flag = false;

        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------------
   * proxyClassFunctionHandler()
   *
   * Function called for creating a Java wrapper function around a c++ function in the 
   * proxy class. Used for both static and non-static C++ class functions.
   * C++ class static functions map to Java static functions.
   * Two extra attributes in the Node must be available. These are "proxyfuncname" - 
   * the name of the Java class proxy function, which in turn will call "imfuncname" - 
   * the intermediary (JNI) function name in the intermediary class.
   * ----------------------------------------------------------------------------- */

    void proxyClassFunctionHandler(Node *n) {
        SwigType *t = Getattr(n, "type");
        ParmList *l = Getattr(n, "parms");
        String *intermediary_function_name = Getattr(n, "imfuncname");
        String *proxy_function_name = Getattr(n, "proxyfuncname");
        String *tm;
        Parm *p;
        int i;
        String *imcall = NewString("");
        String *return_type = NewString("");
        String *function_code = NewString("");
        bool setter_flag = false;
        String *pre_code = NewString("");
        String *post_code = NewString("");
        bool is_interface = Getattr(parentNode(n), "feature:interface") != 0
                            && !static_flag && Getattr(n, "interface:owner") == 0;

        if (!proxy_flag)
            return;

        // Wrappers not wanted for some methods where the parameters cannot be overloaded in Java
        if (Getattr(n, "overload:ignore"))
            return;

        // Don't generate proxy method for additional explicitcall method used in directors
        if (GetFlag(n, "explicitcall"))
            return;

        if (l) {
            if (SwigType_type(Getattr(l, "type")) == T_VOID) {
                l = nextSibling(l);
            }
        }

        /* Attach the non-standard typemaps to the parameter list */
        Swig_typemap_attach_parms("in", l, NULL);
        Swig_typemap_attach_parms("jtype", l, NULL);
        Swig_typemap_attach_parms("jstype", l, NULL);
        Swig_typemap_attach_parms("javain", l, NULL);

        /* Get return types */
        if ((tm = Swig_typemap_lookup("jstype", n, "", 0))) {
            // Note that in the case of polymorphic (covariant) return types, the method's return type is changed to be the base of the C++ return type
            SwigType *covariant = Getattr(n, "covariant");
            substituteClassname(covariant ? covariant : t, tm);
            Printf(return_type, "%s", tm);
            if (covariant)
                Swig_warning(WARN_JAVA_COVARIANT_RET, input_file, line_number,
                             "Covariant return types not supported in Java. Proxy method will return %s.\n", SwigType_str(covariant, 0));
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(t, 0));
        }

        if (wrapping_member_flag && !enum_constant_flag) {
            // For wrapping member variables (Javabean setter)
            setter_flag = (Cmp(Getattr(n, "sym:name"), Swig_name_set(getNSpace(), Swig_name_member(0, getClassPrefix(), variable_name))) == 0);
        }

        /* Start generating the proxy function */
        const String *methodmods = Getattr(n, "feature:java:methodmodifiers");
        methodmods = methodmods ? methodmods : (is_public(n) ? public_string : protected_string);
        Printf(function_code, "  %s ", methodmods);
        if (static_flag)
            Printf(function_code, "static ");
        Printf(function_code, "func %s(", proxy_function_name);

        if (is_interface)
            Printf(interface_class_code, "func %s(", proxy_function_name);

        Printv(imcall, "$imfuncname(", NIL);
        if (!static_flag) {
            Printf(imcall, "swigCPtr");

            String *this_type = Copy(getClassType());
            String *name = NewString("jself");
            String *qualifier = Getattr(n, "qualifier");
            if (qualifier)
                SwigType_push(this_type, qualifier);
            SwigType_add_pointer(this_type);
            Parm *this_parm = NewParm(this_type, name, n);
            Swig_typemap_attach_parms("jtype", this_parm, NULL);
            Swig_typemap_attach_parms("jstype", this_parm, NULL);

// TODO: See if this is necessary.
//            if (prematureGarbageCollectionPreventionParameter(this_type, this_parm))
//                Printf(imcall, ", self");

            Delete(this_parm);
            Delete(name);
            Delete(this_type);
        }

        emit_mark_varargs(l);

        int gencomma = !static_flag;

        /* Output each parameter */
        for (i = 0, p = l; p; i++) {

            /* Ignored varargs */
            if (checkAttribute(p, "varargs:ignore", "1")) {
                p = nextSibling(p);
                continue;
            }

            /* Ignored parameters */
            if (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
                continue;
            }

            /* Ignore the 'this' argument for variable wrappers */
            if (!(variable_wrapper_flag && i == 0) || static_flag) {
                SwigType *pt = Getattr(p, "type");
                String *param_type = NewString("");

                /* Get the Java parameter type */
                if ((tm = Getattr(p, "tmap:jstype"))) {
                    substituteClassname(pt, tm);
                    Printf(param_type, "%s", tm);
                } else {
                    Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(pt, 0));
                }

                if (gencomma)
                    Printf(imcall, ", ");

                String *arg = makeParameterName(n, p, i, setter_flag);

                // Use typemaps to transform type used in Java proxy wrapper (in proxy class) to type used in JNI function (in intermediary class)
                if ((tm = Getattr(p, "tmap:javain"))) {
                    addThrows(n, "tmap:javain", p);
                    substituteClassname(pt, tm);
                    Replaceall(tm, "$javainput", arg);
                    String *pre = Getattr(p, "tmap:javain:pre");
                    if (pre) {
                        substituteClassname(pt, pre);
                        Replaceall(pre, "$javainput", arg);
                        if (Len(pre_code) > 0)
                            Printf(pre_code, "\n");
                        Printv(pre_code, pre, NIL);
                    }
                    String *post = Getattr(p, "tmap:javain:post");
                    if (post) {
                        substituteClassname(pt, post);
                        Replaceall(post, "$javainput", arg);
                        if (Len(post_code) > 0)
                            Printf(post_code, "\n");
                        Printv(post_code, post, NIL);
                    }
                    Printv(imcall, tm, NIL);
                } else {
                    Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, "No javain typemap defined for %s\n", SwigType_str(pt, 0));
                }

                /* Add parameter to proxy function */
                if (gencomma >= 2) {
                    Printf(function_code, ", ");
                    if (is_interface)
                        Printf(interface_class_code, ", ");
                }
                gencomma = 2;
                Printf(function_code, "%s: %s", arg, param_type);
                if (is_interface)
                    Printf(interface_class_code, "%s: %s", arg, param_type);

// TODO: Find out if this is necessary.
//                if (prematureGarbageCollectionPreventionParameter(pt, p)) {
//                    String *pgcppname = Getattr(p, "tmap:javain:pgcppname");
//                    if (pgcppname) {
//                        String *argname = Copy(pgcppname);
//                        Replaceall(argname, "$javainput", arg);
//                        Printf(imcall, ", %s", argname);
//                        Delete(argname);
//                    } else {
//                        Printf(imcall, ", %s", arg);
//                    }
//                }

                Delete(arg);
                Delete(param_type);
            }
            p = Getattr(p, "tmap:in:next");
        }

        Printf(imcall, ")");
        Printf(function_code, ") -> %s", return_type);

        // Transform return type used in JNI function (in intermediary class) to type used in Java wrapper function (in proxy class)
        if ((tm = Swig_typemap_lookup("javaout", n, "", 0))) {
            addThrows(n, "tmap:javaout", n);
            bool is_pre_code = Len(pre_code) > 0;
            bool is_post_code = Len(post_code) > 0;
            if (is_pre_code || is_post_code) {
                Replaceall(tm, "\n ", "\n   "); // add extra indentation to code in typemap
                if (is_post_code) {
                    Insert(tm, 0, "\n    try ");
                    Printv(tm, " finally {\n", post_code, "\n    }", NIL);
                } else {
                    Insert(tm, 0, "\n    ");
                }
                if (is_pre_code) {
                    Insert(tm, 0, pre_code);
                    Insert(tm, 0, "\n");
                }
                Insert(tm, 0, "{");
                Printf(tm, "\n  }");
            }
            if (GetFlag(n, "feature:new"))
                Replaceall(tm, "$owner", "true");
            else
                Replaceall(tm, "$owner", "false");
            substituteClassname(t, tm);

            // For director methods: generate code to selectively make a normal polymorphic call or 
            // an explicit method call - needed to prevent infinite recursion calls in director methods.
            Node *explicit_n = Getattr(n, "explicitcallnode");
            if (explicit_n) {
                String *ex_overloaded_name = getOverloadedName(explicit_n);
                String *ex_intermediary_function_name = Swig_name_member(getNSpace(), getClassPrefix(), ex_overloaded_name);

                String *ex_imcall = Copy(imcall);
                Replaceall(ex_imcall, "$imfuncname", ex_intermediary_function_name);
                Replaceall(imcall, "$imfuncname", intermediary_function_name);

                String *excode = NewString("");
                if (!Cmp(return_type, "void"))
                    Printf(excode, "if (getClass() == %s.class) %s; else %s", proxy_class_name, imcall, ex_imcall);
                else
                    Printf(excode, "(getClass() == %s.class) ? %s : %s", proxy_class_name, imcall, ex_imcall);

                Clear(imcall);
                Printv(imcall, excode, NIL);
                Delete(ex_overloaded_name);
                Delete(excode);
            } else {
                String *imfunction_name = makeValidIntermediateName(intermediary_function_name);
                Replaceall(imcall, "$imfuncname", imfunction_name);
                Delete(imfunction_name);
            }

            Replaceall(tm, "$jnicall", imcall);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JAVAOUT_UNDEF, input_file, line_number, "No javaout typemap defined for %s\n", SwigType_str(t, 0));
        }

        if (is_interface) {
            Printf(interface_class_code, ") -> %s", return_type);
            generateThrowsClause(n, interface_class_code);
            Printf(interface_class_code, ";\n");
        }
        generateThrowsClause(n, function_code);
        Printf(function_code, " %s\n\n", tm ? (const String *) tm : empty_string);
        Printv(proxy_class_code, function_code, NIL);

        Delete(pre_code);
        Delete(post_code);
        Delete(function_code);
        Delete(return_type);
        Delete(imcall);
    }

    /* ----------------------------------------------------------------------
   * constructorHandler()
   * ---------------------------------------------------------------------- */

    virtual int constructorHandler(Node *n) {

        ParmList *l = Getattr(n, "parms");
        String *tm;
        Parm *p;
        int i;
        String *function_code = NewString("");
        String *helper_code = NewString(""); // Holds code for the constructor helper method generated only when the javain typemap has code in the pre or post attributes
        String *helper_args = NewString("");
        String *pre_code = NewString("");
        String *post_code = NewString("");
        String *im_return_type = NewString("");
        bool feature_director = (parentNode(n) && Swig_directorclass(n));

        Language::constructorHandler(n);

        // Wrappers not wanted for some methods where the parameters cannot be overloaded in Java
        if (Getattr(n, "overload:ignore"))
            return SWIG_OK;

        if (proxy_flag) {
            String *overloaded_name = getOverloadedName(n);
            String *imcall = NewString("");

            const String *methodmods = Getattr(n, "feature:java:methodmodifiers");
            methodmods = methodmods ? methodmods : (is_public(n) ? public_string : protected_string);

            tm = Getattr(n, "tmap:jtype"); // typemaps were attached earlier to the node
            Printf(im_return_type, "%s", tm);

            Printf(function_code, "  %s convenience init(", methodmods);
            Printf(helper_code, "  static private %s SwigConstruct%s(", im_return_type, proxy_class_name);

            String *raw_im_name = NewString("new_");
            Append(raw_im_name, overloaded_name);
            String *valid_im_name = makeValidIntermediateName(raw_im_name);

            Printv(imcall, valid_im_name, "(", NIL);

            Delete(valid_im_name);
            Delete(raw_im_name);

            /* Attach the non-standard typemaps to the parameter list */
            Swig_typemap_attach_parms("in", l, NULL);
            Swig_typemap_attach_parms("jtype", l, NULL);
            Swig_typemap_attach_parms("jstype", l, NULL);
            Swig_typemap_attach_parms("javain", l, NULL);

            emit_mark_varargs(l);

            int gencomma = 0;

            /* Output each parameter */
            for (i = 0, p = l; p; i++) {

                /* Ignored varargs */
                if (checkAttribute(p, "varargs:ignore", "1")) {
                    p = nextSibling(p);
                    continue;
                }

                /* Ignored parameters */
                if (checkAttribute(p, "tmap:in:numinputs", "0")) {
                    p = Getattr(p, "tmap:in:next");
                    continue;
                }

                SwigType *pt = Getattr(p, "type");
                String *param_type = NewString("");

                /* Get the Java parameter type */
                if ((tm = Getattr(p, "tmap:jstype"))) {
                    substituteClassname(pt, tm);
                    Printf(param_type, "%s", tm);
                } else {
                    Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(pt, 0));
                }

                if (gencomma)
                    Printf(imcall, ", ");

                String *arg = makeParameterName(n, p, i, false);

                // Use typemaps to transform type used in Java wrapper function (in proxy class) to type used in JNI function (in intermediary class)
                if ((tm = Getattr(p, "tmap:javain"))) {
                    addThrows(n, "tmap:javain", p);
                    substituteClassname(pt, tm);
                    Replaceall(tm, "$javainput", arg);
                    String *pre = Getattr(p, "tmap:javain:pre");
                    if (pre) {
                        substituteClassname(pt, pre);
                        Replaceall(pre, "$javainput", arg);
                        if (Len(pre_code) > 0)
                            Printf(pre_code, "\n");
                        Printv(pre_code, pre, NIL);
                    }
                    String *post = Getattr(p, "tmap:javain:post");
                    if (post) {
                        substituteClassname(pt, post);
                        Replaceall(post, "$javainput", arg);
                        if (Len(post_code) > 0)
                            Printf(post_code, "\n");
                        Printv(post_code, post, NIL);
                    }
                    Printv(imcall, tm, NIL);
                } else {
                    Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, "No javain typemap defined for %s\n", SwigType_str(pt, 0));
                }

                /* Add parameter to proxy function */
                if (gencomma) {
                    Printf(function_code, ", ");
                    Printf(helper_code, ", ");
                    Printf(helper_args, ", ");
                }
                Printf(function_code, "%s %s", param_type, arg);
                Printf(helper_code, "%s %s", param_type, arg);
                Printf(helper_args, "%s", arg);
                ++gencomma;

                if (prematureGarbageCollectionPreventionParameter(pt, p)) {
                    String *pgcppname = Getattr(p, "tmap:javain:pgcppname");
                    if (pgcppname) {
                        String *argname = Copy(pgcppname);
                        Replaceall(argname, "$javainput", arg);
                        Printf(imcall, ", %s", argname);
                        Delete(argname);
                    } else {
                        Printf(imcall, ", %s", arg);
                    }
                }

                Delete(arg);
                Delete(param_type);
                p = Getattr(p, "tmap:in:next");
            }

            Printf(imcall, ")");

            Printf(function_code, ")");
            Printf(helper_code, ")");
            generateThrowsClause(n, function_code);

            /* Insert the javaconstruct typemap, doing the replacement for $directorconnect, as needed */
            Hash *attributes = NewHash();
            String *construct_tm = Copy(typemapLookup(n, "javaconstruct", Getattr(n, "name"),
                                                      WARN_JAVA_TYPEMAP_JAVACONSTRUCT_UNDEF, attributes));
            if (construct_tm) {
                if (!feature_director) {
                    Replaceall(construct_tm, "$directorconnect", "");
                } else {
                    String *connect_attr = Getattr(attributes, "tmap:javaconstruct:directorconnect");

                    if (connect_attr) {
                        Replaceall(construct_tm, "$directorconnect", connect_attr);
                    } else {
                        Swig_warning(WARN_JAVA_NO_DIRECTORCONNECT_ATTR, input_file, line_number, "\"directorconnect\" attribute missing in %s \"javaconstruct\" typemap.\n",
                                     Getattr(n, "name"));
                        Replaceall(construct_tm, "$directorconnect", "");
                    }
                }

                Printv(function_code, " ", construct_tm, "\n", NIL);
            }

            bool is_pre_code = Len(pre_code) > 0;
            bool is_post_code = Len(post_code) > 0;
            if (is_pre_code || is_post_code) {
                generateThrowsClause(n, helper_code);
                Printf(helper_code, " {\n");
                if (is_pre_code) {
                    Printv(helper_code, pre_code, "\n", NIL);
                }
                if (is_post_code) {
                    Printf(helper_code, "    try {\n");
                    Printv(helper_code, "      return ", imcall, ";\n", NIL);
                    Printv(helper_code, "    } finally {\n", post_code, "\n    }", NIL);
                } else {
                    Printv(helper_code, "    return ", imcall, ";", NIL);
                }
                Printf(helper_code, "\n  }\n");
                String *helper_name = NewStringf("%s.SwigConstruct%s(%s)", proxy_class_name, proxy_class_name, helper_args);
                Printv(proxy_class_code, helper_code, "\n", NIL);
                Replaceall(function_code, "$imcall", helper_name);
                Delete(helper_name);
            } else {
                Replaceall(function_code, "$imcall", imcall);
            }

            Printv(proxy_class_code, function_code, "\n", NIL);

            Delete(helper_args);
            Delete(im_return_type);
            Delete(pre_code);
            Delete(post_code);
            Delete(construct_tm);
            Delete(attributes);
            Delete(overloaded_name);
            Delete(imcall);
        }

        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * destructorHandler()
   * ---------------------------------------------------------------------- */

    virtual int destructorHandler(Node *n) {
        Language::destructorHandler(n);
        String *symname = Getattr(n, "sym:name");

        if (proxy_flag) {
            String *raw_im_name = NewString("delete_");
            Append(raw_im_name, symname);
            String *valid_im_name = makeValidIntermediateName(raw_im_name);

            Printv(destructor_call, valid_im_name, "(swigCPtr)", NIL);
            Delete(valid_im_name);
            Delete(raw_im_name);

            generateThrowsClause(n, destructor_throws_clause);
        }
        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * membervariableHandler()
   * ---------------------------------------------------------------------- */

    virtual int membervariableHandler(Node *n) {
        variable_name = Getattr(n, "sym:name");
        wrapping_member_flag = true;
        variable_wrapper_flag = true;
        Language::membervariableHandler(n);
        wrapping_member_flag = false;
        variable_wrapper_flag = false;
        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * staticmembervariableHandler()
   * ---------------------------------------------------------------------- */

    virtual int staticmembervariableHandler(Node *n) {
        variable_name = Getattr(n, "sym:name");
        wrapping_member_flag = true;
        static_flag = true;
        Language::staticmembervariableHandler(n);
        wrapping_member_flag = false;
        static_flag = false;
        return SWIG_OK;
    }

    /* ----------------------------------------------------------------------
   * memberconstantHandler()
   * ---------------------------------------------------------------------- */

    virtual int memberconstantHandler(Node *n) {
        variable_name = Getattr(n, "sym:name");
        wrapping_member_flag = true;
        Language::memberconstantHandler(n);
        wrapping_member_flag = false;
        return SWIG_OK;
    }

    /* -----------------------------------------------------------------------------
   * getOverloadedName()
   * ----------------------------------------------------------------------------- */

    String *getOverloadedName(Node *n) {

        /* Although JNI functions are designed to handle overloaded Java functions,
     * a Java long is used for all classes in the SWIG intermediary class.
     * The intermediary class methods are thus mangled when overloaded to give
     * a unique name. */
        String *overloaded_name = NewStringf("%s", Getattr(n, "sym:name"));

        if (Getattr(n, "sym:overloaded")) {
            Printv(overloaded_name, Getattr(n, "sym:overname"), NIL);
        }

        return overloaded_name;
    }

    /* -----------------------------------------------------------------------------
   * moduleClassFunctionHandler()
   * ----------------------------------------------------------------------------- */

    void moduleClassFunctionHandler(Node *n) {
        SwigType *t = Getattr(n, "type");
        ParmList *l = Getattr(n, "parms");
        String *tm;
        Parm *p;
        int i;
        String *imcall = NewString("");
        String *return_type = NewString("");
        String *function_code = NewString("");
        int num_arguments = 0;
        String *overloaded_name = getOverloadedName(n);
        String *func_name = NULL;
        bool setter_flag = false;
        String *pre_code = NewString("");
        String *post_code = NewString("");

        if (l) {
            if (SwigType_type(Getattr(l, "type")) == T_VOID) {
                l = nextSibling(l);
            }
        }

        /* Attach the non-standard typemaps to the parameter list */
        Swig_typemap_attach_parms("jstype", l, NULL);
        Swig_typemap_attach_parms("javain", l, NULL);

        /* Get return types */
        if ((tm = Swig_typemap_lookup("jstype", n, "", 0))) {
            substituteClassname(t, tm);
            Printf(return_type, "%s", tm);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(t, 0));
        }

        /* Change function name for global variables */
        if (proxy_flag && global_variable_flag) {
            // Capitalize the first letter in the variable to create a JavaBean type getter/setter function name
            func_name = NewString("");
            setter_flag = (Cmp(Getattr(n, "sym:name"), Swig_name_set(getNSpace(), variable_name)) == 0);
            if (setter_flag)
                Printf(func_name, "set");
            else
                Printf(func_name, "get");
            Putc(toupper((int) *Char(variable_name)), func_name);
            Printf(func_name, "%s", Char(variable_name) + 1);
        } else {
            func_name = Copy(Getattr(n, "sym:name"));
        }

        /* Start generating the function */
        const String *methodmods = Getattr(n, "feature:java:methodmodifiers");
        methodmods = methodmods ? methodmods : (is_public(n) ? public_string : protected_string);
        Printf(function_code, "  %s static %s %s(", methodmods, return_type, func_name);
        Printv(imcall, imclass_name, ".", overloaded_name, "(", NIL);

        /* Get number of required and total arguments */
        num_arguments = emit_num_arguments(l);

        bool global_or_member_variable = global_variable_flag || (wrapping_member_flag && !enum_constant_flag);
        int gencomma = 0;

        /* Output each parameter */
        for (i = 0, p = l; i < num_arguments; i++) {

            /* Ignored parameters */
            while (checkAttribute(p, "tmap:in:numinputs", "0")) {
                p = Getattr(p, "tmap:in:next");
            }

            SwigType *pt = Getattr(p, "type");
            String *param_type = NewString("");

            /* Get the Java parameter type */
            if ((tm = Getattr(p, "tmap:jstype"))) {
                substituteClassname(pt, tm);
                Printf(param_type, "%s", tm);
            } else {
                Swig_warning(WARN_JAVA_TYPEMAP_JSTYPE_UNDEF, input_file, line_number, "No jstype typemap defined for %s\n", SwigType_str(pt, 0));
            }

            if (gencomma)
                Printf(imcall, ", ");

            String *arg = makeParameterName(n, p, i, global_or_member_variable);

            // Use typemaps to transform type used in Java wrapper function (in proxy class) to type used in JNI function (in intermediary class)
            if ((tm = Getattr(p, "tmap:javain"))) {
                addThrows(n, "tmap:javain", p);
                substituteClassname(pt, tm);
                Replaceall(tm, "$javainput", arg);
                String *pre = Getattr(p, "tmap:javain:pre");
                if (pre) {
                    substituteClassname(pt, pre);
                    Replaceall(pre, "$javainput", arg);
                    if (Len(pre_code) > 0)
                        Printf(pre_code, "\n");
                    Printv(pre_code, pre, NIL);
                }
                String *post = Getattr(p, "tmap:javain:post");
                if (post) {
                    substituteClassname(pt, post);
                    Replaceall(post, "$javainput", arg);
                    if (Len(post_code) > 0)
                        Printf(post_code, "\n");
                    Printv(post_code, post, NIL);
                }
                Printv(imcall, tm, NIL);
            } else {
                Swig_warning(WARN_JAVA_TYPEMAP_JAVAIN_UNDEF, input_file, line_number, "No javain typemap defined for %s\n", SwigType_str(pt, 0));
            }

            /* Add parameter to module class function */
            if (gencomma >= 2)
                Printf(function_code, ", ");
            gencomma = 2;
            Printf(function_code, "%s %s", param_type, arg);

            if (prematureGarbageCollectionPreventionParameter(pt, p)) {
                String *pgcppname = Getattr(p, "tmap:javain:pgcppname");
                if (pgcppname) {
                    String *argname = Copy(pgcppname);
                    Replaceall(argname, "$javainput", arg);
                    Printf(imcall, ", %s", argname);
                    Delete(argname);
                } else {
                    Printf(imcall, ", %s", arg);
                }
            }

            p = Getattr(p, "tmap:in:next");
            Delete(arg);
            Delete(param_type);
        }

        Printf(imcall, ")");
        Printf(function_code, ")");

        // Transform return type used in JNI function (in intermediary class) to type used in Java wrapper function (in module class)
        if ((tm = Swig_typemap_lookup("javaout", n, "", 0))) {
            addThrows(n, "tmap:javaout", n);
            bool is_pre_code = Len(pre_code) > 0;
            bool is_post_code = Len(post_code) > 0;
            if (is_pre_code || is_post_code) {
                Replaceall(tm, "\n ", "\n   "); // add extra indentation to code in typemap
                if (is_post_code) {
                    Insert(tm, 0, "\n    try ");
                    Printv(tm, " finally {\n", post_code, "\n    }", NIL);
                } else {
                    Insert(tm, 0, "\n    ");
                }
                if (is_pre_code) {
                    Insert(tm, 0, pre_code);
                    Insert(tm, 0, "\n");
                }
                Insert(tm, 0, "{");
                Printf(tm, "\n  }");
            }
            if (GetFlag(n, "feature:new"))
                Replaceall(tm, "$owner", "true");
            else
                Replaceall(tm, "$owner", "false");
            substituteClassname(t, tm);
            Replaceall(tm, "$jnicall", imcall);
        } else {
            Swig_warning(WARN_JAVA_TYPEMAP_JAVAOUT_UNDEF, input_file, line_number, "No javaout typemap defined for %s\n", SwigType_str(t, 0));
        }

        generateThrowsClause(n, function_code);
        Printf(function_code, " %s\n\n", tm ? (const String *) tm : empty_string);
        Printv(module_class_code, function_code, NIL);

        Delete(pre_code);
        Delete(post_code);
        Delete(function_code);
        Delete(return_type);
        Delete(imcall);
        Delete(func_name);
    }

    /*----------------------------------------------------------------------
   * replaceSpecialVariables()
   *--------------------------------------------------------------------*/

    virtual void replaceSpecialVariables(String *method, String *tm, Parm *parm) {
        (void)method;
        SwigType *type = Getattr(parm, "type");
        substituteClassname(type, tm);
    }

    /*----------------------------------------------------------------------
   * decodeEnumFeature()
   * Decode the possible enum features, which are one of:
   *   %javaenum(simple)
   *   %javaenum(typeunsafe) - default
   *   %javaenum(typesafe)
   *   %javaenum(proper)
   *--------------------------------------------------------------------*/

    EnumFeature decodeEnumFeature(Node *n) {
        EnumFeature enum_feature = TypeunsafeEnum;
        String *feature = Getattr(n, "feature:java:enum");
        if (feature) {
            if (Cmp(feature, "simple") == 0)
                enum_feature = SimpleEnum;
            else if (Cmp(feature, "typesafe") == 0)
                enum_feature = TypesafeEnum;
            else if (Cmp(feature, "proper") == 0)
                enum_feature = ProperEnum;
        }
        return enum_feature;
    }

    /* -----------------------------------------------------------------------
   * enumValue()
   * This method will return a string with an enum value to use in Java generated
   * code. If the %javaconst feature is not used, the string will contain the intermediary
   * class call to obtain the enum value. The intermediary class and JNI methods to obtain
   * the enum value will be generated. Otherwise the C/C++ enum value will be used if there
   * is one and hopefully it will compile as Java code - e.g. 20 as in: enum E{e=20};
   * The %javaconstvalue feature overrides all other ways to generate the constant value.
   * The caller must delete memory allocated for the returned string.
   * ------------------------------------------------------------------------ */

    String *enumValue(Node *n) {
        String *symname = Getattr(n, "sym:name");

        // Check for the %javaconstvalue feature
        String *value = Getattr(n, "feature:java:constvalue");

        if (!value) {
            // The %javaconst feature determines how the constant value is obtained
            int const_feature_flag = GetFlag(n, "feature:java:const");

            if (const_feature_flag) {
                // Use the C syntax to make a true Java constant and hope that it compiles as Java code
                value = Getattr(n, "enumvalue") ? Copy(Getattr(n, "enumvalue")) : Copy(Getattr(n, "enumvalueex"));
            } else {
                String *newsymname = 0;
                if (!getCurrentClass() || !proxy_flag) {
                    String *enumClassPrefix = getEnumClassPrefix();
                    if (enumClassPrefix) {
                        // A global scoped enum
                        newsymname = Swig_name_member(0, enumClassPrefix, symname);
                        symname = newsymname;
                    }
                }

                // Get the enumvalue from a JNI call
                if (!getCurrentClass() || !cparse_cplusplus || !proxy_flag) {
                    // Strange hack to change the name
                    Setattr(n, "name", Getattr(n, "value"));	/* for wrapping of enums in a namespace when emit_action is used */
                    constantWrapper(n);
                    value = NewStringf("%s.%s()", full_imclass_name ? full_imclass_name : imclass_name, Swig_name_get(getNSpace(), symname));
                } else {
                    memberconstantHandler(n);
                    value = NewStringf("%s.%s()", full_imclass_name ? full_imclass_name : imclass_name, Swig_name_get(getNSpace(), Swig_name_member(0, getEnumClassPrefix(), symname)));
                }
                Delete(newsymname);
            }
        }
        return value;
    }

    /* -----------------------------------------------------------------------------
   * getEnumName()
   *
   * If jnidescriptor is set, inner class names are separated with '$' otherwise a '.'
   * and the package is also not added to the name.
   * ----------------------------------------------------------------------------- */

    String *getEnumName(SwigType *t, bool jnidescriptor) {
        Node *enumname = NULL;
        Node *n = enumLookup(t);
        if (n) {
            enumname = Getattr(n, "enumname");
            if (!enumname || jnidescriptor) {
                String *symname = Getattr(n, "sym:name");
                if (symname) {
                    // Add in class scope when referencing enum if not a global enum
                    String *scopename_prefix = Swig_scopename_prefix(Getattr(n, "name"));
                    String *proxyname = 0;
                    if (scopename_prefix) {
                        proxyname = getProxyName(scopename_prefix, jnidescriptor);
                    }
                    if (proxyname) {
                        const char *class_separator = jnidescriptor ? "$" : ".";
                        enumname = NewStringf("%s%s%s", proxyname, class_separator, symname);
                    } else {
                        // global enum or enum in a namespace
                        String *nspace = Getattr(n, "sym:nspace");
                        if (nspace) {
                            if (package && !jnidescriptor)
                                enumname = NewStringf("%s.%s.%s", package, nspace, symname);
                            else
                                enumname = NewStringf("%s.%s", nspace, symname);
                        } else {
                            enumname = Copy(symname);
                        }
                    }
                    if (!jnidescriptor) {
                        Setattr(n, "enumname", enumname); // Cache it
                        Delete(enumname);
                    }
                    Delete(scopename_prefix);
                }
            }
        }

        return enumname;
    }

    /* -----------------------------------------------------------------------------
   * substituteClassname()
   *
   * Substitute the special variable $javaclassname with the proxy class name for classes/structs/unions 
   * that SWIG knows about. Also substitutes enums with enum name.
   * Otherwise use the $descriptor name for the Java class name. Note that the $&javaclassname substitution
   * is the same as a $&descriptor substitution, ie one pointer added to descriptor name.
   * Note that the path separator is a '.' unless jnidescriptor is set.
   * Inputs:
   *   pt - parameter type
   *   tm - typemap contents that might contain the special variable to be replaced
   *   jnidescriptor - if set, inner class names are separated with '$' otherwise a '/' is used for the path separator
   * Outputs:
   *   tm - typemap contents complete with the special variable substitution
   * Return:
   *   substitution_performed - flag indicating if a substitution was performed
   * ----------------------------------------------------------------------------- */

    bool substituteClassname(SwigType *pt, String *tm, bool jnidescriptor = false) {
        bool substitution_performed = false;
        SwigType *type = Copy(SwigType_typedef_resolve_all(pt));
        SwigType *strippedtype = SwigType_strip_qualifiers(type);

        if (Strstr(tm, "$javaclassname")) {
            SwigType *classnametype = Copy(strippedtype);
            substituteClassnameSpecialVariable(classnametype, tm, "$javaclassname", jnidescriptor);
            substitution_performed = true;
            Delete(classnametype);
        }
        if (Strstr(tm, "$*javaclassname")) {
            SwigType *classnametype = Copy(strippedtype);
            Delete(SwigType_pop(classnametype));
            if (Len(classnametype) > 0) {
                substituteClassnameSpecialVariable(classnametype, tm, "$*javaclassname", jnidescriptor);
                substitution_performed = true;
            }
            Delete(classnametype);
        }
        if (Strstr(tm, "$&javaclassname")) {
            SwigType *classnametype = Copy(strippedtype);
            SwigType_add_pointer(classnametype);
            substituteClassnameSpecialVariable(classnametype, tm, "$&javaclassname", jnidescriptor);
            substitution_performed = true;
            Delete(classnametype);
        }
        if (Strstr(tm, "$javainterfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            substituteInterfacenameSpecialVariable(interfacenametype, tm, "$javainterfacename", jnidescriptor, true);
            substitution_performed = true;
            Delete(interfacenametype);
        }
        if (Strstr(tm, "$*javainterfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            Delete(SwigType_pop(interfacenametype));
            if (Len(interfacenametype) > 0) {
                substituteInterfacenameSpecialVariable(interfacenametype, tm, "$*javainterfacename", jnidescriptor, true);
                substitution_performed = true;
            }
            Delete(interfacenametype);
        }
        if (Strstr(tm, "$&javainterfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            SwigType_add_pointer(interfacenametype);
            substituteInterfacenameSpecialVariable(interfacenametype, tm, "$&javainterfacename", jnidescriptor, true);
            substitution_performed = true;
            Delete(interfacenametype);
        }
        if (Strstr(tm, "$interfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            substituteInterfacenameSpecialVariable(interfacenametype, tm, "$interfacename", jnidescriptor, false);
            substitution_performed = true;
            Delete(interfacenametype);
        }
        if (Strstr(tm, "$*interfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            Delete(SwigType_pop(interfacenametype));
            if (Len(interfacenametype) > 0) {
                substituteInterfacenameSpecialVariable(interfacenametype, tm, "$*interfacename", jnidescriptor, false);
                substitution_performed = true;
            }
            Delete(interfacenametype);
        }
        if (Strstr(tm, "$&interfacename")) {
            SwigType *interfacenametype = Copy(strippedtype);
            SwigType_add_pointer(interfacenametype);
            substituteInterfacenameSpecialVariable(interfacenametype, tm, "$&interfacename", jnidescriptor, false);
            substitution_performed = true;
            Delete(interfacenametype);
        }

        Delete(strippedtype);
        Delete(type);

        return substitution_performed;
    }

    /* -----------------------------------------------------------------------------
   * substituteClassnameSpecialVariable()
   * ----------------------------------------------------------------------------- */

    void substituteClassnameSpecialVariable(SwigType *classnametype, String *tm, const char *classnamespecialvariable, bool jnidescriptor) {
        String *replacementname;

        if (SwigType_isenum(classnametype)) {
            String *enumname = getEnumName(classnametype, jnidescriptor);
            if (enumname) {
                replacementname = Copy(enumname);
            } else {
                bool anonymous_enum = (Cmp(classnametype, "enum ") == 0);
                if (anonymous_enum) {
                    replacementname = NewString("int");
                } else {
                    // An unknown enum - one that has not been parsed (neither a C enum forward reference nor a definition) or an ignored enum
                    replacementname = NewStringf("SWIGTYPE%s", SwigType_manglestr(classnametype));
                    Replace(replacementname, "enum ", "", DOH_REPLACE_ANY);
                    Setattr(swig_types_hash, replacementname, classnametype);
                }
            }
        } else {
            String *classname = getProxyName(classnametype, jnidescriptor); // getProxyName() works for pointers to classes too
            if (classname) {
                replacementname = Copy(classname);
            } else {
                // use $descriptor if SWIG does not know anything about this type. Note that any typedefs are resolved.
                replacementname = NewStringf("SWIGTYPE%s", SwigType_manglestr(classnametype));

                // Add to hash table so that the type wrapper classes can be created later
                Setattr(swig_types_hash, replacementname, classnametype);
            }
        }
        if (jnidescriptor)
            Replaceall(replacementname,".","/");
        Replaceall(tm, classnamespecialvariable, replacementname);

        Delete(replacementname);
    }

    /* -----------------------------------------------------------------------------
   * substituteInterfacenameSpecialVariable()
   * ----------------------------------------------------------------------------- */

    void substituteInterfacenameSpecialVariable(SwigType *interfacenametype, String *tm, const char *interfacenamespecialvariable, bool jnidescriptor, bool qualified) {

        String *interfacename = getInterfaceName(interfacenametype/*, jnidescriptor*/, qualified);
        if (interfacename) {
            String *replacementname = Copy(interfacename);

            if (jnidescriptor)
                Replaceall(replacementname,".","/");
            Replaceall(tm, interfacenamespecialvariable, replacementname);

            Delete(replacementname);
        }
    }

    /* -----------------------------------------------------------------------------
   * emitTypeWrapperClass()
   * ----------------------------------------------------------------------------- */

    void emitTypeWrapperClass(String *classname, SwigType *type) {
        Node *n = NewHash();
        Setfile(n, input_file);
        Setline(n, line_number);

        String *swigtype = NewString("");
        String *filen = NewStringf("%s%s.java", SWIG_output_directory(), classname);
        File *f_swigtype = NewFile(filen, "w", SWIG_output_files());
        if (!f_swigtype) {
            FileErrorDisplay(filen);
            SWIG_exit(EXIT_FAILURE);
        }
        Append(filenames_list, Copy(filen));
        Delete(filen);
        filen = NULL;

        // Start writing out the type wrapper class file
        emitBanner(f_swigtype);

// I don't think swift uses packages like java.
//        if (package)
//            Printf(f_swigtype, "package %s;\n", package);

        // Pure Java baseclass and interfaces
        const String *pure_baseclass = typemapLookup(n, "javabase", type, WARN_NONE);
        const String *pure_interfaces = typemapLookup(n, "javainterfaces", type, WARN_NONE);

        // Emit the class
        Printv(swigtype, typemapLookup(n, "javaimports", type, WARN_NONE),	// Import statements
               "\n", typemapLookup(n, "javaclassmodifiers", type, WARN_JAVA_TYPEMAP_CLASSMOD_UNDEF),	// Class modifiers
               " $javaclassname",	// Class name and bases
               *Char(pure_baseclass) ? ": " : "", pure_baseclass, *Char(pure_interfaces) ?	// Interfaces
                                                                         ": " : "", pure_interfaces, " {", typemapLookup(n, "javabody", type, WARN_JAVA_TYPEMAP_JAVABODY_UNDEF),	// main body of class
               typemapLookup(n, "javacode", type, WARN_NONE),	// extra Java code
               "}\n", "\n", NIL);

        Replaceall(swigtype, "$javaclassname", classname);
        Replaceall(swigtype, "$module", module_class_name);
        Replaceall(swigtype, "$imclassname", imclass_name);

        // For unknown enums
        Replaceall(swigtype, "$static ", "");
        Replaceall(swigtype, "$enumvalues", "");

        Printv(f_swigtype, swigtype, NIL);

        Delete(f_swigtype);
        Delete(swigtype);
        Delete(n);
    }

    /* -----------------------------------------------------------------------------
   * typemapLookup()
   * n - for input only and must contain info for Getfile(n) and Getline(n) to work
   * tmap_method - typemap method name
   * type - typemap type to lookup
   * warning - warning number to issue if no typemaps found
   * typemap_attributes - the typemap attributes are attached to this node and will 
   *   also be used for temporary storage if non null
   * return is never NULL, unlike Swig_typemap_lookup()
   * ----------------------------------------------------------------------------- */

    const String *typemapLookup(Node *n, const_String_or_char_ptr tmap_method, SwigType *type, int warning, Node *typemap_attributes = 0) {
        Node *node = !typemap_attributes ? NewHash() : typemap_attributes;
        Setattr(node, "type", type);
        Setfile(node, Getfile(n));
        Setline(node, Getline(n));
        const String *tm = Swig_typemap_lookup(tmap_method, node, "", 0);
        if (!tm) {
            tm = empty_string;
            if (warning != WARN_NONE)
                Swig_warning(warning, Getfile(n), Getline(n), "No %s typemap defined for %s\n", tmap_method, SwigType_str(type, 0));
        }
        if (!typemap_attributes)
            Delete(node);
        return tm;
    }

    /* -----------------------------------------------------------------------------
   * addThrows()
   *
   * Adds exception classes to a throws list. The throws list is the list of classes
   * that will form the Java throws clause. Mainly for checked exceptions.
   * ----------------------------------------------------------------------------- */

    void addThrows(Node *n, const String *attribute, Node *parameter) {
        // Get the comma separated exception classes for the throws clause - held in typemap/feature's "throws" attribute
        String *throws_attribute = NewStringf("%s:throws", attribute);
        String *throws = Getattr(parameter, throws_attribute);

        if (throws && Len(throws) > 0) {
            String *throws_list = Getattr(n, "java:throwslist");
            if (!throws_list) {
                throws_list = NewList();
                Setattr(n, "java:throwslist", throws_list);
            }
            // Put the exception classes in the throws clause into a temporary List
            List *temp_classes_list = Split(throws, ',', INT_MAX);

            // Add the exception classes to the node throws list, but don't duplicate if already in list
            if (temp_classes_list && Len(temp_classes_list) > 0) {
                for (Iterator cls = First(temp_classes_list); cls.item; cls = Next(cls)) {
                    String *exception_class = NewString(cls.item);
                    Replaceall(exception_class, " ", "");	// remove spaces
                    Replaceall(exception_class, "\t", "");	// remove tabs
                    if (Len(exception_class) > 0) {
                        // $javaclassname substitution
                        SwigType *pt = Getattr(parameter, "type");
                        substituteClassname(pt, exception_class);

                        // Don't duplicate the Java exception class in the throws clause
                        bool found_flag = false;
                        for (Iterator item = First(throws_list); item.item; item = Next(item)) {
                            if (Strcmp(item.item, exception_class) == 0)
                                found_flag = true;
                        }
                        if (!found_flag)
                            Append(throws_list, exception_class);
                    }
                    Delete(exception_class);
                }
            }
            Delete(temp_classes_list);
        }
        Delete(throws_attribute);
    }

    /* -----------------------------------------------------------------------------
   * generateThrowsClause()
   *
   * Generates throws clause for checked exception
   * ----------------------------------------------------------------------------- */

    void generateThrowsClause(Node *n, String *code) {
        // Add the throws clause into code
        List *throws_list = Getattr(n, "java:throwslist");
        if (throws_list) {
            Iterator cls = First(throws_list);
            Printf(code, " throws %s", cls.item);
            while ((cls = Next(cls)).item)
                Printf(code, ", %s", cls.item);
        }
    }

    /* -----------------------------------------------------------------------------
   * prematureGarbageCollectionPreventionParameter()
   *
   * Get the proxy class name for use in an additional generated parameter. The
   * additional parameter is added to a native method call purely to prevent 
   * premature garbage collection of proxy classes which pass their C++ class pointer
   * in a Java long to the JNI layer.
   * ----------------------------------------------------------------------------- */

    String *prematureGarbageCollectionPreventionParameter(SwigType *t, Parm *p) {
        String *pgcpp_java_type = 0;
        String *jtype = NewString(Getattr(p, "tmap:jtype"));

        // Strip C comments
        String *stripped_jtype = Swig_strip_c_comments(jtype);
        if (stripped_jtype) {
            Delete(jtype);
            jtype = stripped_jtype;
        }

        // Remove whitespace
        Replaceall(jtype, " ", "");
        Replaceall(jtype, "\t", "");

        if (Cmp(jtype, "long") == 0) {
            if (proxy_flag) {
                if (!GetFlag(p, "tmap:jtype:nopgcpp") && !nopgcpp_flag) {
                    String *interface_name = getInterfaceName(t, true);
                    pgcpp_java_type = interface_name ? interface_name : getProxyName(t);
                    if (!pgcpp_java_type) {
                        // Look for proxy class parameters passed to C++ layer using non-default typemaps, ie not one of above types
                        String *jstype = NewString(Getattr(p, "tmap:jstype"));
                        if (jstype) {
                            Hash *classes = getClassHash();
                            if (classes) {
                                // Strip C comments
                                String *stripped_jstype = Swig_strip_c_comments(jstype);
                                if (stripped_jstype) {
                                    Delete(jstype);
                                    jstype = stripped_jstype;
                                }
                                // Remove whitespace
                                Replaceall(jstype, " ", "");
                                Replaceall(jstype, "\t", "");

                                Iterator ki;
                                for (ki = First(classes); ki.key; ki = Next(ki)) {
                                    Node *cls = ki.item;
                                    if (cls && !Getattr(cls, "feature:ignore")) {
                                        String *symname = Getattr(cls, "sym:name");
                                        if (symname && Strcmp(symname, jstype) == 0) {
                                            pgcpp_java_type = symname;
                                        }
                                    }
                                }
                            }
                        }
                        Delete(jstype);
                    }
                }
            }
        }
        Delete(jtype);
        return pgcpp_java_type;
    }

    /* -----------------------------------------------------------------------------
   * outputDirectory()
   *
   * Return the directory to use for generating Java classes/enums and create the
   * subdirectory (does not create if language specific outdir does not exist).
   * ----------------------------------------------------------------------------- */

    String *outputDirectory(String *nspace) {
        String *output_directory = Copy(SWIG_output_directory());
        if (nspace) {
            String *nspace_subdirectory = Copy(nspace);
            Replaceall(nspace_subdirectory, ".", SWIG_FILE_DELIMITER);
            String *newdir_error = Swig_new_subdirectory(output_directory, nspace_subdirectory);
            if (newdir_error) {
                Printf(stderr, "%s\n", newdir_error);
                Delete(newdir_error);
                SWIG_exit(EXIT_FAILURE);
            }
            Printv(output_directory, nspace_subdirectory, SWIG_FILE_DELIMITER, 0);
            Delete(nspace_subdirectory);
        }
        return output_directory;
    }
};				/* class SWIFT */

/* -----------------------------------------------------------------------------
 * swig_java()    - Instantiate module
 * ----------------------------------------------------------------------------- */

static Language *new_swig_swift() {
    return new SWIFT();
}
extern "C" Language *swig_swift(void) {
    return new_swig_swift();
}

/* -----------------------------------------------------------------------------
 * Static member variables
 * ----------------------------------------------------------------------------- */

const char *SWIFT::usage = "\
Java Options (available with -java)\n\
     -nopgcpp        - Suppress premature garbage collection prevention parameter\n\
     -noproxy        - Generate the low-level functional interface instead\n\
                       of proxy classes\n\
     -oldvarnames    - Old intermediary method names for variable wrappers\n\
     -package <name> - Set name of the Java package to <name>\n\
\n";
