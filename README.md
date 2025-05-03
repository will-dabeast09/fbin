# FBin  
FBin scales and does high quality quantization on a video and saves it in this specific binary format:  
|       output.bin       
|------------------------
|palette (2 * num_colors)
|image (8bpp)
|palette
|image
|etc...

**Temporily, and non-Windows users and those using Gallery will need to use the python (it will say not responding, just wait)**  
**In the future, FBin will be only for Cinema, and IBin will be for Gallery, and FBin will be ported for Linux users**

## Command Line Usage
```
fbin [options]

  -i, --input <input_file>    : Input video file (default: input.mp4)  
  -o, --output <output_file>  : Output binary file (default: output.bin)  
  -ss, --start <time>         : Start time for processing (HH:MM:SS or seconds, default: 00:00:00)  
  -to, --stop <time>          : Stop time for processing (HH:MM:SS or seconds)  
  -s, --scale <width:height>  : Scale the video frames (default: 160:96)  
  -r, --framerate <fps>       : Framerate for frame extraction (default: 10.8)  
  -q, --quality <min:max>     : Quantization quality range (0-100, default: 0:100)  
  -b, --min-brightness <factor>: Minimum brightness factor (default: 4)  
  -d, --dither <level>        : Dithering level (0.0-1.0, default: 1.0)  0.0 = no dithering, 1.0 = full dithering  
  -p, --palette <num_colors>  : Max colors for each frame (default: 256)  
  -v, --version               : Show version information  
  -h, --help                  : Show this help message  

Example:
fbin -i input.mp4 -o output.bin -ss 00:00:05 -to 00:00:15 -s 160:96 -r 10.8 -q 0:100 -b 4 -d 0.75 -p 256
Running without flags will use the default values.
```

## Notes about FBin
* For [cinema](https://github.com/will-dabeast09/cinema), my video player for the TI-84 Plus CE, never change num_colors, as it expects a specific format  
* If you find any issues, or have any suggestions, please make them to improve FBin

## Credits:

* (c) 2025 by William "WillDaBeast555" Wierzbowski
  * FFmpeg:        (c) [ffmpeg.org](https://ffmpeg.org/) (LGPL v2.1 or later)
  * libimagequant: (c) Kornel Lesiński (GPLv3 or later)
  * stb:           (c) Sean Barrett (MIT / Public Domain)
