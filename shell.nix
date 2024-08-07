with import (builtins.fetchTarball {
  name = "24.05";
  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/24.05.tar.gz";
  sha256 = "sha256:1lr1h35prqkd1mkmzriwlpvxcb34kmhc9dnr48gkm8hh089hifmx";
}) {};
mkShell {
  buildInputs =
    let
      customModule = {
        name = "ngx_pathological";
        version = "0.1";
        src = ./.;
        meta = with lib; {
          license = with licenses; [ mit ];
        };
      };
      customNginx = nginx.override {
        modules = [
          customModule
        ];
      };
      nginxStart = writeShellScriptBin "ngx-start" ''
        nginx -p nginx -e stderr
      '';
    in
    [ customNginx nginxStart ];

  shellHook = ''
    export HISTFILE=.history
  '';
}
