
#include <string_view>

#include <hooks.h>
#include <dlfcn.h>

#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

template<typename... Args>
void print_header(const char* view, Args&&... args)
{
    printf("********************* ");
    printf(view, args...);
    puts(" *********************");
}

namespace hook
{
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

            print_header("%s", "glPolygonMode");

            get_original_glPolygonMode()(face, whatever);
        });

        if constexpr (should_wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        hook_glEnable([](GLenum to_enable) {
            
            print_header("glEnable: 0x%x", to_enable);
            
            if constexpr (!should_enable_depth_test)
                if (to_enable == GL_DEPTH_TEST) {
                    puts("Enable was DEPTH_TEST, returning..\n");
                    return;
                }

            get_original_glEnable()(to_enable);

        });
        
        if constexpr (!should_enable_depth_test) glDisable(GL_DEPTH_TEST);

        hook_glewBindVertexArray([](GLuint b) {
            print_header("glBindVertexArray: %u", b);

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
            
            print_header("glClear 0x%x", mask);

            get_original_glClear()(mask);
        });

        hook_glewMultiDrawArrays([](GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {
            
            print_header("%s", "glMultiDrawArrays");

            get_original_glewMultiDrawArrays()(mode, first, count, drawcount);
        });

        hook_glDrawElements([](GLenum mode, GLsizei count, GLenum type, const void* indices) {

            //printf("glDrawElements: %d %d %d %p\n", mode, count, type, indices);

            get_original_glDrawElements()(mode, count, type, indices);
        });

        hook_glewDrawElementsInstanced([](GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount) {

            printf("glDrawElementsInstanced: %d %d %d %p %d\n", mode, count, type, indices, instancecount);

            get_original_glewDrawElementsInstanced()(mode, count, type, indices, instancecount);

        });

        enum ShaderType : GLuint
        {
            MENU = 55,
        };

        hook_glewUseProgram([](GLuint program) {

            print_header("glUseProgram: %d", program);

            switch(program)
            {
                case MENU: program = 55;
                case 60: program = 62;
                case 62: program = 60;
            }

            get_original_glewUseProgram()(program);
        });
    }
}