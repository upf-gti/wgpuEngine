import sys
from pathlib import Path

def main(argv):

    if len(sys.argv) == 1:
        print("Missing filename parameter")
        exit()

    original_shader_path = sys.argv[1]
    original_shader_file = open(original_shader_path, "r")

    processed_shader_path = original_shader_path + ".gen.h"
    processed_shader_file = open(processed_shader_path, "w")

    shader_name = Path(original_shader_path).stem

    generated_content = "//Generated file, do not modify!"
    generated_content = "#pragma once\n\n"
    generated_content += "static const char * " + shader_name + " = "

    generated_content += "R\"(" + original_shader_file.read() + ")\";"

    processed_shader_file.write(generated_content)
    processed_shader_file.close()

    original_shader_file.close()

if __name__ == "__main__":
    main(sys.argv[1:])