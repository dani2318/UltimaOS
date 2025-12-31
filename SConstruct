from pathlib import Path
from SCons.Variables import *
from SCons.Environment import *
from SCons.Node import *
import yaml
import os

from scripts.build_scripts.phony_targets import PhonyTargets
from scripts.build_scripts.utility import ParseSize

# 1. TOOLCHAIN DEPS
DEPS = {
    'binutils': '2.44',
    'gcc': '15.1.0'
}

with open("toolchain_settings.yml", 'r') as stream:
    toolchain_info = yaml.safe_load(stream)

toolchain_info["Toolchain Versions"]["BINUTILS_VERSION"] = DEPS.get('binutils')
toolchain_info["Toolchain Versions"]["GCC_VERSION"] = DEPS.get('gcc')

with open("toolchain_settings.yml", 'w') as file:
    yaml.safe_dump(toolchain_info, file)

# 2. VARIABLES
VARS = Variables('scripts/build_scripts/config.py', ARGUMENTS)
VARS.AddVariables(
    EnumVariable("config", "Build configuration", "debug", allowed_values=["debug","release"]),
    EnumVariable("arch", "Target Architecture", "x86_64", allowed_values=("i686", "x86_64")),
    EnumVariable("imageType", "Type of image to generate", "disk", allowed_values=("floppy","disk")),
    EnumVariable("imageFS", "Filesystem to use", "fat32", allowed_values=("fat12","fat16","fat32","ext2"))
)
VARS.Add("imageSize", "Image size (e.g. 250m)", "250m", converter=ParseSize)
VARS.Add("toolchain", "Path to toolchain", "toolchain")

# 3. HOST ENV
HOST_ENVIRONMENT = Environment(
    variables=VARS,
    ENV=os.environ,
    AS='nasm',
    CFLAGS=['-std=c11'],
    CXXFLAGS=["-std=c++17"],
    CCFLAGS=['-g'],
    toolchain='toolchain/'
)

HOST_ENVIRONMENT.Append(ROOTDIR = HOST_ENVIRONMENT.Dir('.').srcnode())

if HOST_ENVIRONMENT['config'] == 'debug':
    HOST_ENVIRONMENT.Append(CCFLAGS=['-O0'])
else:
    HOST_ENVIRONMENT.Append(CCFLAGS=['-O3'])

HOST_ENVIRONMENT.Replace(
    ASCOMSTR="[Assembling] $SOURCE",
    CCCOMSTR="[Compiling]  $SOURCE",
    CXXCOMSTR="[Compiling]  $SOURCE",
    LINKCOMSTR="[Linking]    $TARGET",
    ARCOMSTR="[Archiving]  $TARGET"
)

# 4. TARGET ENV
platform_prefix = ''
toolchain_platform_prefix = ''

if HOST_ENVIRONMENT['arch'] == 'x86_64':
    platform_prefix = 'x86_64-elf-' 
    toolchain_platform_prefix = 'x86_64-elf'
elif HOST_ENVIRONMENT['arch'] == 'i686':
    platform_prefix = 'i686-elf-'
    toolchain_platform_prefix = 'i686-elf'

toolchainDir = Path(HOST_ENVIRONMENT['toolchain'], toolchain_platform_prefix)
toolchainBin = Path(toolchainDir, 'bin')
toolchainGCCLibs = Path(toolchainDir, 'lib','gcc', toolchain_platform_prefix, DEPS['gcc']).absolute()

TARGET_ENVIRONMENT = HOST_ENVIRONMENT.Clone(
    AR=f"{platform_prefix}ar",
    CC=f"{platform_prefix}gcc",
    CXX=f"{platform_prefix}g++",
    LD=f"{platform_prefix}g++",
    RANLIB=f"{platform_prefix}ranlib",
    STRIP=f"{platform_prefix}strip",
    TOOLCHAIN_PREFIX=str(toolchainDir),
    TOOLCHAIN_LIBGCC=str(toolchainGCCLibs),
)

TARGET_ENVIRONMENT.Append(
    ASFLAGS = ['-f', 'elf64' if HOST_ENVIRONMENT['arch'] == 'x86_64' else 'elf', '-g'],
    CCFLAGS = [
        '-ffreestanding',
        '-nostdlib',
        '-mno-red-zone' if HOST_ENVIRONMENT['arch'] == 'x86_64' else ''
    ],
    CXXFLAGS = ['-fno-exceptions', '-fno-rtti'],
    LINKFLAGS = ['-nostdlib'],
    LIBS = ['gcc'],
    LIBPATH = [str(toolchainGCCLibs)],
)

TARGET_ENVIRONMENT['ENV']['PATH'] += os.pathsep + str(toolchainBin)

Export('HOST_ENVIRONMENT')
Export('TARGET_ENVIRONMENT')

# 5. BUILD LOGIC
variantDir = 'build/{0}_{1}'.format(TARGET_ENVIRONMENT['arch'], TARGET_ENVIRONMENT['config'])

SConscript('src/libs/core/SConscript', variant_dir=variantDir + '/libscore', duplicate=0)

# Capture return values from scripts
bootloader = SConscript('src/boot/uefi_boot/SConscript', variant_dir=variantDir + '/uefi_boot', duplicate=0, must_exist=True)
kernel = SConscript('src/kernel/SConscript', variant_dir=variantDir + '/kernel', duplicate=0, must_exist=False)

# Pass valid nodes to image script
image = SConscript('image/SConscript', variant_dir=variantDir + '/image_build', duplicate=0, 
                   must_exist=False, exports={'bootloader': bootloader, 'kernel': kernel})

Default(image)

# 6. PHONY TARGETS SAFETY CHECK
# Check if image build returned a valid target before trying to get its path
image_path = image[0].path if (image and len(image) > 0) else "image_not_built"

PhonyTargets(HOST_ENVIRONMENT, 
             run=['./scripts/run.sh', HOST_ENVIRONMENT['imageType'], image_path],
             debug=['./scripts/debug.sh', HOST_ENVIRONMENT['imageType'], image_path],
             bochs=['./scripts/bochs.sh', HOST_ENVIRONMENT['imageType'], image_path],
             toolchain=['python3 ./scripts/setup_toolchain.py'])

if image:
    Depends('run', image)