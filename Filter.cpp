#include "Filter.h"
#include <iostream>

Filter::Filter(int _dim)
{
  divisor = 1;
  dim = _dim;
  data = new char[9];
}

int Filter::get(int i)
{
  return data[i];
}

void Filter::set(int r, int c, int value)
{
  data[r*3+c] = value;
}

int Filter::getDivisor()
{
  return divisor;
}

void Filter::setDivisor(int value)
{
  divisor = value;
}

int Filter::getSize()
{
  return dim;
}

void Filter::info()
{
  cout << "Filter is.." << endl;
  for (int col = 0; col < dim; col++) {
    for (int row = 0; row < dim; row++) {
      int v = get(row*3 + col);
      cout << v << " ";
    }
    cout << endl;
  }
}
