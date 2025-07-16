{
  description = "The Puzzle Pits - Build environment with zig and optional SDL2";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        
        # Common source files
        commonSources = [
          "DIG.C" "DMA.C" "ENV.C" "FILE.C" "GFX.C" 
          "LOAD.C" "SB.C" "SETMODE.C" "gfxa_stub.c"
        ];
        
        # Build function
        buildPuzzlePits = { withSDL ? false }: pkgs.stdenv.mkDerivation {
          pname = "puzzle-pits${if withSDL then "-sdl" else ""}";
          version = "1.0";
          
          src = ./.;
          
          nativeBuildInputs = [ pkgs.zig ] ++ 
            (if withSDL then [ pkgs.pkg-config ] else []);
          
          buildInputs = if withSDL then [ pkgs.SDL2 ] else [];
          
          buildPhase = ''
            mkdir -p $out/bin
            
            # Set compile flags
            CFLAGS="-Wall -O2 -I."
            if [ "${toString withSDL}" = "1" ]; then
              CFLAGS="$CFLAGS -DUSE_SDL $(pkg-config --cflags sdl2)"
              LDFLAGS="$(pkg-config --libs sdl2)"
              # Compile SDL compatibility layer
              zig cc $CFLAGS -c sdl_compat.c -o sdl_compat.o
            else
              LDFLAGS=""
            fi
            
            # Compile common sources
            for src in ${toString commonSources}; do
              echo "Compiling $src..."
              zig cc $CFLAGS -c $src -o $(basename $src .C).o
            done
            
            # Build DEMO executable
            echo "Linking demo..."
            zig cc -o $out/bin/puzzle-pits-demo \
              DEMO.o DIG.o DMA.o ENV.o FILE.o GFX.o LOAD.o \
              SB.o SETMODE.o gfxa_stub.o \
              ${if withSDL then "sdl_compat.o" else ""} \
              $LDFLAGS
            
            # Build PITS executable  
            echo "Linking pits..."
            zig cc -o $out/bin/puzzle-pits \
              PITS.o DIG.o DMA.o ENV.o FILE.o GFX.o LOAD.o \
              SB.o SETMODE.o gfxa_stub.o \
              ${if withSDL then "sdl_compat.o" else ""} \
              $LDFLAGS
            
            # Copy assets
            if [ -d compiled-version/PITS95 ]; then
              cp -r compiled-version/PITS95 $out/bin/
            fi
          '';
          
          installPhase = ''
            # Already installed in buildPhase
            true
          '';
        };
      in {
        # Packages
        packages = {
          default = self.packages.${system}.puzzle-pits-sdl;
          puzzle-pits-sdl = buildPuzzlePits { withSDL = true; };
          puzzle-pits-nosdl = buildPuzzlePits { withSDL = false; };
        };
        
        # Development shells
        devShells = {
          default = self.devShells.${system}.sdl;
          
          sdl = pkgs.mkShell {
            buildInputs = [
              pkgs.zig
              pkgs.SDL2
              pkgs.pkg-config
            ];
            shellHook = ''
              echo "Development shell for The Puzzle Pits with SDL2"
              echo "Build with: nix build .#puzzle-pits-sdl"
              echo "Or compile with: zig cc -DUSE_SDL ..."
            '';
          };
          
          nosdl = pkgs.mkShell {
            buildInputs = [ pkgs.zig ];
            shellHook = ''
              echo "Development shell for The Puzzle Pits (no SDL)"
              echo "Build with: nix build .#puzzle-pits-nosdl"
              echo "Or compile with: zig cc ..."
            '';
          };
        };
      }
    );
}