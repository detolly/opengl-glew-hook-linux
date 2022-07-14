
#include "sly_cooper.h"

#include <atomic>
#include <string_view>
#include <thread>
#include <chrono>

#include <string.h>
#include <dlfcn.h>

#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <fmt/core.h>

//#define PRINT_glBufferData
#include <hooks.h>

using namespace std::string_view_literals;

namespace hook
{
    template<size_t span = 16>
    void hexdump(const void* ptr, const std::size_t size)
        {
            const auto* data = static_cast<const unsigned char*>(ptr);
            for(int outer = 0; outer < size/span; outer++)
            {
                for(auto i = 0; i < span; i++)
                    fmt::print("{:02x} ", *(data+outer*16+i));
                fmt::print(" (");
                for(auto i = 0; i < span/4; i++)
                    fmt::print("{:08x} ", *(reinterpret_cast<const unsigned int*>(data+outer*16)+i));
                fmt::print(")");

                fmt::print(" (");
                for(auto i = 0; i < span/4; i++)
                    fmt::print("{:.2f} ", *(reinterpret_cast<const float*>(data+outer*16)+i));
                fmt::print(")");

                fmt::print("\n");
            }
                
    };

    void render_init()
    {
        puts("***** Init *****");
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }

    void render_tick()
    {
        //puts("***** Render *****");
        //ImGui_ImplOpenGL3_NewFrame();
        //ImGui::NewFrame();
        //ImGui::Begin("Greetings");
        //ImGui::Button("Greetings");
        //ImGui::End();
        //ImGui::Render();
        //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    constexpr bool should_enable_depth_test = true;
    constexpr bool should_wireframe = false;

    void setup_hook()
    {
        render_init();

        hook_glPolygonMode([](GLenum face, GLenum whatever) {

            //fmt::print("{:*^30}\n", "glPolygonMode");

            get_original_glPolygonMode()(face, whatever);
        });

        if constexpr (should_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        hook_glEnable([](GLenum to_enable) {
            
            //fmt::print("{:*^10}\n", "glEnable");
            
            if constexpr (!should_enable_depth_test)
                if (to_enable == GL_DEPTH_TEST) {
                    puts("Enable was DEPTH_TEST, returning..\n");
                    return;
                }

            get_original_glEnable()(to_enable);

        });
        
        if constexpr (!should_enable_depth_test) glDisable(GL_DEPTH_TEST);

        hook_glewBindVertexArray([](GLuint b) {
            
            fmt::print("{:*^40}\n", fmt::format("glBindVertexArray {}", b));

            get_original_glewBindVertexArray()(b);
        });

        hook_glDepthFunc([](GLenum depth_func) {

            if constexpr (!should_enable_depth_test) {
                get_original_glDepthFunc()(GL_GREATER);
            } else {
                get_original_glDepthFunc()(depth_func);
            }
        });

        hook_glClear([](GLbitfield mask) {
            
            //print_header("glClear 0x%x", mask);

            get_original_glClear()(mask);
        });

        hook_glewMultiDrawArrays([](GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {
            
            //print_header("%s", "glMultiDrawArrays");

            get_original_glewMultiDrawArrays()(mode, first, count, drawcount);
        });

        hook_glDrawElements([](GLenum mode, GLsizei count, GLenum type, const void* indices) {

            //printf("glDrawElements: %d %d %d %p\n", mode, count, type, indices);

            get_original_glDrawElements()(mode, count, type, indices);
        });

        hook_glewDrawElementsInstanced([](GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {

            fmt::print("glDrawElementsInstanced: {} {} {} {} {}\n"sv, mode, count, type, indices, instancecount);

            get_original_glewDrawElementsInstanced()(mode, count, type, indices, instancecount);

        });

        static std::atomic<int> smallest_found{ 1000 };
        static std::atomic<int> highest_found{ 0 };

        hook_glewUseProgram([](GLuint program)
        {
            if (program < smallest_found)
                smallest_found = program;
            if (program > highest_found)
                highest_found = program;

            //print_header("glUseProgram: %d (h: %d, l: %d)", program, static_cast<int>(highest_found), static_cast<int>(smallest_found));

            if (program == STATIC_FADING || program == ANIMATED_MODEL || program == OUTLINES)
                program = 0;

            get_original_glewUseProgram()(program);
        });

        hook_glewBufferAttachMemoryNV([](GLenum target, GLuint memory, GLuint64 offset)
        {
            fmt::print("glBufferAttachMemoryNV: {} {} {}", target, memory, offset);

            get_original_glewBufferAttachMemoryNV()(target, memory, offset);
        });

        hook_glewBufferAddressRangeNV([](GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length)
        {
            fmt::print("glglewBufferAddressRangeNV: {} {} {} {}", pname, index, address, length);

            get_original_glewBufferAddressRangeNV()(pname, index, address, length);
        });

        hook_glewGenBuffers([](GLsizei n, GLuint* buffers)
        {
            
            fmt::print("{:*<40}\n", fmt::format("glGenBuffers: {} {}  ", n, (void*)buffers));

            get_original_glewGenBuffers()(n, buffers); 
        });

        hook_glewBindBuffer([](GLenum target, GLuint buffer)
        {
            
            //fmt::print("glBindBuffer: {} {}\n", target, buffer);

            get_original_glewBindBuffer()(target, buffer);
        });

        hook_glewNamedBufferData([](GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) {

            fmt::print("glNamedBufferData: {} {} {} {}\n", buffer, size, data, usage);

            //hexdump(data, size);

            get_original_glewNamedBufferData()(buffer, size, data, usage);
        });

        /*

        hook_glewNamedBufferStorage([](GLuint buffer, GLsizeiptr size, const void* data, GLbitfield flags) {
            
            fmt::print("glNamedBufferStorage: {} {} {} {}\n", buffer, size, data, flags);

            hexdump(data, size);

            get_original_glewNamedBufferStorage()(buffer, size, data, flags);
        });

        */
        
        hook_glewBufferSubData([](GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {

            fmt::print("{:*^40}\n", fmt::format("glBufferSubData: {} {} {} {}", target, offset, size, data));

            get_original_glewBufferSubData()(target, offset, size, data);
        });

        hook_glewBufferData([](GLenum target, GLsizeiptr count, const void* vertices, GLenum draw_method)
        {

            fmt::print("{:*^40}\n", fmt::format("glBufferData: {} {} {} {}", target, count, vertices, draw_method));
            
            get_original_glewBufferData()(target, count, vertices, draw_method);
        });

        hook_glewMapBuffer([](GLenum target, GLenum access) -> void* {

            fmt::print("glMapBuffer: {} {}\n", target, access);

            return get_original_glewMapBuffer()(target, access);
        });

        hook_glewEnableVertexAttribArray([](GLuint index) {
            
            fmt::print("glEnableVertexAttribArray: \n\tindex:{}\n", index);

            get_original_glewEnableVertexAttribArray()(index);
        });

        hook_glewVertexAttribPointer([](GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {

            fmt::print("glVertexAttribPointer: \n\tindex:{} \n\tsize:{} \n\ttype:{} \n\tnormalized:{} \n\tstride:{} \n\tpointer:{}\n", index, size, type, normalized, stride, pointer);

            get_original_glewVertexAttribPointer()(index, size, type, normalized, stride, pointer);
        });
    }
}