from PIL import Image


def convert_image_to_rgb_array(filename, output_header="image.h", output_preview="preview_16x16.png"):
    # Ouvre et convertit en RGB
    original = Image.open(filename).convert("RGB")

    # Resize avec conservation du ratio
    resized = original.copy()
    resized.thumbnail((16, 16), Image.LANCZOS)

    # Crée une image noire 16x16
    final_img = Image.new("RGB", (16, 16), (0, 0, 0))

    # Calcule les coordonnées pour centrer l’image redimensionnée
    x_offset = (16 - resized.width) // 2
    y_offset = (16 - resized.height) // 2

    # Colle l’image redimensionnée dans l’image finale
    final_img.paste(resized, (x_offset, y_offset))

    # Sauvegarde l’aperçu PNG
    final_img.save(output_preview)
    print(f"Image PNG redimensionnée et centrée sauvegardée : {output_preview}")

    # Génération du fichier .h
    pixels = list(final_img.getdata())
    with open(output_header, "w") as f:
        f.write(f"const uint8_t image[16][16][3] = {{\n")
        for y in range(16):
            f.write("  { ")
            for x in range(16):
                idx = y * 16 + x
                r, g, b = pixels[idx]
                f.write(f"{{{r}, {g}, {b}}}")
                if x < 15:
                    f.write(", ")
            f.write(" }")
            if y < 15:
                f.write(",\n")
            else:
                f.write("\n")
        f.write("};\n")

    print(f"Image convertie (centrée et paddée) sauvegardée dans : {output_header}")


# Exemple
convert_image_to_rgb_array("mario-star-turtle-pixel-art-11563288791dkwnz4gdjm.png")
