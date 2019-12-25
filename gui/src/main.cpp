// dear imgui: standalone example application for SDL2 + OpenGL
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include "imgui/imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>

#undef IMGUI_IMPL_OPENGL_LOADER_GL3W
#define IMGUI_IMPL_OPENGL_LOADER_GLEW

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include <iostream>
#include <string>
#include "video_player.h"
#include <thread>
#include <vector>

float download_progress = 0.0f;
std::string download_speed = "0 mb/s";
std::string torrent_name = "<filename>";
std::string torrent_peers = "<n>";
std::string room_link = "syncwatch://room-<unique_id>";
std::vector<std::string> peers;

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

const char* seconds_to_display(int input, char* output)
{
    input = input % (24 * 3600); 
    int hours = input / 3600; 
  
    input %= 3600; 
    int minutes = input / 60 ; 
  
    input %= 60; 
    int seconds = input; 

    if (hours != 0)
        sprintf(output, "%02d:%02d:%02d", hours, minutes, seconds);
    else
        sprintf(output, "%02d:%02d", minutes, seconds);
    return output;
}

bool fullscreen = true;
void toggle_fullscreen(SDL_Window* window)
{
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    fullscreen = !fullscreen;
}

void wait_stdin()
{
    std::string input;
    std::cin >> input;
    std::cout << input << std::endl;

    if (input.rfind("url:", 0) == 0) 
    {
        std::string url = input.substr(std::string("url:").length());
        const char *cmd[] = {"loadfile", url.c_str(), NULL};
        mpv_command_async(mpv, 0, cmd);
    }

    if (input.rfind("torrent_progress:", 0) == 0)
    {
        download_progress = strtof(input.substr(std::string("torrent_progress:").length()).c_str(),0);
    }

    if (input.rfind("download_speed:", 0) == 0)
    {
        download_speed = input.substr(std::string("download_speed:").length()) + " mb/s" ;
    }

    if (input.rfind("torrent_name:", 0) == 0)
    {
        torrent_name = input.substr(std::string("torrent_name:").length());
    }

    if (input.rfind("torrent_peers:", 0) == 0)
    {
        torrent_peers = input.substr(std::string("torrent_peers:").length());
    }

    if (input.rfind("room_link:", 0) == 0)
    {
        room_link = input.substr(std::string("room_link:").length());
    }

    if (input.rfind("peer_me:", 0) == 0)
    {
        peers.push_back(input.substr(std::string("peer_me:").length()) + " (you)");
    }

    if (input.rfind("new_peer:", 0) == 0)
    {
        peers.push_back(input.substr(std::string("new_peer:").length()));
    }
}

void mpv_input()
{
    while(true)
    {
        wait_stdin();
    }
}

// Main code
int main(int argc, char *argv[])
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Syncwatch", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // mpv
    initialize_mpv();
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "no");

    //if (argc != 2)
    //    die("pass a single media file as argument");

    std::thread input_thread(&mpv_input);
  
    mpv_observe_property(mpv, 0,"duration", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0,"playback-time", MPV_FORMAT_INT64);


    int margin = 40;
    bool show_info_panel = true;

    
    static int slider_position;
    static int last_mouse_motion = 0;
    static bool show_interface = true;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
                
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.window.event == SDL_WINDOWEVENT_EXPOSED)
                redraw = 1;

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    mpv_play_pause();
                }

                if (event.key.keysym.sym == SDLK_f) {
                    toggle_fullscreen(window);
                }

                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_SetWindowFullscreen(window, 0);
                    fullscreen = false;
                }

                if (event.key.keysym.sym == SDLK_RIGHT) {
                    std::cout << "seeking forward" << std::endl;
                    const char *seek[] = {"seek", "10", "relative", NULL};
                    mpv_command_async(mpv, 0, seek);
                }

                if (event.key.keysym.sym == SDLK_LEFT) {
                    std::cout << "seeking backwards" << std::endl;
                    const char *seek[] = {"seek", "-10", "relative", NULL};
                    mpv_command_async(mpv, 0, seek);
                }
            }

            if (event.type == SDL_MOUSEMOTION) {
                last_mouse_motion = event.motion.timestamp;
                //std::cout << "Mouse moting event: " << last_mouse_motion << std::endl;
            }

            mpv_events(event);
        }

        // if mouse is hovered over a ui component or 
        // it's been less than 2 seconds since the last mouse motion
        show_interface = io.WantCaptureMouse || SDL_GetTicks() - last_mouse_motion < 2000;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(12,12);
        style.ItemSpacing = ImVec2(8,12);
        style.FramePadding = ImVec2(14,3);

        //ImGui::ShowDemoWindow();
        SDL_ShowCursor(show_interface);

        if (show_info_panel && show_interface)
        {
            ImGui::SetNextWindowPos(ImVec2(margin, margin));
            ImGui::SetNextItemWidth(350);
            ImGui::Begin("Info", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Separator();

            ImGui::Text("Room invite link");
            ImGui::Text(room_link.c_str());
            if (ImGui::Button("Copy to clipboard"))
            {
                ImGui::LogToClipboard();
                ImGui::LogText(room_link.c_str());
                ImGui::LogFinish();
            }

            ImGui::Separator();

            ImGui::Text("Video source");

            static char str1[1024] = "";
            ImGui::InputTextWithHint("", "Enter magnet link or url", str1, IM_ARRAYSIZE(str1));
            ImGui::SameLine(); 
            if (ImGui::Button("Stream")) 
            {
                // send to parent process
                std::cout << "source:" << str1 << std::endl;
            }

            ImGui::SameLine(); 
            HelpMarker("Enter a magnet link or a video url (youtube etc.)\nA complete list of supported sources can be found on\nhttps://ytdl-org.github.io/youtube-dl/supportedsites.html");

            ImGui::Text("Downloading");
            ImGui::SameLine(); 
            ImGui::TextDisabled("file (?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(torrent_name.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
            ImGui::SameLine(); 
            ImGui::Text(download_progress == 1.0f ? "" : std::string("from " + torrent_peers + " peers").c_str());

            ImGui::ProgressBar(download_progress, ImVec2(0.0f,0.0f));
            ImGui::SameLine(); 

            ImGui::Text(download_progress == 1.0f ? "Done" : download_speed.c_str());

            ImGui::Separator();

            ImGui::Text("Connected Peers");
            if (peers.empty())
            {
                ImGui::Text("None");
            }
            else
            {
                for (std::vector<std::string>::iterator it = peers.begin() ; it != peers.end(); ++it)
                    ImGui::Text((*it).c_str());
            }
            
           

            ImGui::End();
        }

        if (show_interface)
        {
            static int volume = 0.0f;

            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            ImGui::SetNextWindowSize(ImVec2(width - margin*2, 0));
            ImGui::SetNextWindowPos(ImVec2(margin, height - ImGui::GetTextLineHeightWithSpacing()*2 - margin));

            ImGui::Begin("Media Controls", 0, ImGuiWindowFlags_NoTitleBar);

            if (ImGui::Button("Play/Pause"))
            {
                mpv_play_pause();
            }
            
            ImGui::SameLine(0, 10);
            
            if (ImGui::Button("Rewind 10s"))
            {
                std::cout << "Rewind button clicked" << std::endl;
                const char *cmd_rewind[] = {"seek", "-10", "relative", NULL};
                mpv_command_async(mpv, 0, cmd_rewind);
            }

            ImGui::SameLine(0, 10);

            char slider_display[99], position_display[99], duration_display[99];
            seconds_to_display(position, position_display);
            seconds_to_display(duration, duration_display);
            sprintf(slider_display, "%s/%s", position_display, duration_display);

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderInt("", &slider_position, 0, duration, slider_display))
            {
                std::cout << "Slider position: " << slider_position << std::endl;
                const char *cmd_seek[] = {"seek", std::to_string(slider_position).c_str(), "absolute", NULL};
                mpv_command_async(mpv, 0, cmd_seek);
            }
            else
            {
                if (ImGui::IsItemDeactivatedAfterEdit)
                    slider_position = position;
            }
            
            

            ImGui::SetNextItemWidth(100);
            if (ImGui::SliderInt("Volume", &volume, 0, 100))
            {
                
            }

            ImGui::SameLine(0, 10);

            static bool mute;
            ImGui::Checkbox("Mute", &mute);

            ImGui::SameLine(0, 10);
            
            if (ImGui::Button("Fullscreen"))
            {
                toggle_fullscreen(window);
            }

            ImGui::SameLine(0, 10);
            ImGui::Button("Video Source");
            ImGui::SameLine(0, 10);
            ImGui::Button("Join a room");
            ImGui::SameLine(0, 10);
            ImGui::Checkbox("Show Info Panel", &show_info_panel);
            ImGui::SameLine(0, 10);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();

            

        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        mpv_redraw(window);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    mpv_render_context_free(mpv_gl);
    mpv_detach_destroy(mpv);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    input_thread.join();

    return 0;
}