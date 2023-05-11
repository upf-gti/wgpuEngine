#
# Dawn uses a non standard way of downloading some of its dependencies (gclient)
# We replace it with CMake's FetchContent
#

import os
import sys
import subprocess

required_submodules = [
    'third_party/vulkan-deps',
    'spirv-headers/src',
    'spirv-tools/src',
    'vulkan-headers/src',
    'vulkan-loader/src',
    'vulkan-tools/src',
    'third_party/glfw',
    'third_party/abseil-cpp',
    'third_party/jinja2',
    'third_party/markupsafe',
]

class cd:
    """Context manager for changing the current working directory"""
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)

class Var:
    def __init__(self, name):
        self.name = name
    def __add__(self, str):
        return self.name + str
    def __radd__(self, str):
        return str + self.name

def log(msg):
    print(f"-- -- {msg}")

def main():
    if not os.path.isfile('DEPS'):
        return

    log(f"Listing dependencies from {os.getcwd()}")
    DEPS = open('DEPS').read()

    ldict = {}
    exec(DEPS, globals(), ldict)  # yeah I know this is dangerous
    deps = ldict.get('deps')
    if deps is None:
        return

    for path in required_submodules:
        if path not in deps:
            continue
        url, tag = parse_url(deps[path], ldict['vars'])
        log(f"Fetching dependency '{path}'")

        if not os.path.isdir(path):
            log(f"Cloning '{url}' into '{path}'")
            subprocess.run(['git', 'clone', '--recurse-submodules', url, path], capture_output=True)
            with cd(path):
                log(f"Checking out tag '{tag}'")
                subprocess.run(['git', 'checkout', tag], capture_output=True)
        else:
            log(f"(Already cloned)")
        with cd(path):
            subprocess.run([sys.executable, __file__])

def parse_url(dependency, vars):
    url = dependency['url']
    tokens = url.split('@')
    tag = tokens[-1].format(**vars)
    url = '@'.join(tokens[:-1]).format(**vars)
    return url, tag

if __name__ == "__main__":
    main()
