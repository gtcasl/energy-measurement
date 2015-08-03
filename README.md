# energy-measurement
Information about measuring energy on platforms and any tools used to change system behavior relating to energy


The benchmark application gives the user control over the number of compute-intensive and memory-bound operations.
Also, allows the user to bundle up blocks of operations of each type.

To compile: g++ benchmark.cpp -o benchmark-I/usr/local/include -L/usr/local/lib -lpapi -std=c++11

To run: sudo ./benchmark parameter a parameter b parameter c parameter d

Parameter a is the factor that decides the size of each block

Parameter b is the fraction of compute-intensive operations

Parameter c is the fraction of memory-bound operations

Parameter d is the factor that decides the number of iterations of the compute-intensive and memory-bound blocks

PAPI helper functions have been included. The required events can be added in main. Only L1_TCM has been added already.

The program gives the counter values of the requested events and the execution time of the program.

The thread has been given an affinity to a single core (Core 0). This needs to be replaced with OpenMP parallelization.
