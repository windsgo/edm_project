#include <fmt/core.h>
#include <fmt/format.h>

#include <iostream>
#include <string>
#include <vector>

#include <boost/python/dict.hpp>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>

namespace bp = boost::python;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

static void py_add_module_path(const char *module_path) {
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(
        fmt::format("sys.path.append('{}')", module_path).c_str());

    PyRun_SimpleString("print(sys.path)");
}

static void py_add_module_path2(const char *module_path) {
    bp::object module_sys = bp::import("sys");
    bp::object module_sys_path_append =
        module_sys.attr("path").attr("append"); // 修改了内置模块的变量(模块搜索路径),
                                                // 这种改动在解释器全局有效
    module_sys_path_append(module_path);

    // bp::list sys_path(module_sys.attr("path"));
    // for (int i = 0; i < (int)bp::extract<int>(sys_path.attr("__len__")());
    // ++i) {
    //     std::cout << (std::string)bp::extract<std::string>(sys_path[i]) <<
    //     "\n";
    // }
}

static std::vector<std::pair<std::string, bp::object>>
find_instance_of_class(bp::object environment_dict, bp::object class_type) {
    std::vector<std::pair<std::string, bp::object>> ret_name_obj_pairs;

    bp::dict dict{environment_dict};
    int dict_size = bp::extract<int>(dict.attr("__len__")());
    bp::list items{dict.items()};

    for (int i = 0; i < dict_size; ++i) {
        bp::tuple item(items[i]);
        std::string key_str{
            bp::extract<std::string>(static_cast<bp::object>(item[0]))};
        // auto value = static_cast<bp::object>(item[1]);
        bp::object value{item[1]};
        if (PyObject_IsInstance(value.ptr(), class_type.ptr())) {
            ret_name_obj_pairs.push_back(std::make_pair(key_str, value));
        }
    }

    return ret_name_obj_pairs;
}

static void py_print_keys_of_dict(bp::object dict_object) {
    bp::dict d(dict_object);

    for (int i = 0; i < bp::len(d); ++i) {
        auto key = d.keys()[i];
        std::cout << (std::string)bp::extract<std::string>(key) << "\n";
    }
}

class PythonInterpreterWrapper final {
public:
    PythonInterpreterWrapper() {
        std::cout << "py initialize\n";
        Py_Initialize();
    }

    ~PythonInterpreterWrapper() {
        if (Py_IsInitialized()) {
            std::cout << "py finalize\n";
            Py_Finalize();
        }
    }

private:
    PythonInterpreterWrapper(const PythonInterpreterWrapper &) = delete;
    PythonInterpreterWrapper(PythonInterpreterWrapper &&) = delete;
};

static void test_boost_python() {

    // boost::python::c

    // 将 rs274.py 所在路径添加到搜索路径
    auto currpath = fs::current_path();
    currpath += "/pymodule";
    py_add_module_path2(currpath.c_str());

    // py_add_module_path2("./pymodule");

    // 获取 __main__ 作用域
    bp::object main_module =
        boost::python::import("__main__"); // 获取 __main__ 模块
    bp::object main_namespace =
        main_module.attr("__dict__"); // 获取 __main__ 模块的命名空间

    // 在__main__作用域, 导入rs274模块
    // auto rs274 = boost::python::import("rs274"); //
    // 这并不能将模块导入到__main__作用域中(实际上是获取模块成为boost::python::object了)
    // bp::exec("import rs274", main_namespace); // 必须如此导入,
    // 才能将rs274模块导入到__main__作用域中
    bp::exec(
        "from rs274 import *",
        main_namespace); // 必须如此导入, 才能将rs274模块导入到__main__作用域中

    // 在__main__作用域, 运行目标文件
    bp::object ignored = bp::exec_file("./pytest/test1.py", main_namespace);

    // 输出运行过后的 __main__ 作用域
    std::cout << "bp::len(main_namespace): " << bp::len(main_namespace) << "\n";
    py_print_keys_of_dict(main_namespace);

    // 获取 rs274 解释器类型 RS274Interpreter
    bp::object rs274 = bp::import("rs274"); // 获取 rs274 模块
    bp::object rs274_namespace =
        rs274.attr("__dict__"); // 获取 rs274 模块的命名空间
    bp::object RS274Interpreter_class =
        rs274_namespace["RS274Interpreter"]; // 获取 RS274Interpreter 的类型对象

    // 在__main__作用域, 寻找定义的解释器实例
    auto interp_instances_found =
        find_instance_of_class(main_namespace, RS274Interpreter_class);

    std::cout << interp_instances_found.size() << std::endl;

    // 获取第一个实例的变量名, 实例对象
    std::string interp_instance_variable_name = interp_instances_found[0].first;
    bp::object interp_instance = interp_instances_found[0].second;
    std::cout << interp_instance_variable_name << ", "
              << static_cast<std::string>(bp::extract<std::string>(
                     interp_instance.attr("__class__").attr("__name__")))
              << std::endl;

    // 调用该实例的方法, 获取命令列表
    bp::list command_list{
        bp::object{interp_instance.attr("get_command_list")()}};
    int command_list_len = bp::len(command_list);
    std::cout << "command_list_len: " << command_list_len << std::endl;

    // 获取命令列表的json形式字符串
    std::string command_list_json_str{bp::extract<std::string>(
        bp::object{interp_instance.attr("get_command_list_jsonstr")()})};
    std::cout << command_list_json_str << std::endl;
}

int test_main() {

    fmt::print("test boost python\n");

    // Py_Initialize(); // 初始化 Python 解释器

    PythonInterpreterWrapper w;

    try {
        test_boost_python();
    } catch (const boost::python::error_already_set &) {
        fmt::print("error occurred\n");

        fmt::print(
            "Py_IsInitialized(): {}, PyErr_Occurred() == NULL: {}, Line: {}\n",
            Py_IsInitialized(), PyErr_Occurred() == NULL, __LINE__);

        PyObject *type_ptr = NULL;
        PyObject *value_ptr = NULL;
        PyObject *traceback_ptr = NULL;
        PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);
        PyErr_NormalizeException(&type_ptr, &value_ptr, &traceback_ptr);

        fmt::print(
            "Py_IsInitialized(): {}, PyErr_Occurred() == NULL: {}, Line: {}\n",
            Py_IsInitialized(), PyErr_Occurred() == NULL, __LINE__);

        static const char *_unknown_err_type_str = "Unknown Err Type";
        static const char *_unknown_err_value_str = "Unknown Err Type";

        std::string err_type_str{_unknown_err_type_str};
        std::string err_value_str{};

        if (type_ptr) {
            bp::handle h_type(
                type_ptr); // from PyObject* to boost::python::handle
            bp::str pystr_type{h_type};
            bp::extract<std::string> pystr_type_extractor(pystr_type);
            if (pystr_type_extractor.check()) {
                err_type_str = pystr_type_extractor();
            }
        } else {
            fmt::print("type_ptr is NULL\n");
        }

        if (value_ptr) {
            bp::handle h_value(
                value_ptr); // from PyObject* to boost::python::handle
            bp::str pystr_value{h_value};
            bp::extract<std::string> pystr_value_extractor(pystr_value);
            if (pystr_value_extractor.check()) {
                err_value_str = pystr_value_extractor();
            }
        } else {
            fmt::print("value_ptr is NULL\n");
        }

        fmt::print("err type : {}\n", err_type_str);
        fmt::print("err value :\n{}\n", err_value_str);
        fmt::print(
            "Py_IsInitialized(): {}, PyErr_Occurred() == NULL: {}, Line: {}\n",
            Py_IsInitialized(), PyErr_Occurred() == NULL, __LINE__);

        // Py_Finalize(); // 终止 Python 解释器

        fmt::print("Py_IsInitialized(): {}, Line: {}\n", Py_IsInitialized(),
                   __LINE__);

        throw std::out_of_range{""};
    }
    fmt::print(
        "Py_IsInitialized(): {}, PyErr_Occurred() == NULL: {}, Line: {}\n",
        Py_IsInitialized(), PyErr_Occurred() == NULL, __LINE__);
    // Py_Finalize(); // 终止 Python 解释器

    fmt::print("Py_IsInitialized(): {}, Line: {}\n", Py_IsInitialized(),
               __LINE__);

    return 0;
}

int main() {
    try {
        test_main();
    } catch (...) {
        std::cout << "catch ..." << std::endl;
    }
}