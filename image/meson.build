# Image generation
# Note: The complex Python logic from the SCons build_image function
# needs to be preserved in a separate script

# Find all files in root directory
root_dir = meson.current_source_dir() / 'root'
find_root = run_command('find', root_dir, '-type', 'f', check: false)
root_files = []
if find_root.returncode() == 0
  foreach file : find_root.stdout().strip().split('\n')
    if file != ''
      root_files += files(file)
    endif
  endforeach
endif

# Image build script
build_image_script = files('build_image.py')

# Custom target to build the image
image = custom_target('image.img',
  output: 'image.img',
  input: [stage1, stage2, kernel] + root_files,
  command: [
    'python3',
    meson.current_source_dir() / 'build_image.py',
    '@OUTPUT@',
    '@INPUT0@',  # stage1
    '@INPUT1@',  # stage2
    '@INPUT2@',  # kernel
    '--root-dir', root_dir,
    '--image-type', image_type,
    '--image-fs', image_fs,
    '--image-size', image_size,
    '--toolchain-libgcc', toolchain_libgcc,
  ],
  depends: [stage1, stage2, kernel],
  build_by_default: true,
  console: true,
)

# Phony targets for running
run_target('run',
  command: ['./scripts/run.sh', image_type, image]
)

run_target('debug',
  command: ['./scripts/debug.sh', image_type, image]
)

run_target('bochs',
  command: ['./scripts/bochs.sh', image_type, image]
)

run_target('toolchain',
  command: ['python3', './scripts/setup_toolchain.py']
)