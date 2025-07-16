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
        
        # Game data package
        gameDataPackage = pkgs.stdenv.mkDerivation {
          pname = "puzzle-pits-data";
          version = "1.0";
          
          src = pkgs.fetchzip {
            url = "https://archive.org/download/puzzle-pits/pits95.zip";
            sha256 = "sha256-CIWSZReZ//qrP5ek1I5/g/aADr3LiZwU7C40aG4bELY=";
            stripRoot = false;
          };
          
          installPhase = ''
            mkdir -p $out/share/puzzle-pits
            cp -r PITS95/* $out/share/puzzle-pits/
            
            # Create lowercase versions for game compatibility
            mkdir -p $out/share/puzzle-pits/lowercase
            cp PITS95/TILES1.DAT $out/share/puzzle-pits/lowercase/tiles1.dat
            cp PITS95/TILES2.DAT $out/share/puzzle-pits/lowercase/tiles2.dat
            cp PITS95/TILES3.DAT $out/share/puzzle-pits/lowercase/tiles3.dat
            cp PITS95/PLASMA1.FNT $out/share/puzzle-pits/lowercase/plasma1.fnt
            cp PITS95/LOGO.DAT $out/share/puzzle-pits/lowercase/logo.dat
            cp PITS95/TITLE.DAT $out/share/puzzle-pits/lowercase/title.dat
          '';
        };
        
        # Common source files
        commonSources = [
          "DIG.C" "DMA.C" "ENV.C" "FILE.C" "GFX.C" 
          "LOAD.C" "SB.C" "SETMODE.C"
        ];
        
        # Build function
        buildPuzzlePits = { withSDL ? false }: pkgs.stdenv.mkDerivation {
          pname = "puzzle-pits${if withSDL then "-sdl" else ""}";
          version = "1.0";
          
          src = ./.;
          
          nativeBuildInputs = [ pkgs.gcc ] ++ 
            (if withSDL then [ pkgs.pkg-config ] else []);
          
          buildInputs = [ gameDataPackage ] ++ (if withSDL then [ pkgs.SDL2 ] else []);
          
          buildPhase = ''
            mkdir -p $out/bin
            
            # Set up game data files
            mkdir -p $out/bin/PITS95
            cp -r ${gameDataPackage}/share/puzzle-pits/* $out/bin/PITS95/
            
            # Copy lowercase versions for game compatibility
            cp ${gameDataPackage}/share/puzzle-pits/lowercase/* $out/bin/
            
            # Set compile flags
            CFLAGS="-Wall -O2 -I. -Wno-pointer-sign -fno-strict-aliasing -Wno-format-zero-length -std=gnu99"
            
            if [ "${toString withSDL}" = "1" ]; then
              CFLAGS="$CFLAGS -DUSE_SDL $(pkg-config --cflags sdl2)"
              LDFLAGS="$(pkg-config --libs sdl2)"
              # Compile SDL compatibility layer
              gcc $CFLAGS -x c -c ./sdl_compat.c -o sdl_compat.o
            else
              LDFLAGS=""
            fi
            
            # Compile common sources
            for src in ${toString commonSources}; do
              basename_no_ext=$(basename "$src" | cut -d. -f1)
              gcc $CFLAGS -x c -c ./$src -o $basename_no_ext.o
            done
            
            # Compile main executables
            gcc $CFLAGS -x c -c ./DEMO.C -o DEMO.o
            gcc $CFLAGS -x c -c ./PITS.C -o PITS.o
            
            # Build DEMO executable
            gcc -o $out/bin/puzzle-pits-demo \
              DEMO.o DIG.o DMA.o ENV.o FILE.o GFX.o LOAD.o \
              SB.o SETMODE.o \
              ${if withSDL then "sdl_compat.o" else ""} \
              $LDFLAGS
            
            # Build PITS executable  
            gcc -o $out/bin/puzzle-pits \
              PITS.o DIG.o DMA.o ENV.o FILE.o GFX.o LOAD.o \
              SB.o SETMODE.o \
              ${if withSDL then "sdl_compat.o" else ""} \
              $LDFLAGS

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
          puzzle-pits-data = gameDataPackage;
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
              echo "Or compile with: gcc -DUSE_SDL ..."
              echo ""
              echo "Game data will be automatically downloaded from archive.org"
            '';
          };
          
          nosdl = pkgs.mkShell {
            buildInputs = [ pkgs.gcc ];
            shellHook = ''
              echo "Development shell for The Puzzle Pits (no SDL)"
              echo "Build with: nix build .#puzzle-pits-nosdl"
              echo "Or compile with: gcc ..."
              echo ""
              echo "Game data will be automatically downloaded from archive.org"
            '';
          };
        };
      }
    );
}