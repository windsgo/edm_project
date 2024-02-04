#include "RS274InterpreterWrapper.h"
#include "fmt/format.h"
#include <iostream>

static void test(const char *filename, const char *module_path) {
    try {
        edm::interpreter::RS274InterpreterWrapper::instance()
            ->set_rs274_py_module_dir(module_path);
        std::string str_ret{
            edm::interpreter::RS274InterpreterWrapper::parse_file(filename)};

        std::cout << str_ret << std::endl;
    } catch (const edm::interpreter::RS274InterpreterException &e) {
        std::cout << "error1\n";

        std::cout << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cout << "error2\n";

        std::cout << e.what() << std::endl;
    }
}

static void test2(const char *filename, const char *module_path) {
    try {
        edm::interpreter::RS274InterpreterWrapper::instance()
            ->set_rs274_py_module_dir(module_path);
        json::value json_ret{
            edm::interpreter::RS274InterpreterWrapper::parse_file_to_json(
                filename)};

        // std::cout << (int)json_ret.as_array().size() << std::endl;
    } catch (const edm::interpreter::RS274InterpreterException &e) {
        std::cout << "error1\n";

        std::cout << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cout << "error2\n";

        std::cout << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {

#if 0
    if (argc < 3) {
        fmt::print(R"(
    Usage: build/tests/test_interp_wrapper [test.py] [module_path]
        )");
    
        return 1;
    }

    const char* filename = argv[1];
    const char* module_path = argv[2];
#else
    const char *filename = "/home/windsgo/Documents/edmproject/edm_project/"
                           "tests/Interpreter/pytest/test1.py";
    const char *filename2 = "/home/windsgo/Documents/edmproject/edm_project/"
                            "tests/Interpreter/pytest/test2.py";
    const char *module_path =
        "/home/windsgo/Documents/edmproject/edm_project/Interpreter/"
        "rs274pyInterpreter/pymodule/";
#endif
    for (int i = 0; i < 1; ++i) {
        test(filename, module_path);
        // test(filename2, module_path);
        // test2(filename, module_path);
    }

    return 0;
}