#include "sgimage.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void sgSetImagePixel (SGimage *img, int x, int y, SGcolor c) {
  if (x > img->width - 1 || y > img->height - 1 || x < 0 || y < 0)
    return;
  long const base = (x + y * img->width) * img->channels;
  if (base + 3 >= img->width * img->height * img->channels)
    return;
  switch (img->channels) {
  case 4:
    img->data[base + 3] = c.a;
  case 3:
    img->data[base + 2] = c.b;
  case 2:
    img->data[base + 1] = c.g;
  case 1:
    img->data[base] = c.r;
    break;
  }
}

SGcolor sgGetImagePixel (SGimage *img, int x, int y) {
  return (SGcolor){0};
}

SGimage sgGenImage (int width, int height, int channels) {
  SGimage img = {0};
  img.data    = malloc (sizeof (uint8_t) * width * height * channels);
  memset (img.data, 0, sizeof (uint8_t) * width * height * channels);
  img.width    = width;
  img.height   = height;
  img.channels = channels;
  return img;
}

/*
SGimage sgGenImageFromFile(const char *path) {
  SGimage img = {0};
  if (!io_exists(path)) {
    printf("Path \"%s\" does not exist\n", path);
    return (SGimage){0};
  }
  img.data = stbi_load(io_fixhome(path), &img.width, &img.height, &img.channels,
0); if (!img.data) { printf("Failed to load texture: %s\n",
stbi_failure_reason()); return (SGimage){0};
  }
  img.type = 1;
  return img;
}*/
int sgGLImageTextureType (SGimage *img) {
  int type = GL_RGBA;
  switch (img->channels) {
  case 3:
    type = GL_RGB;
    break;
  case 2:
    type = GL_RG;
    break;
  case 1:
    // type = GL_R; // Use GL_RED for Core Profile
    type = GL_RED;
    break;
  }
  return type;
}

void sgGenImageTexture (SGimage *img, int mip, int wrap_s, int wrap_t,
                        int min_filter, int max_filter) {
  if (img->channels > 4 || img->channels < 1) {
    printf (
        "Invalid channel count of %i. Must be an integer between and including "
        "1 and 4\n",
        img->channels);
    return;
  }
  // glGenTextures (1, &img->tex); // Warning: passing 'int *' to parameter of type 'GLuint *'
  glGenTextures (1, (GLuint*)&img->tex); // Cast to fix warning
  glBindTexture (GL_TEXTURE_2D, img->tex);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filter);

  int type = sgGLImageTextureType (img);

  glTexImage2D (GL_TEXTURE_2D, mip, type, img->width, img->height, 0, type,
                GL_UNSIGNED_BYTE, img->data);
  glGenerateMipmap (GL_TEXTURE_2D);

  glBindTexture (GL_TEXTURE_2D, 0);
}

void sgBindImageTexture (SGimage *img) {
  glBindTexture (GL_TEXTURE_2D, img->tex);
}

void sgUpdateImageTexture (SGimage *img, int mip, int x, int y) {
  sgBindImageTexture (img);
  glTexSubImage2D (GL_TEXTURE_2D, mip, x, y, img->width, img->height,
                   sgGLImageTextureType (img), GL_UNSIGNED_BYTE, img->data);
  glGenerateMipmap (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, 0);
}

void sgClearImage (SGimage *img) {
  memset (img->data, 0, img->width * img->height * img->channels);
}

void sgFreeImage (SGimage *img) {
  if (img->data)
    free (img->data);
  img->data = NULL;
}

void sgFreeImageTexture (SGimage *img) {
  // glDeleteTextures (1, &img->tex); // Warning: passing 'int *' to parameter of type 'const GLuint *'
  glDeleteTextures (1, (const GLuint*)&img->tex); // Cast to fix warning
  img->tex = 0;
}

int sgGenTextures2D (int min_filter, int mag_filter, int wrap_s, int wrap_t,
                     int format, int w, int h, int n, SGtexture *texturev) {
  // uint32_t texv[n]; // Use GLuint array directly
  GLuint tex_ids[n]; // Array to hold generated texture IDs
  // glCreateTextures (GL_TEXTURE_2D, n, texv); // DSA function
  glGenTextures(n, tex_ids); // Non-DSA equivalent

  for (int i = 0; i < n; ++i) {
    glBindTexture(GL_TEXTURE_2D, tex_ids[i]); // Bind the current texture

    // glTextureParameteri (texv[i], GL_TEXTURE_MIN_FILTER, min_filter); // DSA
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter); // Non-DSA
    // glTextureParameteri (texv[i], GL_TEXTURE_MAG_FILTER, mag_filter); // DSA
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter); // Non-DSA
    // glTextureParameteri (texv[i], GL_TEXTURE_WRAP_S, wrap_s); // DSA
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s); // Non-DSA
    // glTextureParameteri (texv[i], GL_TEXTURE_WRAP_T, wrap_t); // DSA
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t); // Non-DSA

    // glTextureStorage2D (texv[i], 1, format, w, h); // DSA
    // Use glTexStorage2D (requires GL 4.2) or glTexImage2D (compatible)
    // Assuming format is a sized internal format like GL_RGBA32F
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_FLOAT, NULL); // Example, adjust format/type if needed
    // Or if glTexStorage2D is available in your target GL version (4.1 doesn't guarantee it, 4.2+ does)
    // glTexStorage2D(GL_TEXTURE_2D, 1, format, w, h); // Non-DSA

    // texturev[i].tex      = texv[i];
    texturev[i].tex      = tex_ids[i]; // Store the generated ID
    texturev[i].w        = w;
    texturev[i].h        = h;
    texturev[i].format   = format;
    texturev[i].min_filt = min_filter;
    texturev[i].mag_filt = mag_filter;
    texturev[i].wrap_s   = wrap_s;
    texturev[i].wrap_t   = wrap_t;
  }
  glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
  return 1;
}

void sgBindTexture (SGtexture tex, int rw) {
  // glBindImageTexture (0, tex.tex, 0, GL_FALSE, 0, rw, tex.format); // DSA / Image Load/Store - Comment out
  // TODO: Implement using glActiveTexture/glBindTexture if needed for rendering
  (void)tex; // Suppress unused parameter warning for now
  (void)rw;  // Suppress unused parameter warning for now
}

void sgUnbindTexture (void) {
  // glBindImageTexture (0, 0, 0, 0, 0, GL_READ_ONLY, 0); // DSA / Image Load/Store - Comment out
  // TODO: Implement using glActiveTexture/glBindTexture if needed
}

void sgRegenerateTexture (SGtexture *tex) {
  // glBindImageTexture (0, 0, 0, 0, 0, GL_READ_ONLY, 0); // DSA / Image Load/Store - Comment out

  sgFreeTextures (tex, 1); // This now uses glDeleteTextures correctly

  // glCreateTextures (GL_TEXTURE_2D, 1, &tex->tex); // DSA
  glGenTextures(1, &tex->tex); // Non-DSA
  glBindTexture(GL_TEXTURE_2D, tex->tex); // Bind the new texture

  // glTextureParameteri (tex->tex, GL_TEXTURE_MIN_FILTER, tex->min_filt); // DSA
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->min_filt); // Non-DSA
  // glTextureParameteri (tex->tex, GL_TEXTURE_MAG_FILTER, tex->mag_filt); // DSA
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->mag_filt); // Non-DSA
  // glTextureParameteri (tex->tex, GL_TEXTURE_WRAP_S, tex->wrap_s); // DSA
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrap_s); // Non-DSA
  // glTextureParameteri (tex->tex, GL_TEXTURE_WRAP_T, tex->wrap_t); // DSA
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrap_t); // Non-DSA

  // glTextureStorage2D (tex->tex, 1, tex->format, tex->w, tex->h); // DSA
  // Use glTexStorage2D (requires GL 4.2) or glTexImage2D (compatible)
  glTexImage2D(GL_TEXTURE_2D, 0, tex->format, tex->w, tex->h, 0, GL_RGBA, GL_FLOAT, NULL); // Example, adjust format/type if needed
  // Or if glTexStorage2D is available:
  // glTexStorage2D(GL_TEXTURE_2D, 1, tex->format, tex->w, tex->h); // Non-DSA

  glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
}

void sgFreeTextures (SGtexture *texv, int texc) {
  int i;
  // Need an array of GLuints to pass to glDeleteTextures
  GLuint *tex_ids = malloc(sizeof(GLuint) * texc);
  if (!tex_ids) return; // Handle allocation failure

  for (i = 0; i < texc; ++i) {
    // glDeleteTextures (1, &texv->tex); // Incorrect loop logic and type
    tex_ids[i] = texv[i].tex; // Collect texture IDs
    texv[i].tex = 0; // Clear the ID in the struct
  }
  glDeleteTextures(texc, tex_ids); // Delete all textures at once
  free(tex_ids); // Free the temporary array
}
