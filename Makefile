all:
	gcc ./src/main.c -o quirkwm -ltoml -lX11 

clean:
	rm -rf quirkwm

