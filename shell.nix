with import <nixpkgs> {};
(mkShell.override { stdenv = llvmPackages_19.stdenv; }) {
  buildInputs = [
    pkg-config
    python312 cmake
    zlib
    SDL2
    xorg.libX11
    llvmPackages.libclang
  ];
}
