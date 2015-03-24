##
##

CXX= g++
##
## Use our standard compiler flags for the course...
##
#CXXFLAGS= -m32 -g -O3 -fno-omit-frame-pointer
CXXFLAGS= -m32 -g -fno-omit-frame-pointer -O3 -fopenmp

goals: judge
		echo "Done"

filter: FilterMain.cpp Filter.cpp cs1300bmp.cc
	$(CXX) $(CXXFLAGS) -o filter FilterMain.cpp Filter.cpp cs1300bmp.cc
	##
	## Parameters for the test run
	##
	-FILTERS= gauss.filter vline.filter hline.filter emboss.filter
	-IMAGES= boats.bmp blocks-small.bmp
	-TRIALS= 1 2 3 4

judge: filter
	-./Judge -p ./filter -i boats.bmp
	-./Judge -p ./filter -i blocks-small.bmp

clean:
	-rm *.o
	-rm filter
	-rm filtered-*.bmp

valgrind: filter
	-valgrind --tool=callgrind --simulate-cache=yes --collect-jumps=yes ./filter gausss.filter blocks-small.bmp


