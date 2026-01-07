#!/usr/bin/env python3
"""
Generate C++ shader headers from KiCad GLSL files.

This script converts GLSL shader files to C++ headers compatible with
KiCad's BUILTIN_SHADERS namespace.
"""

import os
import sys

import re

def convert_glsl120_to_es100(shader_source, is_fragment_shader):
    """
    Convert GLSL 1.20 (OpenGL 2.1) shader to GLSL ES 1.00 (WebGL 1.0).

    GLSL ES 1.00 is similar to GLSL 1.20 but:
    - No #version directive needed (defaults to 100)
    - Needs precision qualifiers
    - attribute/varying are the correct keywords (not in/out)
    - gl_FragColor is correct (not custom output)
    - texture2D is correct (not texture)
    - STRICT: int * float not allowed - need explicit float literals

    Main changes needed:
    - Remove #version 120 (ES 1.00 is default)
    - Add precision qualifiers
    - Fix int * float type issues (2 * x -> 2.0 * x)
    """
    lines = shader_source.split('\n')
    result = []
    added_precision = False

    # Pattern to find integer literals multiplied by variables
    # Matches patterns like "2 * var" or "2* var" etc
    int_mult_pattern = re.compile(r'\b(\d+)\s*\*\s*([a-zA-Z_])')

    for line in lines:
        stripped = line.strip()

        # Skip the #version directive - ES 1.00 doesn't use it
        if stripped.startswith('#version'):
            # Add precision qualifiers instead
            if not added_precision:
                result.append('// GLSL ES 1.00 (WebGL 1.0) - converted from GLSL 1.20')
                result.append('precision highp float;')
                result.append('precision highp int;')
                result.append('')
                added_precision = True
            continue

        # Fix int * float type issues: "2 * x" -> "2.0 * x"
        # Only convert integers that don't already have a decimal point
        def fix_int_mult(match):
            int_val = match.group(1)
            var_start = match.group(2)
            # Add .0 to make it a float literal
            return f'{int_val}.0 * {var_start}'

        line = int_mult_pattern.sub(fix_int_mult, line)

        result.append(line)

    # If no version was found but we haven't added precision yet, add it at the start
    if not added_precision:
        prefix = ['// GLSL ES 1.00 (WebGL 1.0)', 'precision highp float;', 'precision highp int;', '']
        result = prefix + result

    return '\n'.join(result)


def convert_shader_to_cpp(source_path, var_name):
    """Convert a shader file to C++ header content."""
    with open(source_path, 'rb') as f:
        data = f.read()

    # Convert GLSL 1.20 to GLSL ES 1.00 for WebGL 1.0 compatibility
    # (ES 1.00 is closer to GLSL 1.20 and doesn't conflict with Emscripten prepends)
    shader_source = data.decode('utf-8')
    is_fragment = '_frag' in var_name or 'frag' in source_path.lower()
    shader_source = convert_glsl120_to_es100(shader_source, is_fragment)
    data = shader_source.encode('utf-8')

    # Convert to hex array
    hex_values = ', '.join(f'0x{b:02x}' for b in data)
    hex_values += ', 0x00'  # Null terminate

    array_size = len(data)

    header_content = f"""// Auto-generated shader header from {os.path.basename(source_path)}
#ifndef {var_name.upper()}_H
#define {var_name.upper()}_H

#include <string>

namespace KIGFX {{
namespace BUILTIN_SHADERS {{
    extern std::string {var_name};
}}
}}

#endif // {var_name.upper()}_H
"""

    cpp_content = f"""// Auto-generated from {os.path.basename(source_path)}
#include <string>
#include "{var_name}.h"

namespace KIGFX {{
namespace BUILTIN_SHADERS {{

static unsigned char {var_name}_bytes[] = {{ {hex_values} }};

std::string {var_name} = std::string(reinterpret_cast<char const*>({var_name}_bytes), {array_size});

}}
}}
"""

    return header_content, cpp_content

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    kicad_root = os.path.abspath(os.path.join(script_dir, '..', '..', '..', 'kicad'))
    shaders_dir = os.path.join(kicad_root, 'common', 'gal', 'shaders')

    output_dir = os.path.join(script_dir, 'generated')
    os.makedirs(output_dir, exist_ok=True)

    shaders = [
        ('kicad_frag.glsl', 'glsl_kicad_frag'),
        ('kicad_vert.glsl', 'glsl_kicad_vert'),
        ('smaa_base.glsl', 'glsl_smaa_base'),
        ('smaa_pass_1_frag_color.glsl', 'glsl_smaa_pass_1_frag_color'),
        ('smaa_pass_1_frag_luma.glsl', 'glsl_smaa_pass_1_frag_luma'),
        ('smaa_pass_1_vert.glsl', 'glsl_smaa_pass_1_vert'),
        ('smaa_pass_2_frag.glsl', 'glsl_smaa_pass_2_frag'),
        ('smaa_pass_2_vert.glsl', 'glsl_smaa_pass_2_vert'),
        ('smaa_pass_3_frag.glsl', 'glsl_smaa_pass_3_frag'),
        ('smaa_pass_3_vert.glsl', 'glsl_smaa_pass_3_vert'),
    ]

    generated_cpp_files = []

    for shader_file, var_name in shaders:
        source_path = os.path.join(shaders_dir, shader_file)

        if not os.path.exists(source_path):
            print(f"Warning: {source_path} not found, skipping")
            continue

        header_content, cpp_content = convert_shader_to_cpp(source_path, var_name)

        header_path = os.path.join(output_dir, f'{var_name}.h')
        cpp_path = os.path.join(output_dir, f'{var_name}.cpp')

        with open(header_path, 'w') as f:
            f.write(header_content)

        with open(cpp_path, 'w') as f:
            f.write(cpp_content)

        generated_cpp_files.append(cpp_path)
        print(f"Generated {var_name}.h and {var_name}.cpp")

    print(f"\nGenerated {len(generated_cpp_files)} shader files in {output_dir}")

if __name__ == '__main__':
    main()
