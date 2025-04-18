#include "sgshader.h"

#include <stdio.h>
#include <stdlib.h>
#include "glad/glad.h"

uint32_t sgCompileShader (int stage, char const* source, int size) {
  uint32_t shader = glCreateShader (stage);
  glShaderSource (shader, 1, &source, &size);
  glCompileShader (shader);
  int  success;
  char infolog[512];
  glGetShaderiv (shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog (shader, sizeof (infolog), NULL, infolog);
    fprintf (stderr, "Shader Compile Failed: %s\n", infolog);
    return 0;
  }
  return shader;
}

uint32_t sgLinkShaderProgram (uint32_t* shaderv, int shaderc) {
  int      i;
  uint32_t prog = glCreateProgram();
  for (i = 0; i < shaderc; ++i) {
    glAttachShader (prog, shaderv[i]);
  }
  glLinkProgram (prog);
  int  program_success;
  char infolog[512];
  glGetProgramiv (prog, GL_LINK_STATUS, &program_success);
  if (!program_success) {
    glGetProgramInfoLog (prog, sizeof (infolog), NULL, infolog);
    fprintf (stderr, "Shader Program Fail: %s\n", infolog);
    return 0;
  }
  return prog;
}

sgShader sgCompileShaderFromMemory(const char* vSource, size_t vSize, const char* fSource, size_t fSize) {
  sgShader shader = {0};
  uint32_t vShader = sgCompileShader(GL_VERTEX_SHADER, vSource, vSize);
  if (!vShader) {
      return shader; // Return empty shader on failure
  }
  uint32_t fShader = sgCompileShader(GL_FRAGMENT_SHADER, fSource, fSize);
  if (!fShader) {
      glDeleteShader(vShader);
      return shader; // Return empty shader on failure
  }

  uint32_t shaders[] = {vShader, fShader};
  shader.prog = sgLinkShaderProgram(shaders, 2);

  // Delete the shaders as they're linked into the program now and no longer needed
  glDeleteShader(vShader);
  glDeleteShader(fShader);

  return shader;
}

void sgDeleteShader(sgShader* shader) {
  if (shader && shader->prog != 0) {
      glDeleteProgram(shader->prog);
      shader->prog = 0;
  }
}