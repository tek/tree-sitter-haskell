{
  description = "The Haskell grammar for tree-sitter";

  inputs = {
    hix.url = "github:tek/hix";
    hix.inputs.nixpkgs.url = "github:nixos/nixpkgs/02f5696b0e6097e589076d886b317b83ff0437d7";
    rust-overlay.url = "github:oxalica/rust-overlay";
    nix-filter.url = "github:numtide/nix-filter";
    tree-sitter.url = "github:tree-sitter/tree-sitter/v0.27.0";
    tree-sitter.inputs.nixpkgs.follows = "hix/nixpkgs";
  };

  outputs = {self, hix, rust-overlay, nix-filter, tree-sitter, ...}: hix.lib.pro ({config, lib, util, ...}: {

    cabal = {
      license = "MIT";
      license-file = "LICENSE";
      author = "Torsten Schmits";
      language = "GHC2021";
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
          "exon >= 1.4 && < 1.9"
          "optparse-applicative >= 0.17 && < 0.19"
          "path ^>= 0.9"
          "path-io >= 1.7 && < 1.9"
          "transformers"
        ];
      };
      executable.enable = true;
      test = {
        enable = true;
        dependencies = [
          "hedgehog >= 1.1 && < 1.6"
          "path ^>= 0.9"
          "tasty >= 1.4 && < 1.6"
          "tasty-hedgehog >= 1.3 && < 1.5"
        ];
      };
      override = {fast, ...}: fast;
    };

    outputs = let
      inherit (config) pkgs;

      outputs = import ./nix/outputs.nix { inherit config util pkgs rust-overlay tree-sitter; filter = nix-filter.lib; };

      hs = outputs.dialect-haskell;
      hsc = outputs.dialect-hsc;

    in {
      packages = {
        default = lib.mkForce outputs.tree-sitter-haskell;
        inherit (outputs) bitmap-test tree-sitter-haskell rust;

        parser-gen = hs.parserGen;
        parser-src = hs.parserSrc;
        parser-lib = hs.parserLib;
        parser-wasm = hs.parserWasm;

        parser-hsc-gen = hsc.parserGen;
        parser-hsc-src = hsc.parserSrc;
        parser-hsc-lib = hsc.parserLib;
        parser-hsc-wasm = hsc.parserWasm;
      };

      apps = lib.genAttrs ["tests" "unit-tests" "ci" "tests-gen"] (name: util.app outputs.${name}) // {
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
        gen-parsers = util.app outputs.gen-parsers;
      };

      devShells = {

        default = lib.mkForce outputs.shell;

        tools = self.devShells.${config.system}.dev;

      };

    };

  });
}
