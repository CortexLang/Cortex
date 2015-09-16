
#include "cx.h"
#include "cx_generator.h"
#include "cx_files.h"
#include "cx_file.h"
#include "c_common.h"

/* Generate file containing component loader */
static cx_int16 c_projectGenerateMainFile(cx_generator g) {
    cx_id filename;
    g_file file;
    cx_id topLevelName;
    cx_bool isComponent = !strcmp(gen_getAttribute(g, "component"), "true");

    sprintf(filename, "%s__load.c", g_getName(g));

    file = g_hiddenFileOpen(g, filename);
    if(!file) {
        goto error;
    }

    g_fileWrite(file, "/* %s\n", filename);
    g_fileWrite(file, " *\n");
    g_fileWrite(file, " * This file is generated. Do not modify.\n");
    g_fileWrite(file, " */\n\n");

    if (g_getCurrent(g)) {
        cx_id path;
        g_fileWrite(file, "#define %s_LIB\n", c_topath(g_getCurrent(g), path, '_'));
        g_fileWrite(file, "#include \"%s.h\"\n\n", g_fullOid(g, g_getCurrent(g), topLevelName));
        g_fileWrite(file, "int cortomain(int argc, char* argv[]) {\n");
        g_fileIndent(file);
        g_fileWrite(file, "if (%s_load()) return -1;\n", g_getName(g));
        g_fileWrite(file, "int %sMain(int argc, char* argv[]);\n", g_getName(g));
        g_fileWrite(file, "if (%sMain(argc, argv)) return -1;\n", g_getName(g));
        g_fileWrite(file, "return 0;\n");
        g_fileDedent(file);
        g_fileWrite(file, "}\n\n");
    } else {
        g_fileWrite(file, "#include \"corto.h\"\n\n");
        if (isComponent) {
            g_fileWrite(file, "int cortomain(int argc, char* argv[]) {\n");
        } else {
            g_fileWrite(file, "int main(int argc, char* argv[]) {\n");
            g_fileWrite(file, "cx_start();\n");
        }
        g_fileIndent(file);
        g_fileWrite(file, "int %sMain(int argc, char* argv[]);\n", g_getName(g));
        g_fileWrite(file, "if (%sMain(argc, argv)) return -1;\n", g_getName(g));
        if (!isComponent) {
            g_fileWrite(file, "cx_stop();\n");
        }
        g_fileWrite(file, "return 0;\n");
        g_fileDedent(file);
        g_fileWrite(file, "}\n\n");
    }

    return 0;
error:
    return -1;
}

/* Generate main header containing includes to dependencies */
static cx_int16 c_projectGenerateMainHeaderFile(cx_generator g) {
    cx_id filename;
    g_file file;
    cx_ll packages, components;

    sprintf(filename, "include/%s.h", g_getName(g));

    file = g_fileOpen(g, filename);
    if(!file) {
        goto error;
    }

    g_fileWrite(file, "/* %s\n", filename);
    g_fileWrite(file, " *\n");
    g_fileWrite(file, " * This file is generated. Do not modify.\n");
    g_fileWrite(file, " */\n\n");

    g_fileWrite(file, "#ifndef %s_H\n", g_getName(g));
    g_fileWrite(file, "#define %s_H\n\n", g_getName(g));

    g_fileWrite(file, "#include \"corto.h\"\n");

    if ((packages = cx_loadGetPackages())) {
        cx_id path;
        cx_iter iter = cx_llIter(packages);
        while (cx_iterHasNext(&iter)) {
            cx_string str = cx_iterNext(&iter);
            cx_object package = cx_resolve(NULL, str);
            if (!package) {
                cx_error("corto: package.txt contains unresolved package '%s'", str);
                goto error;
            }
            g_fileWrite(file, "#include \"%s/%s.h\"\n", c_topath(package, path, '/'), cx_nameof(package));
            cx_release(package);
        }
        cx_loadFreePackages(packages);
    }

    if ((components = cx_loadGetComponents())) {
        cx_iter iter = cx_llIter(components);
        while (cx_iterHasNext(&iter)) {
            cx_string str = cx_iterNext(&iter);
            g_fileWrite(file, "#include \"%s.h\"\n", str);
        }
        cx_loadFreeComponents(components);
    }

    g_fileWrite(file, "#ifdef __cplusplus\n");
    g_fileWrite(file, "extern \"C\" {\n");
    g_fileWrite(file, "#endif\n");

    g_fileWrite(file, "\n");
    g_fileWrite(file, "/* $header()");

    /* Lookup the header snippet */
    cx_string snippet = g_fileLookupHeader(file, "");
    if (snippet) {
        g_fileWrite(file, snippet);
    } else {
        g_fileWrite(file, " */\n");
        g_fileWrite(file, "/* You can put your own definitions here! */\n");
        g_fileWrite(file, "/* ");
    }
    g_fileWrite(file, "$end */\n");

    g_fileWrite(file, "\n");

    g_fileWrite(file, "#ifdef __cplusplus\n");
    g_fileWrite(file, "}\n");
    g_fileWrite(file, "#endif\n");
    g_fileWrite(file, "#endif\n\n");

    g_fileClose(file);

    return 0;
error:
    return -1;
}

typedef struct c_projectCleanInclude_t {
    cx_generator g;
    g_file file;
} c_projectCleanInclude_t;

static int c_projectCleanInclude(cx_object o, void *userData) {
    c_projectCleanInclude_t *data = userData;

    if (cx_instanceof(cx_type(cx_interface_o), o) || cx_instanceof(cx_type(cx_package_o), o)) {
        cx_id id;
        g_fileWrite(data->file, "CLOBBER.include(\"include/%s.h\")\n", g_fullOid(data->g, o, id));
    }

    return 1;
}

/* Generate dependency makefile for project */
static cx_int16 c_projectGenerateDepMakefile(cx_generator g) {
    g_file file;
    c_projectCleanInclude_t walkData;

    file = g_hiddenFileOpen(g, "dep.rb");
    if(!file) {
        goto error;
    }

    g_fileWrite(file, "require 'rake/clean'\n");

    g_fileWrite(file, "\n");
    g_fileWrite(file, "# Clobber generated header files\n");
    walkData.file = file;
    walkData.g = g;
    g_fileWrite(file, "\n");
    g_walkRecursive(g, c_projectCleanInclude, &walkData);
    g_fileWrite(file, "CLOBBER.include(\".corto/dep.rb\")\n");

    return 0;
error:
    return -1;
}

/* Generate dependency makefile for project */
static cx_int16 c_projectGeneratePackageFile(cx_generator g) {
    cx_file file = NULL;

    g_resolveImports(g);
    if (g->imports) {
        file = cx_fileAppend(".corto/packages.txt");
        cx_iter iter = cx_llIter(g->imports);

        while (cx_iterHasNext(&iter)) {
            cx_object import = cx_iterNext(&iter);
            cx_id id;
            cx_fullname(import, id);
            if (!cx_loadRequiresPackage(id)) {
                fprintf(cx_fileGet(file), "%s\n", id);
            }
        }

        cx_fileClose(file);
    }

    return 0;
}

/* Generator main */
cx_int16 corto_genMain(cx_generator g) {

    /* Create source and include directories */
    cx_mkdir("include");
    cx_mkdir("src");
    cx_mkdir(".corto");

    if(c_projectGenerateMainFile(g)) {
        goto error;
    }

    if (g->objects) {
        if(c_projectGenerateDepMakefile(g)) {
            goto error;
        }
        if(c_projectGeneratePackageFile(g)) {
            goto error;
        }
    } else {
        if(c_projectGenerateMainHeaderFile(g)) {
            goto error;
        }
    }

    return 0;
error:
    return -1;
}
