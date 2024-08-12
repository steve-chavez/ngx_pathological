{ nginx, nginxModules, lib }:
let
  customModule = {
    name = "ngx_pathological";
    version = "0.1";
    src = ../.;
    meta = with lib; {
      license = with licenses; [ mit ];
    };
  };
in
nginx.override {
  modules = [
    customModule
    nginxModules.echo
  ];
}

