import os
import sys
import tkinter as tk
from tkinter import filedialog
import numpy as np
from PIL import Image

def rgb_to_1555(r, g, b):
    """Convert RGB888 to TI-style RGB1555 (no alpha)."""
    # Swap r and b for BGR1555 format
    r, b = b, r
    r = (r >> 3) & 0x1F
    g = (g >> 3) & 0x1F
    b = (b >> 3) & 0x1F
    return (b << 10) | (g << 5) | r

def convert_single_image(img, image_name):
    """Convert a single image to TI-84 format and return the bytes."""
    # Resize the image
    img = img.convert('RGB')
    img = img.resize((320, 240), Image.Resampling.LANCZOS)
    
    # Quantize to 256 colors
    img_quantized = img.quantize(colors=256, method=2)
    palette = img_quantized.getpalette()
    
    # Create 1555 format palette
    palette_1555 = []
    for i in range(256):
        idx = i * 3
        if idx < len(palette):
            r, g, b = palette[idx], palette[idx+1], palette[idx+2]
        else:
            r, g, b = 0, 0, 0
        
        color_1555 = rgb_to_1555(r, g, b)
        palette_1555.append(color_1555)
    
    # Get image data
    img_data = np.array(img_quantized, dtype=np.uint8)
    
    # Sort palette by R values (which were swapped with B)
    palette_indices = list(range(256))
    palette_indices.sort(key=lambda i: palette_1555[i] & 0x1F)
    
    palette_remap = {old: new for new, old in enumerate(palette_indices)}
    remapped_data = np.vectorize(lambda x: palette_remap.get(x, 0))(img_data)
    
    # Create sorted palette
    sorted_palette = [palette_1555[i] for i in palette_indices]
    
    # Create bytes for palette
    palette_bytes = bytearray()
    for color in sorted_palette:
        palette_bytes.extend(color.to_bytes(2, byteorder='little'))
    
    # Create bytes for image data
    image_bytes = remapped_data.astype(np.uint8).tobytes()
    
    print(f"Processed: {image_name}")
    print(f"- Palette: 512 bytes (256 colors)")
    print(f"- Image: {len(image_bytes)} bytes (320×240 pixels, 8bpp)")
    
    return palette_bytes, image_bytes

def convert_folder_to_ti84_format(input_folder, output_path):
    # Find all image files in the folder
    image_extensions = ['.png', '.jpg', '.jpeg', '.bmp', '.gif']
    image_files = []
    
    for file in os.listdir(input_folder):
        if any(file.lower().endswith(ext) for ext in image_extensions):
            image_files.append(os.path.join(input_folder, file))
    
    if not image_files:
        print("No image files found in the selected folder.")
        return
    
    # Sort files alphabetically
    image_files.sort()
    
    # Process each image and combine into one file
    with open(output_path, 'wb') as outfile:
        for i, image_path in enumerate(image_files):
            try:
                img = Image.open(image_path)
                palette_bytes, image_bytes = convert_single_image(img, os.path.basename(image_path))
                
                # Write palette followed by image data
                outfile.write(palette_bytes)
                outfile.write(image_bytes)
                
                print(f"Added image {i+1}/{len(image_files)}: {os.path.basename(image_path)}")
            except Exception as e:
                print(f"Error processing {os.path.basename(image_path)}: {e}")
    
    print(f"\nConversion complete! All images combined into {output_path}")
    print(f"File size: {os.path.getsize(output_path)} bytes")
    print(f"Number of images: {len(image_files)}")

def select_folder_and_convert():
    root = tk.Tk()
    root.withdraw()
    
    input_folder = filedialog.askdirectory(
        title="Select folder containing images"
    )
    
    if not input_folder:
        print("No folder selected. Exiting.")
        return
    
    output_path = filedialog.asksaveasfilename(
        title="Save combined image file as",
        defaultextension=".bin",
        filetypes=[("Binary files", "*.bin")]
    )
    
    if not output_path:
        print("No output location selected. Exiting.")
        return
    
    convert_folder_to_ti84_format(input_folder, output_path)

if __name__ == "__main__":
    select_folder_and_convert()