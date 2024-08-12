# Created by doing:
# ```
# ./maintainers/scripts/nix-generate-from-cpan.pl Test::Nginx
# ```
# as mentioned on https://nixos.wiki/wiki/Perl
{ perlPackages, fetchurl, lib }:

with perlPackages;

buildPerlPackage {
  pname = "Test-Nginx";
  version = "0.30";

  src = fetchurl {
    url = "mirror://cpan/authors/id/A/AG/AGENT/Test-Nginx-0.30.tar.gz";
    hash = "sha256-gJdYU1HsIOLANWByNv54tSeevmJDXh/7kokV0m6xJrI=";
  };

  propagatedBuildInputs = [ HTTPMessage IPCRun LWP ListMoreUtils TestBase TestLongString TextDiff URI ];

  meta = {
    description = "Data-driven test scaffold for Nginx C module and Nginx/OpenResty-based libraries and applications";
    license = lib.licenses.bsd3;
  };
}
