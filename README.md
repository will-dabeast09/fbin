# fbin

A utility that converts frames/images to a binary file, with a 256-color palette for each frame.

1. Run `fbin.py`  
3. Select the folder containing your frames/images  
4. Select the name and directory for the new file  
5. Select output size and dithering
   (It will say "not responding", just wait for it to finish).

## Output Format

The outputted file follows this structure:

| Content | Description | Size |
|---------|-------------|------|
| Palette | 256 colors | 512 bytes |
| Image | 8bp format | 76800 bytes |
| Palette | Next frame's palette | 512 bytes |
| Image | Next frame's image | 76800 bytes |
| ... | And so on | ... |

Each frame consists of its own palette followed by image data.
