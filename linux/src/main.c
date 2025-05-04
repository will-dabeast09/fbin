/*
 *--------------------------------------
 * Program Name: FBin
 * Author: William "WillDaBeast555" Wierzbowski
 * License: GPL v3
 * Description: Scales and quantizes an input video
 *--------------------------------------
*/

#include "../include/libimagequant.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"
#include <omp.h>
#include <time.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 260
#define MEM_LIMIT_DENOM 4

typedef struct {
    unsigned char *indexed_pixels;
    liq_palette palette;
    int frame_number;
} ProcessedFrame;

int get_total_frames(const char *frames_folder, const char *frame_name);
void print_instructions(void);
void clear_console(void);
void hide_cursor(void);
void show_cursor(void);

int main(int argc, char *argv[]) {
    //Set the default options
    const char *input_filename = "input.mp4";
    const char *output_filename = "output.bin";
    const char *start_string = "00:00:00";
    const char *stop_string = NULL;
    const char *scale_str = "160:96"; 
    float framerate = 10.8;
    const char *quality_str = "0:100";
    float dither_level = 1.0;
    int min_brightness = 4;
    int num_colors = 256;

    //Other variables
    const char *frames_folder = "frames";
    const char *frame_name = "frame";
    int scale_x, scale_y;
    int qual_min, qual_max;

    {   //handle the input flags
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
            } else if ((strcmp(arg, "-p") == 0) || (strcmp(arg, "--palette") == 0)) {
                if (i + 1 < argc) {
                    num_colors = atoi(argv[i + 1]);
                    i++;
                } else {
                    printf("missing argument after %s\n", arg);
                    print_instructions();
                    return 1;
                }
            } else if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--version") == 0)) {
                printf("\nFBin Linux\n");
                printf("authored by WillDaBeast555\n\n");
                return 0;
            } else if ((strcmp(arg, "-h") == 0)  || (strcmp(arg, "--h") == 0) || (strcmp(arg, "-help") == 0) || (strcmp(arg, "--help") == 0)) {
                print_instructions();
                return 0;
            }
            else {
                print_instructions();
                return 1;
            }
        }
    }
    {   //parse the strings
        if (sscanf(scale_str, "%d:%d", &scale_x, &scale_y) != 2) {
            fprintf(stderr, "Error: Invalid scale format. Expected scale_x:scale_y\n");
            print_instructions();
            return 1;
        }

        if (sscanf(quality_str, "%d:%d", &qual_min, &qual_max) != 2) {
            fprintf(stderr, "Error: Invalid quality format. Expected min_quality:max_quality\n");
            print_instructions();
            return 1;
        }

        if ((qual_min < 0) || (qual_max > 100)) {
            printf("qual_min >= 0 and qual_max <= 100\n");
            return 0;
        } 
    }
    {   //convert the video into resized frames
        {//set up folder
            struct stat st = {0};
            if (stat(frames_folder, &st) == -1) {
                if (mkdir(frames_folder, 0700) == 0) {
                    printf("created folder '%s'\n", frames_folder);
                } else {
                    perror("failed to create folder");
                    return 1;
                }
            } else {
                printf("folder '%s' already exists\n", frames_folder);
            }
        }
        {//ffmpeg
            char ffmpeg_command[512];
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
                perror("ffmpeg command failed");
                fprintf(stderr, "; error code: %d\n", ffmpeg_err);
                return 1;
            }
        }
    }
    {   //quantize each frame into 256 colors each and write file
        FILE *file = fopen(output_filename, "wb");
        int total_frames = get_total_frames(frames_folder, frame_name);
        int batch_size;
        int processing_errors = 0;
        int current_frame = 0;
        double start_time, end_time;
        double elapsed_time;

        {   //initialize the file to write
            printf("initializing frame processing\n");
            if (file == NULL) {
                perror("error opening output file\n");
                return 1;
            }
            fclose(file); // Close file to wipe it
            printf("cleared '%s'\n", output_filename);

            file = fopen(output_filename, "ab");
            if (file == NULL) {
                perror("error opening output file for appending\n");
                return 1;
            }
            printf("opened '%s' for appending\n", output_filename);
        }  
        {   //initialize parallel processing / batch sizing
            //get the system information for batch sizing
            int num_processors = sysconf(_SC_NPROCESSORS_ONLN);

            //set the number of OpenMP threads
            omp_set_num_threads(num_processors);
            printf("using %d threads\n", omp_get_max_threads());

            //determine the batch size based on available memory
            struct sysinfo memInfo;
            sysinfo(&memInfo);
            unsigned long long totalPhysMem = memInfo.totalram;
            totalPhysMem *= memInfo.mem_unit;

            unsigned long long mem_limit = totalPhysMem / MEM_LIMIT_DENOM;

            size_t approx_frame_size = (scale_x * scale_y) + sizeof(liq_palette);
            batch_size = (int)(mem_limit / approx_frame_size);
            
            //ensure batch size is at least 10 frames
            batch_size = (batch_size < 10) ? 10 : batch_size;
            batch_size = (batch_size > total_frames) ? total_frames : batch_size;

            //print batch information
            printf("batch size set to %d\n", batch_size);
            printf("(using ~%zu MB of mem per batch)\n", (approx_frame_size * batch_size) / (1024 * 1024));
        }
        {   //disable cursor
            hide_cursor();
        }
        
        clear_console();
        
        start_time = omp_get_wtime();

        while (current_frame < total_frames) {//(main loop) process frames
            //allocate memory for current batch of processed frames
            int frames_in_batch = (current_frame + batch_size <= total_frames) ? batch_size : (total_frames - current_frame);
            printf("frames in batch: %d\n\n", frames_in_batch);

            ProcessedFrame *processed_frames = (ProcessedFrame *)malloc(frames_in_batch * sizeof(ProcessedFrame));
            if (!processed_frames) {
                printf("failed to allocate memory for processes frames");
                return 1;
            }

            //initialize the processed frames array for the current batch
            for (int i = 0; i < frames_in_batch; i++) {
                processed_frames[i].indexed_pixels = NULL;
                processed_frames[i].frame_number = current_frame + i + 1;
            }

            //atomic counter for completed frames
            int completed_frames = 0;

            //process frames in this batch in parallel
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < frames_in_batch; i++) {
                int frame_num = current_frame + i + 1;
                char filename[MAX_PATH_LENGTH];

                sprintf(filename, "%s/%s_%d.png", frames_folder, frame_name, frame_num);

                //load the image
                int width, height, channels;
                unsigned char *pixels = stbi_load(filename, &width, &height, &channels, 4);
                if (!pixels) {
                    fprintf(stderr, "failed to load image '%s'\n", filename);
                    processing_errors++;
                    continue;
                }

                //create attributes
                liq_attr *attr = liq_attr_create();
                liq_set_max_colors(attr, num_colors);
                liq_set_quality(attr, qual_min, qual_max);

                //create image
                liq_image *image = liq_image_create_rgba(attr, pixels, scale_x, scale_y, 0);

                //quantize!
                liq_result *result;
                if (liq_image_quantize(image, attr, &result) != LIQ_OK) {
                    #pragma omp critical
                    {
                        fprintf(stderr, "quantization failed for frame %d\n", frame_num);
                        processing_errors++;
                    }
                    free(pixels);
                    liq_image_destroy(image);
                    liq_attr_destroy(attr);
                    continue;
                }
                liq_set_dithering_level(result, dither_level);

                //remap pixels to palette
                processed_frames[i].indexed_pixels = malloc(scale_x * scale_y);
                if (!processed_frames[i].indexed_pixels) {
                    #pragma omp critical
                    {
                        fprintf(stderr, "memory allocation failed for indexed pixels in frame %d\n", frame_num);
                        processing_errors++;
                    }
                    free(pixels);
                    liq_result_destroy(result);
                    liq_image_destroy(image);
                    liq_attr_destroy(attr);
                    continue;
                }

                liq_write_remapped_image(result, image, processed_frames[i].indexed_pixels, scale_x * scale_y);

                //copy palette
                const liq_palette *result_palette = liq_get_palette(result);
                memcpy(&processed_frames[i].palette, result_palette, sizeof(liq_palette));

                //clean up
                liq_result_destroy(result);
                liq_image_destroy(image);
                liq_attr_destroy(attr);
                free(pixels);

                //update progress for each completed frame
                #pragma omp critical
                {
                    completed_frames++;
                    if ((completed_frames % 25 == 0) || (completed_frames == frames_in_batch)) {
                        float overall_progress = (float)(current_frame + completed_frames) / total_frames;
                        // Move cursor to beginning of line, clear the line, and print the progress
                        printf("\r\033[K");  // \r moves cursor to start of line, \033[K clears to end of line
                        printf("processing: %d/%d frames (%.1f%%)", 
                            current_frame + completed_frames, 
                            total_frames,
                            overall_progress * 100);
                        fflush(stdout);
                    }
                }
            }

            //write current batch of processed frames in order
            for (int i = 0; i < frames_in_batch; i++) {
                if (processed_frames[i].indexed_pixels) {
                    //write palette
                    for (int j = 0; j < num_colors; j++) {
                        liq_color rgba_color = processed_frames[i].palette.entries[j];

                        uint16_t rgb1555_color = 0;
                        rgb1555_color = (((rgba_color.r >> 3) & 0x1F) << 10) |
                                        (((rgba_color.g >> 3) & 0x1F) << 5)  |
                                        (((rgba_color.b >> 3) & 0x1F) << 0);
                        fwrite(&rgb1555_color, sizeof(rgb1555_color), 1, file);
                    }

                    //write indexed pixels
                    fwrite(processed_frames[i].indexed_pixels, 1, scale_x * scale_y, file);

                    //free memory
                    free(processed_frames[i].indexed_pixels);
                }
            }

            //free the processed frames array for this batch
            free(processed_frames);

            //move to next batch
            current_frame += frames_in_batch;
        }
        
        {   //get elapsed time
            end_time = omp_get_wtime();
            elapsed_time = end_time - start_time;
            printf("\nprocessed in %lf seconds\n", elapsed_time);
        }
        {   //prepare to terminate program
            fclose(file);
            show_cursor();
        }
    }
    
    return 0;
}

int get_total_frames(const char *frames_folder, const char *frame_name) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    char prefix[MAX_PATH_LENGTH];
    
    // Create the prefix to match
    snprintf(prefix, MAX_PATH_LENGTH, "%s_", frame_name);
    size_t prefix_len = strlen(prefix);
    
    if ((dir = opendir(frames_folder)) == NULL) {
        printf("no frames found or directory error\n");
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Check if the file starts with prefix and ends with ".png"
        if (strncmp(entry->d_name, prefix, prefix_len) == 0 && 
            strstr(entry->d_name, ".png") != NULL) {
            count++;
        }
    }
    
    closedir(dir);
    printf("total frames: %d\n", count);
    return count;
}

void print_instructions(void) {
    //prints instructions on how to use FBin
    printf("./fbin [-i|--input <input_file>] [-o|--output <output_file>] [-ss|--start <time>] [-to|--stop <time>] [-s|--scale <width:height>] [-r|--framerate <fps>] [-q|--quality <min:max>] [-b|--min-brightness <factor>] [-d|--dither <level>]\n");
    printf("  -i, --input <input_file>    : Input video file (default: input.mp4)\n");
    printf("  -o, --output <output_file>  : Output binary file (default: output.bin)\n");
    printf("  -ss, --start <time>         : Start time for processing (HH:MM:SS or seconds, default: 00:00:00)\n");
    printf("  -to, --stop <time>          : Stop time for processing (HH:MM:SS or seconds)\n");
    printf("  -s, --scale <width:height>  : Scale the video frames (default: 160:96)\n");
    printf("  -r, --framerate <fps>       : Framerate for frame extraction (default: 10.8)\n");
    printf("  -q, --quality <min:max>     : Quantization quality range (0-100, default: 0:100)\n");
    printf("  -b, --min-brightness <factor>: Minimum brightness factor (default: 4)\n");
    printf("  -d, --dither <level>        : Dithering level (0.0-1.0, default: 1.0)\n");
    printf("        0.0 = no dithering, 1.0 = full dithering\n");
    printf("  -p, --palette <num_colors>  : Max colors for each frame (default: 256)\n");
    printf("  -v, --version               : Show version information\n");
    printf("  -h, --help                  : Show this help message\n");
    printf("\nExample:\n");
    printf("./fbin -i input.mp4 -o output.bin -ss 00:00:05 -to 00:00:15 -s 160:96 -r 10.8 -q 0:100 -b 4 -d 0.75 -p 256\n");
    printf("Running without flags will use the default values.\n");
}

// Linux-specific helper functions to replace Windows console functions
void clear_console(void) {
    printf("\033[H\033[J"); // ANSI escape sequence to clear screen
}

void hide_cursor(void) {
    printf("\033[?25l"); // ANSI escape sequence to hide cursor
    fflush(stdout);
}

void show_cursor(void) {
    printf("\033[?25h"); // ANSI escape sequence to show cursor
    fflush(stdout);
}