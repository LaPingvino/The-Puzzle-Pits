# The Puzzle Pits: the build is build.zig; these are shortcuts to it.

.PHONY: all nosdl run download-data clean

all:
	zig build

nosdl:
	zig build -Dsdl=false

run:
	zig build run

download-data:
	zig build download-data

clean:
	rm -rf .zig-cache zig-out
