CC=g++
LDIRS=/usr/local/lib
LFLAGS=-lpng -lc++
FRAMEWORK_DIR=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks

all: mandelbrot info
#all: opencl

%.o: %.cpp *.h
	${CC} -c -o $@ $<

mandelbrot: main.o opencl.o image.o color.o animation.o spline.o
	${CC} -o mandelbrot -F${FRAMEWORK_DIR} -framework OpenCL $^ ${LFLAGS}

info: opencl_info.cpp
	${CC} -o info -F${FRAMEWORK_DIR} -framework OpenCL $^

spline: spline.cpp
	${CC} -o spline $^

clean:
	rm -rf *.o
	rm -rf mandelbrot
	rm -rf info
	rm -rf spline