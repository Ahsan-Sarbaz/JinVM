#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <malloc.h>
#include "imgui/imgui_memory_editor.h"

#include <stdint.h>
#include <vector>
#include <string>

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

        FILE* input = fopen("out.bin", "rb");
        fread(memory, 1, memory_size, input);
        fclose(input);

        return;

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


Chip* chip;

struct CLArgState;

struct CLArg
{
    const char* arg;
    void(*callback)(CLArgState*);
    const char* usage;
};

struct CLArgState
{
    CLArg* args;
    int argsCount;
    char** argv;
    int currentIndex;
    bool exit_imdtly;
    bool is_verbose;
    int width, height;
    char outfile_name[255];
    char inputfile_name[255];
    bool assemblerMode;
};


void printUsage(CLArgState* state)
{
    for (int i = 0; i < state->argsCount; i++)
    {
        if (state->args[i].usage != NULL)
        {
            printf("%s : %s\n", state->args[i].arg, state->args[i].usage);
        }
        else
        {
            printf("%s\n", state->args[i].arg);
        }
    }
}

CLArgState handleArgs(int argc, char** argv)
{
    CLArg args[] = {
        {"--version", [](CLArgState* state)
            {
                printf("JinVM version 0.1\n");
                state->exit_imdtly = true;
            },
            "show version number"
        },

        {"--verbose", [](CLArgState* state)
            {
                printf("Verbose Mode\n");
                state->is_verbose = true;
            },
            "show verbose data"
        },
        {"--usage", [](CLArgState* state)
            {
                printUsage(state);
                state->exit_imdtly = true;
            },
            "print usage"
        },
        { "--assemble", [](CLArgState* state) 
            {
                if (state->argv[state->currentIndex + 1] == NULL)
                {
                    printf("Please provide input file name\n");
                    printUsage(state);
                    state->exit_imdtly = true;
                    return;
                }

                sscanf(state->argv[state->currentIndex + 1], "%s", state->inputfile_name);
                if (state->inputfile_name == nullptr)
                {
                    printf("\nIncorrect input file name\n");
                    state->exit_imdtly = true;
                    return;
                }

                state->assemblerMode = true;
            },
            "assembes asm file to JinVM code"
        },
        {"--out", [](CLArgState* state)
            {
                if (state->argv[state->currentIndex + 1] == NULL)
                {
                    printf("Please provide output file name\n");
                    printUsage(state);
                    state->exit_imdtly = true;
                    return;
                }

                sscanf(state->argv[state->currentIndex + 1], "%s.bin", state->outfile_name);
                if (state->outfile_name == nullptr)
                {
                    printf("\nIncorrect out file name\n");
                    state->exit_imdtly = true;
                }
            },
            "sets the output binary file name (default is out.bin)"
        },
        {"--size", [](CLArgState* state)
            {
                if (state->argv[state->currentIndex + 1] == NULL)
                {
                    printf("Please provide window size\n");
                    printUsage(state);
                    state->exit_imdtly = true;
                    return;
                }

                sscanf(state->argv[state->currentIndex + 1], "%d,%d", &state->width, &state->height);
                if (state->width <= 0 || state->height <= 0)
                {
                    printf("\nIncorrect size\n");
                    state->exit_imdtly = true;
                }
            },
            "set size of the window format w,h (e.g --size 640,480)"
        }
    };

    CLArgState argState = { args, sizeof(args) / sizeof(CLArg), argv };

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < argState.argsCount; j++)
        {
            if (!strcmp(argState.argv[i], args[j].arg))
            {
                argState.currentIndex = i;
                args[j].callback(&argState);
            }
        }
    }

    return argState;
}

#define WRITE_BYTE(x) out_buffer[++buffer_offset] = x
#define WRITE_SHORT(x) (out_buffer)[++buffer_offset] = x >> 8; (out_buffer)[++buffer_offset] = x
#define WRITE_BYTE_XY out_buffer[++buffer_offset] = x << 4 | y

void assemble(const char* inputFilePath, const char* outputFilePath)
{
    if (outputFilePath && !outputFilePath[0])
    {
        outputFilePath = "out.bin";
    }

    if (inputFilePath && !inputFilePath[0])
    {
        printf("No Input file. how did this even happen!\n");
        return;
    }

    printf("Output file is %s\n", outputFilePath);

    FILE* in = fopen(inputFilePath, "rb");
    if (!in)
    {
        printf("Failed to open file %s\n", inputFilePath);
        return;
    }

    fseek(in, 0, SEEK_END);
    int input_file_size = ftell(in);
    if (!input_file_size)
    {
        printf("file %s is empty\n", inputFilePath);
        fclose(in);
        return;
    }

    fseek(in, 0, SEEK_SET);

    char* in_buffer = (char*)malloc(sizeof(char) * input_file_size);
    fread(in_buffer, 1, input_file_size, in);
    in_buffer[input_file_size] = '\0';
    //in_buffer[input_file_size + 1] = '\0';

    char* out_buffer = (char*)malloc(sizeof(char) * input_file_size);
    int buffer_offset = -1;
    memset(out_buffer, 0, input_file_size);

    std::vector<std::string> lines;

    auto l = strtok(in_buffer, "\n");
    while (l != NULL)
    {
        lines.emplace_back(l);
        l = strtok(NULL, "\n");
    }

    for (int i = 0; i < lines.size(); i++)
    {
        std::string ins;
        std::string arg;
        sscanf(lines[i].c_str(), "%s %[^\n]", ins.data(), arg.data());
        
        uint8_t x = 0;
        uint8_t y = 0;
        uint16_t imm = 0;
        uint16_t address = 0;

        if (!strcmp(ins.c_str(), "LD"))
        {   
            sscanf(arg.c_str(), "R%d, %hu", &x, &imm);
            WRITE_BYTE(LDIMM);
            WRITE_BYTE(x << 4);
            WRITE_SHORT(imm);
        }
        else if (!strcmp(ins.c_str(), "RESET"))
        {
            WRITE_BYTE(RESET);
        }
        else if(!strcmp(ins.c_str(), "ADD"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(ADD);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "SUB"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(SUB);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "MUL"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(MUL);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "DIV"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(DIV);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "AND"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(AND);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "OR"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(OR);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "NOT"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(NOT);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "XOR"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(XOR);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "SHR"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(SHR);
            WRITE_BYTE_XY;
        }
        else if (!strcmp(ins.c_str(), "SHL"))
        {
            sscanf(arg.c_str(), "R%d, R%d", &x, &y);
            WRITE_BYTE(SHL);
            WRITE_BYTE_XY;
        }

    }



    FILE* out = fopen(outputFilePath, "wb");
    fwrite(out_buffer, 1, sizeof(char) * input_file_size, out);


    fclose(out);
    fclose(in);

    //this produces error i dont know why on windows
#ifndef _WIN32
    free(out_buffer);
    free(in_buffer);
#endif
}

int main(int argc, char **argv)
{

    auto argState = handleArgs(argc, argv);

    if (argState.exit_imdtly)
    {
        return 0;
    }

    if (argState.assemblerMode)
    {

        assemble(argState.inputfile_name, argState.outfile_name);

        return 0;
    }

    int WIDTH = argState.width;
    int HEIGHT = argState.height;

    if (argState.width <= 0 || argState.height <= 0)
    {
        WIDTH = 640;
        HEIGHT = 480;
    }


    if(glfwInit() != GLFW_TRUE)
    {
        printf("FAILED to init GLFW!\n");
        return 1;
    }
    else
    {
        if (argState.is_verbose)
        {
            printf("GLFW Initialized\n");
        }
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "JinVM", 0, 0);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    chip = new Chip;

    glfwSetKeyCallback(window , [](GLFWwindow* window, int key, int scanecode, int action, int mods){
        if(action == GLFW_PRESS)
        {
            switch(key)
            {
                case GLFW_KEY_SPACE:
                    chip->tick();
                break;

                case GLFW_KEY_R:
                    chip->restart();
                break;

                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;

            }
        }
    });

    const char *glsl_version = "#version 130";
    
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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    ImVec4 clear_color = {};

    glMatrixMode(GL_PROJECTION_MATRIX);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);

    MemoryEditor memEditor;

    chip->restart();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
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


        memEditor.DrawWindow("Memory Editor", chip->memory, memory_size);

        if(ImGui::Begin("Debug"))
        {
            ImGui::Text("OpCode %X", chip->opcode);
            ImGui::Text("PC %d", chip->pc);
            ImGui::Text("R0 %d R8 %d", chip->R[0], chip->R[8]);
            ImGui::Text("R1 %d R9 %d", chip->R[1], chip->R[9]);
            ImGui::Text("R2 %d R10 %d", chip->R[2], chip->R[10]);
            ImGui::Text("R3 %d R11 %d", chip->R[3], chip->R[11]);
            ImGui::Text("R4 %d R12 %d", chip->R[4], chip->R[12]);
            ImGui::Text("R5 %d R13 %d", chip->R[5], chip->R[13]);
            ImGui::Text("R6 %d R14 %d", chip->R[6], chip->R[14]);
            ImGui::Text("R7 %d R15 %d", chip->R[7], chip->R[15]);

            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
