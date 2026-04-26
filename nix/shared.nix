{ pkgs ? import <nixpkgs> {} }:
{
  buildInputs = with pkgs; [
    sv-lang
    gtest
  ] ++ sv-lang.buildInputs;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
  ];

  shellOnlyPackages = with pkgs; [
    llvmPackages_22.clang-tools
  ];
}
