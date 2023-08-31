
<p align="center">
  <img src="https://github.com/Underrout/callisto/assets/8695490/a83fa818-b294-412e-91dc-da34c827710d" />
  <br>
</p>

# Callisto

A build-system for Super Mario World projects with a focus on git-compatibility and flexibility.

For detailed setup/usage instructions and feature descriptions have a look at [the wiki](https://github.com/Underrout/callisto/wiki).

## Features

- Automated building and packaging of Super Mario World projects
- Multiple emulator support
- Multiple build profiles
- Globally shareable code/data thanks to modules
- Conflict detection between tools, patches and other resources
- Global clean ROM support (i.e. no longer necessary to provide a clean ROM for each project)
- Compatible with arbitrary tools rather than only standardized tools
- Improved dependency tracking
- Standardized configuration files (TOML format)
- Global data split into four individual patches: overworld, title screen, credits and global ExAnimation
- Automatic resource exports from Lunar Magic
- Automatic ROM reloading in Lunar Magic after successful builds
- Customizable Graphics and ExGraphics folder location via symlinks/junctions
- Update can detect patch hijack changes that require a rebuild
- Additional safety measures to prevent loss of unsaved work inside and outside of ROM
