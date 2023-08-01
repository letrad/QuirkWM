{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = [
    pkgs.cmake
    pkgs.gnumake
    pkgs.gcc
    pkgs.git
    pkgs.xorg.libX11.dev
    pkgs.tomlc99
  ];
 shellHook = ''
    alias build='make all'
    alias clean="rm -rf build quirkwm QuirkWM"
  '';
}