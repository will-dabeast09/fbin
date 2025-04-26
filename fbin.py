import os
import sys
import tkinter as tk
from tkinter import filedialog, ttk
import numpy as np
from PIL import Image
import threading
import queue
import time

def rgb_to_1555(r, g, b):
    """Convert RGB888 to TI-style RGB1555 (no alpha)."""
    # Swap r and b for BGR1555 format
    r, b = b, r
    r = (r >> 3) & 0x1F
    g = (g >> 3) & 0x1F
    b = (b >> 3) & 0x1F
    return (b << 10) | (g << 5) | r

def convert_single_image(img, image_name, output_size, dither_method, log_queue=None):
    """Convert a single image to TI-84 format and return the bytes."""
    def log(message):
        if log_queue:
            log_queue.put(message)
        else:
            print(message)
    
    # Resize the image to the selected output size
    img = img.convert('RGB')
    img = img.resize(output_size, Image.Resampling.LANCZOS)
    
    # Quantize to 256 colors with dithering
    if dither_method == "None":
        dither = Image.Dither.NONE
    elif dither_method == "Floyd-Steinberg":
        dither = Image.Dither.FLOYDSTEINBERG
    else:  # Ordered
        dither = Image.Dither.ORDERED
    
    img_quantized = img.quantize(colors=256, method=2, dither=dither)
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
    
    log(f"Processed: {image_name}")
    log(f"- Palette: 512 bytes (256 colors)")
    log(f"- Image: {len(image_bytes)} bytes ({output_size[0]}×{output_size[1]} pixels, 8bpp)")
    log(f"- Dithering: {dither_method}")
    
    return palette_bytes, image_bytes

def convert_folder_to_ti84_format(input_folder, output_path, output_size, dither_method, log_queue=None, status_queue=None):
    def log(message):
        if log_queue:
            log_queue.put(message)
        else:
            print(message)
            
    def update_status(message):
        if status_queue:
            status_queue.put(message)
    
    # Find all image files in the folder
    image_extensions = ['.png', '.jpg', '.jpeg', '.bmp', '.gif']
    image_files = []
    
    for file in os.listdir(input_folder):
        if any(file.lower().endswith(ext) for ext in image_extensions):
            image_files.append(os.path.join(input_folder, file))
    
    if not image_files:
        log("No image files found in the selected folder.")
        update_status("No image files found")
        return
    
    # Sort files alphabetically
    image_files.sort()
    
    # Process each image and combine into one file
    try:
        with open(output_path, 'wb') as outfile:
            for i, image_path in enumerate(image_files):
                try:
                    update_status(f"Processing {i+1}/{len(image_files)}: {os.path.basename(image_path)}")
                    img = Image.open(image_path)
                    palette_bytes, image_bytes = convert_single_image(
                        img, 
                        os.path.basename(image_path),
                        output_size,
                        dither_method,
                        log_queue
                    )
                    
                    # Write palette followed by image data
                    outfile.write(palette_bytes)
                    outfile.write(image_bytes)
                    
                    log(f"Added image {i+1}/{len(image_files)}: {os.path.basename(image_path)}")
                except Exception as e:
                    log(f"Error processing {os.path.basename(image_path)}: {e}")
        
        log(f"\nConversion complete! All images combined into {output_path}")
        log(f"File size: {os.path.getsize(output_path)} bytes")
        log(f"Number of images: {len(image_files)}")
        log(f"Resolution: {output_size[0]}×{output_size[1]} pixels")
        log(f"Dithering: {dither_method}")
        
        update_status(f"Conversion complete! Saved to {output_path}")
        
    except Exception as e:
        error_msg = f"Error during conversion: {str(e)}"
        log(error_msg)
        update_status(error_msg)

def create_gui():
    root = tk.Tk()
    root.title("TI-84 Image Converter")
    root.geometry("500x400")
    root.resizable(False, False)
    
    # Set up frame structure
    main_frame = ttk.Frame(root, padding="10")
    main_frame.pack(fill=tk.BOTH, expand=True)
    
    # Input folder selection
    ttk.Label(main_frame, text="Input Folder:").grid(column=0, row=0, sticky=tk.W, pady=5)
    input_var = tk.StringVar()
    ttk.Entry(main_frame, textvariable=input_var, width=40).grid(column=1, row=0, pady=5)
    ttk.Button(main_frame, text="Browse...", 
               command=lambda: input_var.set(filedialog.askdirectory(title="Select folder containing images"))
              ).grid(column=2, row=0, padx=5, pady=5)
    
    # Output file selection
    ttk.Label(main_frame, text="Output File:").grid(column=0, row=1, sticky=tk.W, pady=5)
    output_var = tk.StringVar()
    ttk.Entry(main_frame, textvariable=output_var, width=40).grid(column=1, row=1, pady=5)
    ttk.Button(main_frame, text="Browse...",
               command=lambda: output_var.set(filedialog.asksaveasfilename(
                   title="Save combined image file as",
                   defaultextension=".bin",
                   filetypes=[("Binary files", "*.bin")])
               )).grid(column=2, row=1, padx=5, pady=5)
    
    # Resolution selection
    ttk.Label(main_frame, text="Output Resolution:").grid(column=0, row=2, sticky=tk.W, pady=5)
    resolution_var = tk.StringVar(value="320×240")
    resolution_combo = ttk.Combobox(main_frame, textvariable=resolution_var, width=15)
    resolution_combo['values'] = ('320×240', '160×96')
    resolution_combo['state'] = 'readonly'
    resolution_combo.grid(column=1, row=2, sticky=tk.W, pady=5)
    
    # Dithering selection
    ttk.Label(main_frame, text="Dithering Method:").grid(column=0, row=3, sticky=tk.W, pady=5)
    dither_var = tk.StringVar(value="Floyd-Steinberg")
    dither_combo = ttk.Combobox(main_frame, textvariable=dither_var, width=15)
    dither_combo['values'] = ('None', 'Floyd-Steinberg', 'Ordered')
    dither_combo['state'] = 'readonly'
    dither_combo.grid(column=1, row=3, sticky=tk.W, pady=5)
    
    # Status text
    status_var = tk.StringVar(value="Ready to convert images")
    status_label = ttk.Label(main_frame, textvariable=status_var, wraplength=480)
    status_label.grid(column=0, row=5, columnspan=3, sticky=tk.W, pady=10)
    
    # Console output text area
    console_frame = ttk.LabelFrame(main_frame, text="Console Output")
    console_frame.grid(column=0, row=6, columnspan=3, sticky=(tk.W, tk.E), pady=5)
    console_frame.columnconfigure(0, weight=1)
    
    console = tk.Text(console_frame, height=10, width=60, wrap=tk.WORD)
    console.grid(column=0, row=0, sticky=(tk.W, tk.E), padx=5, pady=5)
    
    scrollbar = ttk.Scrollbar(console_frame, orient=tk.VERTICAL, command=console.yview)
    scrollbar.grid(column=1, row=0, sticky=(tk.N, tk.S))
    console['yscrollcommand'] = scrollbar.set
    
    # Set up queues for thread-safe communication
    log_queue = queue.Queue()
    status_queue = queue.Queue()
    
    # Function to process messages from the queues
    def process_queues():
        # Process log messages
        while not log_queue.empty():
            try:
                message = log_queue.get_nowait()
                console.insert(tk.END, message + "\n")
                console.see(tk.END)
            except queue.Empty:
                pass
        
        # Process status updates
        while not status_queue.empty():
            try:
                message = status_queue.get_nowait()
                status_var.set(message)
            except queue.Empty:
                pass
        
        # Schedule the next queue check
        root.after(100, process_queues)
    
    # Start the queue processing
    process_queues()
    
    # Convert button
    def start_conversion():
        input_folder = input_var.get()
        output_path = output_var.get()
        
        if not input_folder or not output_path:
            status_var.set("Please select both input folder and output file")
            return
        
        # Parse resolution
        res_text = resolution_var.get()
        if res_text == "320×240":
            output_size = (320, 240)
        else:  # 160×96
            output_size = (160, 96)
        
        # Get dithering method
        dither_method = dither_var.get()
        
        # Clear console
        console.delete(1.0, tk.END)
        
        status_var.set("Starting conversion...")
        
        # Run conversion in a separate thread
        conversion_thread = threading.Thread(
            target=convert_folder_to_ti84_format,
            args=(input_folder, output_path, output_size, dither_method, log_queue, status_queue),
            daemon=True
        )
        conversion_thread.start()
    
    convert_btn = ttk.Button(main_frame, text="Convert Images", command=start_conversion)
    convert_btn.grid(column=1, row=4, pady=10)
    
    return root

if __name__ == "__main__":
    root = create_gui()
    root.mainloop()