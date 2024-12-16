import sys
from pathlib import Path
import os

def preprocess_includes(shader_source, shader_directory):
    shader_lines = shader_source.splitlines()

    shader_libs = ""

    for line_idx in range(len(shader_lines)):
        line_words = shader_lines[line_idx].split()
        if len(line_words) > 0 and line_words[0] == "#include":
            shader_include_path = line_words[1]
            shader_libs += "\"" + shader_include_path + "\", "
            shader_include_file = open(shader_directory + shader_include_path, "r")
            shader_lines[line_idx] = shader_include_file.read()
            shader_include_file.close()

    # remove last ", "
    shader_libs = shader_libs[:-2]

    output = []
    for line in shader_lines:
        if line:
            output.append(",".join(str(ord(c)) for c in line))
            output.append("%s" % ord("\n"))
    output.append("0")
    return ",".join(output), shader_libs


def main(argv):

    if len(sys.argv) == 1:
        print("Missing filename parameter")
        exit()

    original_shader_path = sys.argv[1]    
    original_shader_file = open(original_shader_path, "r")


    shader_directory = os.path.dirname(os.path.realpath(original_shader_path))

    processed_shader_path = "../../src/shaders/" + original_shader_path + ".gen.h"

    processed_shader_path = Path(processed_shader_path)
    processed_shader_path.parent.mkdir(exist_ok=True, parents=True)

    processed_shader_file = open(processed_shader_path, "w")

    preprocessed_shader, shader_libs = preprocess_includes(original_shader_file.read(), shader_directory + "/")

    shader_name = Path(original_shader_path).stem

    generated_content = "//Generated file, do not modify!\n"
    generated_content += "#pragma once\n\n"
    generated_content += "#include <string>\n"
    generated_content += "#include <vector>\n\n"
    generated_content += "namespace shaders {\n\n"
    generated_content += "struct " + shader_name + " {\n\n"
    generated_content += "inline static const char path[] = \"" + original_shader_path + "\";\n"
    generated_content += "inline static const std::vector<std::string> libraries = {" + shader_libs + "};\n"
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
