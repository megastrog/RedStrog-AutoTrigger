clang qaim.c -Ofast -mfma -lX11 -lxdo -lespeak -lm -o aim
strip --strip-unneeded aim
upx --best aim
sleep 1
./aim
