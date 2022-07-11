
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
        std::cerr << "Usage: " << argv[0] << " /path/to/glew.h" << std::endl;
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

#include <cstdio>

extern "C"
{

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

    std::unordered_map<std::string, def> typedefs;
    std::vector<function> functions;

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
        else if (starts_with(line, "#") || starts_with(line, "//") || starts_with(line, "GLAPI"))
        {

            std::cout << line << "\n";
        }
        else if (starts_with(line, "GLEW_FUN_EXPORT"))
        {
            // GLEW_FUN_EXPORT PFNGLVERTEXATTRIBI4USVPROC __glewVertexAttribI4usv;
            function f;

            auto get_string_view = [](auto&& view) {
                return std::string_view { view.begin(), view.end() };
            };
            
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
            
            std::cout << "extern \"C\" __attribute__((visibility(\"default\"))) " << f.return_type << " " << f.function_name << " = nullptr;" << std::endl;

            functions.emplace_back(std::move(f));

        }
    }

    std::cout <<
R"asd(

namespace hook
{
    void __init_hook();
    void setup_hook();


)asd"sv;

    for(const auto& f : functions) 
    {
        const auto original_name = "original_" + f.function_name.substr(2);
        std::cout << f.return_type << " " << original_name << ";\n";
        std::cout << "static void hook_" << f.function_name.substr(2) << "(" << f.return_type << " " << " func" << ") { " << original_name << " = " << f.function_name << "; "
            << f.function_name << " = func;" << " }" << std::endl;
        std::cout << "static " << f.return_type << " get_" << original_name << "() { return " << original_name << "; } ";
    }

    std::cout <<
R"asd(

GLenum(*original_glewInit)(void);
#undef glewInit
extern "C" GLenum glewInit()
{
    if (original_glewInit == nullptr) {
        original_glewInit = (decltype(original_glewInit))dlsym(RTLD_NEXT, "glewInit");
    }
    
    const auto ret = original_glewInit();

    hook::__init_hook();
    hook::setup_hook();

    return ret;
}

void __init_hook()
{
)asd";
    for(const auto& f : functions) {
        const auto original_name = "original_" + f.function_name.substr(2);
        std::cout << original_name << " = " << f.function_name << ";"; 
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
void nullptr_all() {

)asd"sv;

    std::for_each(functions.begin(), functions.end(), [](const auto& function) {
        std::cout << function.function_name << " = nullptr;\n";    
    });

std::cout << "}\n\n}\n}\n" << std::endl;
}