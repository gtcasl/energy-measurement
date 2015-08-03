#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <math.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <memory.h>
#include <malloc.h>
#include <vector>
#include "papi.h"

using namespace std;


void print_usage()
{
    std::cerr << "Usage: ./benchmark <parameter a> <parameter b> <parameter c> <parameter d>" << std::endl;
    std::cerr << "                  Parameter a is the factor that decides the size of each block" << std::endl;
    std::cerr << "                  Parameter b is the fraction of compute-intensive operations" << std::endl;
    std::cerr << "                  Parameter c is the fraction of memory-bound operations" << std::endl;
    std::cerr << "                  Parameter d is the factor that decides the number of iterations" << std::endl;
    std::cerr << "                  of the compute-intensive and memory-bound blocks"  << std::endl;
}


///helper functions
//namespace papi{
struct event_info_t{
  int component;
  int set;
  std::vector<int> codes;
  std::vector<std::string> names;
};

std::vector<event_info_t> init_papi_counters(
    const std::vector<std::string>& event_names) {
  std::vector<event_info_t> eventsets;
  int retval;
  for (auto& event_name : event_names) {
    int event_code;
    retval = PAPI_event_name_to_code(const_cast<char*>(event_name.c_str()),
                                     &event_code);
    //printf("code=%d",event_code);
    if (retval != PAPI_OK) {
      std::cerr << "Error: bad PAPI event name \"" << event_name << "\" to code\n";
      PAPI_perror(NULL);
      exit(-1);
    }
    int component = PAPI_get_event_component(event_code);
    auto elem = find_if(
        begin(eventsets), end(eventsets),
        [&](const event_info_t& c) { return c.component == component; });
    if (elem == end(eventsets)) {
      event_info_t new_info;
      new_info.component = component;
      new_info.codes.push_back(event_code);
      new_info.names.emplace_back(event_name);
      eventsets.push_back(new_info);
    } else {
      elem->codes.push_back(event_code);
      elem->names.emplace_back(event_name);
    }
  }

  for (auto& event : eventsets) {
    int eventset = PAPI_NULL;
    retval = PAPI_create_eventset(&eventset);
    if (retval != PAPI_OK) {
      std::cerr << "Error: bad PAPI create eventset: ";
      PAPI_perror(NULL);
      exit(-1);
    }
    retval = PAPI_add_events(eventset, &event.codes[0], event.codes.size());
    if (retval != PAPI_OK) {
      std::cerr << "Error: bad PAPI add eventset: ";
      PAPI_perror(NULL);
      exit(-1);
    }
    event.set = eventset;
  }
  return eventsets;
}

void start_counters(const std::vector<event_info_t>& counters){
  for(auto& counter : counters){
    auto retval = PAPI_start(counter.set);
    if (retval != PAPI_OK) {
      std::cerr << "Error: bad PAPI start eventset: ";
      PAPI_perror(NULL);
      exit(-1);
    }
  }
}

std::vector<std::vector<long long>> stop_counters(const std::vector<event_info_t>& counters){
  std::vector<std::vector<long long>> results;
  for(const auto& counter : counters){
    std::vector<long long> counter_results(counter.codes.size());
    auto retval = PAPI_stop(counter.set, &counter_results[0]);
    if(retval != PAPI_OK){
      std::cerr << "Error: bad PAPI stop eventset: ";
      PAPI_perror(NULL);
      exit(-1);
    }
    results.push_back(counter_results);
  }
  return results;
}

void attach_counters_to_core(const std::vector<event_info_t>& counters, int cpu_num) {
  for(auto& counter : counters){
    PAPI_option_t options;
    options.cpu.eventset = counter.set;
    options.cpu.cpu_num = cpu_num;
    int retval = PAPI_set_opt(PAPI_CPU_ATTACH, &options);
    if(retval != PAPI_OK) {
      std::cerr << "Error: unable to CPU_ATTACH core " << cpu_num << ": ";
      PAPI_perror(NULL);
      exit(-1);
    }
  }
}

int main(int argc, char **argv)
{

    if (argc != 5)
    {
      print_usage();
      exit(EXIT_FAILURE);
    }
    else
    {
    const long MUL_FACTOR = atoi(argv[1]);
    const int COMPUTE_FR = atoi(argv[2]);
    const int MEMORY_FR = atoi(argv[3]);
    const long ITERATIVE_FACTOR = atoi(argv[4]);

    const long n = ITERATIVE_FACTOR;
    const long x = COMPUTE_FR * MUL_FACTOR;
    const long y = MEMORY_FR * MUL_FACTOR;


//********************************* SETTING CORE AFFINITY *******************************************
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0,&mask);
    if (sched_setaffinity(0, sizeof(mask), &mask)<0)
        printf("Error");


    int i,j,k;
    double a,b,c;
    double* M = (double*)std::malloc(y*sizeof(int));
    struct timeval start, end;


   vector<string>ctr_names;
   vector<event_info_t>events_info;
   std::vector<std::vector<long long>> results;
//   ctr_names.push_back("PAPI_L1_TCM");
//   ctr_names.push_back("PAPI_L1_ICM");
     ctr_names.push_back("PAPI_L1_TCH");
//   ctr_names.push_back("rapl:::PP0_ENERGY:PACKAGE0");


    printf("Init PAPI\n");
    int retval;
    if ((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
     cerr << "Unable to init PAPI library - " << PAPI_strerror(retval) << endl;
     exit(-1);
    }

   //initializing PAPI counters

    events_info=init_papi_counters(ctr_names);

   //starting PAPI counters
    start_counters(events_info);

   //performing operations

    gettimeofday(&start, NULL);

    for (i=0; i<n; i++)
    {
        srand(time(NULL));
        a = rand() % x;
        b = rand() % x;
//      ********************************* COMPUTATE BOUND OPERATIONS *****************************
        for (j=0; j<x; j++)
        {
            c = a * b;
        }
//      ********************************* MEMORY BOUND OPERATIONS *******************************
        for (j=0; j<y; j++)
        {
            srand(time(NULL));
            c = M[rand() % y];
        }
    }

    gettimeofday(&end, NULL);

    printf("Running time: %ld us\n", ((end.tv_sec * 1000000 + end.tv_usec)
		  - (start.tv_sec * 1000000 + start.tv_usec)));


    //stopping PAPI counters
    results = stop_counters(events_info);
    printf("the counters stopped successfully\n");

    for(j=0;j<results.size();j++)
    {
        for(k=0;k<results[j].size();k++)
            printf("ctr value =%lu  j=%d    k=%d \n",results[j][k],j,k);
    }


    }
    //return 0;

}
