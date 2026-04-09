
# Setup


## Install Requirements

Fedora:
```bash
sudo dnf install clang cmake clang-tools-extra 
```

Arch:
```bash
sudo pacman -S clang cmake clang-tools-extra llvm-libs
```

## Setup 

```
git clone git@github.com:sandeshkhadka/distributed-job-scheduler.git
cd distributed-job-scheduler
git config core.hooksPath .githooks # from inside the project-root
```

## Build Instructions
```
cmake -B build
cmake --build build
```
