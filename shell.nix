{ pkgs ? import <nixpkgs> {} }:

let
  llvmPackages = pkgs.llvmPackages_19;
  clangBin = "${llvmPackages.clang}/bin";
  CC = "${clangBin}/clang";
  CXX = "${clangBin}/clang++";

  pythonEnv = pkgs.python311.withPackages (ps: with ps; [ numpy matplotlib seaborn llvmlite]);
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    llvmPackages.clang
    llvmPackages.llvm 
    libffi
    cmake
    ninja
    SDL2
    pkg-config
    pythonEnv
    bison
    flex
    flexcpp
  ];

  shellHook = ''
    export CC=${CC}
    export CXX=${CXX}
    export CMAKE_C_COMPILER=${CC}
    export CMAKE_CXX_COMPILER=${CXX}

    export LLVM_DIR=${llvmPackages.llvm.dev}/lib/cmake/llvm/

    export PKG_CONFIG_PATH=${pkgs.SDL2.dev}/lib/pkgconfig:$PKG_CONFIG_PATH
  '';
}