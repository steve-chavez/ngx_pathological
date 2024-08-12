with import (builtins.fetchTarball {
  name = "24.05";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/24.05.tar.gz";
  sha256 = "sha256:1lr1h35prqkd1mkmzriwlpvxcb34kmhc9dnr48gkm8hh089hifmx";
}) {};
mkShell {
  buildInputs =
    let
      nginxStart = writeShellScriptBin "ngx-start" ''
        nginx -p nginx -e stderr
      '';

      nginxTest = writeShellScriptBin "ngx-test" ''
        prove -I. -r t
      '';
    in
    [
      (callPackage ./nix/customNginx.nix {})
      nginxStart
      nginxTest

      (perl.withPackages(ps: [
        (callPackage ./nix/perl-test-nginx.nix {})
      ]))
    ];

  shellHook = ''
    export HISTFILE=.history
  '';
}
