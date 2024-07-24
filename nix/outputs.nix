{config, pkgs, rust-overlay, filter}: let

  version = "1.0.0";

  tools = config.outputs.packages.tools;

  console = ''
  message_part() {
    echo -en "\e[35m>>>\e[m $*"
  }
  message() {
    message_part "$*\n"
  }
  '';

  scriptErr = name: text: pkgs.writeScript name ''
  #!${pkgs.runtimeShell}
  ${text}
  '';

  script = name: text: scriptErr name ''
   set -e
   ${text}
   '';

  grammarSrc = filter {
    root = ../.;
    include = [
      "grammar"
      (filter.matchExt "js")
    ];
  };

  parserGen = pkgs.stdenv.mkDerivation {
    name = "tree-sitter-haskell-parser-sources";
    inherit version;
    src = grammarSrc;
    nativeBuildInputs = [pkgs.nodejs pkgs.tree-sitter];

    buildPhase = ''
    runHook preBuild
    tree-sitter generate
    runHook postBuild
    '';

    installPhase = ''
    runHook preInstall
    install -Dt $out/src src/parser.c src/grammar.json src/node-types.json
    runHook postInstall
    '';

  };

  libSrc = filter {
    root = ../.;
    include = [
      "src"
      (filter.matchExt "h")
      "src/scanner.c"
    ];
  };

  parserSrc = pkgs.symlinkJoin { name = "lib-source"; paths = [parserGen libSrc]; };

  parserLib = pkgs.stdenv.mkDerivation {
    name = "tree-sitter-haskell-parser-lib";
    inherit version;
    src = parserSrc;
    stripDebugList = ["haskell.so"];

    buildPhase = ''
    runHook preBuild
    $CC -fPIC -shared -Isrc -O2 -o haskell.so src/parser.c src/scanner.c
    runHook postBuild
    '';

    installPhase = ''
    runHook preInstall
    install -Dt $out/lib haskell.so
    runHook postInstall
    '';

  };

  parserWasm = pkgs.stdenv.mkDerivation {
    name = "tree-sitter-haskell-parser-wasm";
    inherit version;
    src = parserSrc;
    nativeBuildInputs = [pkgs.nodejs pkgs.tree-sitter pkgs.emscripten];

    buildPhase = ''
    runHook preBuild
    mkdir -p .emscriptencache
    export EM_CACHE=$(pwd)/.emscriptencache
    tree-sitter build --wasm
    runHook postBuild
    '';

    installPhase = ''
    runHook preInstall
    mkdir -p $out/
    cp tree-sitter-haskell.wasm $out/
    runHook postInstall
    '';
  };

  tree-sitter-haskell = pkgs.symlinkJoin {
    name = "tree-sitter-haskell";
    paths = [parserLib parserWasm];
  };

  rustSrc = filter {
    root = ../.;
    include = [
      "Cargo.toml"
      "Cargo.lock"
      "test/rust"
      "bindings"
      "package.json"
      "package-lock.json"
      "queries"
    ];
  };

  rustPkgs = rust-overlay.inputs.nixpkgs.legacyPackages.${pkgs.system};

  rust = rustPkgs.rustPlatform.buildRustPackage {
    pname = "tree-sitter-haskell";
    inherit version;
    src = pkgs.symlinkJoin { name = "rust-source"; paths = [parserSrc rustSrc]; };

    cargoLock.lockFile = ../Cargo.lock;

    nativeBuildInputs = [pkgs.nodejs pkgs.tree-sitter];
  };

  bitmap-test = pkgs.stdenv.mkDerivation {
    name = "bitmap-test";
    src = ../tools/UnicodeData.txt;
    dontUnpack = true;
    doCheck = true;
    nativeBuildInputs = [tools];

    buildPhase = ''
    runHook preBuild
    mkdir $out
    tools bitmap-test --unicode-data=$src --file=$out/bitmap-test.c --gap-size=10000
    runHook postBuild
    '';

    checkPhase = ''
    runHook preCheck
    gcc -o ./test-bitmap $out/bitmap-test.c
    ./test-bitmap
    runHook postCheck
    '';

  };

  setup = ''
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  mkdir -p "$TREE_SITTER_LIBDIR"
  export EM_CACHE="''${TREE_SITTER_LIBDIR}/.emscriptencache"
  '';

  initConfig = ''
  if [[ ! -f ''${XDG_CONFIG_HOME}/tree-sitter/config.json ]]
  then
    tree-sitter init-config
  fi
  '';

  shell = pkgs.mkShell {
    name = "tree-sitter-haskell";
    packages = [
      pkgs.tree-sitter
      pkgs.nodejs
      pkgs.emscripten
      pkgs.python3
      pkgs.rustc
      pkgs.cargo
      pkgs.bc
      pkgs.gnumake
      pkgs.gcc
      pkgs.gdb
      pkgs.graphviz
    ];
    shellHook = setup;
  };

  gen-bitmaps = script "gen-bitmaps" ''
  gap_size=''${1-10000}
  ${tools}/bin/tools bitmaps --preset=id --preset=varid-start --preset=conid-start --preset=symop --preset=space \
    --file=$PWD/src/unicode.h --gap-size=$gap_size
  '';

  collectFiles = script "collect-files" ''
  known="test/known-failures/''${1##*/}.txt"
  if [[ -e $known ]]
  then
    ${pkgs.coreutils-full}/bin/comm -13 <(sort $known) <(find test/libs/$1 -name '*.hs' | sort)
  else
    find test/libs/$1 -name '*.hs' | sort
  fi
  '';

  collectFilesBench = script "collect-files-bench" ''
  ${collectFiles} $1 > $2
  '';

  parseFiles = script "parse-files" ''
  tree-sitter parse -q $(< $1)
  '';

  compileQuiet = script "tree-sitter-haskell-compile-quiet" ''
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  ${../test/ensure-parser.bash}
  '';

  resolveLib = ''
  ghc_lib="tsh-test-ghc/libraries/$lib"
  if [[ $lib == compiler ]]
  then
    lib="tsh-test-ghc/compiler"
  elif [[ -d "test/libs/$ghc_lib" ]]
  then
    lib=$ghc_lib
  fi
  '';

  benchWith = {warmup ? 3, max ? null}: libs: let
    extra = if max == null then "" else "--max-runs ${toString max}";
  in script "bench" ''
  files=$(mktemp --tmpdir)
  trap "rm -f $files" EXIT
  ${compileQuiet}
  for lib in ${libs}
  do
    ${resolveLib}
    ${pkgs.hyperfine}/bin/hyperfine \
      --warmup ${toString warmup} \
      --command-name "$lib" \
      --min-runs ''${1-10} \
      --setup "${collectFilesBench} $lib $files" \
      ${extra} \
      "${parseFiles} $files"
  done
  '';

  bench = benchWith {};

  bench-libs = script "bench-libs" ''
  export test_libs=$@
  ${bench "$test_libs"} 10
  '';

  askBenchHistory = ''
  message 'This will stash uncommitted changes to benchmark a range of commits and clean/hard-reset after each step.'
  message_part 'Continue? [yN] '
  decision=""
  read -n1 decision
  if [[ -n $decision ]]
  then
    echo ""
  fi
  if [[ $decision != 'y' ]]
  then
    exit 1
  fi
  '';

  bench-history = scriptErr "bench-history" ''
  ${console}
  if [[ $1 == '--force' ]]
  then
    shift
  else
    ${askBenchHistory}
  fi
  if ! git diff --quiet
  then
    dirty='true'
    git stash push -u
  fi
  prev=$(git name-rev --name-only HEAD)
  restore() {
    git checkout -f $prev
    if [[ -n $dirty ]]
    then
      git stash pop
    fi

  }
  trap 'restore' EXIT
  trap 'restore' INT
  range="''${1:-$prev~10..$prev}"
  message "Benchmarking range '$range'"
  for rev in $(git rev-list $range)
  do
    git checkout $rev
    tree-sitter generate --no-bindings --build
    eval ${benchWith {warmup = 0;} "\${bench_history_libs:-haskell-language-server}"} ''${bench_history_runs:-5}
    git clean -fd
    git reset --hard
  done
  '';

  unit-tests = script "tree-sitter-haskell-unit-tests" ''
  tree-sitter test
  test/parse/run.bash
  test/query/run.bash
  '';

  tests = script "tree-sitter-haskell-tests" ''
  ${unit-tests}
  test/parse-libs native
  test/parse-libs wasm
  cargo test
  '';

  ciApp = pkgs.writeShellApplication {
    name = "tree-sitter-haskell-ci";
    runtimeInputs = [
      pkgs.tree-sitter
      pkgs.nodejs
      pkgs.emscripten
      pkgs.python3
      pkgs.rustc
      pkgs.cargo
      pkgs.bc
      pkgs.gnumake
      pkgs.gcc
    ];
    text = ''
    ${setup}
    ${initConfig}
    npm install
    ${tests}
    '';
  };

  ci = "${ciApp}/bin/tree-sitter-haskell-ci";

  time = "${pkgs.time}/bin/time";

  report-mem = let
    parseQuiet = script "parse-hls-quiet" ''
    tree-sitter parse -q "$@" >/dev/null
    '';
  in script "tree-sitter-haskell-report-mem" ''
  set -Eo pipefail
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  ${compileQuiet}
  files=$(${collectFiles} tsh-test-ghc/compiler)
  echo -en '\e[35m>>>\e[m \e[34mMemory\e[m: '
  ${time} -f '%Mk rss | %c invol | %w vol' ${parseQuiet} $files
  '';

  report-quick = script "tree-sitter-haskell-report-quick" ''
  set -o pipefail
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  echo -en '\e[35m>>>\e[m \e[34mCompile\e[m: '
  ${time} -f '%es (%c, %w)' ${compileQuiet}
  echo -e "\e[35m>>>\e[m \e[34mFile size\e[m: $(stat -c %s $TREE_SITTER_LIBDIR/haskell.so)"
  ${unit-tests} >/dev/null || (echo -e '\e[35m>>>\e[m \e[31mTests failed!\e[m' && false)
  ${report-mem}
  '';

  report-size = script "tree-sitter-haskell-report-size" ''
  set -o pipefail
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  echo -en '\e[35m>>>\e[m \e[34mGenerate\e[m: '
  ${time} -f '%es (%c, %w)' tree-sitter generate
  ${report-quick}
  '';

  report = script "tree-sitter-haskell-report" ''
  set -o pipefail
  export TREE_SITTER_LIBDIR="$PWD/.lib"
  ${report-size}
  test/parse-libs
  ${bench "postgrest haskell-language-server semantic"}
  '';

  gen-parser = script "tree-sitter-haskell-gen-parser" ''
  cp ${parserGen}/src/* src/
  '';

in {
  inherit
    parserGen
    parserSrc
    parserLib
    parserWasm
    bitmap-test
    shell
    gen-bitmaps
    tree-sitter-haskell
    bench
    bench-libs
    bench-history
    benchWith
    unit-tests
    tests
    ci
    rust
    report
    report-mem
    report-size
    report-quick
    gen-parser
    ;
}
