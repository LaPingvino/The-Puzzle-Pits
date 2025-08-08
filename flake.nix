{
  description = "The Puzzle Pits - 1995 DOS puzzle game ported to modern systems";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        
        # Game data package - downloads original assets
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
            
            # Create lowercase versions for Linux compatibility
            mkdir -p $out/share/puzzle-pits/lowercase
            for file in PITS95/*.DAT PITS95/*.FNT; do
              if [ -f "$file" ]; then
                basename_lower=$(basename "$file" | tr '[:upper:]' '[:lower:]')
                cp "$file" "$out/share/puzzle-pits/lowercase/$basename_lower"
              fi
            done
            
            # Create lowercase levels directory
            if [ -d "PITS95/LEVELS" ]; then
              mkdir -p $out/share/puzzle-pits/lowercase/levels
              for level in PITS95/LEVELS/*.PIT; do
                if [ -f "$level" ]; then
                  basename_lower=$(basename "$level" | tr '[:upper:]' '[:lower:]')
                  cp "$level" "$out/share/puzzle-pits/lowercase/levels/$basename_lower"
                fi
              done
            fi
          '';
        };
        
        # Common build function
        buildPuzzlePits = { withSDL ? false }: pkgs.stdenv.mkDerivation {
          pname = "puzzle-pits${if withSDL then "-sdl" else ""}";
          version = "1.0";
          
          src = ./.;
          
          nativeBuildInputs = [ pkgs.zig ] ++ 
            (if withSDL then [ pkgs.pkg-config ] else []);
          
          buildInputs = (if withSDL then [ pkgs.SDL2 ] else []);
          
          buildPhase = ''
            export HOME=$TMPDIR
            
            # Build using unified Zig build system
            ${if withSDL then "zig build" else "zig build -Dsdl=false"}
          '';
          
          installPhase = ''
            mkdir -p $out/bin
            
            # Copy executables
            cp zig-out/bin/* $out/bin/
            
            # Copy game data
            mkdir -p $out/share/puzzle-pits
            cp -r ${gameDataPackage}/share/puzzle-pits/* $out/share/puzzle-pits/
            
            # Create wrapper scripts that set up game data
            for exe in $out/bin/puzzle-pits*; do
              if [ -f "$exe" ]; then
                mv "$exe" "$exe.real"
                cat > "$exe" << EOF
#!/bin/bash
cd "\$(dirname "\$0")/../share/puzzle-pits"
exec "\$(dirname "\$0")/\$(basename "$exe").real" "\$@"
EOF
                chmod +x "$exe"
              fi
            done
          '';
          
          meta = with pkgs.lib; {
            description = "The Puzzle Pits - Classic 1995 DOS puzzle game";
            homepage = "https://github.com/your-repo/the-puzzle-pits";
            license = licenses.unfree; # Original game license
            platforms = platforms.unix;
            maintainers = [ ];
          };
        };
      in {
        # Main packages
        packages = {
          default = self.packages.${system}.puzzle-pits-sdl;
          puzzle-pits-sdl = buildPuzzlePits { withSDL = true; };
          puzzle-pits = buildPuzzlePits { withSDL = false; };
          puzzle-pits-data = gameDataPackage;
        };
        
        # Development environments
        devShells = {
          default = self.devShells.${system}.sdl;
          
          # Full development environment with SDL2
          sdl = pkgs.mkShell {
            buildInputs = with pkgs; [
              zig
              SDL2
              pkg-config
              gcc
              curl
              unzip
            ];
            
            shellHook = ''
              echo "🎮 The Puzzle Pits Development Environment (SDL2)"
              echo ""
              echo "Quick start:"
              echo "  make setup     # Download game data and build"
              echo "  make run       # Run the game"
              echo ""
              echo "Build commands:"
              echo "  make sdl       # Build with SDL2 (full game)"
              echo "  make nosdl     # Build without SDL2 (stub mode)"
              echo "  make test      # Run tests"
              echo "  make clean     # Clean artifacts"
              echo ""
              echo "All commands use the unified Zig build system."
              echo ""
              
              # Auto-download game data if not present
              if [ ! -d "PITS95" ] && [ ! -f "tiles1.dat" ]; then
                echo "⬇️  Auto-downloading game data..."
                make download-data
                echo ""
              fi
            '';
          };
          
          # Minimal development environment without SDL2
          minimal = pkgs.mkShell {
            buildInputs = with pkgs; [
              zig
              gcc
              curl
              unzip
            ];
            
            shellHook = ''
              echo "🔧 The Puzzle Pits Development Environment (Minimal)"
              echo ""
              echo "Quick start:"
              echo "  make nosdl     # Build without SDL2"
              echo "  make test      # Run tests"
              echo ""
              echo "Note: SDL2 not available in this shell."
              echo "Use 'nix develop' for full SDL2 support."
              echo ""
              
              # Auto-download game data if not present
              if [ ! -d "PITS95" ] && [ ! -f "tiles1.dat" ]; then
                echo "⬇️  Auto-downloading game data..."
                make download-data
                echo ""
              fi
            '';
          };
        };
        
        # Development apps
        apps = {
          default = self.apps.${system}.puzzle-pits;
          
          puzzle-pits = {
            type = "app";
            program = "${self.packages.${system}.puzzle-pits-sdl}/bin/puzzle-pits";
          };
          
          demo = {
            type = "app";
            program = "${self.packages.${system}.puzzle-pits-sdl}/bin/puzzle-pits-demo";
          };
        };
        
        # Hydra/CI jobs
        checks = {
          puzzle-pits-sdl = self.packages.${system}.puzzle-pits-sdl;
          puzzle-pits = self.packages.${system}.puzzle-pits;
          
          # Test build without SDL2
          test-build-nosdl = pkgs.runCommand "test-build-nosdl" {
            buildInputs = [ pkgs.zig ];
          } ''
            cp -r ${./.} source
            cd source
            chmod -R u+w .
            export HOME=$TMPDIR
            zig build -Dsdl=false
            touch $out
          '';
        };
      }
    );
}