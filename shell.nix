{ pkgs ? import <nixpkgs> {} }:
let
  libmonome = pkgs.stdenv.mkDerivation rec {
    name = "libmonome";

    src = pkgs.fetchFromGitHub {
      owner = "monome";
      repo = "libmonome";
      rev = "v1.4.2";
      sha256 = "157ansswnq9i5y10mh4cfwn00hwdbgbmyzh4qnghcqvfs08jrsnd";
    };

    nativeBuildInputs = with pkgs; [ wafHook ];
    buildInputs = with pkgs; [ liblo libudev ];
  };

  avr32-toolchain = pkgs.stdenv.mkDerivation rec {
    name = "av32-toolchain";

    src = pkgs.fetchFromGitHub {
      owner = "samdoshi";
      repo = "avr32-toolchain-linux";
      rev = "ac4fe6f831ffcd026147ce88bf8427df1678737b";
      sha256 = "0mj7pp9masp6fycl3g4sp7mji4pdpyksvvcxywp6c0kfrv54d4mb";
    };

    buildInputs = [ pkgs.unzip ];

    phases = [ "installPhase" ];

    installPhase = ''
      tar xvfz $src/avr32-gnu-toolchain-3.4.3.820-linux.any.x86_64.tar.gz
      mv avr32-gnu-toolchain-linux_x86_64 $out
      unzip $src/avr32-headers-6.2.0.742.zip -d $out/avr32/include
    '';
  };

  python = pkgs.python38.withPackages (ps: [ ps.pyudev ]);

in
  pkgs.mkShell {
    buildInputs = with pkgs; [
      avr32-toolchain
      clang
      clang-tools
      csound
      dfu-programmer
      libmonome
      lldb
      python
      unixtools.xxd
      uucp
    ];
  }
