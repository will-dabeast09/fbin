/*
    BGR1555 color-format
    - Stored as an uint16_t
    - 1 bit unused
    - 5 bits for blue  (bits 14-10)
    - 5 bits for green (bits 9-5)
    - 5 bits for red   (bits 4-0)
*/

/*
    8bpp image format
    - 256 possible colors (2^8)
    - indexes the BGR1555-format color palette
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <windows.h>
#include <math.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Make sure to include this header

#define CHANNELS 3
#define PALETTE_ENTRIES 256
#define TOTAL_POSSIBLE_BGR1555_COLORS 32768 // 2^15 colors (5 bits per R,G,B)

// Size constants
#define SIZE_LARGE_WIDTH 320
#define SIZE_LARGE_HEIGHT 240
#define SIZE_SMALL_WIDTH 160
#define SIZE_SMALL_HEIGHT 96

// Dithering mode constants
#define DITHER_NONE 0     // No dithering
#define DITHER_STANDARD 1 // Standard Floyd-Steinberg
#define DITHER_ENHANCED 2 // Enhanced quality dithering

// Structure to represent colors with RGB components
typedef struct {
    uint8_t r, g, b;
} RGB;

// Structure for color with frequency
typedef struct {
    uint16_t color;
    int count;
} ColorCount;

// Function prototypes
uint16_t fbin_RGB888ToBGR1555(uint8_t r, uint8_t g, uint8_t b);
void fbin_BGR1555ToRGB888(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b);
int fbin_BGR1555ColorDistance(uint16_t color1, uint16_t color2);
int processImage(const char* input_filename, FILE* output_file, int target_width, int target_height, int dither_mode);
int compare_strings(const void* a, const void* b);
void updateProgressBar(int current, int total);
int compare_color_count(const void* a, const void* b);
void generate_optimized_palette(uint16_t* image, int size, uint16_t* palette, int palette_size);
void apply_no_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height);
void floyd_steinberg_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height);
void enhanced_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height);

int main(int argc, char *argv[]) {
    // Check command prompt inputs
    if (argc < 5) {
        printf("Error: requires input folder, output filename, size option, and dithering option\n");
        printf("Usage: %s <input_folder> <output_filename> <size_option> <dither_option>\n", argv[0]);
        printf("Size options: 1 = 320x240, 2 = 160x96\n");
        printf("Dither options: 0 = None, 1 = Standard, 2 = Enhanced\n");
        return 1;
    }

    // Set the folder and output file names
    char *input_folder = argv[1];
    char *output_filename = argv[2];
    int size_option = atoi(argv[3]);
    int dither_option = atoi(argv[4]);
    
    // Validate dithering option
    if (dither_option < 0 || dither_option > 2) {
        printf("Invalid dithering option. Use 0 for None, 1 for Standard, or 2 for Enhanced\n");
        return 1;
    }
    
    // Display dithering mode
    printf("Dithering mode: ");
    switch (dither_option) {
        case DITHER_NONE:
            printf("None (direct color quantization)\n");
            break;
        case DITHER_STANDARD:
            printf("Standard (Floyd-Steinberg)\n");
            break;
        case DITHER_ENHANCED:
            printf("Enhanced (Sierra Lite)\n");
            break;
    }
    
    // Set target dimensions based on size option
    int target_width, target_height;
    if (size_option == 1) {
        target_width = SIZE_LARGE_WIDTH;
        target_height = SIZE_LARGE_HEIGHT;
        printf("Using image size: 320x240\n");
    } else if (size_option == 2) {
        target_width = SIZE_SMALL_WIDTH;
        target_height = SIZE_SMALL_HEIGHT;
        printf("Using image size: 160x96\n");
    } else {
        printf("Invalid size option. Use 1 for 320x240 or 2 for 160x96\n");
        return 1;
    }

    // Open output file for writing
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        printf("Failed to create output file: %s\n", output_filename);
        return 1;
    }

    // Get all files from the input folder
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    char filePath[MAX_PATH];
    
    // Prepare search pattern
    snprintf(searchPath, MAX_PATH, "%s\\*", input_folder);
    
    // Create array to store filenames
    char **filenames = NULL;
    int fileCount = 0;
    
    // Start finding files
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Failed to open directory: %s\n", input_folder);
        fclose(output_file);
        return 1;
    }
    
    do {
        // Skip directories and special entries
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        
        // Check if file has common image extension
        const char *ext = strrchr(findFileData.cFileName, '.');
        if (!ext) continue;
        
        if (stricmp(ext, ".jpg") == 0 || stricmp(ext, ".jpeg") == 0 || 
            stricmp(ext, ".png") == 0 || stricmp(ext, ".bmp") == 0) {
            
            // Add to our file list
            fileCount++;
            filenames = (char**)realloc(filenames, fileCount * sizeof(char*));
            if (!filenames) {
                printf("Memory allocation error\n");
                FindClose(hFind);
                fclose(output_file);
                return 1;
            }
            
            filenames[fileCount - 1] = _strdup(findFileData.cFileName);
        }
    } while (FindNextFile(hFind, &findFileData));
    
    FindClose(hFind);
    
    // Sort filenames alphabetically
    qsort(filenames, fileCount, sizeof(char*), compare_strings);
    
    // Process each file in alphabetical order
    printf("Found %d image files to process\n", fileCount);
    printf("Progress: ");
    
    // Initial progress bar
    updateProgressBar(0, fileCount);
    
    for (int i = 0; i < fileCount; i++) {
        snprintf(filePath, MAX_PATH, "%s\\%s", input_folder, filenames[i]);
        
        int result = processImage(filePath, output_file, target_width, target_height, dither_option);
        if (result != 0) {
            // Move to a new line before printing error
            printf("\nError processing image: %s\n", filenames[i]);
            printf("Progress: "); // Restart progress line
        }
        
        // Update progress
        updateProgressBar(i + 1, fileCount);
        
        free(filenames[i]);
    }
    
    // Complete progress bar and move to next line
    printf("\nSuccessfully processed all images\n");
    printf("Output saved to: %s\n", output_filename);
    
    free(filenames);
    fclose(output_file);
    
    return 0;
}

// String comparison function for qsort
int compare_strings(const void* a, const void* b) {
    return _stricmp(*(const char**)a, *(const char**)b);
}

// ColorCount comparison function for qsort
int compare_color_count(const void* a, const void* b) {
    const ColorCount* c1 = (const ColorCount*)a;
    const ColorCount* c2 = (const ColorCount*)b;
    return c2->count - c1->count; // Sort in descending order of frequency
}

// Update progress bar in place
void updateProgressBar(int current, int total) {
    const int barWidth = 50;
    float progress = (float)current / total;
    int pos = barWidth * progress;
    
    // Calculate percentage
    int percent = (int)(progress * 100);
    
    // Print progress bar
    printf("\r[");
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d%% (%d/%d)", percent, current, total);
    fflush(stdout); // Ensure output is displayed immediately
}

// Generate an optimized palette by using the most frequent colors
void generate_optimized_palette(uint16_t* image, int size, uint16_t* palette, int palette_size) {
    // Count color frequencies
    ColorCount color_counts[TOTAL_POSSIBLE_BGR1555_COLORS] = {0};
    int unique_colors = 0;
    
    // First pass: count color occurrences
    for (int i = 0; i < size; i++) {
        uint16_t color = image[i];
        
        // Find if color already exists in our count
        int found = 0;
        for (int j = 0; j < unique_colors; j++) {
            if (color_counts[j].color == color) {
                color_counts[j].count++;
                found = 1;
                break;
            }
        }
        
        // Add new color if not found
        if (!found && unique_colors < TOTAL_POSSIBLE_BGR1555_COLORS) {
            color_counts[unique_colors].color = color;
            color_counts[unique_colors].count = 1;
            unique_colors++;
        }
    }
    
    // Sort colors by frequency
    qsort(color_counts, unique_colors, sizeof(ColorCount), compare_color_count);
    
    // Take the most frequent colors for the palette
    if (unique_colors <= palette_size) {
        // If we have fewer unique colors than palette entries, use them all
        for (int i = 0; i < unique_colors; i++) {
            palette[i] = color_counts[i].color;
        }
        // Fill the rest with black (or any default color)
        for (int i = unique_colors; i < palette_size; i++) {
            palette[i] = 0;
        }
    } else {
        // Take the most frequent colors
        for (int i = 0; i < palette_size; i++) {
            palette[i] = color_counts[i].color;
        }
    }
}

// Convert BGR1555 color to RGB888 components
void fbin_BGR1555ToRGB888(uint16_t color, uint8_t* r, uint8_t* g, uint8_t* b) {
    // Remember: we've swapped red and blue positions
    *r = ((color >> 10) & 0x1F) << 3; // Red from bits 14-10
    *g = ((color >> 5) & 0x1F) << 3;  // Green from bits 9-5
    *b = ((color >> 0) & 0x1F) << 3;  // Blue from bits 4-0
    
    // Set the lowest 3 bits to the highest 3 bits to maximize value range
    *r |= (*r >> 5);
    *g |= (*g >> 5);
    *b |= (*b >> 5);
}

// Simple direct quantization with no dithering
void apply_no_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height) {
    // Convert palette to RGB array for faster lookups
    RGB palette_rgb[PALETTE_ENTRIES];
    for (int i = 0; i < PALETTE_ENTRIES; i++) {
        fbin_BGR1555ToRGB888(palette[i], &palette_rgb[i].r, &palette_rgb[i].g, &palette_rgb[i].b);
    }
    
    // For each pixel, find the closest color in the palette
    for (int i = 0; i < width * height; i++) {
        uint8_t r, g, b;
        fbin_BGR1555ToRGB888(image[i], &r, &g, &b);
        
        // Find closest color in palette
        uint8_t best_index = 0;
        int best_distance = INT_MAX;
        
        for (int j = 0; j < PALETTE_ENTRIES; j++) {
            RGB pal_color = palette_rgb[j];
            int dr = (int)r - (int)pal_color.r;
            int dg = (int)g - (int)pal_color.g;
            int db = (int)b - (int)pal_color.b;
            int distance = dr * dr + dg * dg + db * db;
            
            if (distance < best_distance) {
                best_distance = distance;
                best_index = j;
            }
        }
        
        // Assign the closest palette color index
        indexed_image[i] = best_index;
    }
}

// Apply Floyd-Steinberg dithering to reduce color banding
void floyd_steinberg_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height) {
    // Create a copy of the image to work with (as RGB values)
    RGB* rgb_image = (RGB*)malloc(width * height * sizeof(RGB));
    if (!rgb_image) return;
    
    // Convert BGR1555 colors to RGB for easier error diffusion
    for (int i = 0; i < width * height; i++) {
        fbin_BGR1555ToRGB888(image[i], &rgb_image[i].r, &rgb_image[i].g, &rgb_image[i].b);
    }
    
    // Convert palette to RGB array for faster lookups
    RGB palette_rgb[PALETTE_ENTRIES];
    for (int i = 0; i < PALETTE_ENTRIES; i++) {
        fbin_BGR1555ToRGB888(palette[i], &palette_rgb[i].r, &palette_rgb[i].g, &palette_rgb[i].b);
    }

    // Process each pixel and apply dithering
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            RGB old_pixel = rgb_image[idx];
            
            // Find closest color in palette
            uint8_t best_index = 0;
            int best_distance = INT_MAX;
            
            for (int i = 0; i < PALETTE_ENTRIES; i++) {
                RGB pal_color = palette_rgb[i];
                int dr = (int)old_pixel.r - (int)pal_color.r;
                int dg = (int)old_pixel.g - (int)pal_color.g;
                int db = (int)old_pixel.b - (int)pal_color.b;
                int distance = dr * dr + dg * dg + db * db;
                
                if (distance < best_distance) {
                    best_distance = distance;
                    best_index = i;
                }
            }
            
            // Store the best palette index for this pixel
            indexed_image[idx] = best_index;
            
            // Calculate quantization error
            RGB new_pixel = palette_rgb[best_index];
            int error_r = old_pixel.r - new_pixel.r;
            int error_g = old_pixel.g - new_pixel.g;
            int error_b = old_pixel.b - new_pixel.b;
            
            // Distribute error using Floyd-Steinberg algorithm
            // Neighbor pattern:
            //    X  7/16
            // 3/16 5/16 1/16
            
            // Right neighbor (x+1, y)
            if (x < width - 1) {
                int idx_right = idx + 1;
                rgb_image[idx_right].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].r + (error_r * 7) / 16));
                rgb_image[idx_right].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].g + (error_g * 7) / 16));
                rgb_image[idx_right].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].b + (error_b * 7) / 16));
            }
            
            if (y < height - 1) {
                // Bottom-left neighbor (x-1, y+1)
                if (x > 0) {
                    int idx_bottom_left = (y + 1) * width + (x - 1);
                    rgb_image[idx_bottom_left].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].r + (error_r * 3) / 16));
                    rgb_image[idx_bottom_left].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].g + (error_g * 3) / 16));
                    rgb_image[idx_bottom_left].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].b + (error_b * 3) / 16));
                }
                
                // Bottom neighbor (x, y+1)
                int idx_bottom = (y + 1) * width + x;
                rgb_image[idx_bottom].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].r + (error_r * 5) / 16));
                rgb_image[idx_bottom].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].g + (error_g * 5) / 16));
                rgb_image[idx_bottom].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].b + (error_b * 5) / 16));
                
                // Bottom-right neighbor (x+1, y+1)
                if (x < width - 1) {
                    int idx_bottom_right = (y + 1) * width + (x + 1);
                    rgb_image[idx_bottom_right].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].r + (error_r * 1) / 16));
                    rgb_image[idx_bottom_right].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].g + (error_g * 1) / 16));
                    rgb_image[idx_bottom_right].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].b + (error_b * 1) / 16));
                }
            }
        }
    }
    
    free(rgb_image);
}

// Enhanced dithering using Sierra Lite algorithm for better quality
void enhanced_dithering(uint16_t* image, uint8_t* indexed_image, uint16_t* palette, int width, int height) {
    // Create a copy of the image to work with (as RGB values)
    RGB* rgb_image = (RGB*)malloc(width * height * sizeof(RGB));
    if (!rgb_image) return;
    
    // Convert BGR1555 colors to RGB for easier error diffusion
    for (int i = 0; i < width * height; i++) {
        fbin_BGR1555ToRGB888(image[i], &rgb_image[i].r, &rgb_image[i].g, &rgb_image[i].b);
    }
    
    // Convert palette to RGB array for faster lookups
    RGB palette_rgb[PALETTE_ENTRIES];
    for (int i = 0; i < PALETTE_ENTRIES; i++) {
        fbin_BGR1555ToRGB888(palette[i], &palette_rgb[i].r, &palette_rgb[i].g, &palette_rgb[i].b);
    }

    // Process each pixel and apply dithering
    for (int y = 0; y < height; y++) {
        // Serpentine scanning (alternate left-to-right and right-to-left)
        // This reduces visual artifacts in the dithering pattern
        bool right_to_left = (y & 1); // Alternate direction for each row
        
        for (int x_iter = 0; x_iter < width; x_iter++) {
            // Calculate x based on direction
            int x = right_to_left ? width - 1 - x_iter : x_iter;
            
            int idx = y * width + x;
            RGB old_pixel = rgb_image[idx];
            
            // Find closest color in palette using weighted components
            // Human eyes are more sensitive to green, then red, then blue
            uint8_t best_index = 0;
            int best_distance = INT_MAX;
            
            for (int i = 0; i < PALETTE_ENTRIES; i++) {
                RGB pal_color = palette_rgb[i];
                // Weighted color distance (human perception)
                int dr = (int)old_pixel.r - (int)pal_color.r;
                int dg = (int)old_pixel.g - (int)pal_color.g;
                int db = (int)old_pixel.b - (int)pal_color.b;
                int distance = (dr * dr * 3) + (dg * dg * 6) + (db * db * 1); // Weighted by perceived brightness
                
                if (distance < best_distance) {
                    best_distance = distance;
                    best_index = i;
                }
            }
            
            // Store the best palette index for this pixel
            indexed_image[idx] = best_index;
            
            // Calculate quantization error
            RGB new_pixel = palette_rgb[best_index];
            int error_r = old_pixel.r - new_pixel.r;
            int error_g = old_pixel.g - new_pixel.g;
            int error_b = old_pixel.b - new_pixel.b;
            
            // Apply Sierra Lite dithering algorithm
            // Pattern:
            //     X  2/4
            // 1/4 1/4
            
            // Direction-aware error diffusion
            if (right_to_left) {
                // Right-to-left processing
                // Left neighbor (x-1, y) - in this case it's actually to the right
                if (x > 0) {
                    int idx_left = idx - 1;
                    rgb_image[idx_left].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_left].r + (error_r * 2) / 4));
                    rgb_image[idx_left].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_left].g + (error_g * 2) / 4));
                    rgb_image[idx_left].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_left].b + (error_b * 2) / 4));
                }
                
                if (y < height - 1) {
                    // Bottom-right neighbor (x+1, y+1)
                    if (x < width - 1) {
                        int idx_bottom_right = (y + 1) * width + (x + 1);
                        rgb_image[idx_bottom_right].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].r + (error_r * 1) / 4));
                        rgb_image[idx_bottom_right].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].g + (error_g * 1) / 4));
                        rgb_image[idx_bottom_right].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_right].b + (error_b * 1) / 4));
                    }
                    
                    // Bottom neighbor (x, y+1)
                    int idx_bottom = (y + 1) * width + x;
                    rgb_image[idx_bottom].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].r + (error_r * 1) / 4));
                    rgb_image[idx_bottom].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].g + (error_g * 1) / 4));
                    rgb_image[idx_bottom].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].b + (error_b * 1) / 4));
                }
            } else {
                // Left-to-right processing (original direction)
                // Right neighbor (x+1, y)
                if (x < width - 1) {
                    int idx_right = idx + 1;
                    rgb_image[idx_right].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].r + (error_r * 2) / 4));
                    rgb_image[idx_right].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].g + (error_g * 2) / 4));
                    rgb_image[idx_right].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_right].b + (error_b * 2) / 4));
                }
                
                if (y < height - 1) {
                    // Bottom-left neighbor (x-1, y+1)
                    if (x > 0) {
                        int idx_bottom_left = (y + 1) * width + (x - 1);
                        rgb_image[idx_bottom_left].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].r + (error_r * 1) / 4));
                        rgb_image[idx_bottom_left].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].g + (error_g * 1) / 4));
                        rgb_image[idx_bottom_left].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom_left].b + (error_b * 1) / 4));
                    }
                    
                    // Bottom neighbor (x, y+1)
                    int idx_bottom = (y + 1) * width + x;
                    rgb_image[idx_bottom].r = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].r + (error_r * 1) / 4));
                    rgb_image[idx_bottom].g = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].g + (error_g * 1) / 4));
                    rgb_image[idx_bottom].b = (uint8_t)fmin(255, fmax(0, rgb_image[idx_bottom].b + (error_b * 1) / 4));
                }
            }
        }
    }
    
    free(rgb_image);
}

// Process a single image and write palette + image data to the output file
int processImage(const char* input_filename, FILE* output_file, int target_width, int target_height, int dither_mode) {
    // Initialize the original pixel data
    int width, height, channels;
    unsigned char *pixels = stbi_load(input_filename, &width, &height, &channels, CHANNELS);
    if (!pixels) {
        printf("\nFailed to load image: %s\n", input_filename);
        return 1;
    }

    if ((width != target_width) || (height != target_height)) {
        printf("\nWrong image dimensions for %s, expected %dx%d, got %dx%d\n", 
               input_filename, target_width, target_height, width, height);
        stbi_image_free(pixels);
        return 1;
    }

    // Allocate memory for the BGR1555 image
    uint16_t *image_bgr1555 = (uint16_t*)malloc(target_width * target_height * sizeof(uint16_t));
    if (!image_bgr1555) {
        printf("\nMemory allocation error for image buffer\n");
        stbi_image_free(pixels);
        return 1;
    }
    
    // Convert image to BGR1555
    for (int i = 0; i < target_width * target_height; i++) {
        int index = i * CHANNELS;
        image_bgr1555[i] = fbin_RGB888ToBGR1555(pixels[index], pixels[index + 1], pixels[index + 2]);
    }

    // Create a quantized color palette with 256 colors using frequency-based approach
    uint16_t palette_quantized[PALETTE_ENTRIES];
    generate_optimized_palette(image_bgr1555, target_width * target_height, palette_quantized, PALETTE_ENTRIES);

    // Create a new 8-bit image
    uint8_t *image_8bpp = (uint8_t*)malloc(target_width * target_height * sizeof(uint8_t));
    if (!image_8bpp) {
        printf("\nMemory allocation error for 8bpp image\n");
        free(image_bgr1555);
        stbi_image_free(pixels);
        return 1;
    }
    
    // Apply the selected dithering method
    switch (dither_mode) {
        case DITHER_NONE:
            apply_no_dithering(image_bgr1555, image_8bpp, palette_quantized, target_width, target_height);
            break;
        case DITHER_STANDARD:
            floyd_steinberg_dithering(image_bgr1555, image_8bpp, palette_quantized, target_width, target_height);
            break;
        case DITHER_ENHANCED:
            enhanced_dithering(image_bgr1555, image_8bpp, palette_quantized, target_width, target_height);
            break;
        default:
            // Default to standard dithering if an invalid option is somehow provided
            floyd_steinberg_dithering(image_bgr1555, image_8bpp, palette_quantized, target_width, target_height);
    }

    // Write palette first
    fwrite(palette_quantized, sizeof(uint16_t), PALETTE_ENTRIES, output_file);
    
    // Write image data
    fwrite(image_8bpp, sizeof(uint8_t), target_width * target_height, output_file);
    
    // Cleanup
    free(image_bgr1555);
    free(image_8bpp);
    stbi_image_free(pixels);
    return 0;
}

uint16_t fbin_RGB888ToBGR1555(uint8_t r, uint8_t g, uint8_t b) {
    // Converts an RGB888 color to a BGR1555 color
    // Swapped red and blue as requested
    return   (((r >> 3) & 0x1F) << 10) // Extract top 5 bits of RED and shift to bits 14-10 (B position)
           | (((g >> 3) & 0x1F) << 5)  // Extract top 5 bits of green and shift to bits 9-5 (G position)
           | (((b >> 3) & 0x1F) << 0); // Extract top 5 bits of BLUE and keep in bits 4-0 (R position)
}

int fbin_BGR1555ColorDistance(uint16_t color1, uint16_t color2) {
    // Extract color components from BGR1555 format
    // Note: We've swapped the meaning of the positions, but the extraction remains the same
    int r1 = (color1 >> 10) & 0x1F; // RED is now in the blue position (bits 14-10)
    int g1 = (color1 >> 5) & 0x1F;  // GREEN stays in the middle (bits 9-5)
    int b1 = (color1 >> 0) & 0x1F;  // BLUE is now in the red position (bits 4-0)
    
    int r2 = (color2 >> 10) & 0x1F;
    int g2 = (color2 >> 5) & 0x1F;
    int b2 = (color2 >> 0) & 0x1F;
    
    // Calculate squared Euclidean distance
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;
    
    return dr * dr + dg * dg + db * db;
}