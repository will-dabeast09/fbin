# Linux FBin Instructions

FBin is a command line tool that converts frames/images to a binary file for Cinema and Gallery, my TI-84 Plus CE programs.  
Palettte colors are in BGR1555 format. Images are in 8bpp format, referencing the palette.  
Every frame/image can therefore have up to 256 (2^8) possible colors.   

## Project Dependencies  

* Have FFmpeg installed
* Have HDD Raw Copy Tool or a similar program installed  
* If you're using Gallery or Cinema, ensure you have CLibs installed on your TI-84 Plus CE
    * _You may need the nightly version_

## FBin  

Here is the expected format:  
```fbin <input_folder> <output_filename> <size option> <dither option>```  

```<input_folder>```  
The folder containing the frames/images to convert  

```<output_filename>```
The name of the outputted file.  
Recommended to end in ```*.bin```

```<size option>```  
1 = 320 x 240 (Gallery)  
2 = 160 x 96 (Cinema)  

```<dither option>```  
0 = None (Quickest)  
1 = Standard (Floyd Steinberg)  
2 = Enhanced  (Sierra Lite) + Most accurate color quantization  

###### Example Usage  
```./fbin images images.bin 1 2``` (Gallery)  
```./fbin frames video.bin 2 2``` (Cinema)  

## Instructions

Both Cinema and Gallery don't use a file system; they use expect an unformatted drive

#### For GALLERY

1. Put all of your images into one folder (If you want it in a specific order, note FBin goes alphabetically)  
2. Run FBin. For example, ```./fbin images images.bin 1 2```  
3. Using HDD Raw Copy Tool, set the SOURCE to your output file, the TARGET to your thumbdrive, and then press START  
4. Now you can run GALLERY with the USB connected  

#### For CINEMA  

1. In the video's directory ```ffmpeg -i input.mp4 -ss 0 -to 00:10:00 -vf scale=160:96 -r 10.75 frames/frame_%04d.png```
    * You of course can change the command to your needs. Anywhere from 10 - 12 fps will look good  
2. Run FBin. For example, ```fbin frames video.bin 2 2```  
3. Using HDD Raw Copy Tool, set the SOURCE to your output file, the TARGET to your thumbdrive, and then press START  
4. Now you can run CINEMA with the USB connected

### Author  
_WillDaBeast555_ ```Discord: WillDaBeast555 willdabeast555``` ```(WillDaBeast555#7692)```
