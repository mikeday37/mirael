from PIL import Image, ImageEnhance

img = Image.open("Mirael.png")
dest_filename = "mirael.ico"

icon_sizes = [16, 24, 32, 48, 64, 128, 256]
boost_factors = {
    16: 1.6,
    24: 1.4,
    32: 1.2,
    48: 1.1,
    64: 1.05,
    128: 1.025
}

images_by_size = {}
for size in icon_sizes:
    resized = img.resize((size, size), resample=Image.LANCZOS)
    images_by_size[size] = resized

    factor = boost_factors.get(size, 1.0)
    if factor != 1.0:
        enhancer_b = ImageEnhance.Brightness(resized)
        resized = enhancer_b.enhance(factor)
        enhancer_c = ImageEnhance.Contrast(resized)
        resized = enhancer_c.enhance(factor)
    images_by_size[size] = resized

img.save(
    dest_filename,
    format="ICO",
    sizes=[(s, s) for s in icon_sizes],
    append_images=[images_by_size[s] for s in icon_sizes]
)

ico = Image.open(dest_filename)
print(f"Number of layers: {len(ico.info.get('sizes', []))}")
for size in ico.info.get('sizes', []):
    print(f"Found embedded size: {size[0]}x{size[1]}")
