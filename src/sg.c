#include "sg.h" // Include sg.h first to get definitions
#include "sys/time.h"

#ifdef _WIN32
#  include <dwmapi.h>
#  define GLFW_EXPOSE_NATIVE_WIN32 1
#endif
#include "GLFW/glfw3native.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sgshader.h"
#include "sgcli.h"
#include "sginput.h"
#include "sgapi.h"
#include "sgimage.h"
#include "cJSON/cJSON.h"

//grabbing shaders hex strings
extern unsigned char shaders_display_vert[];
extern unsigned int shaders_display_vert_len;
extern unsigned char shaders_display_frag[];
extern unsigned int shaders_display_frag_len;

static SGstate state; // SGstate should now be defined via sg.h

static int W = 1280;
static int H = 720;

static float vertices[] = {
    1.f,  1.f,  1.f, 1.f, /* top right */
    1.f,  -1.f, 1.f, 0.f, /* bottom right */
    -1.f, -1.f, 0.f, 0.f, /* bottom left */
    -1.f, 1.f,  0.f, 1.f, /* top left */
};
static unsigned int indices[] = {
    /* note that we start from 0! */
    0, 1, 3, /* first Triangle */
    1, 2, 3  /* second Triangle */
};

struct SSBONOPRIM {
  float time;
  int   primc;
};

static SGprimitive tprim;

typedef struct PackedVert {
  h_vec2 p;
  h_vec3 c;
} PackedVert;

/*long sgPackTriangles (uint32_t VAO, uint32_t VBO) {
  PackedVert* pvv = NULL;

  int pvc = 0;
  int i;
  for (i = 0; i < state.tric; ++i) {
    struct PackedVert v3[3] = {0};
    SGtriangle        t     = state.triv[i];
    float             r     = (float)t.c.r / 255.f;
    float             g     = (float)t.c.g / 255.f;
    float             b     = (float)t.c.b / 255.f;
    v3[0].x                 = t.p1.x;
    v3[0].y                 = t.p1.y;
    v3[1].x                 = t.p2.x;
    v3[1].y                 = t.p2.y;
    v3[2].x                 = t.p3.x;
    v3[2].y                 = t.p3.y;
    printf ("%f\n", v3[0].x);
    int i2;
    for (i2 = 0; i2 < 3; ++i2) {
      v3[i2].r = r;
      v3[i2].g = g;
      v3[i2].b = b;
    }
    pvv = mem_grow (pvv, sizeof (struct PackedVert), pvc, v3, 3);
    pvc += 3;
  }
  // printf ("SUPERBLY FUN %i\n", pvc);
  glBindBuffer (GL_ARRAY_BUFFER, VBO);
  glBufferData (GL_ARRAY_BUFFER, sizeof (struct PackedVert) * pvc, pvv,
                GL_DYNAMIC_DRAW);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  free (pvv);
  // printf ("BLACKOUT FUN\n");
  return pvc;
}*/

#define funny(name)                                              \
  printf ("case %u: /* " name " */ return GLFW_MOUSE_BUTTON_\n", \
          str_hash (name))

double now_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

int main (int argc, char** argv) {
#if 0
  funny ("left");
  funny ("right");
  funny ("middle");
  funny ("button1");
  funny ("button2");
  funny ("button3");
  funny ("button4");
  funny ("button5");
  funny ("button6");
  funny ("button7");
  funny ("button8");

#endif
#if 0
  funny ("lshift");
  funny ("rshift");
  funny ("lcontrol");
  funny ("rcontrol");
  funny ("lalt");
  funny ("ralt");
  funny ("lbracket");
  funny ("rbracket");
  funny ("space");
  funny ("backspace");
  funny ("tab");
  funny ("enter");
  funny ("minus");
  funny ("equal");
  funny ("up");
  funny ("down");
  funny ("left");
  funny ("right");
  funny ("comma");
  funny ("period");
  funny ("escape");
  funny ("slash");
  funny ("backslash");
  funny ("semicolon");
  funny ("delete");
  funny ("page up");
  funny ("page down");
  funny ("home");
  funny ("end");
  funny ("insert");



  int ii;
  for (ii = 48; ii <= 57; ++ii) {
    char const s[2] = {(char)ii, '\0'};
    printf ("case %u: /* %c */ return GLFW_KEY_%c;\n", str_hash (s), ii, ii);
    /*uint32_t   hash = str_hash (s);
    printf ("%c = %u\n", (char)ii, hash);*/
  }
#endif

  state = (SGstate){0};
  state.pixels = NULL;

  // Allocate and initialize SSBO immediately
  state.ssbo = malloc(sizeof(SSBO));
  if (state.ssbo == NULL) {
      errorf("Failed to allocate ssbo buffer.\n");
      return 1;
  }
  state.ssbo->primc = 0;
  state.ssbo->time = 0.0f;

  state.tfps       = 60.f;
  state.projectDir = (char*)str_cpy (".", npos);

  if (doTheDoThing (&state, argc, argv)) {
    return 1;
  }
  cJSON* projectJSON = cJSON_ParseWithLength (state.projectFileContent.data,
                                              state.projectFileContent.size);
  if (!projectJSON) {
    errorf ("Failed to parse project json!\n\t%s\n", cJSON_GetErrorPtr());
    return 1;
  }
  if (projectJSON->type != cJSON_Object) {
    errorf ("project json root is not an object\n");
    exit (2);
  }
  if (!projectJSON->child) {
    errorf ("project json root does not have a child node\n");
    exit (2);
  }
  cJSON* itr = projectJSON->child;
  while (itr) {
    if (!strcmp (itr->string, "monitorWidth") && itr->type == cJSON_Number) {
      state.width = itr->valueint;
      if (state.width < 1 || state.width > 1080) {
        errorf (
            "project monitor width is outside acceptable bounds\n\t%i is not "
            "within such bounds\n",
            state.width);
        exit (2);
      }
    } else if (!strcmp (itr->string, "monitorHeight") &&
               itr->type == cJSON_Number) {
      state.height = itr->valueint;
      if (state.height < 1 || state.height > 1080) {
        errorf (
            "project monitor height is outside acceptable bounds\n\t%i is not "
            "within such bounds\n",
            state.height);
        exit (2);
      }
    } else if (!strcmp (itr->string, "name") && itr->type == cJSON_String) {
      state.name = (char*)str_cpy (itr->valuestring, npos);
    }
    itr = itr->next;
  }
  if (!state.width || !state.height) {
    fprintf (stderr,
             "Project settings doesnt set monitor width and height. Using "
             "default: 96x96.\n");
    state.width  = 96;
    state.height = 96;
    // errorf ("Project settings is missing monitor width and height\n");
    // exit (2);
  }
  if (!state.name) {
    state.name = (char*)str_cpy ("stormground", npos);
  }

  // Allocate CPU pixel buffer AFTER getting width/height
  state.pixels = malloc(state.width * state.height * 4 * sizeof(float));
  if (!state.pixels) {
      errorf("Failed to allocate CPU pixel buffer\n");
      // Perform necessary cleanup before exiting
      if (state.ssbo) free(state.ssbo);
      free((void*)state.name);
      free((void*)state.projectFileContent.data);
      free((void*)state.projectDir);
      cJSON_Delete(projectJSON);
      return 1;
  }
  // Initialize pixels to BLACK!
  memset(state.pixels, 0, state.width * state.height * 4 * sizeof(float));

  cJSON_Delete (projectJSON);

  /* char* fpath = io_fullpath ("~/hello/./yes"); */
  
  glfwInit();
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_RESIZABLE, GLFW_TRUE);
  state.win = glfwCreateWindow (W, H, state.name, NULL, NULL);

  if (!state.win) {
    fprintf (stderr, "Failed to init GLFW window\n");
    glfwTerminate();
    return 1;
  }

#if defined(_THEWINDOWS) && defined(GLFW_EXPOSE_NATIVE_WIN32)
  HWND hwnd      = glfwGetWin32Window (state.win);
  BOOL dark_mode = 1;
  DwmSetWindowAttribute (hwnd, 20, &dark_mode, sizeof (dark_mode));
  HICON hi =
      (HICON)LoadImageW (GetModuleHandle (NULL), MAKEINTRESOURCEW (IDI_ICON),
                         IMAGE_ICON, 180, 180, 0);
  // fprintf (stderr, "%p\n", hi);
  if (hi) {
    ICONINFO hiinfo;
    GetIconInfo (hi, &hiinfo);
    HBITMAP hbit = hiinfo.hbmColor;
    // fprintf (stderr, "%p\n", hbit);
    BITMAP bit;
    GetObject (hbit, sizeof (bit), (LPVOID)&bit);
    // fprintf (stderr, "%p, %li %li\n", bit.bmBits, bit.bmWidth, bit.bmHeight);
    if (bit.bmWidth * bit.bmHeight > 0) {
      GLFWimage      image;
      unsigned char* copy =
          (unsigned char*)malloc (bit.bmWidth * bit.bmHeight * 4);
      GetBitmapBits (hbit, bit.bmWidth * bit.bmHeight * 4, copy);
      image.pixels = copy;
      image.width  = bit.bmWidth;
      image.height = bit.bmHeight;
      glfwSetWindowIcon (state.win, 1, &image);
    }
  }
#endif

  glfwMakeContextCurrent (state.win);
  glfwSwapInterval (0);

  sgSetInputState (&state);
  glfwSetWindowUserPointer (state.win, &state);

  if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) {
    fprintf (stderr, "Failed to load OpenGL\n");
    glfwTerminate();
    return 2;
  }

  int fbW, fbH;
  glfwGetFramebufferSize (state.win, &fbW, &fbH);
  glViewport (0, 0, fbW, fbH);


  glfwSetFramebufferSizeCallback (state.win, sgFramebufSizeCallback);
  glfwSetKeyCallback (state.win, sgKeyCallback);
  glfwSetScrollCallback (state.win, sgScrollCallback);
  glfwSetMouseButtonCallback (state.win, sgMouseButtonCallback);
  glfwSetCursorPosCallback (state.win, sgCursorPosCallback);
  // glfwSetJoystickCallback (sgJoystickCallback);

  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable (GL_BLEND);

  // --- Load Display Shader ---
  // Use the correct embedded shader data
  state.displayShader = sgCompileShaderFromMemory((const char*)shaders_display_vert, shaders_display_vert_len,
                                                (const char*)shaders_display_frag, shaders_display_frag_len);
  if (state.displayShader.prog == 0) {
      errorf("Failed to compile/link display shader\n");
      // Perform necessary cleanup before exiting
      if (state.pixels) free(state.pixels);
      if (state.ssbo) free(state.ssbo);
      free((void*)state.name);
      free((void*)state.projectFileContent.data);
      free((void*)state.projectDir);
      glfwTerminate();
      return 3; // Use a different exit code
  }

  unsigned int VBO, VAO, EBO;
  // Replace DSA functions with non-DSA equivalents
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO); // Bind the VAO first

  // VBO setup
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // EBO setup
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Vertex attribute pointers
  // Position attribute (location = 0)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Texture coord attribute (location = 1)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Unbind VBO (VAO keeps track of EBO binding)
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // You can unbind the VAO now, will re-bind it in the render loop
  glBindVertexArray(0);
  // Do NOT unbind the EBO while the VAO is bound.
  // Since we unbound the VAO above, we can unbind the EBO here if needed,
  // but it's not strictly necessary as it will be bound via the VAO later.
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Optional


  /* remember: do NOT unbind the EBO while a VAO is active as the bound element
   * buffer object IS stored in the VAO; keep the EBO bound.
   */

  // --- Create Target Texture (state.mon) ---
  // This texture will receive the data from state.pixels
  sgGenTextures2D (GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, // Use NEAREST and CLAMP for pixel-perfect display
                   GL_CLAMP_TO_EDGE, GL_RGBA32F, state.width, state.height, 1,
                   &state.mon);
  // sgBindTexture (state.mon, GL_READ_WRITE); // This was for compute shader image load/store


  SGscript sgscr = {0};
  sgDoFile (&sgscr, &state, strcat(state.projectDir, "/main.lua"));

  double ls      = now_ms();
  h_timepoint tnow = timenow();
  double      cputime = 0.0;
  float       delta   = 0.0;
  size_t      frame   = 0;
  double      fps     = 0.0;
  while (1) {
    if (glfwWindowShouldClose (state.win)) {
      state.runstate = SG_RUNSTATE_STOP;
    }
    if (state.runstate == SG_RUNSTATE_STOP)
      break;

    ++frame;

    glClearColor (.1f, .1f, .1f, 1.f);
    glClear (GL_COLOR_BUFFER_BIT);

    glfwGetFramebufferSize (state.win, &W, &H);

    float aspect1 = (float)W / (float)H;
    float aspect2 = (float)state.width / (float)state.height;
    float daspect = aspect1 / aspect2;
    /* y = -(y - H); */
    float fx, fy, fx2, fy2, fw, fh;
    if (daspect > 1.0) {
      fw = (float)W / daspect;
      fh = H;
      fx = ((float)W - fw) / 2.f;
      fy = 0;
    } else {
      fw = W;
      fh = (float)H * daspect;
      fx = 0;
      fy = ((float)H - fh) / 2.f;
    }
    fx2        = (state.fakeCurX - fx) / fw * state.width;
    fy2        = (state.fakeCurY - fy) / fh * state.height;
    fx2        = floorf ((fx2 < 0) ? fx2 - 1.f : fx2);
    fy2        = floorf ((fy2 < 0) ? fy2 - 1.f : fy2);
    state.curx = fx2;
    state.cury = fy2;

    int i;
    /*if (state.gpads[1].connected &&
        state.gpads[1].buttons[GLFW_GAMEPAD_BUTTON_A] == SG_HOLD) {
      notef ("Pressed A! %.3f\n",
             state.gpads[1].gstate.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
    }*/
    if (sgCallGlobal (&sgscr, "onTick")) {
      exit (5);
    }

    // --- Render Primitives to CPU Buffer ---
    sgClearPixels(&state, 0.1f, 0.1f, 0.1f); // Clear CPU buffer (use desired background)
    sgRenderCPUBuffer(&state); // Process primitives from ssbo->primv and draw to state.pixels

    // --- Upload CPU Buffer to Texture ---
    glBindTexture(GL_TEXTURE_2D, state.mon.tex);
    // Ensure format and type match allocation (GL_RGBA, GL_FLOAT) and setPixel usage
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, state.width, state.height, GL_RGBA, GL_FLOAT, state.pixels);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind

    // --- Draw Fullscreen Quad ---
    glUseProgram (state.displayShader.prog); // Use the loaded display shader
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, state.mon.tex); // Bind the texture we just updated
    glUniform1i (glGetUniformLocation (state.displayShader.prog, "screenTexture"), 0);

    // --- Calculate and Pass Scale Uniform ---
    float windowAspect = (float)W / (float)H;
    float textureAspect = (float)state.width / (float)state.height;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (windowAspect > textureAspect) { // Window wider than texture (pillarbox)
        scaleX = textureAspect / windowAspect;
    } else { // Window taller than texture (letterbox)
        scaleY = windowAspect / textureAspect;
    }
    glUniform2f(glGetUniformLocation(state.displayShader.prog, "scale"), scaleX, scaleY);

    glBindVertexArray (VAO); // Bind quad VAO
    glDrawElements (GL_TRIANGLES, sizeof (indices) / sizeof (indices[0]),
                    GL_UNSIGNED_INT, 0);
    glBindVertexArray (0); // Unbind VAO
    glUseProgram(0); // Unbind shader

    glfwSwapBuffers (state.win);
    sgAdvanceInputs();
    glfwPollEvents();

    //tnow = timenow();
    //cputime = timeduration (timenow(), tnow, milliseconds_e);
    //double _t = timeduration (timenow(), ls, milliseconds_e);
    double cur = now_ms();
    double _t  = cur - ls;
    if (_t < 0.0001) _t = 0.0001; //hehe /0 exPLODES!
    //fps = (1000.0f / _t);
    if (_t < 1000/state.tfps) t_waitms (1000/state.tfps - _t), ls = now_ms(), _t = now_ms() - ls;
    if (frame % 60 == 0) {
      /*printf ("x: %lf, y: %lf\n", x, y);
      printf ("fx: %f, fy: %f\n", fx, fy);
      printf ("fx2: %f, fy2: %f\n", fx2, fy2);*/
      //printf ("FPS: %0.0lf\nCPU time: %0.03lfms\n", fps, cputime);
      //printf("FPS: %0.0lf\n", fps);
    }
    delta = _t;
    state.time += _t / 1000.0;
    state.delta = delta;
    //ls          = timenow();
    ls = now_ms();
  }

  glDeleteVertexArrays (1, &VAO);
  // ADD Delete buffers
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  // Free CPU pixel buffer
  if (state.pixels) {
      free(state.pixels);
      state.pixels = NULL;
  }

  // Free SSBO memory
  if (state.ssbo) {
      free(state.ssbo);
      state.ssbo = NULL; // Good practice
  }

  free ((void*)state.name);
  free ((void*)state.projectFileContent.data);
  free ((void*)state.projectDir);

  glfwDestroyWindow (state.win);
  glfwTerminate();

  return 0;
}

// CPU-side drawing function implementations

// Helper to set a pixel in the CPU buffer
void setPixel(SGstate *sgs, int x, int y, float r, float g, float b) {
    if (!sgs || !sgs->pixels || x < 0 || x >= sgs->width || y < 0 || y >= sgs->height) return; // Add checks for sgs and sgs->pixels
    // Calculate index based on sgs->width and assuming RGBA float buffer
    int index = (y * sgs->width + x) * 4;
    sgs->pixels[index + 0] = r;
    sgs->pixels[index + 1] = g;
    sgs->pixels[index + 2] = b;
    sgs->pixels[index + 3] = 1.0f; // Alpha (assuming opaque)
}

// Clear the CPU pixel buffer
void sgClearPixels(SGstate *sgs, float r, float g, float b) {
    if (!sgs || !sgs->pixels) return; // Add checks
    int numPixels = sgs->width * sgs->height;
    for (int i = 0; i < numPixels; ++i) {
        sgs->pixels[i * 4 + 0] = r;
        sgs->pixels[i * 4 + 1] = g;
        sgs->pixels[i * 4 + 2] = b;
        sgs->pixels[i * 4 + 3] = 1.0f; // Alpha
    }
}

// Ported drawing functions (simplified examples)
void sgDrawRectF_cpu (SGstate *sgs, float r, float g, float b, float x1, float y1, float x2, float y2) {
    int ix1 = (int)fmax(0, floorf(x1));
    int iy1 = (int)fmax(0, floorf(y1));
    int ix2 = (int)fmin(sgs->width - 1, floorf(x2));
    int iy2 = (int)fmin(sgs->height - 1, floorf(y2));

    for (int y = iy1; y <= iy2; ++y) {
        for (int x = ix1; x <= ix2; ++x) {
            setPixel(sgs, x, y, r, g, b);
        }
    }
}

void sgDrawRect_cpu (SGstate *sgs, float r, float g, float b, float x1, float y1, float x2, float y2) {
    int ix1 = (int)floorf(x1);
    int iy1 = (int)floorf(y1);
    int ix2 = (int)floorf(x2);
    int iy2 = (int)floorf(y2);

    // Draw horizontal lines
    for (int x = ix1; x <= ix2; ++x) {
        setPixel(sgs, x, iy1, r, g, b);
        setPixel(sgs, x, iy2, r, g, b);
    }
    // Draw vertical lines (avoid double-drawing corners)
    for (int y = iy1 + 1; y < iy2; ++y) {
        setPixel(sgs, ix1, y, r, g, b);
        setPixel(sgs, ix2, y, r, g, b);
    }
}

// Ported from GLSL sign function
float sign (float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) {
  return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
}

// Ported from GLSL PointInTriangle
char PointInTriangle (float ptx, float pty, float v1x, float v1y, float v2x, float v2y, float v3x, float v3y) {
  float d1, d2, d3;
  char  has_neg, has_pos;

  d1 = sign (ptx, pty, v1x, v1y, v2x, v2y);
  d2 = sign (ptx, pty, v2x, v2y, v3x, v3y);
  d3 = sign (ptx, pty, v3x, v3y, v1x, v1y);

  has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

  return !(has_neg && has_pos);
}

void sgDrawTriF_cpu (SGstate *sgs, float r, float g, float b, float x1, float y1, float x2, float y2, float x3, float y3) {
    // Find bounding box
    float minX = fminf(x1, fminf(x2, x3));
    float minY = fminf(y1, fminf(y2, y3));
    float maxX = fmaxf(x1, fmaxf(x2, x3));
    float maxY = fmaxf(y1, fmaxf(y2, y3));

    // Clip bounding box to screen
    int startX = (int)fmax(0, floorf(minX));
    int startY = (int)fmax(0, floorf(minY));
    int endX   = (int)fmin(sgs->width - 1, floorf(maxX));
    int endY   = (int)fmin(sgs->height - 1, floorf(maxY));

    // Iterate over pixels in bounding box
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            if (PointInTriangle((float)x + 0.5f, (float)y + 0.5f, x1, y1, x2, y2, x3, y3)) {
                setPixel(sgs, x, y, r, g, b);
            }
        }
    }
}

void sgDrawCircleF_cpu (SGstate *sgs, float r, float g, float b, float cx, float cy, float radius, float innerRadius) {
    float r2 = radius * radius;
    float ir2 = innerRadius * innerRadius;
    int startX = (int)fmax(0, floorf(cx - radius));
    int startY = (int)fmax(0, floorf(cy - radius));
    int endX   = (int)fmin(sgs->width - 1, floorf(cx + radius));
    int endY   = (int)fmin(sgs->height - 1, floorf(cy + radius));

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            float dx = (float)x + 0.5f - cx;
            float dy = (float)y + 0.5f - cy;
            float distSq = dx*dx + dy*dy;
            if (distSq <= r2 && distSq >= ir2) {
                setPixel(sgs, x, y, r, g, b);
            }
        }
    }
}

// Ported from GLSL withinb
char withinb(float x, float y, float z, float bias) {
  if (y < z) {
    return x >= y-bias && x <= z+bias;
  } else 
    return x >= z-bias && x <= y+bias;
}

// Ported from GLSL lineFun
float lineFun(float m, float x, float a, float b) {
  return m*(x - a) + b;
}

void sgDrawLine_cpu (SGstate *sgs, float r, float g, float b, float x1, float y1, float x2, float y2) {
    // Bresenham's line algorithm or similar would be more efficient
    // This is a direct port of the shader logic, which is less ideal for CPU
    float minX = fminf(x1, x2);
    float minY = fminf(y1, y2);
    float maxX = fmaxf(x1, x2);
    float maxY = fmaxf(y1, y2);

    int startX = (int)fmax(0, floorf(minX - 1.0f));
    int startY = (int)fmax(0, floorf(minY - 1.0f));
    int endX   = (int)fmin(sgs->width - 1, floorf(maxX + 1.0f));
    int endY   = (int)fmin(sgs->height - 1, floorf(maxY + 1.0f));

    if (fabsf(x2 - x1) < 0.1f) { // Vertical line
        startX = (int)fmax(0, floorf(x1 - 0.5f));
        endX   = (int)fmin(sgs->width - 1, floorf(x1 + 0.5f));
        for (int y = startY; y <= endY; ++y) {
             if (withinb((float)y + 0.5f, y1, y2, 0.5f)) {
                for (int x = startX; x <= endX; ++x) {
                    setPixel(sgs, x, y, r, g, b);
                }
            }
        }
    } else { // Non-vertical line
        float m = (y2 - y1) / (x2 - x1);
        for (int y = startY; y <= endY; ++y) {
            for (int x = startX; x <= endX; ++x) {
                float fx = (float)x + 0.5f;
                float fy = (float)y + 0.5f;
                float lineY = lineFun(m, fx, x1, y1);
                float bias = 0.5f * fmaxf(fabsf(m), 1.0f);
                if (withinb(fy, lineY, lineY, bias) && withinb(fx, x1, x2, 0.5f) && withinb(fy, y1, y2, 0.5f)) {
                    setPixel(sgs, x, y, r, g, b);
                }
            }
        }
    }
}

// Render the primitives from the SSBO onto the pixel buffer
void sgRenderCPUBuffer(SGstate *sgs) {
    if (!sgs || !sgs->ssbo) return; // Add checks

    // Clear buffer first (optional, depends if you clear per frame elsewhere)
    // sgClearPixels(sgs, 0.0f, 0.0f, 0.0f); // Clearing is now done in the main loop before this call

    // Iterate through primitives in the SSBO buffer
    for (int i = 0; i < sgs->ssbo->primc; ++i) {
        SGprimitive *p = &sgs->ssbo->primv[i]; // Read from ssbo->primv
        switch(p->type) {
            case SG_PRIMITIVE_RECT:
                // Use p3.x as the hollow flag, consistent with sgapi.c's dr function
                if (p->p3.x == 1.0f) { // Check the flag for outline vs filled
                  sgDrawRect_cpu(sgs, p->c[0], p->c[1], p->c[2], p->p1.x, p->p1.y, p->p2.x, p->p2.y);
                } else {
                    sgDrawRectF_cpu(sgs, p->c[0], p->c[1], p->c[2], p->p1.x, p->p1.y, p->p2.x, p->p2.y);
                }
                break;
            case SG_PRIMITIVE_CIRCLE:
                 // Assuming p2.x is radius, p2.y is inner radius (consistent with sgapi.c)
                sgDrawCircleF_cpu(sgs, p->c[0], p->c[1], p->c[2], p->p1.x, p->p1.y, p->p2.x, p->p2.y);
                break;
            case SG_PRIMITIVE_TRIANGLE:
                sgDrawTriF_cpu(sgs, p->c[0], p->c[1], p->c[2], p->p1.x, p->p1.y, p->p2.x, p->p2.y, p->p3.x, p->p3.y);
                break;
            case SG_PRIMITIVE_LINE:
                sgDrawLine_cpu(sgs, p->c[0], p->c[1], p->c[2], p->p1.x, p->p1.y, p->p2.x, p->p2.y);
                break;
            // Add cases for other primitive types if they exist (e.g., text)
        }
    }
    // Reset primitive count for next frame
    sgs->ssbo->primc = 0; // Reset count in ssbo
}