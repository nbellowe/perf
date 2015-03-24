CXX	=g++
##
## Use our standard compiler flags for the course...
##
CXXFLAGS= -m32 -g -funroll-loops -fno-omit-frame-pointer -O3 -fopenmp

goals: judge
	echo "Done"

filter: FilterMain.cpp Filter.cpp cs1300bmp.cc
	$(CXX) $(CXXFLAGS) -o filter FilterMain.cpp Filter.cpp cs1300bmp.cc
##
## Parameters for the test run
##
FILTERS = filters/gauss.filter filters/vline.filter filters/hline.filter filters/emboss.filter
IMAGES = pics/boats.bmp pics/blocks-small.bmp
TRIALS = 1 2 3 4

judge: filter
	-./Judge -p ./filter -i pics/boats.bmp
	-./Judge -p ./filter -i pics/blocks-small.bmp

clean:
	-rm filter
	-rm filtered-*.bmp

valgrind: filter
	-valgrind --tool=callgrind --simulate-cache=yes --collect-jumps=yes ./filter filters/gauss.filter pics/blocks-small.bmp
