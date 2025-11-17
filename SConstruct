from pathlib import Path
from SCons.Variables import *
from SCons.Environment import *
from SCons.Node import *
import yaml

from scripts.build_scripts.phony_targets import PhonyTargets
from scripts.build_scripts.utility import ParseSize

########################################################################
## LOADS DEPS STRING FROM YAML UPDATE THEM THEN SAVE TO THE YAML FILE ##
########################################################################

DEPS = {
    'binutils':'2.44',
    'gcc':'15.1.0'
}

with open("toolchain_settings.yml", 'r') as stream:
    toolchain_info = yaml.safe_load(stream)

toolchain_info["Toolchain Versions"]["BINUTILS_VERSION"] = DEPS.get('binutils')
toolchain_info["Toolchain Versions"]["GCC_VERSION"] = DEPS.get('gcc')

with open("toolchain_settings.yml", 'w') as file:
    yaml.safe_dump(toolchain_info,file)

########################################################################

VARS = Variables('scripts/build_scripts/config.py', ARGUMENTS)

VARS.AddVariables(
    EnumVariable(
        "config",
        help="Build configuration",
        default="debug",
        allowed_values=["debug","release"],
    ),
    EnumVariable(
        "arch",
        help="Target Architecture",
        default="i686",
        allowed_values=("i686")
    ),
    EnumVariable(
        "imageType",
        help="Type of image to generate",
        default="disk",
        allowed_values=("floppy","disk")
    ),
    EnumVariable(
        "imageFS",
        help="Filesystem to use for the image",
        default="fat32",
        allowed_values=("fat12","fat16","fat32","ext2")
    )
)

VARS.Add(
    "imageSize",
    help="The size of the image, will be rounded up to the nearest multiple of 512"+
         "You can use suffixes (k,m,g)"+
         "For floppies, the size is fixed to 1.44MB",
    default="250m",
    converter=ParseSize
)

VARS.Add(
    "toolchain",
    help="Path to toolchain directory.",
    default="toolchain"
)


HOST_ENVIRONMENT = Environment(
    variables=VARS,
    ENV=os.environ,
    AS='nasm',
    CFLAGS=['-std=c99'],
    CXXFLAGS=["-std=c++17"],
    CCFLAGS=['-g'],
    STRIP='strip',
    toolchain='toolchain/'
)

HOST_ENVIRONMENT.Append(
    ROOTDIR = HOST_ENVIRONMENT.Dir('.').srcnode()
)

if HOST_ENVIRONMENT['config'] == 'debug':
    HOST_ENVIRONMENT.Append(CCFLAGS= ['-O0'])
else:
    HOST_ENVIRONMENT.Append(CCFLAGS= ['-O3'])

if HOST_ENVIRONMENT['imageType'] == 'floppy':
    HOST_ENVIRONMENT['imageType'] = 'fat12'

HOST_ENVIRONMENT.Replace(ASCOMSTR        = "[Assembling] --> [$SOURCE]",
                         CCCOMSTR        = "[Compiling] --> [$SOURCE]",
                         CXXCOMSTR       = "[Compiling] -->  [$SOURCE]",
                         FORTRANPPCOMSTR = "[Compiling] -->  [$SOURCE]",
                         FORTRANCOMSTR   = "[Compiling] -->  [$SOURCE]",
                         SHCCCOMSTR      = "[Compiling] -->  [$SOURCE]",
                         SHCXXCOMSTR     = "[Compiling] -->  [$SOURCE]",
                         LINKCOMSTR      = "[Linking] -->    [$TARGET]",
                         SHLINKCOMSTR    = "[Linking] -->    [$TARGET]",
                         INSTALLSTR      = "[Installing] --> [$TARGET]",
                         ARCOMSTR        = "[Archiving] -->  [$TARGET]",
                         RANLIBCOMSTR    = "[Ranlib] -->     [$TARGET]")

platform_prefix = ''
toolchain_platform_prefix = ''
if HOST_ENVIRONMENT['arch'] == 'i686':
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
    ASFLAGS = [
        '-f', 'elf',
        '-g'
    ],
    CCFLAGS = [
        '-ffreestanding',
        '-nostdlib'
    ],
    CXXFLAGS = [
        '-fno-exceptions',
        '-fno-rtti'
    ],
    LINKFLAGS = [
        '-nostdlib'
    ],
    LIBS = ['gcc'],
    LIBPATH = [str(toolchainGCCLibs)],
)

TARGET_ENVIRONMENT['ENV']['PATH'] += os.pathsep + str(toolchainBin)
Help(VARS.GenerateHelpText(HOST_ENVIRONMENT))
Export('HOST_ENVIRONMENT')
Export('TARGET_ENVIRONMENT')

variantDir = 'build/{0}_{1}'.format(TARGET_ENVIRONMENT['arch'], TARGET_ENVIRONMENT['config'])
variantDirStage1 = variantDir + '/stage1_{0}'.format(TARGET_ENVIRONMENT['imageFS'])

SConscript('src/libs/core/SConscript', variant_dir=variantDir + '/libscore', duplicate=0)

SConscript('src/boot/loader/SConscript', variant_dir=variantDirStage1, duplicate=0)
SConscript('src/boot/bootloader/SConscript', variant_dir=variantDir + '/stage2', duplicate=0)
SConscript('src/kernel/SConscript', variant_dir=variantDir + '/kernel', duplicate=0)
SConscript('image/SConscript', variant_dir=variantDir, duplicate=0)


Import('image')
Default(image)

# Phony targets
PhonyTargets(HOST_ENVIRONMENT, 
             run=['./scripts/run.sh', HOST_ENVIRONMENT['imageType'], image[0].path],
             debug=['./scripts/debug.sh', HOST_ENVIRONMENT['imageType'], image[0].path],
             bochs=['./scripts/bochs.sh', HOST_ENVIRONMENT['imageType'], image[0].path],
             toolchain=['python3 ./scripts/setup_toolchain.py'])

Depends('run', image)