import os
import subprocess
from shutil import copyfile, rmtree, make_archive
from distutils.dir_util import copy_tree
from pathlib import Path
import sys

from git import Repo

from bs4 import BeautifulSoup


VERSION_FORMAT = 'v{}.{}.{}'

BUILD_FOLDER = 'build'

CUSTOM_ASAR_REPO_URL = 'https://github.com/Underrout/asar.git'
CUSTOM_ASAR_BRANCH_181 = 'v1.81-callisto'
CUSTOM_ASAR_TAG_181 = 'c-v1.81-2'
CUSTOM_ASAR_FOLDER_NAME = 'asar'
BUILT_ASAR_FILE_PATH_181 = 'asar/Release/asar.dll'

CUSTOM_ASAR_BRANCH_19 = 'v1.9-callisto'
CUSTOM_ASAR_TAG_19 = 'c-v1.91-2'
BUILT_ASAR_FILE_PATH_19 = 'asar/lib/Release/asar.dll'

CALLISTO_DOCS_URL = 'https://github.com/Underrout/callisto.wiki.git'
CALLISTO_DOCS_FOLDER = 'callisto-docs'
CALLISTO_DOCS_OUTPUT_FOLDER = 'documentation'
CALLISTO_ICON_FILE = '../../callisto/icons/callisto1.ico'

ASAR_BUILD_FOLDER = 'asar-build'

CALLISTO_REPO_PATH = '../..'
CALLISTO_BUILD_PATH = 'callisto'
CALLISTO_BUILD_OUTPUT_PATH = 'callisto'
CALLISTO_BUILD_OUTPUT_FRAGMENTS = [
    ( 'config', 'config' ), 
    ( 'initial_patches', 'initial_patches' ), 
    ( 'Release/asar.dll', 'asar.dll'), 
    ( 'Release/ASAR_LICENSE', 'ASAR_LICENSE' ), 
    ( 'Release/callisto.exe', 'callisto.exe' ), 
    ( 'Release/eloper.exe', 'eloper.exe' ), 
    ( 'Release/LICENSE', 'LICENSE' )
]

PANDOC_COMMAND = 'pandoc {} -o {} --template ../pandoc-bootstrap/template.html --include-in-header ../pandoc-bootstrap/header.html ' \
'--include-after-body ../pandoc-bootstrap/footer.html ' \
'--standalone --mathjax --toc --toc-depth 2'

NEW_VERSION = tuple(int(x) for x in sys.argv[1].split('.'))


def compile_callisto(output_location: str, callisto_location: str, callisto_build_folder: str, 
                     output_fragments: list[tuple[str, str]], version: tuple[int, int, int]):
    if not os.path.exists(callisto_build_folder):
        os.makedirs(callisto_build_folder)

    main_dir = os.path.abspath(os.curdir)

    os.chdir(callisto_build_folder)

    try:
        s = subprocess.run(['cmake', f'../{callisto_location}', 
                            f'-DVERSION_MAJOR={version[0]}',
                            f'-DVERSION_MINOR={version[1]}',
                            f'-DVERSION_PATCH={version[2]}'
        ])
        if s.returncode != 0:
            raise RuntimeError('Failed to create build files for callisto')

        s = subprocess.run(['cmake', '--build', '.', '--config', 'Release'])
        if s.returncode != 0:
            raise RuntimeError('Failed to compile callisto')
    
    finally:
        os.chdir(main_dir)
    
    for build_location, relative_output_location in output_fragments:
        rel_build_path = f'{callisto_build_folder}/{CALLISTO_BUILD_OUTPUT_PATH}/{build_location}'
        rel_output_path = f'{output_location}/{relative_output_location}'

        if os.path.isfile(rel_build_path):
            copyfile(rel_build_path, rel_output_path)
        else:
            copy_tree(rel_build_path, rel_output_path)


def compile_custom_asar181_at(output_location: str, asar_folder: str, architecture: str):
    build_path = f'{ASAR_BUILD_FOLDER}-{architecture}'

    if not os.path.exists(build_path):
        os.makedirs(build_path)

    main_dir = os.path.abspath(os.curdir)

    os.chdir(build_path)

    try:
        s = subprocess.run(['cmake', f'-A {architecture}', f'../{asar_folder}/src'])
        if s.returncode != 0:
            raise RuntimeError(f'Failed to create build files for asar on architecture {architecture}')

        s = subprocess.run(['cmake', '--build', '.', '--config', 'Release'])
        if s.returncode != 0:
            raise RuntimeError(f'Failed to compile asar on architecture {architecture}')
    
    finally:
        os.chdir(main_dir)
    
    copyfile(f'./{build_path}/{BUILT_ASAR_FILE_PATH_181}', output_location)


def compile_custom_asar19_at(output_location: str, asar_folder: str, architecture: str):
    build_path = f'{ASAR_BUILD_FOLDER}-{architecture}'

    if not os.path.exists(build_path):
        os.makedirs(build_path)

    main_dir = os.path.abspath(os.curdir)

    os.chdir(build_path)

    try:
        s = subprocess.run(['cmake', f'-A {architecture}', f'../{asar_folder}/src'])
        if s.returncode != 0:
            raise RuntimeError(f'Failed to create build files for asar on architecture {architecture}')

        s = subprocess.run(['cmake', '--build', '.', '--config', 'Release'])
        if s.returncode != 0:
            raise RuntimeError(f'Failed to compile asar on architecture {architecture}')
    
    finally:
        os.chdir(main_dir)
    
    copyfile(f'./{build_path}/{BUILT_ASAR_FILE_PATH_19}', output_location)


def check_unique_version(callisto_repo_path: str, proposed_version: tuple[int, int, int]):
    proposed_version_string = VERSION_FORMAT.format(*proposed_version)
    previous_versions = get_callisto_version_strings(callisto_repo_path)
    if proposed_version_string in previous_versions:
        raise RuntimeError(f'Version {proposed_version_string} already used!')


def get_callisto_version_strings(callisto_repo_path) -> list[str]:
    repo = Repo(callisto_repo_path)
    tags = repo.git.ls_remote('--tags', 'origin')
    return tags


def ensure_custom_asar_repo(folder_name: str, repo_url: str, branch_name: str, tag: str):
    if os.path.exists(folder_name):
        repo = Repo(folder_name)
        repo.remote().fetch()
        repo.git.checkout(branch_name)
        repo.git.reset("--hard", f"origin/{branch_name}")
    else:
        repo = Repo.clone_from(repo_url, folder_name)
        repo.git.checkout(tag)


def ensure_doc_repo(folder_name: str, repo_url: str):
    if os.path.exists(folder_name):
        repo = Repo(folder_name)
        repo.remotes.origin.pull('master:master')  # smh, default branch for wiki is still master
    else:
        Repo.clone_from(repo_url, folder_name)


def post_process_doc(html_path: str, other_doc_names: list[str]):
    html = open(html_path, encoding='utf-8', errors='ignore')
    soup = BeautifulSoup(html, 'html.parser')

    for element in soup.find_all('a'):
        try:
            if element['href'] in other_doc_names:
                element['href'] += '.html'
        except:
            pass

    with open(html_path, 'wb') as out:
        out.write(soup.prettify('utf-8'))


def create_html_docs_at(output_folder: str, input_folder: str, pandoc_command: str, package_folder: str, version: tuple[int, int, int]):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    filenames = []
    for filename in os.listdir(input_folder):
        if not filename.endswith('.md') or filename.startswith('_'):
            continue

        input_path = os.path.join(input_folder, filename)
        name = os.path.splitext(filename)[0]
        filenames.append(name)
        output_path = os.path.join(output_folder, name)
        output_path += '.html'

        spaced = name.replace('-', ' ')

        subprocess.run(pandoc_command.format(input_path, output_path).split() + 
                       [f'--metadata=title:{spaced}'] +
                       ['-V', f'vmajor={version[0]}'] +
                       ['-V', f'vminor={version[1]}'] +
                       ['-V', f'vpatch={version[2]}']
        )

    for filename in os.listdir(output_folder):
        html_path = os.path.join(output_folder, filename)
        post_process_doc(html_path, filenames)

    copy_tree(f'{input_folder}/images', f'{output_folder}/images')
    copyfile(CALLISTO_ICON_FILE, f'{output_folder}/{os.path.basename(CALLISTO_ICON_FILE)}')
    copyfile('../pandoc-bootstrap/styles.css', f'{output_folder}/styles.css')


def main(package_path: str):
    check_unique_version(CALLISTO_REPO_PATH, NEW_VERSION)

    os.makedirs(package_path)
    os.makedirs(f'{package_path}/asar/v1.81/32-bit')
    os.makedirs(f'{package_path}/asar/v1.81/64-bit')
    os.makedirs(f'{package_path}/asar/v1.91/32-bit')
    os.makedirs(f'{package_path}/asar/v1.91/64-bit')

    ensure_custom_asar_repo(CUSTOM_ASAR_FOLDER_NAME, CUSTOM_ASAR_REPO_URL, CUSTOM_ASAR_BRANCH_181, CUSTOM_ASAR_TAG_181)
    compile_custom_asar181_at(f'{package_path}/asar/v1.81/64-bit/asar.dll', CUSTOM_ASAR_FOLDER_NAME, 'x64')
    compile_custom_asar181_at(f'{package_path}/asar/v1.81/32-bit/asar.dll', CUSTOM_ASAR_FOLDER_NAME, 'Win32')

    ensure_custom_asar_repo(CUSTOM_ASAR_FOLDER_NAME, CUSTOM_ASAR_REPO_URL, CUSTOM_ASAR_BRANCH_19, CUSTOM_ASAR_TAG_19)
    compile_custom_asar19_at(f'{package_path}/asar/v1.91/64-bit/asar.dll', CUSTOM_ASAR_FOLDER_NAME, 'x64')
    compile_custom_asar19_at(f'{package_path}/asar/v1.91/32-bit/asar.dll', CUSTOM_ASAR_FOLDER_NAME, 'Win32')

    compile_callisto(package_path, CALLISTO_REPO_PATH, CALLISTO_BUILD_PATH, CALLISTO_BUILD_OUTPUT_FRAGMENTS, NEW_VERSION)

    ensure_doc_repo(CALLISTO_DOCS_FOLDER, CALLISTO_DOCS_URL)
    create_html_docs_at(f'{package_path}/{CALLISTO_DOCS_OUTPUT_FOLDER}', CALLISTO_DOCS_FOLDER, 
                        PANDOC_COMMAND, package_path, NEW_VERSION)
    
    make_archive(package_path, 'zip', package_path)
    # rmtree(package_path)


if __name__ == '__main__':
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    if not os.path.exists(BUILD_FOLDER):
        os.makedirs(BUILD_FOLDER)
    os.chdir(BUILD_FOLDER)

    package_path = f'callisto-{VERSION_FORMAT.format(*NEW_VERSION)}'
    if os.path.exists(package_path):
        rmtree(package_path)

    main(package_path)
