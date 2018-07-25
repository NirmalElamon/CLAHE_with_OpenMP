
CC=g++

CFLAGS= \
-O2 \
-I/usr/local/include \
-I../openmp/include \
-Xpreprocessor -fopenmp \
-static

LIBS= \
-L/usr/local/lib \
-lopencv_core \
-lopencv_imgproc \
-lopencv_features2d \
-lopencv_highgui \
-lopencv_imgcodecs \
-lgomp -liomp5 -lomp

all: clahe

#
# clahe
#    Apply the CLAHE histogram equalization to an image
#
clahe: clahe.o
	$(CC) -o clahe clahe.o $(LIBS)

clahe.o: clahe.cpp
	$(CC) $(CFLAGS) -c clahe.cpp

#
# Install
#
install:
	../install_tool_mac.sh clahe ../bin


#
# Clean up
#
clean :
	rm -f clahe
	rm -f *.o
	


