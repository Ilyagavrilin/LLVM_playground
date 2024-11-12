# LLVM_Playground

This project contains tasks for LLVM course in University.
Main course repo: <https://github.com/lisitsynSA/llvm_course/blob/main/>

## Build and execution

All tasks tested with clang 19.1.1, older version had not tested yet.

### Nix shell

To cover all dependencies and make builds reproducible nix packages was selected as dependency control. All comands should be executed under `nix-shell`.
To install nix:

```
$> sh <(curl -L https://nixos.org/nix/install) --daemon
# single user installation
$> sh <(curl -L https://nixos.org/nix/install) --no-daemon
```

Then:

```
$> nix-shell
$> cd ...
# other build steps
```
