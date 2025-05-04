# FBin Linux  
To use FBin on Linux distros, you can simply copy the executable, fbin, from linux/bin and place it into the same directory as your video.  
In the terminal, do something like:  
```bash  
cd path/to/input/folder/
./fbin  
```
You may get permission errors. Type:  
```bash
chmod +x fbin  
```  
Then run the executable again.  
For a full list of commands, type:  
```bash
./fbin --help  
```  
## Note
This has only been tested on Ubuntu. If FBin linux does not work on your distro or version, let me know and chances are I'll port it.  
Then you can temporarily use the Python file in v1.0.0. 
## Features
* Parallel processing  
* Customizable settings
* Support for usage beyond Cinema (whatever that may be) 
## Credits:
* (c) 2025 by William "WillDaBeast555" Wierzbowski  
    * FFmpeg: (c) ffmpeg.org (LGPL v2.1 or later)  
    * libimagequant: (c) Kornel Lesiński (GPLv3 or later)  
    * stb: (c) Sean Barrett (MIT / Public Domain)  
