clang qaim.c -Ofast -mfma -lX11 -lxdo -lespeak -lm -o aim
strip --strip-unneeded aim
upx --lzma --best aim
./aim
