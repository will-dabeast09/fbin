# FBin - video conversion tool
FBin is a command line tool that converts an input video into a binary file that is used by the TI-84 Plus CE.  
The images use BGR1555 format, and each frame has a 256 color palette.  

*note: this has been tested on Ubuntu. If you have any issues or recommendations, please let me know.*  
# **Installation Instructions**

  Below are instructions for installing FBin on Linux based OSes.  

---

## Install cargo via rust to build the imagequant lib
```
curl https://sh.rustup.rs -sSf | sh  
source $HOME/.cargo/env  
cargo --version  
```

## Create the imagequant library (in case the included lib does not work for you)**  
```
git clone https://github.com/ImageOptim/libimagequant.git    
cd /imagequant/imagequant-sys  
make  
```
---    

## Build FBin  
```
gcc -o fbin main.c -I./include -L./lib -limagequant_sys -lm  
chmod +x ./fbin  
./fbin --help 
```  
# How To Use

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
    Running without flags will use the default values.


# Credits
  2025 by William Wierzbowski (WillDaBeast555).

  ****This program utilizes the following libraries****:  
  [FFmpeg](https://ffmpeg.org/)  
  [libimagequant](https://github.com/ImageOptim/libimagequant): Kornel Lesiński  
  [stb](https://github.com/nothings/stb/tree/master): Sean Barrett  
