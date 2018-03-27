#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "SDL.h"
#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <cstdint>

/** TODO: 
 * 
 * Should we actually use windows.h rather than SDL?
 * If we'd do that, we could use some useful windows API calls.
 * Maybe still use SDL, but only use those calls for useful stuff?
 * How would I go about portability in a situation like that?
 * 
 **/

#define internal static

//#define arrayCount(arr) (sizeof(arr)/sizeof(*arr))
#define arrayCount(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#include "types.cpp"
#include "math.cpp"

#include "shaders.h"
#include "objects.h"

internal const f32 PI = glm::pi<f32>();

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

internal u32
compile_shader(const char *vertexShaderCode, const char *fragmentShaderCode)
{
    u32 vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    u32 fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    i32 result = 0;
    i32 info_log_length;

    // Compile Vertex Shader
    glShaderSource(vertex_shader_id, 1, &vertexShaderCode, NULL);
    glCompileShader(vertex_shader_id);

    // Check Vertex Shader
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0)
    {
        std::vector<char> vertex_shader_error_message(info_log_length + 1);
        glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, &vertex_shader_error_message[0]);
        printf("%s\n", &vertex_shader_error_message[0]);
    }

    // Compile Fragment Shader
    glShaderSource(fragment_shader_id, 1, &fragmentShaderCode, NULL);
    glCompileShader(fragment_shader_id);

    // Check Fragment Shader
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0)
    {
        std::vector<char> fragment_shader_error_message(info_log_length + 1);
        glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, &fragment_shader_error_message[0]);
        printf("%s\n", &fragment_shader_error_message[0]);
    }

    // Link the program
    u32 program_ID = glCreateProgram();
    glAttachShader(program_ID, vertex_shader_id);
    glAttachShader(program_ID, fragment_shader_id);
    glLinkProgram(program_ID);

    // Check the program
    glGetProgramiv(program_ID, GL_LINK_STATUS, &result);
    glGetProgramiv(program_ID, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0)
    {
        std::vector<char> program_error_message(info_log_length + 1);
        glGetProgramInfoLog(program_ID, info_log_length, NULL, &program_error_message[0]);
        printf("%s\n", &program_error_message[0]);
    }

    glDetachShader(program_ID, vertex_shader_id);
    glDetachShader(program_ID, fragment_shader_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_ID;
}

//TODO: Should these set uniform functions do glUseProgram every time?
// I got confused, because I wasn't doing that before using these functions.
internal void
set_uniform_m4(u32 shader, const char *name, m4 matrix)
{
    u32 handle = glGetUniformLocation(shader, name);
    glUniformMatrix4fv(handle, 1, GL_FALSE, glm::value_ptr(matrix));
}

internal void
set_uniform_3f(u32 shader, const char *name, f32 f1, f32 f2, f32 f3)
{
    u32 handle = glGetUniformLocation(shader, name);
    glUniform3f(handle, f1, f2, f3);
}

internal void
set_uniform_v3(u32 shader, const char *name, v3 v)
{
    set_uniform_3f(shader, name, v.x, v.y, v.z);
}

i32 main(i32 argc, char **argv)
{
    // Init SDL stuff
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    atexit(SDL_Quit);

    SDL_Window *window = SDL_CreateWindow(
        "Snake A*",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (!window)
    {
        printf("SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Init OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext context = SDL_GL_CreateContext(window);

    SDL_GL_SetSwapInterval(0);

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Load shaders
    u32 basic_shader = compile_shader(basic_vertex_shader, basic_fragment_shader);
    if (!basic_shader)
    {
        printf("Error loading shaders.\n");
        return -1;
    }

//Load textures
#if 0
    u32 texture0 = loadTextureJPG("data/textures/container.jpg", true);
    u32 texture1 = loadTexturePNG("data/textures/awesomeface.png", true);
    if(!texture0 ||
       !texture1) {
        printf("Error loading textures.\n");
        return -1;
    }
#endif

//Quads
#if 0
    v3 quadPositions[]{
        v3(0, 0, 0),
        v3(1, 0, 0),
        v3(0, 1, 0),
        v3(0, 0, 0),
    };
    Rotation quadRotations[]{
        {v3(0, 0, 1), glm::radians((float)0)},
        {v3(0, 0, 1), glm::radians((float)0)},
        {v3(0, 0, 1), glm::radians((float)30)},
        {v3(0, 0, 1), glm::radians((float)60)},
    };
    v3 quadScales[]{
        v3(1.0f, 1.0f, 1.0f),
        v3(0.1f, 0.1f, 1.0f),
        v3(0.5f, 0.5f, 1.0f),
        v3(2.5f, 0.4f, 1.0f),
    };

    const i32 quadCount = 1;
    // const i32 quadCount = arrayCount(quadPositions);
    Mesh quadMeshArray[quadCount];

    u32 quadVertexArray;
    glGenVertexArrays(1, &quadVertexArray);

    u32 quadElementBuffer;
    glGenBuffers(1, &quadElementBuffer);

    u32 quadVertexBuffer;
    glGenBuffers(1, &quadVertexBuffer);
    u32 quadColorBuffer;
    glGenBuffers(1, &quadColorBuffer);
    u32 quadTexCoordBuffer;
    glGenBuffers(1, &quadTexCoordBuffer);

    glBindVertexArray(quadVertexArray);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexPositions), quadVertexPositions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, quadColorBuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexColors), quadVertexColors, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, quadTexCoordBuffer);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadTexCoords), quadTexCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);

    for(i32 i = 0; i < quadCount; i++) {
        quadMeshArray[i].vao           = quadVertexArray;
        quadMeshArray[i].count         = arrayCount(quadIndices);
        quadMeshArray[i].shaderProgram = basicShader;
    }
    //END Quads
#endif

    b32 running = true;
    f64 current_time = (f32)SDL_GetPerformanceCounter() /
                       (f32)SDL_GetPerformanceFrequency();
    f64 last_time = 0;
    f64 delta_time = 0;
    i32 frame_counter = 0;
    i32 last_frame_count = 0;
    f64 last_fps_time = 0;
    while (running)
    {
        last_time = current_time;
        current_time = (f64)SDL_GetPerformanceCounter() /
                       (f64)SDL_GetPerformanceFrequency();
        delta_time = (f64)(current_time - last_time);

        // Count frames for every second and print it as the title of the window
        ++frame_counter;
        if (current_time >= (last_fps_time + 1.f))
        {
            last_fps_time = current_time;
            i32 deltaFrames = frame_counter - last_frame_count;
            last_frame_count = frame_counter;
            char title[64];
            sprintf(title, "FPS: %d", deltaFrames);
            SDL_SetWindowTitle(window, title);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            {
                running = false;
            }
            break;

            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                {
                    running = false;
                }
                break;

                case '1':
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                break;

                case '2':
                {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }
                break;
                }
            }
            break;
            }
        }

        // glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// Draw quads
#if 0
        for (i32 i = 0; i < quadCount; i++)
        {
            v3 scale = quadScales[i];
            v3 position = quadPositions[i];
            Mesh mesh = quadMeshArray[i];
            Rotation rotation = quadRotations[i];

            m4 m = m4(1.0f);
            m4 translate = glm::translate(m, position);
            m4 rotate = glm::rotate(m, rotation.angle, rotation.axis);
            rotate = glm::rotate(rotate, glm::radians(10.f), v3(1, 0, 0));
            m4 scaleM = glm::scale(m, scale);
            m4 model = scaleM * translate * rotate;
            m4 mvp = projection * view * model;

            u32 mvpHandle = glGetUniformLocation(mesh.shaderProgram, "MVP");
            glUniformMatrix4fv(mvpHandle, 1, GL_FALSE, glm::value_ptr(mvp));

            glUseProgram(mesh.shaderProgram);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, 0);
        }
#endif

        glBindVertexArray(0);

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    return 0;
}
