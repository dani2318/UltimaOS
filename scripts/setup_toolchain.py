import os
import subprocess
import urllib.request
import platform
from pathlib import Path


import yaml

toolchain_info = None

with open("toolchain_settings.yml", 'r') as stream:
    toolchain_info = yaml.safe_load(stream)
print("TOOLCHAIN SETTINGS:")
print(toolchain_info)

TARGET = toolchain_info["Build Info"]["TARGET"]

BINUTILS_VER = toolchain_info["Toolchain Versions"]["BINUTILS_VERSION"]
GCC_VER= toolchain_info["Toolchain Versions"]["GCC_VERSION"]
NASM_VER= toolchain_info["Toolchain Versions"]["NASM_VERSION"]

BINUTILS_URL:str = toolchain_info["Urls"]["BINUTILS_URL"]
BINUTILS_URL = BINUTILS_URL.replace("BINUTILS_VERSION",BINUTILS_VER)

GCC_URL:str = toolchain_info["Urls"]["GCC_URL"]
GCC_URL = GCC_URL.replace("GCC_VERSION",GCC_VER)

NASM_URL_WIN:str = toolchain_info["Urls"]["NASM_URL_WIN"]
NASM_URL_WIN = NASM_URL_WIN.replace("NASM_VERSION",NASM_VER)

NASM_EXE_WIN:str = toolchain_info["Executables"]["NASM_EXE_WIN"]
NASM_EXE_WIN = NASM_EXE_WIN.replace("NASM_VERSION",NASM_VER)

jobs:int = int(toolchain_info["ADVANCED"]["JOBS"])


LIN_DIST:str = None

try:
    LIN_DIST = platform.freedesktop_os_release();
except FileNotFoundError:
    LIN_DIST = platform.system()


MAIN_PATH = Path(__file__.replace("setup_toolchain.py","")).parent.absolute()

TOOLCHAIN_FOLDER = f"{MAIN_PATH}/toolchain/"
print(TOOLCHAIN_FOLDER)

TOOLCHAIN_PREFIX = os.path.abspath(f"{TOOLCHAIN_FOLDER}/{TARGET}")

BINUTILS_SRC = f"{TOOLCHAIN_FOLDER}binutils-{BINUTILS_VER}"
BINUTILS_BUILD = f"{TOOLCHAIN_FOLDER}build-binutils-{BINUTILS_VER}"
GCC_BUILD = f"{TOOLCHAIN_FOLDER}build-gcc-{GCC_VER}"

formatted_str = "Target: %s\n Binutils build path: %s \n Binutils src path: %s \n\r gcc url: %s \n  GCC version: %s \n "
data = (TARGET,BINUTILS_BUILD,BINUTILS_SRC,GCC_URL,GCC_VER)
print(formatted_str % data)

BINUTLIS_FILENAME = "binutils-"+BINUTILS_VER+".tar.gz"
GCC_FILENAME = "gcc-"+GCC_VER+".tar.gz"

def show_progress(block_num, block_size, total_size):
    print("Downloading: " + str(round(block_num * block_size / total_size *100,2)), end="%\r")

def download(url, where:str,filename):
    if not os.path.exists(where):
        os.mkdir(where)
    print(f"WHERE: {where}")
    os.system("cd " + where)

    print("Moving to "+str(where)+"..")

    print(f"Downloading {filename}...\n\r",)

    if not where.endswith("/"): where = where + "/"

    urllib.request.urlretrieve(url,where + filename,show_progress)

    return True

windows = platform.system() == "Windows"

def download_binutils():


    os.system("cd "+ TOOLCHAIN_FOLDER)

    while(not os.path.exists(TOOLCHAIN_FOLDER)):
        continue

    if not os.path.exists("toolchain/"+BINUTLIS_FILENAME):
        download(BINUTILS_URL,TOOLCHAIN_FOLDER, BINUTLIS_FILENAME)

    if not os.path.exists(BINUTILS_BUILD):
        os.mkdir(BINUTILS_BUILD)


    if(windows):
        print("Windows build not implemented!")
        exit(-1)
        with open("binutils_config.ps1", "w") as f:
            f.write(f'$TOOLCHAIN_FOLDER = "{TOOLCHAIN_FOLDER}"\n')
            f.write(f'$BINUTILS_VER = "{BINUTILS_VER}"\n')
            f.write(f'$BINUTILS_FILENAME = "binutils-{BINUTILS_VER}.tar.gz"\n')
            f.write(f'$BINUTILS_BUILD = "{BINUTILS_BUILD}"\n')
            f.write(f'$TARGET = "{TARGET}"\n')
            f.write(f'$TOOLCHAIN_PREFIX = "{os.path.abspath(os.path.join(TOOLCHAIN_FOLDER, "install"))}"\n')
            f.write(f'$JOBS = {jobs}\n')

        # Call PowerShell script
        result = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", "build_binutils_win.ps1"])
        if result.returncode != 0:
            raise RuntimeError("PowerShell build script failed.")
        return


    # Linux build
    os.system("cd "+ TOOLCHAIN_FOLDER + "&& tar -xf " + BINUTLIS_FILENAME)
    os.system("cd "+BINUTILS_BUILD+" && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINK_FLAGS= LIBS= && ../binutils-"+BINUTILS_VER+"/configure \
		--prefix=" + TOOLCHAIN_PREFIX +"\
		--target=" + TARGET+ " \
		--with-sysroot \
		--disable-nls \
		--disable-werror")

    os.system(f"make -j{jobs} -C "+BINUTILS_BUILD)
    os.system(f"make -j{jobs} -C "+BINUTILS_BUILD + " install")
    return

def download_gcc():
    os.system("cd "+ TOOLCHAIN_FOLDER)

    while(not os.path.exists(TOOLCHAIN_FOLDER)):
        continue

    if not os.path.exists("toolchain/"+GCC_FILENAME):
        download(GCC_URL,TOOLCHAIN_FOLDER, GCC_FILENAME)

    if not os.path.exists(GCC_BUILD):
        os.mkdir(GCC_BUILD)


    if windows:
        print("Windows build not implemented!")
        exit(-1)
        # Write PowerShell config for GCC build
        with open("gcc_config.ps1", "w") as f:
            f.write(f'$TOOLCHAIN_FOLDER = "{TOOLCHAIN_FOLDER}"\n')
            f.write(f'$GCC_VER = "{GCC_VER}"\n')
            f.write(f'$GCC_FILENAME = "gcc-{GCC_VER}.tar.gz"\n')
            f.write(f'$GCC_BUILD = "{GCC_BUILD}"\n')
            f.write(f'$TARGET = "{TARGET}"\n')
            f.write(f'$TOOLCHAIN_PREFIX = "{os.path.abspath(os.path.join(TOOLCHAIN_FOLDER, "install"))}"\n')
            f.write(f'$JOBS = {jobs}\n')

        # Call the PowerShell build script for GCC
        result = subprocess.run(["powershell", "-ExecutionPolicy", "Bypass", "-File", "build_gcc_win.ps1"])
        if result.returncode != 0:
            raise RuntimeError("PowerShell GCC build script failed.")
        return

    os.system("cd "+ TOOLCHAIN_FOLDER + "&& tar -xf " + GCC_FILENAME)
    os.system("cd "+GCC_BUILD+" && CFLAGS= ASMFLAGS= CC= CXX= LD= ASM= LINK_FLAGS= LIBS= && ../gcc-"+GCC_VER+"/configure \
		--prefix=" + TOOLCHAIN_PREFIX +"\
		--target=" + TARGET+ " \
		--disable-nls \
		--enable-languages=c,c++ \
		--without-headers")

    os.system(f"make -j{jobs} -C "+GCC_BUILD + " all-gcc all-target-libgcc")
    os.system(f"make -j{jobs} -C "+GCC_BUILD + " install-gcc")
    os.system(f"make -j{jobs} -C "+GCC_BUILD + " install-target-libgcc")
    return;

def install_nasm():
    if(LIN_DIST == "Windows"):
        download(NASM_URL_WIN,TOOLCHAIN_FOLDER, f"nasm-{NASM_VER}-installer-x64.exe")
        os.system(f"cd {TOOLCHAIN_FOLDER} && nasm-{NASM_VER}-installer-x64.exe /S")
    


    return

def cleanup():
    os.system(f"rm -ri {TOOLCHAIN_FOLDER}/binutils-{BINUTILS_VER}")
    os.system(f"rm -ri {TOOLCHAIN_FOLDER}/build-binutils-{BINUTILS_VER}")
    os.system(f"rm -ri {TOOLCHAIN_FOLDER}/gcc-{GCC_VER}")
    os.system(f"rm -ri {TOOLCHAIN_FOLDER}/build-gcc-{GCC_VER}")
    return

def bootstrap_toolchain():
    if not os.path.exists(TOOLCHAIN_FOLDER):
        os.mkdir(TOOLCHAIN_FOLDER)


    install_nasm()
    download_binutils()
    download_gcc()
    print("Now you can delete the build folders in the toolchain folder! (do not delete \"i686-elf\")")
    #cleanup()

if __name__ == "__main__":
    if windows: exit(-1)
    if not os.path.exists(TOOLCHAIN_FOLDER):
        os.mkdir(TOOLCHAIN_FOLDER)


    install_nasm()
    download_binutils()
    download_gcc()

# git clone https://github.com/tianocore/edk2.git 
# cd ~/edk2
# git submodule update --init
# make -C BaseTools
# export EDK2_DIR="$HOME/NeoOS/toolchain/edk2"
# export WORKSPACE=$EDK2_DIR/Build
# export EDK_TOOLS_PATH=$EDK2_DIR/BaseTools
# source $EDK2_DIR/edksetup.sh