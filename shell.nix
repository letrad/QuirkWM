{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = [
    pkgs.cmake
    pkgs.gnumake
    pkgs.gcc
    pkgs.git
  ];
 shellHook = ''
    alias build='make all'
    alias clean="rm -rf build quirkwm QuirkWM"
  '';
}