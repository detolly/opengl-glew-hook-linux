
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ranges>
#include <vector>
#include <string>
#include <sstream>
#include <string_view>
#include <unordered_map>

int main(int argc, const char** argv)
{
    using namespace std::string_view_literals;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " /path/to/glew.h > /path/to/output.h" << std::endl;
        std::exit(1);
    }

    std::ifstream stream;

    stream.open(argv[1], std::ios::in);

    if (!stream.is_open())
    {
        perror(argv[1]);
        std::exit(1);
    }

    std::cout << R"ez(
// The following is generated code.

#pragma once

#include <dlfcn.h>

#include <fmt/core.h>

#define VISIBLE_HOOK __attribute__((visibility("default")))
#define GLAPI static

#include <cstdio>
#include <iostream>
#include <string_view>
#include <type_traits>

)ez";

    const auto starts_with = [](const std::string_view string, const std::string_view looking_for)
    {
        return string.find(looking_for) == 0;
    };

    std::string _line;

    struct function
    {
        std::string return_type, function_name;
    };

    struct def
    {
        std::string return_type, args_list;
    };

    struct gl_function
    {
        std::string return_type, function_name, args_list;
    };

    std::unordered_map<std::string, def> typedefs;
    std::vector<function> functions;
    std::vector<gl_function> gl_functions;

    constexpr auto get_string_view = [](auto&& range) {
        return std::string_view { range.begin(), range.end() };
    };

    _line.reserve(1024);
    while(std::getline(stream, _line))
    {
        const std::string_view line = _line;
        if (starts_with(line, "typedef"))
        {
            def a;
            auto start_return_value = _line.find("*") + 2;
            auto stop_return_value = _line.find(")");

            a.return_type = _line.substr(start_return_value, stop_return_value-start_return_value);

            auto start_argslist = line.find(")") + 3;

            a.args_list = line.substr(start_argslist, line.size() - start_argslist - 2);

            typedefs.insert(std::make_pair<>(a.return_type, std::move(a)));

            std::cout << line << "\n";
        }
        else if (starts_with(line, "GLAPI"))
        {
            gl_function a;

            auto start_return_value = 6; // GLAPI _
            auto stop_return_value = _line.find(" GLAPI", start_return_value);
            a.return_type = _line.substr(start_return_value, stop_return_value - start_return_value);

            constexpr static auto look_for = "GLAPIENTRY"sv;
            const auto start_fn_name = _line.find(look_for) + look_for.size()+1;
            const auto stop_fn_name = _line.find(" ", start_fn_name);
            a.function_name = line.substr(start_fn_name, stop_fn_name - start_fn_name);

            const auto start_argslist = line.find("(") + 1;
            const auto stop_argslist = line.size() - 2;
            a.args_list = line.substr(start_argslist, stop_argslist - start_argslist);

            if (a.function_name == "glGetString" || a.function_name == "glGetIntegerv")
                continue;

            gl_functions.emplace_back(std::move(a));

            //std::cout << "extern \"C\" { " << line << " } " << "\n";
        }
        else if (starts_with(line, "#") || starts_with(line, "//"))
        {
            std::cout << line << "\n";
        }
        else if (starts_with(line, "GLEW_FUN_EXPORT"))
        {
            // GLEW_FUN_EXPORT PFNGLVERTEXATTRIBI4USVPROC __glewVertexAttribI4usv;
            function f;
            
            int i = 0;
            for(const auto word : std::views::split(line, " "sv) | std::views::transform(get_string_view))
            {
                switch(i)
                {
                    case 0:
                        break;
                    case 1:
                        f.return_type = word;
                        break;
                    case 2:
                        f.function_name = word.substr(0, word.size()-1);
                        break;
                }
                i++;
            }
            
            std::cout << "extern \"C\" " << f.return_type << " " << f.function_name << ";" << std::endl;

            functions.emplace_back(std::move(f));

        }
    }

    const auto get_forward_call_from_arguments = [&get_string_view](const auto& args_list)
    {
        std::stringstream ss;
        std::size_t i = 0;
        auto range = std::views::split(args_list, ", "sv) | std::views::transform(get_string_view);
        for(auto argument : range)
        {
            if (!argument.compare("void"))
                return std::string();

            const auto last_space = argument.rfind(" ");
            auto str = argument.substr(last_space + 1);
            if (*str.begin() == '*')
                str = { str.begin()+1, str.end() };
            ss << str;
            ss << ", ";
            i++;
        }
        if (i) {
            const auto string = ss.str();
            return string.substr(0, string.size()-2);
        }
        
        return std::string();
    };

    const auto get_correct_forward_call_from_arguments = [&get_string_view](const auto& args_list)
    {
        std::stringstream ss;
        std::size_t i = 0;
        auto range = std::views::split(args_list, ", "sv) | std::views::transform(get_string_view);
        for(auto argument : range)
        {
            if (!argument.compare("void"))
                return std::string();

            const auto last_space = argument.rfind(" ");
            auto str = argument.substr(last_space + 1);
            if (*str.begin() == '*')
                str = { str.begin()+1, str.end() };
            ss << "get_correct_arg(" << str << ")";
            ss << ", ";
            i++;
        }
        if (i) {
            const auto string = ss.str();
            return string.substr(0, string.size()-2);
        }
        
        return std::string();
    };

    std::cout << R"asd(
    template<typename T>
    constexpr auto get_correct_arg(const T* arg)
    {
        return static_cast<const void*>(arg);
    };
    template<typename T>
    constexpr auto get_correct_arg(T* arg)
    {
        return static_cast<void*>(arg);
    };
    template<typename T>
    constexpr auto get_correct_arg(T arg) requires(!std::is_pointer_v<T>)
    {
        return std::forward<T>(arg);
    };
    )asd";

    for(const auto& f : gl_functions)
    {
        const auto function_ptr_type_with_name = [](const auto& f, const std::string& name) {
            return f.return_type + "(*" + name + ")(" + f.args_list + ")";
        };

        std::cout << "typedef " << function_ptr_type_with_name(f, f.function_name + "_t") << ";";
        std::cout << "static " << f.function_name << "_t __original_" << f.function_name << " = nullptr;\n";
        std::cout << "static " << f.function_name << "_t __hooked_" << f.function_name << " = nullptr;\n";
        //std::cout << "extern \"C\"\n{\n";
        std::cout << "extern \"C\" __attribute__((visibility(\"default\"))) " << f.return_type << " " << f.function_name << "(" << f.args_list << ")\n{\n";
        std::cout << "#if defined(PRINT_" << f.function_name << ") || defined(PRINT_ALL)\n\tusing namespace std::string_view_literals;\n";
        std::cout << "\tfmt::print(\"" << f.function_name << ": ";
        int i = 0;
        for(const auto _ : std::views::split(f.args_list, ", ") | std::views::transform(get_string_view))
        {
            if (_ == "void")
                continue;
            std::cout << "{} ";
            i++;
        }
        std::cout << "\\n\"";
        if (i > 0) 
            std::cout << ", " << get_correct_forward_call_from_arguments(f.args_list) << ");\n";
        else
            std::cout << ");\n";
        std::cout << "#endif\n";
        std::cout << "\tif(__hooked_" << f.function_name << " == nullptr)\n";
        bool is_void = f.return_type == "void";
        if (is_void)
        {
            std::cout << "\t\t__original_" << f.function_name << "(" << get_forward_call_from_arguments(f.args_list) << ");\n";
            std::cout << "\telse\n\t\t__hooked_" << f.function_name << "(" << get_forward_call_from_arguments(f.args_list) << ");\n";
        }
        else
        {
            std::cout << "\t\treturn __original_" << f.function_name << "(" << get_forward_call_from_arguments(f.args_list) << ");\n";
            std::cout << "\telse\n\t\treturn __hooked_" << f.function_name << "(" << get_forward_call_from_arguments(f.args_list) << ");\n";
        }
        //std::cout << "}\n";
        std::cout << "}\n";
    }

    std::cout <<
R"asd(

namespace hook
{
    static void __init_hook();

    static void setup_hook();
    static void render_tick();

)asd"sv;

    for(const auto& f : functions) 
    {
        std::cout << '\t';
        const auto __original_name = "original_" + f.function_name.substr(2);
        std::cout << "static " << f.return_type << " " << __original_name << ";\n";
        std::cout << "static inline void hook_" << f.function_name.substr(2) << "(" << f.return_type << " " << " func" << ") { " << __original_name << " = " << f.function_name << "; "
            << f.function_name << " = func;" << " }" << std::endl;
        std::cout << "[[nodiscard]] static inline " << f.return_type << " get_" << __original_name << "() { return " << __original_name << "; }\n";
    }

    for(const auto& f : gl_functions)
    {
        std::cout << "[[nodiscard]] static inline " << f.function_name << "_t" << " get_original_" << f.function_name << "() { return __original_" << f.function_name << "; }\n";
        std::cout << "static inline void hook_" << f.function_name << "(" << f.function_name << "_t func) { __hooked_" << f.function_name << " = func; }\n";
    }

    std::cout <<
R"asd(

void __init_hook()
{
)asd";
    for(const auto& f : functions) {
        const auto __original_name = "original_" + f.function_name.substr(2);
        std::cout << __original_name << " = " << f.function_name << ";\n"; 
    }
    for(const auto& f : gl_functions)
    {
        std::cout << "\t\t__original_" << f.function_name << " = (decltype(__original_" << f.function_name << "))dlsym(RTLD_NEXT, \"" << f.function_name << "\");\n";
    }
    std::cout <<
R"asd(
}

)asd"sv;
//void setup_hook()
//{
//)asd"sv;
//    for(const auto& f : functions) 
//    {
//        if (typedefs.contains(f.return_type))
//            std::cout << "hook_" << f.function_name.substr(2) << "([](" << typedefs.at(f.return_type).args_list << ") { print_header(\"" << f.function_name << "\");" << "});" << std::endl;
//        else
//            std::cerr << f.return_type << std::endl;
//    }
//    
//std::cout << R"asd(
//
//}
std::cout <<
R"asd(
static void nullptr_all() {

)asd"sv;

    std::for_each(functions.begin(), functions.end(), [](const auto& function) {
        std::cout << function.function_name << " = nullptr;\n";    
    });

std::cout << "}\n\n}";

std::cout << R"asd(

extern "C"
{

void(*original_glXSwapBuffers)(void*, unsigned long);
#undef glXSwapBuffers
VISIBLE_HOOK void glXSwapBuffers(void* display, unsigned long drawable_id)
{
    if (original_glXSwapBuffers == nullptr) {
        original_glXSwapBuffers = (decltype(original_glXSwapBuffers))dlsym(RTLD_NEXT, "glXSwapBuffers");
    }

    //fmt::print("{:*^20}", fmt::format("glXSwapBuffers: {} {}\n", display, drawable_id));
    hook::render_tick();

    original_glXSwapBuffers(display, drawable_id);
}

GLenum(*original_glewInit)(void) = nullptr;
#undef glewInit
VISIBLE_HOOK GLenum glewInit()
{
    printf("*** glewInit ***\n");

    if (original_glewInit == nullptr) {
        original_glewInit = (decltype(original_glewInit))dlsym(RTLD_NEXT, "glewInit");
    }
    
    const auto ret = original_glewInit();

    hook::__init_hook();

    printf("Sanity: %p\n", hook::get_original_glewActiveProgramEXT());
    
    hook::setup_hook();

    return ret;
}

}
)asd" << std::endl;

}