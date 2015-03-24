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


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

    long long cycStart, cycStop;
    cycStart = rdtscll();

    int cache_filter[3][3];
    #pragma omp parallel for
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            cache_filter[i][j] =  filter -> get(i,j);
        }
    }

    int row_size = input -> height - 1;
    int col_size = input -> width  - 1;
    int filter_divisor = filter -> getDivisor();
    #pragma omp parallel for
    for(int plane = 2; plane >= 0 ; --plane ) { //best place for multicore performance benefits.
        for(int row = row_size; row > 0; --row ) {
            for(int col = col_size; col > 0; --col ) {
                int new_pixel;
                new_pixel  = input -> color[plane][row    ][col    ] * cache_filter[0][0];
                new_pixel += input -> color[plane][row    ][col + 1] * cache_filter[0][1];
                new_pixel += input -> color[plane][row    ][col + 2] * cache_filter[0][2];
                new_pixel += input -> color[plane][row + 1][col    ] * cache_filter[1][0];
                new_pixel += input -> color[plane][row + 1][col + 1] * cache_filter[1][1];
                new_pixel += input -> color[plane][row + 1][col + 2] * cache_filter[1][2];
                new_pixel += input -> color[plane][row + 2][col    ] * cache_filter[2][0];
                new_pixel += input -> color[plane][row + 2][col + 1] * cache_filter[2][1];
                new_pixel += input -> color[plane][row + 2][col + 2] * cache_filter[2][2];

                new_pixel = new_pixel / filter_divisor;

                new_pixel = new_pixel < 0   ? 0   : new_pixel;
                new_pixel = new_pixel > 255 ? 255 : new_pixel;

                output -> color[plane][row][col] = new_pixel;
            }
        }
    }

    output -> width = input -> width;
    output -> height = input -> height;

    cycStop = rdtscll();
    double diff = cycStop - cycStart;
    double diffPerPixel = diff / (output -> width * output -> height);
    fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
        diff, diff / (output -> width * output -> height));
    return diffPerPixel;
}
