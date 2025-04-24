# fbin

A utility that converts frames/images to a binary file, with a 256-color palette for each frame.

## Instructions
***If you're on Windows, you can follow the instructions in fbin/windows***
1. Run `main.py`
2. Select the folder containing your frames/images
3. Select the name and directory for the new file

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
