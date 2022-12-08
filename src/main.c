#pragma once
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MAX_NUM_INPUTS (255)

static void print_help_section()
{
    printf(
        "Usage: griddy [OPTION]...\n"
        "Available options:\n"
        "   -h\n"
        "   --help\n"
        "       Show this help section\n"
        "   -x <value>\n" 
        "       Number of output columns\n"
        "   -i <path>\n"
        "   --input <path>\n"
        "       Add an input image path\n"
        "   -o <path>\n"
        "   --output <path>\n"
        "       Select the output image path\n"
        "   -q <value>\n"
        "   --quality <value>\n"
        "       Select the output quality (1-100)\n"
    );
}

static void copy_image_to_buffer(const uint8_t *src, int src_width, int src_height, 
    uint8_t *dst, int dst_width, int dst_height, int x, int y)
{
    struct Color {
        uint8_t r, g, b;
    };

    struct Color *src_p = (struct Color *)src;
    struct Color *dst_p = (struct Color *)dst;

    const int dst_step_y = dst_width - src_width;
    dst_p += x + y * dst_width;

    for (int j = 0; j < src_height; ++j) {
        for (int i = 0; i < src_width; ++i) {
            *dst_p++ = *src_p++;
        }
        dst_p += dst_step_y;
    } 
}

int main(int argc, char **argv) {
    
    bool show_help = false;

    const char *output_path = "output.jpg";
    const char *input_paths[MAX_NUM_INPUTS];
    int output_cols = 1;
    int quality = 90;
    int num_inputs = 0;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            show_help = true;
        }
        else if (!strcmp(argv[i], "-x")) {
            output_cols = strtol(argv[++i], 0, 10);
        }
        else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input")) {
            input_paths[num_inputs++] = argv[++i];
        }
        else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            output_path = argv[++i];
        }
        else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quality")) {
            quality = strtol(argv[++i], 0, 10);
        }
    }

    if (show_help) {
        print_help_section();
        return 0;
    }

    if (num_inputs == 0) {
        printf("No input images provided!\n");
        return 1;
    }

    int tile_w = 0;
    int tile_h = 0;
    const int num_channels = 3;

    struct Image_Data {
        uint8_t *pixels;
        int w;
        int h;
    };

    struct Image_Data input_data[MAX_NUM_INPUTS];
    for (int i = 0; i < num_inputs; ++i) {
        int w, h, c;
        uint8_t *data = stbi_load(input_paths[i], &w, &h, &c, num_channels);

        if (data == 0) {
            printf("Unable to load image from path '%s'\n", input_paths[i]);
            continue;
        }

        // Find global image tile size
        if (w > tile_w) tile_w = w;
        if (h > tile_h) tile_h = h;

        input_data[i] = (struct Image_Data) {
            .pixels = data,
            .w = w,
            .h = h,
        };
    }

    // Limit output columns
    if (num_inputs < output_cols)
        output_cols = num_inputs;

    // Calculate number of rows needed
    int output_rows = num_inputs / output_cols;

    int output_w = tile_w * output_cols;
    int output_h = tile_h * output_rows;
    uint64_t output_size = output_w * output_h * num_channels * sizeof(uint8_t);
    uint8_t *output_data = malloc(output_size);
    memset(output_data, 0, output_size);

    for (int i = 0, x = 0, y = 0; i < num_inputs; ++i) {
        copy_image_to_buffer(input_data[i].pixels, input_data[i].w, input_data[i].h, output_data, output_w, output_h, x * tile_w, y * tile_h);
        printf("Copy image %i to x=%i, y=%i\n", i, x * tile_w, y * tile_h);
        if (++x >= output_cols) {
            x = 0;
            ++y;
        }
    }

    stbi_write_jpg(output_path, output_w, output_h, num_channels, output_data, quality);
    printf("Write output to '%s' (cols=%i, rows=%i, w=%i, h=%i)\n", output_path, output_cols, output_rows, output_w, output_h);

    for (int i = 0; i < num_inputs; ++i)
        stbi_image_free(input_data[i].pixels);

    free(output_data);
    return 0;
}