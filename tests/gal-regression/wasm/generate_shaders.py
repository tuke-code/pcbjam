#!/usr/bin/env python3
"""
Generate C++ shader headers from KiCad GLSL files.

This script converts GLSL shader files to C++ headers compatible with
KiCad's BUILTIN_SHADERS namespace.
"""

import os
import sys

import re

def convert_glsl120_to_es300(shader_source, is_fragment_shader):
    """
    Convert GLSL 1.20 (OpenGL 2.1) shader to GLSL ES 3.00 (WebGL 2.0).

    Key changes:
    - #version 120 -> #version 300 es (must be ABSOLUTE first line!)
    - Move any comments before #version to after declarations
    - Add precision qualifiers
    - attribute -> in
    - varying -> out (vertex) / in (fragment)
    - gl_FragColor -> custom output variable (fragment)
    - texture2D -> texture
    - Fix int * float type issues (2 * x -> 2.0 * x)
    - Fix int / float type issues (x / 4 -> x / 4.0)

    Legacy GL built-in conversions (for KiCad shaders):
    - gl_ModelViewProjectionMatrix -> uniform u_modelViewProjectionMatrix
    - gl_Vertex -> attribute a_vertex
    - gl_Color -> attribute a_color (vertex) / varying v_color (fragment)
    - gl_FrontColor -> varying v_color (vertex output)
    - gl_TexCoord[0] -> varying v_texCoord
    - ftransform() -> u_modelViewProjectionMatrix * a_vertex
    """
    lines = shader_source.split('\n')
    result = []
    version_replaced = False

    # Track what legacy built-ins are used so we can add declarations
    uses_mvp_matrix = 'gl_ModelViewProjectionMatrix' in shader_source or 'ftransform()' in shader_source
    uses_gl_vertex = 'gl_Vertex' in shader_source or 'ftransform()' in shader_source
    uses_gl_color = 'gl_Color' in shader_source
    uses_gl_front_color = 'gl_FrontColor' in shader_source
    uses_gl_texcoord = 'gl_TexCoord' in shader_source
    uses_gl_multitexcoord0 = 'gl_MultiTexCoord0' in shader_source

    # GLSL ES 3.00 requires #version to be the ABSOLUTE first line
    # Collect any comments before #version to add after declarations
    pre_version_comments = []
    in_multiline_comment = False

    # Patterns to fix type issues
    int_mult_pattern = re.compile(r'\b(\d+)\s*\*\s*([a-zA-Z_])')
    div_int_pattern = re.compile(r'([a-zA-Z_)\]]+)\s*/\s*(\d+)(?!\.)')

    for line in lines:
        stripped = line.strip()

        # Before #version is found, collect comments and empty lines
        if not version_replaced:
            # Track multiline comment state
            if '/*' in stripped and '*/' not in stripped:
                in_multiline_comment = True
                pre_version_comments.append(line)
                continue
            elif in_multiline_comment:
                pre_version_comments.append(line)
                if '*/' in stripped:
                    in_multiline_comment = False
                continue
            elif stripped.startswith('//') or stripped == '' or ('/*' in stripped and '*/' in stripped):
                pre_version_comments.append(line)
                continue

        # Replace #version 120 with #version 300 es + precision + legacy built-in replacements
        if stripped.startswith('#version'):
            result.append('#version 300 es')
            result.append('precision highp float;')
            result.append('precision highp int;')

            if is_fragment_shader:
                result.append('out vec4 fragColor;')
                # Fragment shader receives varyings from vertex shader
                if uses_gl_color or uses_gl_front_color:
                    result.append('in vec4 v_color;')
                if uses_gl_texcoord:
                    result.append('in vec2 v_texCoord;')
            else:
                # Vertex shader - add uniforms and attributes for legacy built-ins
                if uses_mvp_matrix:
                    result.append('uniform mat4 u_modelViewProjectionMatrix;')
                if uses_gl_vertex:
                    result.append('in vec4 a_vertex;')
                if uses_gl_color:
                    result.append('in vec4 a_color;')
                if uses_gl_multitexcoord0:
                    result.append('in vec4 a_texCoord0;')
                # Vertex shader outputs varyings
                if uses_gl_front_color or uses_gl_color:
                    result.append('out vec4 v_color;')
                if uses_gl_texcoord:
                    result.append('out vec2 v_texCoord;')

            # Add back any pre-version comments after the declarations
            if pre_version_comments:
                result.append('')  # Blank line before comments
                result.extend(pre_version_comments)

            version_replaced = True
            continue

        # Convert attribute to in (vertex shaders only)
        if not is_fragment_shader and stripped.startswith('attribute '):
            line = line.replace('attribute ', 'in ', 1)

        # Convert varying to out (vertex) or in (fragment)
        if stripped.startswith('varying '):
            if is_fragment_shader:
                line = line.replace('varying ', 'in ', 1)
            else:
                line = line.replace('varying ', 'out ', 1)

        # Convert gl_FragColor to fragColor (fragment shaders)
        if is_fragment_shader and 'gl_FragColor' in line:
            line = line.replace('gl_FragColor', 'fragColor')

        # Convert texture2D to texture
        if 'texture2D' in line:
            line = line.replace('texture2D', 'texture')

        # Convert legacy GL built-ins
        # ftransform() -> u_modelViewProjectionMatrix * a_vertex (must be done before other replacements)
        if 'ftransform()' in line:
            line = line.replace('ftransform()', 'u_modelViewProjectionMatrix * a_vertex')

        # gl_ModelViewProjectionMatrix -> u_modelViewProjectionMatrix
        if 'gl_ModelViewProjectionMatrix' in line:
            line = line.replace('gl_ModelViewProjectionMatrix', 'u_modelViewProjectionMatrix')

        # gl_Vertex -> a_vertex
        if 'gl_Vertex' in line:
            line = line.replace('gl_Vertex', 'a_vertex')

        # gl_FrontColor -> v_color (vertex shader output)
        if 'gl_FrontColor' in line:
            line = line.replace('gl_FrontColor', 'v_color')

        # gl_Color -> a_color (vertex) or v_color (fragment)
        if 'gl_Color' in line:
            if is_fragment_shader:
                line = line.replace('gl_Color', 'v_color')
            else:
                line = line.replace('gl_Color', 'a_color')

        # gl_TexCoord[0].st or gl_TexCoord[0].xy -> v_texCoord
        if 'gl_TexCoord' in line:
            # Handle gl_TexCoord[0].st and gl_TexCoord[0].xy
            line = re.sub(r'gl_TexCoord\[0\]\.st', 'v_texCoord', line)
            line = re.sub(r'gl_TexCoord\[0\]\.xy', 'v_texCoord', line)
            # Handle bare gl_TexCoord[0] (less common)
            line = re.sub(r'gl_TexCoord\[0\]', 'vec4(v_texCoord, 0.0, 0.0)', line)

        # gl_MultiTexCoord0 -> a_texCoord0 (for SMAA shaders)
        if 'gl_MultiTexCoord0' in line:
            line = re.sub(r'gl_MultiTexCoord0\.st', 'a_texCoord0.st', line)
            line = re.sub(r'gl_MultiTexCoord0\.xy', 'a_texCoord0.xy', line)
            line = line.replace('gl_MultiTexCoord0', 'a_texCoord0')

        # Fix uniform int -> uniform float (GLSL ES 3.00 doesn't allow implicit int/float conversion)
        # This specifically handles u_fontTextureWidth which is multiplied with floats
        if 'uniform int ' in line:
            line = line.replace('uniform int ', 'uniform float ')

        # Fix int * float type issues: "2 * x" -> "2.0 * x"
        def fix_int_mult(match):
            int_val = match.group(1)
            var_start = match.group(2)
            return f'{int_val}.0 * {var_start}'
        line = int_mult_pattern.sub(fix_int_mult, line)

        # Fix float / int type issues: "x / 4" -> "x / 4.0"
        def fix_div_int(match):
            var_part = match.group(1)
            int_val = match.group(2)
            return f'{var_part} / {int_val}.0'
        line = div_int_pattern.sub(fix_div_int, line)

        result.append(line)

    # If no #version was found, DON'T add one - the shader will get its version
    # from a runtime preamble (e.g. SMAA shaders). Just do basic conversions.
    # NOTE: If the shader has no #version, we still need to convert legacy GL built-ins

    return '\n'.join(result)


def convert_shader_to_cpp(source_path, var_name):
    """Convert a shader file to C++ header content."""
    with open(source_path, 'rb') as f:
        data = f.read()

    # Convert GLSL 1.20 to GLSL ES 3.00 for WebGL 2.0
    shader_source = data.decode('utf-8')
    is_fragment = '_frag' in var_name or 'frag' in source_path.lower()
    shader_source = convert_glsl120_to_es300(shader_source, is_fragment)
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
