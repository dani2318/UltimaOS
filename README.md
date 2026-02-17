# UltimaOS

UltimaOS is an operating system project designed for educational and experimental purposes. This repository contains all the necessary files to build, run, and explore NeoOS.

## Prerequisites

Before working with UltimaOS, ensure you have Python and gcc installed on your system as it is required to set up the toolchain; and qemu to run the OS image and nasm to build the assembly part of the project.

## Setting Up the Toolchain

To build the project, you first need to set up the required toolchain. This can be done using the script provided in the `scripts` folder. Follow these steps:

1. Navigate to the `scripts` folder:
    
    ```bash
    cd scripts
    
    ```
    
2. Run the `setup_toolchain.py` script:
    
    ```bash
    python3 setup_toolchain.py
    
    ```
    
3. The script will download and configure the necessary tools for building the project.

## Building the Project

Once the toolchain is set up, you can proceed to build the repository. Use the following commands:

1. Navigate back to the root directory of the repository:
    
    ```bash
    cd ..
    ```
    
2. Build the project:
    
    ```bash
    scons
    ```
    
    This will compile all the necessary components of NeoOS.
    

## Running the Project

After building the project, you can run NeoOS using an emulator such as QEMU. Use the following command:

```bash
scons run
```

This will launch NeoOS in QEMU, allowing you to test and explore the operating system.

## Documentation used:
Nanobyte: [here the link to the YT playlist](https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN) <br>
OS Dev: [link](https://wiki.osdev.org/Expanded_Main_Page) <br>

## Notes

- The project is actively being developed, so refer to the repository for the latest updates and documentation.
- **FOR WINDOWS USERS:** To build the repository and the toolchain it’s suggested to use the WSL2 subsystem.
