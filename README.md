
# Setup


## Install Requirements

```bash
sudo dnf install clang cmake clang-tools-extra
```

## Setup pre-commit hook
```
git config core.hooksPath .githooks
```

## Build Instructions
```
cmake -B build
cmake --build build
```
