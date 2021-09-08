EXT = .exe
GCC = x86_64-w64-mingw32-gcc
FLAGS = -Wall

layerPause:
	$(GCC) $(FLAGS) layerPause.c -o layerPause$(EXT)
