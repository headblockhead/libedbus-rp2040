{
  description = "A library that implements the EDBUS protocol using the Raspberry Pi RP2040's PIO";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs =
    { nixpkgs, ... }:
    let
      pkgsForSystem = system: import nixpkgs { inherit system; };
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forEachSystem = nixpkgs.lib.genAttrs supportedSystems;
      forEachSystemWithPkgs = f: forEachSystem (system: f (pkgsForSystem system));
    in
    {
      packages = forEachSystemWithPkgs (pkgs: rec {
        libedbus-rp2040 = pkgs.stdenv.mkDerivation {
          name = "libedbus-rp2040";
          src = pkgs.lib.cleanSource ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            gcc-arm-embedded
            python313
          ];
          cmakeFlags = [
            "-DCMAKE_C_COMPILER=${pkgs.gcc-arm-embedded}/bin/arm-none-eabi-gcc"
            "-DCMAKE_CXX_COMPILER=${pkgs.gcc-arm-embedded}/bin/arm-none-eabi-g++"
          ];
          env.PICO_SDK_PATH = "${pkgs.pico-sdk}/lib/pico-sdk";
        };
        default = libedbus-rp2040;
      });
      devShells = forEachSystemWithPkgs (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            gcc-arm-embedded
            python313
          ];
          PICO_SDK_PATH = "${pkgs.pico-sdk}/lib/pico-sdk";
        };
      });
    };
}
