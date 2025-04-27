//gcc -o fbin main.c -I./include -L./lib -limagequant

#include <direct.h>
#include "include/libimagequant.h"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include for strcmp

#define NUM_COLORS 256

void print_instructions(void);

int main(int argc, char *argv[]) {
    const char *frames_folder = "frames";
    const char *frame_name = "frame";

    // Set the default options
    const char *input_filename = "input.mp4";
    const char *output_filename = "output.bin";
    const char *start_string = "00:00:00";
    const char *stop_string = NULL;
    const char *scale_str = "160:96";
    float framerate = 10.8;
    const char *quality_str = "0:100";
    float dither_level = 1.0;
    int min_brightness = 4;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            char *arg = argv[i];
            if ((strcmp(arg, "-i") == 0) || (strcmp(arg, "--input") == 0)) {
                if (i + 1 < argc) {
                    input_filename = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-o") == 0) || (strcmp(arg, "--output") == 0)) {
                if (i + 1 < argc) {
                    output_filename = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-ss") == 0) || (strcmp(arg, "--start") == 0)) {
                if (i + 1 < argc) {
                    start_string = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-to") == 0) || (strcmp(arg, "--stop") == 0)) {
                if (i + 1 < argc) {
                    stop_string = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-s") == 0) || (strcmp(arg, "--scale") == 0)) {
                if (i + 1 < argc) {
                    scale_str = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-r") == 0) || (strcmp(arg, "--framerate") == 0)) {
                if (i + 1 < argc) {
                    framerate = atof(argv[i + 1]);
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-b") == 0) || (strcmp(arg, "--quality") == 0)) {
                if (i + 1 < argc) {
                    min_brightness = atoi(argv[i + 1]);
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-q") == 0) || (strcmp(arg, "--min-brightness") == 0)) {
                if (i + 1 < argc) {
                    quality_str = argv[i + 1];
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-d") == 0) || (strcmp(arg, "--dither") == 0)) {
                if (i + 1 < argc) {
                    dither_level = atof(argv[i + 1]);
                    if ((dither_level < 0.0) || (dither_level > 1.0)) {
                        printf("dither: 0.0 - 1.0\n0.0 = no dithering\n1.0 = most dithering");
                        return 1;
                    }
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--version") == 0)) {
                printf("\nFBin Windows\n");
                printf("-- version v1.2.0\n");
                printf("authored by WillDaBeast555\n");
                return 0;
            } else if ((strcmp(arg, "-h") == 0)  || (strcmp(arg, "--h") == 0) || (strcmp(arg, "-help") == 0) || (strcmp(arg, "--help") == 0)) {
                print_instructions();
                return 0;
            } else if ((strcmp(arg, "-c") == 0) || (strcmp(arg, "--clean") == 0)) {
                char delete_command[256];
                snprintf(delete_command, sizeof(delete_command), "rd /s /q %s", frames_folder);
                int delete_result = system(delete_command);
                if (delete_result == 0) {
                    printf("successfully removed the '%s' folder (if it existed).\n", frames_folder);
                    return 0;
                }
                else if (delete_result != 0)
                {
                    printf("could not remove the '%s' folder.\n", frames_folder);
                    return 1;
                }
                
            }
            else {
                print_instructions();
                return 1;
            }
        }
    }

    // Parse the scale string
    int scale_x, scale_y;
    if (sscanf(scale_str, "%d:%d", &scale_x, &scale_y) != 2) {
        fprintf(stderr, "Error: Invalid scale format. Expected scale_x:scale_y\n");
        print_instructions();
        return 1;
    }

    // Parse the quality string (min:max)
    int qual_min, qual_max;
    if (sscanf(quality_str, "%d:%d", &qual_min, &qual_max) != 2) {
        fprintf(stderr, "Error: Invalid quality format. Expected min_quality:max_quality\n");
        print_instructions();
        return 1;
    }

    if ((qual_min < 0) || (qual_max > 100)) {
        printf("qual_min >= 0 and qual_max <= 100\n");
        return 0;
    }

    printf("Scale: %d:%d, Quality: %d:%d, Min Brightness: %d\n", scale_x, scale_y, qual_min, qual_max, min_brightness);

    {   //convert the video into frames
        //set up folder

        int dir_err = _mkdir(frames_folder);
        if (!dir_err) {
            printf("created folder '%s'\n", frames_folder);
        }
        else {
            printf("did not create folder '%s'\n", frames_folder);
        }


        //ffmpeg command


        //run the command
        char ffmpeg_command[256];
        if (stop_string != NULL) {
            snprintf(ffmpeg_command, sizeof(ffmpeg_command),
                "ffmpeg -i %s -ss %s -to %s -vf \"scale=%d:%d, lutrgb=r='if(gte(val,0.5),val,val*%d)':g='if(gte(val,0.5),val,val*%d)':b='if(gte(val,0.5),val,val*%d)'\" -r %1f %s/%s_%%d.png",
                input_filename, start_string, stop_string, scale_x, scale_y, min_brightness, min_brightness, min_brightness, framerate, frames_folder, frame_name);
        }
        else {
            snprintf(ffmpeg_command, sizeof(ffmpeg_command),
                "ffmpeg -i %s -ss %s -vf \"scale=%d:%d, lutrgb=r='if(gte(val,0.5),val,val*%d)':g='if(gte(val,0.5),val,val*%d)':b='if(gte(val,0.5),val,val*%d)'\" -r %1f %s/%s_%%d.png",
                input_filename, start_string, scale_x, scale_y, min_brightness, min_brightness, min_brightness, framerate, frames_folder, frame_name);
        }

        int ffmpeg_err = system(ffmpeg_command);
        if (!ffmpeg_err) {
            printf("ffmpeg command executed successfully\n");
        }
        else {
            printf("ffmpeg command failed; error code: %d\n", ffmpeg_err);
            return 1;
        }

    }
    {   //quantize each frame into 256 colors each
        FILE *file = fopen(output_filename, "wb");
        if (file == NULL) {
            printf("error opening output file\n");
            return 1;
        }
        fclose(file); // Close right away, just needed to reset it

        file = fopen(output_filename, "ab");
        if (file == NULL) {
            perror("error opening output file for appending\n");
            return 1;
        }

        int frame_count = 1;
        while (1) {
            char filename[256];
            sprintf(filename, "%s/%s_%d.png", frames_folder, frame_name, frame_count);
            printf("%s\n", filename);

            int width, height, channels;
            unsigned char *pixels = stbi_load(filename, &width, &height, &channels, 4);
            if (!pixels) {
                printf("no more images; exiting\n");
                break;
            }

            //create attributes
            liq_attr *attr = liq_attr_create();
            liq_set_max_colors(attr, NUM_COLORS);
            liq_set_quality(attr, qual_min, qual_max);


            //create image
            liq_image *image = liq_image_create_rgba(attr, pixels, scale_x, scale_y, 0);

            //quantize
            liq_result *result;
            if (liq_image_quantize(image, attr, &result) != LIQ_OK) {
                printf("quantization failed (try lowering the quality)\n");
                return 1;
            }

            liq_set_dithering_level(result, dither_level);

            //remap pixels to the palette
            unsigned char *indexed_pixels = malloc(scale_x * scale_y);
            liq_write_remapped_image(result, image, indexed_pixels, scale_x * scale_y);

            //create an output rgba buffer to write
            unsigned char *output_pixels = malloc(scale_x * scale_y * 4);

            const liq_palette *palette = liq_get_palette(result);
            for (int i = 0; i < NUM_COLORS; i++) {
                liq_color rgba_color = palette->entries[i];

                uint16_t rgb1555_color = 0;
                rgb1555_color = (((rgba_color.r >> 3) & 0x1F) << 10)    |
                                (((rgba_color.g >> 3) & 0x1F) << 5)     |
                                (((rgba_color.b >> 3) & 0x1F) << 0);


                fwrite(&rgb1555_color, sizeof(rgb1555_color), 1, file);
            }

            fwrite(indexed_pixels, 1, scale_x * scale_y, file);

            //clean up, clean up, everybody clean up
            liq_result_destroy(result);
            liq_image_destroy(image);
            liq_attr_destroy(attr);
            free(indexed_pixels);
            free(output_pixels);
            free(pixels);

            //update which frame is to be read
            frame_count++;
        }
        fclose(file);
    }

    return 0;
}

void print_instructions(void) {
    //prints instructions on how to use FBin
    printf("fbin [-i|--input <input_file>] [-o|--output <output_file>] [-ss|--start <time>] [-to|--stop <time>] [-s|--scale <width:height>] [-r|--framerate <fps>] [-q|--quality <min:max>] [-b|--min-brightness <factor>] [-d|--dither <level>]\n");
    printf("  -i, --input <input_file>    : Input video file (default: input.mp4)\n");
    printf("  -o, --output <output_file>  : Output binary file (default: output.bin)\n");
    printf("  -ss, --start <time>         : Start time for processing (HH:MM:SS or seconds, default: 00:00:00)\n");
    printf("  -to, --stop <time>          : Stop time for processing (HH:MM:SS or seconds)\n");
    printf("  -s, --scale <width:height>  : Scale the video frames (default: 160:96)\n");
    printf("  -r, --framerate <fps>       : Framerate for frame extraction (default: 10.8)\n");
    printf("  -q, --quality <min:max>     : Quantization quality range (0-100, default: 0:100)\n");
    printf("  -b, --min-brightness <factor>: Minimum brightness factor (default: 4)\n");
    printf("  -d, --dither <level>        : Dithering level (0.0-1.0, default: 1.0)\n");
    printf("      0.0 = no dithering, 1.0 = full dithering\n");
    printf("  -c, --clean                 : Deletes the frames folder\n");
    printf("  -v, --version               : Show version information\n");
    printf("  -h, --help                  : Show this help message\n");
    printf("\nExample:\n");
    printf("fbin -i input.mp4 -o output.bin -ss 00:00:05 -to 00:00:15 -s 160:96 -r 10.8 -q 0:100 -b 4 -d 0.75\n");
    printf("Running without flags will use the default values.\n");
}