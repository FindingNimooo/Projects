//Filename: main.cpp
//Assignment: Project 3: RISC-V Pipeline Simulator Using C++
//Name: Peter Estacio
//Grouped with Ni�o De Mesa and Osh Ong
//Section: ENGG 123.01 - J1


#include "simulator.h"

#include <iostream>

int main()
{
  Simulator simulator;
  simulator.run(std::cin, std::cout);
  return 0;
}
