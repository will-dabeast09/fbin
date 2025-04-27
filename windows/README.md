```fbin [-i|--input <input_file>] [-o|--output <output_file>] [-ss|--start <time>] [-to|--stop <time>] [-s|--scale <width:height>] [-r|--framerate <fps>] [-q|--quality <min:max>] [-b|--min-brightness <factor>] [-d|--dither <level>]
  -i, --input <input_file>    : Input video file (default: input.mp4)
  -o, --output <output_file>  : Output binary file (default: output.bin)
  -ss, --start <time>         : Start time for processing (HH:MM:SS or seconds, default: 00:00:00)
  -to, --stop <time>          : Stop time for processing (HH:MM:SS or seconds)
  -s, --scale <width:height>  : Scale the video frames (default: 160:96)
  -r, --framerate <fps>       : Framerate for frame extraction (default: 10.8)
  -q, --quality <min:max>     : Quantization quality range (0-100, default: 0:100)
  -b, --min-brightness <factor>: Minimum brightness factor (default: 4)
  -d, --dither <level>        : Dithering level (0.0-1.0, default: 1.0)
      0.0 = no dithering, 1.0 = full dithering
  -c, --clean                 : Deletes the frames folder
  -v, --version               : Show version information
  -h, --help                  : Show this help message

Example:
fbin -i input.mp4 -o output.bin -ss 00:00:05 -to 00:00:15 -s 160:96 -r 10.8 -q 0:100 -b 4 -d 0.75
Running without flags will use the default values.```


```Credits:
    2025 by William Wierzbowski (WillDaBeast555).

    This program utilizes the following libraries:
        libimagequant: Kornel Lesiński.
        stb: Sean Barrett.
```
