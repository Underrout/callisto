# callisto
A build-system for Super Mario World projects with a focus on git-compatibility and flexibility.

## Features

- Automated building and packaging of Super Mario World projects
- Multiple emulator support
- Global clean ROM support (i.e. no longer necessary to provide a clean ROM for each project)
- Optional conflict detection between tools, patches and other resources
- Compatible with arbitrary tools (outside of Lunar Magic) rather than only standardized tools (as long as they fit certain criteria)
- More reliable and faster quickbuild thanks to a customized asar build
- Standardized config files (TOML format)
- Global data split into four individual patches: overworld, title screen, credits and global ExAnimation
- Lunar Monitor replaced by manual save function
- Customizable graphics and exgraphics folder location via symlinks/junctions
- Quickbuild can detect patch hijack changes that require a rebuild
- Quick switching between recent projects via a "recent projects" list
- Additional safety measures to prevent loss of unsaved work inside ROM
