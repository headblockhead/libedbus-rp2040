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
        libedbus-rp2040 = builtins.path {
          path = pkgs.lib.cleanSource ./.;
          name = "edbus-rp2040";
        };
        default = libedbus-rp2040;
      });
    };
}
