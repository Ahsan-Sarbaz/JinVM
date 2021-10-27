#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "imgui_memory_editor.h"

#include <stdint.h>

#define RX R[(opcode & 0x00F0) >> 4]
#define RY R[(opcode & 0x000F) >> 0]

#define INS (opcode & 0xF000)
#define F   (opcode & 0x0F00)
#define X   (opcode & 0x00F0) >> 4
#define Y   (opcode & 0x000F) >> 0

#define ADD 0x00
#define SUB 0x01
#define MUL 0x02
#define DIV 0x03
#define AND 0x04
#define OR  0x05
#define NOT 0x06
#define XOR 0x07
#define SHR 0x08
#define SHL 0x09

#define LDIMM 0x10
#define LDREG 0x11
#define SWAP  0x12

#define JMP  0x20


#define RESET 0xF0

constexpr size_t memory_size = sizeof(uint8_t) * 1024 * 64;

struct Chip
{
    uint16_t opcode;
    uint8_t* memory;
    uint16_t pc;

    uint16_t R[16];

    void restart()
    {
        memory = (uint8_t*)malloc(memory_size);
        memset(memory, 0, memory_size);
        memset(R, 0, sizeof(R));
        opcode = 0;
        pc = 0;

        uint16_t i = 0;

        memory[i] = LDIMM; // LD R0, 3
        memory[++i] = 0x00;

        memory[++i] = 0x00;
        memory[++i] = 3;

        memory[++i] = LDIMM; // LD R1, 6
        memory[++i] = 0x10;

        memory[++i] = 0x00;
        memory[++i] = 6;

        uint8_t here = (uint8_t)i;
        // SWAP R0, R1
        memory[++i] = SWAP;
        memory[++i] = 0x01;

        // ADD R0, R1
        memory[++i] = ADD;
        memory[++i] = 0x01;

        // SUB R0, R1
        memory[++i] = SUB;
        memory[++i] = 0x01;

        // MUL R0, R1
        memory[++i] = MUL;
        memory[++i] = 0x01;

        // DIV R0, R1
        memory[++i] = DIV;
        memory[++i] = 0x01;

        // JMP here
        memory[++i] = JMP;
        memory[++i] = 0x00;

        memory[++i] = 0x00;
        memory[++i] = here + 1;

        // AND R0, R1
        memory[++i] = AND;
        memory[++i] = 0x01;

        // OR R0, R1
        memory[++i] = OR;
        memory[++i] = 0x01;

        // SUB R0, R1
        memory[++i] = SUB;
        memory[++i] = 0x01;


        // XOR R0, R0
        memory[++i] = XOR;
        memory[++i] = 0x00;

        // RESET
        memory[++i] = RESET;
        memory[++i] = 0x00;
    }

    void tick()
    {
        opcode = (uint32_t)((memory[pc] << 8) | (memory[pc+1] << 0));

        // printf("OpCode 0x%X\n", opcode & 0xF000);
        switch (INS)
        {
        case 0x0000: // 0x0__ // MATHS
            switch (F)
            {
                case 0x0000: // ADD
                    RX = RX + RY;
                break;

                case 0x0100: // SUB
                    RX = RX - RY;
                break;

                case 0x0200: // MUL
                    RX = RX * RY;
                break;

                case 0x0300: // DIV
                    RX = RX / RY;
                break;

                case 0x0400: // AND
                    RX = RX & RY;
                break;
                
                case 0x0500: // OR
                    RX = RX | RY;
                break;

                case 0x0600: // NOT
                    RX = ~RX;
                break;

                case 0x0700: // XOR
                    RX = RX ^ RY;
                break;

                case 0x0800: // SHIFT RIGHT
                    RX = RX >> RY;
                break;

                case 0x0900: // SHIFT LEFT
                    RX = RX << RY;
                break;

            }
            pc += 2;
        break;

        case 0x1000: // 0x10__ // REG MANIP
            // printf("REG MANIP\n");
            switch (F) // 0x0F00
            {
                case 0x0000: // LOAD IMM
                    RX = read_next_short(); 
                    pc += 2;
                break;
                case 0x0100: // LOAD REG
                    RX = RY;
                break;
                case 0x0200: // SWAP
                {   
                    auto temp = RX;
                    RX = RY;
                    RY = temp;
                }
                break;
            }
            pc += 2;
        break;

        case 0x2000: // 0x2__ // Control Flow
            switch (F)
            {
                case 0x0000: // JMP
                    pc = read_next_short();
                break;
            }

        break;


        case 0xF000: // 0xF__ // RESET
            pc = 0;
            memset(R, 0, sizeof(R));
        break;
        }

        // printf("----\n");
    }

    uint16_t read_next_short()
    {
        return (uint16_t)((memory[pc + 2] << 8) | (memory[pc+ 3] << 0));

    }
};


int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Window *window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool running = true;

    ImVec4 clear_color = {};

    glMatrixMode(GL_PROJECTION_MATRIX);
    glLoadIdentity();
    glOrtho(0, 640, 480, 0, -1, 1);

    MemoryEditor memEditor;

    Chip chip;
    chip.restart();

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.scancode)
                {
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                case SDL_SCANCODE_SPACE:
                    chip.tick();
                    break;

                }
                break;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // for (int y = 0; y < 32; y++)
        // {
        //     for (int x = 0; x < 64; x++)
        //     {
        //         if (chip.display[(y * 64) + x] == 1)
        //         {
        //             glBegin(GL_QUADS);
        //             glVertex2f(x * 10, y * 15);
        //             glVertex2f(x * 10, y * 15 + 15);
        //             glVertex2f(x * 10 + 10, y * 15 + 15);
        //             glVertex2f(x * 10 + 10, y * 15);
        //             glEnd();
        //         }
        //     }
        // }


        memEditor.DrawWindow("Memory Editor", chip.memory, memory_size);

        ImGui::Begin("Debug");
        ImGui::Text("OpCode %X", chip.opcode);
        ImGui::Text("PC %d", chip.pc);
        ImGui::Text("R0 %d R8 %d", chip.R[0], chip.R[8]);
        ImGui::Text("R1 %d R9 %d", chip.R[1], chip.R[9]);
        ImGui::Text("R2 %d R10 %d", chip.R[2], chip.R[10]);
        ImGui::Text("R3 %d R11 %d", chip.R[3], chip.R[11]);
        ImGui::Text("R4 %d R12 %d", chip.R[4], chip.R[12]);
        ImGui::Text("R5 %d R13 %d", chip.R[5], chip.R[13]);
        ImGui::Text("R6 %d R14 %d", chip.R[6], chip.R[14]);
        ImGui::Text("R7 %d R15 %d", chip.R[7], chip.R[15]);

        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
