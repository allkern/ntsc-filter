$SDL2_INCLUDE_DIR = ""
$SDL2_LIB_DIR = ""
$LGW_INCLUDE_DIR = ""

c++ -c main.cpp -o main.o -I"`"$($SDL2_INCLUDE_DIR)`"" -I"`"$($LGW_INCLUDE_DIR)`"" -fopenmp

c++ main.o -o main.exe -L"`"$($SDL2_LIB_DIR)`"" -lSDL2main -lSDL2 -fopenmp