#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  unsigned char r, g, b, a;
} pixel_t;

typedef struct {
  int width;
  int height;
  pixel_t *pixels;
} image_t;

image_t *read_png(const char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Error: Could not open file %s\n", filename);
    return NULL;
  }

  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    fprintf(stderr, "Error: Could not create PNG read struct\n");
    return NULL;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(fp);
    fprintf(stderr, "Error: Could not create PNG info struct\n");
    return NULL;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    fprintf(stderr, "Error: PNG error during read\n");
    return NULL;
  }

  png_init_io(png, fp);
  png_read_info(png, info);

  int width = png_get_image_width(png, info);
  int height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth = png_get_bit_depth(png, info);

  // Convert to RGBA if needed
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
  }
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png);
  }
  if (bit_depth == 16) {
    png_set_strip_16(png);
  }

  png_read_update_info(png, info);

  // Allocate memory for image
  image_t *img = malloc(sizeof(image_t));
  img->width = width;
  img->height = height;
  img->pixels = malloc(width * height * sizeof(pixel_t));

  // Read image data
  png_bytep *row_pointers = malloc(height * sizeof(png_bytep));
  for (int y = 0; y < height; y++) {
    row_pointers[y] = malloc(png_get_rowbytes(png, info));
  }

  png_read_image(png, row_pointers);

  // Convert to pixel_t format
  for (int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for (int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      int idx = y * width + x;
      img->pixels[idx].r = px[0];
      img->pixels[idx].g = px[1];
      img->pixels[idx].b = px[2];
      img->pixels[idx].a = px[3];
    }
    free(row_pointers[y]);
  }

  free(row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);

  return img;
}

void free_image(image_t *img) {
  if (img) {
    free(img->pixels);
    free(img);
  }
}

void write_c_header(const char *output_filename, const char *varname,
                    image_t *img) {
  FILE *fp = fopen(output_filename, "w");
  if (!fp) {
    fprintf(stderr, "Error: Could not open output file %s\n", output_filename);
    return;
  }

  // Write header
  fprintf(fp, "// Auto-generated from PNG image\n");
  fprintf(fp, "#ifndef _%s_H\n", varname);
  fprintf(fp, "#define _%s_H\n\n", varname);

  // Write image dimensions
  fprintf(fp, "#define %s_WIDTH %d\n", varname, img->width);
  fprintf(fp, "#define %s_HEIGHT %d\n", varname, img->height);

  // Write pixel data as array
  fprintf(fp, "\nstatic const unsigned int %s_DATA[] = {\n", varname);

  for (int y = 0; y < img->height; y++) {
    fprintf(fp, "    ");
    for (int x = 0; x < img->width; x++) {
      int idx = y * img->width + x;
      pixel_t *p = &img->pixels[idx];

      // Convert RGBA to ARGB (for your framebuffer format)
      unsigned int color = (p->a << 24) | // Alpha
                           (p->r << 16) | // Red
                           (p->g << 8) |  // Green
                           (p->b);        // Blue

      fprintf(fp, "0x%08X", color);

      if (!(y == img->height - 1 && x == img->width - 1)) {
        fprintf(fp, ", ");
      }

      // Break line after 4 pixels
      if ((x + 1) % 4 == 0 && x != img->width - 1) {
        fprintf(fp, "\n    ");
      }
    }
    if (y != img->height - 1) {
      fprintf(fp, ",\n");
    }
  }

  fprintf(fp, "\n};\n\n");
  fprintf(fp, "#endif // _%s_H\n", varname);

  fclose(fp);
  printf("Generated %s with %d x %d image data\n", output_filename, img->width,
         img->height);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <input.png> <output.h> <varname>\n", argv[0]);
    fprintf(stderr, "Example: %s icon.png icon.h ICON\n", argv[0]);
    return 1;
  }

  const char *input_file = argv[1];
  const char *output_file = argv[2];
  const char *varname = argv[3];

  printf("Converting %s to %s (variable: %s)\n", input_file, output_file,
         varname);

  image_t *img = read_png(input_file);
  if (!img) {
    fprintf(stderr, "Error reading PNG file\n");
    return 1;
  }

  printf("Image: %d x %d pixels\n", img->width, img->height);

  write_c_header(output_file, varname, img);

  free_image(img);
  return 0;
}
