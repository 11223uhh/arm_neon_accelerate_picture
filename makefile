C = gcc

Cpp = g++
TARGET = test



LIBRARY_PATH=$(wildcard ./mingw/bin/*.dll)

OBJS = test.o  yuv_bmp.o nv12.o bmp_yuv_420.o
INCLUDE = -Iinclude
$(TARGET):$(OBJS)
	$(Cpp) $(OBJS) -o $(TARGET)    $(LIBRARY_PATH) -std=c++14 -mfma -Wno-error=deprecated-declarations -Wno-deprecated-declarations 
%.o:%.c
	$(C) $(INCLUDE) -c $^ -o $@  $(LIBRARY_PATH)   -mfma -Wno-error=deprecated-declarations -Wno-deprecated-declarations  

%.o:%.cpp
	$(Cpp) $(INCLUDE) -c $^ -o $@  $(LIBRARY_PATH)  -std=c++14 -mfma -Wno-error=deprecated-declarations -Wno-deprecated-declarations                        

