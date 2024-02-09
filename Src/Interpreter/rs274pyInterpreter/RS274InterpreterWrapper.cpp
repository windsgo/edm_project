#include "RS274InterpreterWrapper.h"

#include <Python.h>

#include <iostream>
#include <string>
#include <vector>

#include <boost/python/dict.hpp>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>

// log
#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER_NAME(s_logger, "interp");

// #define DEBUG(str)
#define DEBUG(str) std::cout << str;

namespace bp = boost::python;

namespace edm {
namespace interpreter {

RS274InterpreterWrapper RS274InterpreterWrapper::s_instance;

RS274InterpreterWrapper *edm::interpreter::RS274InterpreterWrapper::instance() {
    return &s_instance;
}

// Ensure that initialize and finalize are all done
// EveryTime ReStart the python interpreter, in case
// variables are all destroyed before next parse call
class PythonInterpreterWrapper final {
public:
    PythonInterpreterWrapper() {
        if (!Py_IsInitialized()) {
            s_logger->trace("py initialize");
            Py_Initialize();
        }
    }

    ~PythonInterpreterWrapper() {
        if (Py_IsInitialized()) {
            s_logger->trace("py finalize");
            Py_Finalize();
        }
    }

private:
    PythonInterpreterWrapper(const PythonInterpreterWrapper &) = delete;
    PythonInterpreterWrapper(PythonInterpreterWrapper &&) = delete;
};

static void py_add_module_path(const char *module_path) {
    bp::object module_sys = bp::import("sys");
    bp::object module_sys_path_append =
        module_sys.attr("path").attr("append"); // 修改了内置模块的变量(模块搜索路径),
                                                // 这种改动在解释器全局有效
    module_sys_path_append(module_path);
}

static void py_print_keys_of_dict(bp::object dict_object) {
    bp::dict d(dict_object);

    DEBUG("\n");

    for (int i = 0; i < bp::len(d); ++i) {
        auto key = d.keys()[i];
        DEBUG((std::string)bp::extract<std::string>(key) << "\n");
    }
    DEBUG("\n");
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

class RS274InterpreterWrapperImpl final {
public:
    friend class RS274InterpreterWrapper;

    RS274InterpreterWrapperImpl() noexcept {
        s_logger->trace("RS274InterpreterWrapperImpl ctor");
    }

    ~RS274InterpreterWrapperImpl() noexcept {
        s_logger->trace("RS274InterpreterWrapperImpl dtor");
    }

    ::std::string parse_file_to_str(std::string_view filename);

    RS274InterpreterWrapper::json_value
    parse_file_to_json(std::string_view filename);

private:
    bp::object get_parsed_py_interpreter_instance(std::string_view filename);

    void handle_py_exception_and_throw();

private:
    RS274InterpreterWrapperImpl(const RS274InterpreterWrapperImpl &) = delete;
    RS274InterpreterWrapperImpl(RS274InterpreterWrapperImpl &&) = delete;
};

bp::object RS274InterpreterWrapperImpl::get_parsed_py_interpreter_instance(
    std::string_view filename) {
    py_add_module_path(
        RS274InterpreterWrapper::instance()->get_rs274_py_module_dir().data());

    bp::object main_module_ =
        boost::python::import("__main__"); // 获取 __main__ 模块
    bp::object main_module_namespace_ =
        main_module_.attr("__dict__"); // 获取 __main__ 模块的命名空间

    // py_print_keys_of_dict(main_module_namespace_);

    bp::exec("from rs274 import *",
             main_module_namespace_); // 必须如此导入,
                                      // 才能将rs274模块导入到__main__作用域中

    // 在__main__作用域, 运行目标文件
    bp::object ignored = bp::exec_file(filename.data(), main_module_namespace_);

    bp::object rs274 = bp::import("rs274"); // 获取 rs274 模块
    bp::object rs274_namespace =
        rs274.attr("__dict__"); // 获取 rs274 模块的命名空间
    bp::object RS274Interpreter_class =
        rs274_namespace["RS274Interpreter"]; // 获取 RS274Interpreter 的类型对象

    // py_print_keys_of_dict(rs274_namespace);

    // bp::object ignored = bp::exec_file(filename.data(), rs274_namespace);

    // py_print_keys_of_dict(rs274_namespace);
    // py_print_keys_of_dict(rs274_namespace);

    // 在__main__作用域, 寻找定义的解释器实例
    auto interp_instances_found =
        find_instance_of_class(main_module_namespace_, RS274Interpreter_class);
    // auto interp_instances_found = find_instance_of_class(rs274_namespace,
    // RS274Interpreter_class);

    if (interp_instances_found.empty()) {
        throw RS274InterpreterException("Interperter Error",
                                        "No Interpeter instance");
    }

    if (interp_instances_found.size() > 1) {
        throw RS274InterpreterException("Interperter Error",
                                        "Interpeter instances not unique");
    }

    // 获取第一个实例的变量名, 实例对象
    std::string interp_instance_variable_name = interp_instances_found[0].first;
    bp::object interp_instance = interp_instances_found[0].second;
    // std::cout << interp_instance_variable_name << ", " <<
    // static_cast<std::string>(
    // bp::extract<std::string>(interp_instance.attr("__class__").attr("__name__"))
    // ) << std::endl;

    return interp_instance;
}

::std::string
RS274InterpreterWrapperImpl::parse_file_to_str(std::string_view filename) {
    PythonInterpreterWrapper wrapper; // wrapper to finialize python when throw
                                      // RS274InterpreterException

    try {
        bp::object interp_instance =
            get_parsed_py_interpreter_instance(filename);

        std::string command_list_json_str{bp::extract<std::string>(
            bp::object{interp_instance.attr("get_command_list_jsonstr")()})};

        return command_list_json_str;

    } catch (const bp::error_already_set &) {
        s_logger->error("parse_file_to_str error occured");

        handle_py_exception_and_throw();

        // will not reached here, just avoid compile warning
        throw;
    }

    // do not handle other exceptions
}

RS274InterpreterWrapper::json_value
RS274InterpreterWrapperImpl::parse_file_to_json(std::string_view filename) {
    // try to parse str to cpp-json object

    std::string s{parse_file_to_str(filename)};

    try {
        return json::parse(s).value();
    } catch (const json::exception &e) {
        throw RS274InterpreterException("json operate error", e.what());
    } catch (const std::exception &e) {
        throw RS274InterpreterException("json parse runtime error", e.what());
    }

    // do not handle other exceptions
}

void RS274InterpreterWrapperImpl::handle_py_exception_and_throw() {
    static const char *_unknown_err_type_str = "Unknown Err Type";
    static const char *_unknown_err_value_str = "Unknown Err Value";

    std::string err_type_str{_unknown_err_type_str};
    std::string err_value_str{_unknown_err_value_str};

    // assert(Py_IsInitialized());

    PyObject *type_ptr = NULL;
    PyObject *value_ptr = NULL;
    PyObject *traceback_ptr = NULL;
    PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);
    PyErr_NormalizeException(&type_ptr, &value_ptr, &traceback_ptr);

    if (type_ptr) {
        bp::handle h_type(type_ptr); // from PyObject* to boost::python::handle
        bp::str pystr_type{h_type};
        bp::extract<std::string> pystr_type_extractor(pystr_type);
        if (pystr_type_extractor.check()) {
            err_type_str = pystr_type_extractor();
        }
    }

    if (value_ptr) {
        bp::handle h_value(
            value_ptr); // from PyObject* to boost::python::handle

        bp::str pystr_value{h_value};
        bp::extract<std::string> pystr_value_extractor(pystr_value);
        if (pystr_value_extractor.check()) {
            err_value_str = pystr_value_extractor();
        }
    }

    if (err_type_str != R"(<class 'rs274.InterpreterException'>)" || true) {
        if (traceback_ptr) {
            try {
                bp::handle h_tb{traceback_ptr};

                bp::object o_tb{h_tb};

                bp::object tblineno = o_tb.attr("tb_lineno");

                bp::object trackback = bp::import("traceback");
                bp::object trackback_extract_tb = trackback.attr("extract_tb");
                bp::object extracted_list = trackback_extract_tb(o_tb);
                bp::object file = extracted_list[0].attr("filename");

                std::string file_str = bp::extract<std::string>(file);

                std::string::size_type iPos = file_str.find_last_of('/') + 1;
                file_str = file_str.substr(iPos, file_str.length() - iPos);

                int line = bp::extract<int>(tblineno);

                err_value_str +=
                    " *(" + file_str + ", line " + std::to_string(line) + ")";
            } catch (const bp::error_already_set &) {
                s_logger->error("exception in pyerror handle");
                PyErr_Print();
            }
        }
    }

    throw RS274InterpreterException(err_type_str, err_value_str);
}

::std::string RS274InterpreterWrapper::parse_file(std::string_view filename) {
    auto impl_ = std::make_shared<RS274InterpreterWrapperImpl>();

    return impl_->parse_file_to_str(filename);
}

RS274InterpreterWrapper::json_value
RS274InterpreterWrapper::parse_file_to_json(std::string_view filename) {
    auto impl_ = std::make_shared<RS274InterpreterWrapperImpl>();

    return impl_->parse_file_to_json(filename);
}

} // namespace interpreter
} // namespace edm
