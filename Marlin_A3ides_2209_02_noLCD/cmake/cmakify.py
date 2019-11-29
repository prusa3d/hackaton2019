import json
import glob
import os
import xmltodict
from jq import jq
from pathlib import Path

output = []


def define_var(name_components, value):
    name = 'CPROJECT_' + '_'.join(name_components)
    if isinstance(value, str):
        output.append(f'set({name} "{value}")')
    elif isinstance(value, list):
        values = ' '.join(f'\t"{val}"\n' for val in value)
        output.append(f'set({name}\n{values})')


def path_is_parent(parent_path, child_path):
    parent_path = os.path.abspath(parent_path)
    child_path = os.path.abspath(child_path)
    return os.path.commonpath([parent_path]) == os.path.commonpath(
        [parent_path, child_path])


def is_same_file_or_subdir(path_a, path_b):
    try:
        path_a = Path(path_a).resolve()
        path_b = Path(path_b).resolve()
        return path_a.samefile(path_b) or path_is_parent(path_a, path_b)
    except FileNotFoundError:
        return False


with open('./.cproject') as f:
    data = xmltodict.parse(f.read())


configurations = jq('.cproject.storageModule[] | select(."@moduleId" == "org.eclipse.cdt.core.settings") | .cconfiguration[]').transform(data, multiple_output=True)
for configuration in configurations:
    configuration_name = jq('.storageModule[]."@name"').transform(configuration).upper()
    output.extend(['#', f'#{configuration_name}', '#'])

    build_system = jq('.storageModule[] | select(."@moduleId" == "cdtBuildSystem")').transform(configuration)
    sources = jq('.configuration.sourceEntries.entry').transform(build_system)

    source_files = []
    for sources_entry in sources:
        sources_name = sources_entry['@name']
        sources_excluding = [sources_name + '/' + s for s
                             in sources_entry.get('@excluding', '').split('|')]
        if sources_excluding == [sources_name + '/']:
            sources_excluding = []

        source_files += [source
                   for ext in ('c', 'cpp', 's')
                   for source in glob.glob(sources_name + f'/**/*.{ext}', recursive=True)
                   if not any(is_same_file_or_subdir(x, source) for x in sources_excluding)]
    define_var([configuration_name, 'SOURCES'], source_files)

    tool_settings = jq('.configuration.folderInfo | if type == "array" then . else [.] end | .[].toolChain.tool[]').transform(build_system, multiple_output=True)
    get_tool_settings = lambda tool_name: jq(f'.[] | select(."@name" == "{tool_name}")').transform(tool_settings)
    for tool_name in 'C Compiler', 'C++ Compiler':
        cmake_tool_name = tool_name.replace(' ', '_').replace('+', 'P').upper()
        settings = get_tool_settings(tool_name)

        includes = jq('.option[] | select(."@valueType" == "includePath") | .listOptionValue[] | ."@value"').transform(settings, multiple_output=True)
        includes = [str(Path(s).absolute()) for s in includes]
        define_var([configuration_name, cmake_tool_name, 'INCLUDES'], includes)

        macros = jq('.option[] | select(."@valueType" == "definedSymbols") | .listOptionValue[] | ."@value"').transform(settings, multiple_output=True)
        define_var([configuration_name, cmake_tool_name, 'MACROS'], macros)

    linker_settings = get_tool_settings('C Linker')
    linker_script = jq('.option[] | select(."@superClass" == "com.atollic.truestudio.ld.general.scriptfile") | ."@value"').transform(linker_settings, multiple_output=True)
    define_var([configuration_name, 'LINKER_SCRIPT'], str(Path(Path(linker_script[0]).name).absolute()))

    output.extend(['',''])


with open('CProjectDefines.cmake', 'w') as f:
    f.writelines(f'{line}\n' for line in output)
