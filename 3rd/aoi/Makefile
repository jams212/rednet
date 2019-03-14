CXX := g++
AR := ar

CXXFLAG := -O2 -W -Wall 

INC_PATH := -I./

TARGET = ./libaoi.a
OBJS = aoi.o
SRCS = aoi.cpp


 
linux : $(TARGET)
$(OBJS): $(SRCS)
	$(CXX) $(CXXFLAG) $(INC_PATH) -c $(SRCS)

test :
	g++ -o aoi -g -Wall -std=c++11 aoi.cpp test.cpp

$(TARGET) : $(OBJS)
	$(AR) cq $@ $^
	rm -f *.o



clean :
	rm -f *.a aoi *.o

