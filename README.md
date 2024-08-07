# cbuild
(Another) C build system for C

# Philosophy

* Open world assumption
  * Introducing a new file/directory is not exceptional
  * Dependencies between files can change at any moment
* One project can produce several executables
* Setting up a project and building is easy
* Builds are frequent
* Projects are shared between computers in version control
* Testing is important
* Programmers are already familliar with the command line, i.e. git

# Usage

A project can be initialized by running the following command:

```bash
bld init
```

This will create a folder `.bld` in the current directory which informs the build system that the current directory is the root of a project. After a project has been initialized you can introduce a target with

```bash
bld init <target name> <path to main file>
```

A target is a description of an executable, this encompasses which file contains the entry-point function, the compiler information (which compiler and which flags) and the linker information (which linker and which flags). After a target has been initialized one should set the default compiler by setting the compiler of the root:

```bash
bld <target name> compiler <path to root of project> cc <compiler>
```

See `bld help compiler` for detailed information on how to use this command. The linker information also needs to be set up for the root:

```bash
bld <target name> linker <path to root of project> ll <linker>
```

See `bld help linker` for detailed information on how to use this command. Note that the compiler that was used to compile the code can very often be used as the linker which means that if you choose `gcc` as the compiler you can also choose `gcc` as the linker.

The target has now been set up and can be compiled with

```bash
bld <target name>
```

See `bld help build` for more information on building. This command generates an executable in the  project root named `<target name>.out`

# Installation

So far only Linux is supported (only Ubuntu tested) but Windows and MacOs will hopefully also be supported in the future.

For "maximum portability" this build system has been written in ansi C. At the moment there are no binaries to download which means that building from source is the only option. To do this you need to clone this repo with:

```bash
git clone <path>
```

then navigate into the source folder:

```bash
cd <name>
```

Once situated in the root of the git repo the bootstrap script can be compiled which will compile the final executable:

```bash
<ansi C compiler> ./bld_core/*.c ./bld_core/linker/*.c ./bld_core/compiler/*.c ./bld_core/language/*.c ./bootstrap.c -o ./bootstrap.out
```

If this compilation step worked you can then run the executable that was generated to compile the build system:

```bash
./bootstrap.out
```

This will, if successful, generate the `bld.out` executable which is the build system. This executable can then be put on the path, to start out you can run `bld help` and `bld help init` to see how to start a project.

# Supported os/compiler

The following operating systems are supported:
* Ubuntu

The following compilers are supported
* gcc
* clang
