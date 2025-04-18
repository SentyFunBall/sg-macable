#pragma once

#include <stdint.h> // For uint32_t
#include <stdlib.h> // For size_t

// #include "sg.h" // REMOVED: Causes circular dependency

// Define the sgShader struct
typedef struct sgShader {
    uint32_t prog; // OpenGL shader program ID
    // Add other relevant fields if needed, e.g., uniform locations
} sgShader;

// Function declarations
uint32_t sgCompileShader (int stage, char const* source, int size);
uint32_t sgLinkShaderProgram (uint32_t* shaderv, int shaderc);

// Function to compile shader from memory (declaration needed based on usage in sg.c)
sgShader sgCompileShaderFromMemory(const char* vSource, size_t vSize, const char* fSource, size_t fSize);

// Function to delete shader (declaration needed based on usage in sg.c)
void sgDeleteShader(sgShader* shader);
