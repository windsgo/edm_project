#include <iostream>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <Python.h>

static void py_add_module_path(const char *module_path) {
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(
        fmt::format("sys.path.append('{}')", module_path).c_str());

    PyRun_SimpleString("print(sys.path)");
}

static void py_import_module(const char *module_name) {
    PyImport_ImportModule(module_name);
}

static void py_run_file() {}

static void test_python() {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        fmt::print("python not initialized.");
        return;
    }

    auto currpath = fs::current_path();
    currpath += "/pymodule";
    py_add_module_path(currpath.c_str());
    py_import_module("rs274");
    py_import_module("test1");
    // PyRun_SimpleString("aaaaa");

    // const char* filename = "pymodule/test1.py";

    // FILE* f = fopen(filename, "rb");
    // if (!f) {
    //     fmt::print("file open failed");
    //     return;
    // }

    // auto ret = PyRun_SimpleFile(f, filename);

    // fmt::print("file [{}] run ret : {}\n", filename, ret);

    auto err_occured = PyErr_Occurred(); // borrowed reference, 不需要DECREF
    if (err_occured) {
        fmt::print("err_occured!\n");
    } else {
        fmt::print("err_occured NULL\n");
    }

    PyErr_Print();

    // fclose(f);

    fmt::print("**End\n");

    Py_Finalize();
}

int main(int argc, char **argv) {

    test_python();

    return 0;
}