import sys
from pathlib import Path
import os

def preprocess_includes(shader_source, shader_directory):
    shader_lines = shader_source.splitlines()

    for line_idx in range(len(shader_lines)):
        line_words = shader_lines[line_idx].split()
        if len(line_words) > 0 and line_words[0] == "#include":
            shader_include_path = line_words[1]
            shader_include_file = open(shader_directory + shader_include_path, "r")
            shader_lines[line_idx] = shader_include_file.read()
            shader_include_file.close()

    output = []
    for line in shader_lines:
        if line:
            output.append(",".join(str(ord(c)) for c in line))
            output.append("%s" % ord("\n"))
    output.append("0")
    return ",".join(output)


def main(argv):

    if len(sys.argv) == 1:
        print("Missing filename parameter")
        exit()

    original_shader_path = sys.argv[1]    
    original_shader_file = open(original_shader_path, "r")

    shader_directory = os.path.dirname(os.path.realpath(original_shader_path))

    processed_shader_path = original_shader_path + ".gen.h"
    processed_shader_file = open(processed_shader_path, "w")

    preprocessed_shader = preprocess_includes(original_shader_file.read(), shader_directory + "/")

    shader_name = Path(original_shader_path).stem

    generated_content = "//Generated file, do not modify!\n"
    generated_content += "#pragma once\n\n"
    generated_content += "namespace Shaders {\n\n"
    generated_content += "struct " + shader_name + " {\n\n"
    generated_content += "inline static const char path[] = \"" + original_shader_path + "\";\n"
    generated_content += "inline static const char source[] = " 

    generated_content += "{\n%s\n\t\t};" % preprocessed_shader

    # close struct
    generated_content += "\n};\n"

    # close namespace
    generated_content += "\n}\n"

    processed_shader_file.write(generated_content)

    processed_shader_file.close()

    original_shader_file.close()

if __name__ == "__main__":
    main(sys.argv[1:])