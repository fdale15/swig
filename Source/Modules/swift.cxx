#include "swigmod.h"
#include <limits.h>		// for INT_MAX
#include "cparse.h"
#include <ctype.h>

class SWIFT : public Language {
protected:
    /* General DOH objects used for holding the strings */
    File *f_begin;
    File *f_runtime;
    File *f_header;
    File *f_wrappers;
    File *f_init;

public:
    virtual void main(int argc, char *argv[]) {

        /* Set language-specific subdirectory in SWIG library */
        SWIG_library_directory("swift");

        /* Set language-specific preprocessing symbol */
        Preprocessor_define("SWIGSWIFT 1", 0);

        /* Set language-specific configuration file */
//        SWIG_config_file("swift.swg");

        /* Set typemap language (historical) */
        SWIG_typemap_lang("swift");
    }

    int top(Node *n) override {
        /* Get the module name */
        String *module = Getattr(n, "name");
        /* Get the output file name */
        String *outfile = Getattr(n, "outfile");


        /* Initialize I/O */
        f_begin = NewFile(outfile, "w", SWIG_output_files());
        if (!f_begin) {
            FileErrorDisplay(outfile);
            SWIG_exit(EXIT_FAILURE);
        }
        f_runtime = NewString("");
        f_init = NewString("");
        f_header = NewString("");
        f_wrappers = NewString("");

        /* Register file targets with the SWIG file handler */
        Swig_register_filebyname("begin", f_begin);
        Swig_register_filebyname("header", f_header);
        Swig_register_filebyname("wrapper", f_wrappers);
        Swig_register_filebyname("runtime", f_runtime);
        Swig_register_filebyname("init", f_init);

        /* Output module initialization code */
        Swig_banner(f_begin);

        /* Emit code for children */
        Language::top(n);

        /* Write all to the file */
        Dump(f_runtime, f_begin);
        Dump(f_header, f_begin);
        Dump(f_wrappers, f_begin);
        Wrapper_pretty_print(f_init, f_begin);

        /* Cleanup files */
        Delete(f_runtime);
        Delete(f_header);
        Delete(f_wrappers);
        Delete(f_init);
        Delete(f_begin);

        return SWIG_OK;
    }

//    int functionWrapper(Node *n) override {
//        /* Get some useful attributes of this function */
//        String   *name   = Getattr(n, "sym:name");
//        SwigType *type   = Getattr(n, "type");
//        ParmList *parms  = Getattr(n, "parms");
//        String   *parmstr= ParmList_str_defaultargs(parms); // to string
//        String   *func   = SwigType_str(type, NewStringf("%s(%s)", name, parmstr));
//        String   *action = Getattr(n, "wrap:action");
//
//        Printf(f_wrappers, "%s {\n", func);
//        Printf(f_wrappers, "\t%s\n", action);
//        Printf(f_wrappers, "}\n\n");
//        return SWIG_OK;
//     }

    virtual int functionWrapper(Node *n) {
        /* get useful attributes */
        String *action   = Getattr(n, "wrap:action");
        String   *name   = Getattr(n, "sym:name");
        SwigType *type   = Getattr(n, "type");
        ParmList *parms  = Getattr(n, "parms");
        String   *parmstr= ParmList_str_defaultargs(parms);

        String   *func   = SwigType_str(type, NewStringf("%s(%s)", name, parmstr));
        String *returnType = SwigType_str(type, NewStringf(""));

        bool is_void_return = (Cmp(returnType, "void") == 0);

        /* create the wrapper object */
        Wrapper *wrapper = NewWrapper();

        /* create the functions wrappered name */
        String *wname = Swig_name_wrapper(name);

        /* write the wrapper function definition */
        Printv(wrapper->def, func, " {\n", NIL);

        if (!is_void_return) {
            Wrapper_add_localv(wrapper, "result", returnType, "result = 0", NIL);
        }

        /* write the list of locals/arguments required */
//        emit_args(type, parms, wrapper);
        // Emit all of the local variables for holding arguments.
        emit_parameter_variables(parms, wrapper);

        /* Attach the standard typemaps */
        emit_attach_parmmaps(parms, wrapper);

        int num_arguments = emit_num_arguments(parms);

        // Now walk the function parameter list and generate code to get arguments

        Printv(wrapper->code, action, "\n", NIL);

        if (!is_void_return) {
            Printv(wrapper->code, "return result;\n", NIL);
        }
        Printv(wrapper->code, "}\n", NIL);

        /* Dump the function out */
        Wrapper_print(wrapper, f_wrappers);

        /* tidy up */
        Delete(wname);
        Delete(func);
        Delete(returnType);
        DelWrapper(wrapper);

        return SWIG_OK;
    }

private:
    void emit_args(SwigType *type, ParmList *parms, Wrapper *wrapper) {

    }
};

extern "C" Language *
swig_swift(void) {
    return new SWIFT();
}
