{
  description = "The Haskell grammar for tree-sitter";

  inputs = {
    hix.url = "github:tek/hix";
    hix.inputs.nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    rust-overlay.url = "github:oxalica/rust-overlay";
    nix-filter.url = "github:numtide/nix-filter";
  };

  outputs = {self, hix, rust-overlay, nix-filter, ...}: hix.lib.pro ({config, lib, util, ...}: {

    cabal = {
      license = "MIT";
      license-file = "LICENSE";
      author = "Torsten Schmits";
      prelude = {
        enable = true;
        package = {
          name = "incipit-base";
          version = ">= 0.5";
        };
        module = "IncipitBase";
      };
      paths = false;
    };

    packages.tools = {
      src = ./tools;
      library = {
        enable = true;
        dependencies = [
          "exon >= 1.4 && < 1.7"
          "optparse-applicative ^>= 0.17"
          "path ^>= 0.9"
          "path-io >= 1.7 && < 1.9"
          "transformers"
        ];
      };
      executable.enable = true;
      test = {
        enable = true;
        dependencies = [
          "hedgehog >= 1.1 && < 1.3"
          "path ^>= 0.9"
          "tasty ^>= 1.4"
          "tasty-hedgehog >= 1.3 && < 1.5"
        ];
      };
      override = {fast, ...}: fast;
    };

    outputs = let
      inherit (config) pkgs;

      outputs = import ./nix/outputs.nix { inherit config pkgs rust-overlay; filter = nix-filter.lib; };

    in {
      packages = {
        default = lib.mkForce outputs.tree-sitter-haskell;
        inherit (outputs) bitmap-test tree-sitter-haskell rust;
        parser-gen = outputs.parserGen;
        parser-src = outputs.parserSrc;
        parser-lib = outputs.parserLib;
        parser-wasm = outputs.parserWasm;
      };

      apps = lib.genAttrs ["tests" "unit-tests" "ci"] (name: util.app outputs.${name}) // {
        gen-bitmaps = util.app outputs.gen-bitmaps;
        bench-all = util.app (outputs.bench "effects postgrest polysemy ivory haskell-language-server");
        bench-hls = util.app (outputs.bench "haskell-language-server");
        bench-ghc = util.app (pkgs.writeScript "bench-ghc" "${outputs.benchWith {warmup = 0; max = 3;} "tsh-test-ghc/compiler"} 3");
        bench-libs = util.app (outputs.bench-libs);
        bench-history = util.app (outputs.bench-history);
        files = util.app outputs.collectFiles;
        parse = util.app "${outputs.rust}/bin/parse";
        show = util.app "${outputs.rust}/bin/show";
        report = util.app outputs.report;
        report-mem = util.app outputs.report-mem;
        report-size = util.app outputs.report-size;
        report-quick = util.app outputs.report-quick;
        gen-parser = util.app outputs.gen-parser;
      };

      devShells = {

        default = lib.mkForce outputs.shell;

        tools = self.devShells.${config.system}.dev;

      };

    };

  });
}
