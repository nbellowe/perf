#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rtdsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

    if ( argc < 2) {
        fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
    }

    //
    // Convert to C++ strings to simplify manipulation
    //
    string filtername = argv[1];

    //
    // remove any ".filter" in the filtername
    //
    string filterOutputName = filtername;
    string::size_type loc = filterOutputName.find(".filter");
    if (loc != string::npos) {
        //
        // Remove the ".filter" name, which should occur on all the provided filters
        //
        filterOutputName = filtername.substr(0, loc);
    }

    Filter *filter = readFilter(filtername);

    double sum = 0.0;
    int samples = 0;

    for (int inNum = 2; inNum < argc; inNum++) {
        string inputFilename = argv[inNum];
        string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
        struct cs1300bmp *input = new struct cs1300bmp;
        struct cs1300bmp *output = new struct cs1300bmp;
        int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

        if ( ok ) {
            double sample = applyFilter(filter, input, output);
            sum += sample;
            samples++;
            cs1300bmp_writefile((char *) outputFilename.c_str(), output);
        }
        delete input;
        delete output;
    }
    fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
    ifstream input(filename.c_str());

    if ( ! input.bad() ) {
        int size = 0;
        input >> size;
        Filter *filter = new Filter(size);
        int div;
        input >> div;
        filter -> setDivisor(div);
        for (int i=0; i < size; i++) {
            for (int j=0; j < size; j++) {
                int value;
                input >> value;
                filter -> set(i,j,value);
            }
        }
        return filter;
    }
}

;
double
applyFilter(struct Filter *_filter, cs1300bmp *input, cs1300bmp *output)
{

    long long cycStart, cycStop;
    cycStart = rdtscll();

    char *filter = _filter -> data;
    const short filter_divisor = _filter -> getDivisor(); //save clock cycles later on.

    const short in_height = input -> height; //these are used in row_size and also for final output
    const short in_width  = input -> width;

    const short row_size = in_height - 1; //we use this rather than recompute every time we are checking the condition of the next for loops.
    const short col_size = in_width  - 1;

    //note the order is best for pipelining.
    #pragma omp parallel for
    for(short plane = 2; plane >= 0 ; --plane ) { //best place for multicore performance benefits.
        for(short row = row_size, row_one = row+1, row_two=row+2; row > 0; --row, row_two=row_one-- ) {
            for(short col = col_size, col_one = col_size+1, col_two = col_size+2; col > 0; --col, col_two=col_one-- ) {o
                //new_pixel will be initialized each time.
                short new_pixel; //needs to be short even though working with chars, as char + char can overflow before division.

                //Unrolled the two loops that are always 3 * 3 and made filter into a single dimensional array.
                //I could speed this up farther, if I could figure out how to make a pointer to a triple dimensional array that is cached, e.g:
                //unsigned char (*ptr)[8192][8192] = &(input -> color);
                //ptr[plane][row][col]
                //
                //Also, I didn't want to have to recalclulate col+1, col+2 multiple times and row+1, row+2 so I cached col+1 up in the for loop
                //initialization/increment statements. I did this weird: col_two=col_one--; is equivalent to col_two=col_one; --col_one
                new_pixel  = input -> color[plane][row    ][col    ] * filter[0];
                new_pixel += input -> color[plane][row    ][col_one] * filter[1];
                new_pixel += input -> color[plane][row    ][col_two] * filter[2];
                new_pixel += input -> color[plane][row_one][col    ] * filter[3];
                new_pixel += input -> color[plane][row_one][col_one] * filter[4];
                new_pixel += input -> color[plane][row_one][col_two] * filter[5];
                new_pixel += input -> color[plane][row_two][col    ] * filter[6];
                new_pixel += input -> color[plane][row_two][col_one] * filter[7];
                new_pixel += input -> color[plane][row_two][col_two] * filter[8];

                // Now divide it.
                new_pixel /= filter_divisor;

                // I read somewhere conditionals are slightly faster than true if else... I'm not sure, but this is just a max/min function
                // First checks if it is less than zero.
                output -> color[plane][row][col] = new_pixel < 0   ? 0   : (new_pixel > 255 ? 255 : new_pixel);
            }
        }
    }

    output -> width = in_width;
    output -> height = in_height;

    cycStop = rdtscll();
    double diff = cycStop - cycStart;
    double diffPerPixel = diff / (output -> width * output -> height);
    fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
        diff, diff / (output -> width * output -> height));
    return diffPerPixel;
}
