# Compiler and flags
CC = gcc

compile:
	gcc -o encode encode.c -IC:~\SSRL\vcpkg\installed\x64-windows\include -IC:~\SSRL\vcpkg\packages\libpng_x64-windows\include -LC:~\SSRL\vcpkg\packages\libpng_x64-windows\lib  -LC:~\SSRL\vcpkg\installed\x64-windows\lib  -lz -lpng
	gcc -o decode decode.c -IC:~\SSRL\vcpkg\installed\x64-windows\include -IC:~\SSRL\vcpkg\packages\libpng_x64-windows\include -LC:~\SSRL\vcpkg\packages\libpng_x64-windows\lib  -LC:~\SSRL\vcpkg\installed\x64-windows\lib  -lz -lpng

run:
	./encode
	./decode

clean:
	del /f encode.exe encode
	del /f decode.exe decode