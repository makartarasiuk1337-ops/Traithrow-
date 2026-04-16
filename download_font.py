import urllib.request
import os

print("Downloading FontAwesome... (this might take a few seconds)")

# Download IconsFontAwesome5.h
h_url = "https://raw.githubusercontent.com/juliettef/IconFontCppHeaders/main/IconsFontAwesome5.h"
h_path = r"c:\Users\pozor\Documents\traithrow\src\IconsFontAwesome5.h"
urllib.request.urlretrieve(h_url, h_path)
print("Saved IconsFontAwesome5.h")

# Download FontAwesome5 Solid TTF
ttf_url = "https://github.com/FortAwesome/Font-Awesome/raw/5.15.4/webfonts/fa-solid-900.ttf"
ttf_path = "fa-solid-900.ttf"
urllib.request.urlretrieve(ttf_url, ttf_path)

# Convert TTF to C header byte array
out_h_path = r"c:\Users\pozor\Documents\traithrow\src\fa_solid_900.h"
with open(ttf_path, "rb") as f:
    data = f.read()

with open(out_h_path, "w") as f:
    f.write("#pragma once\n\n")
    f.write(f"const unsigned int fa_solid_900_size = {len(data)};\n")
    f.write("const unsigned char fa_solid_900_data[] = {\n")
    
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02x}, ")
        if (i + 1) % 16 == 0:
            f.write("\n")
            
    f.write("\n};\n")

print("Saved fa_solid_900.h")
os.remove(ttf_path)
