#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <memory.h>
#include <malloc.h>
#include "papi.h"
#include <iostream>

//#include "papi-helpers.hpp"

long getFrequency(int cpu_id){
	char ffile[60];
	// Since Interlagos cpus share a frequency domain within the module,
	// you have to set both scaling_setspeed files for any change to
	// take effect.
	sprintf(ffile,
					"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
					cpu_id);
	FILE* fp  = fopen(ffile, "r");
	if(!fp){
		fprintf(stderr, "Error: Unable to open cpufreq file %s\n", ffile);
		return 0;
	}
	long res;
	fscanf(fp, "%ld", &res);
	fclose(fp);
	return res;

}

int setFrequency(int cpu_id, long freq){
	char ffile[60];
	// Since Interlagos cpus share a frequency domain within the module,
	// you have to set both scaling_setspeed files for any change to
	// take effect.
    int i=cpu_id;
	while(getFrequency(cpu_id) != freq){
		while( i==cpu_id ){
            printf("entred\n");
        //for(int i = cpu_id; i <= cpu_id+4; i+4){
			sprintf(ffile,
							"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed",
							i);
			FILE* fp  = fopen(ffile, "w");
			if(!fp){
				fprintf(stderr, "Error: Unable to open cpufreq file %s\n", ffile);
				return 0;
			}
			fprintf(fp, "%ld", freq);
			fclose(fp);
            i++;
		}
	}
	return 1;
}

int main()
{
    long fq_current,fq_target;
    fq_target=800000;
    int node_id=0;
    fq_current=getFrequency(node_id);
    printf("the current proc freq is:%lu \n",fq_current);
    setFrequency(node_id,fq_target);
    printf("freq set successfully to:%lu \n", fq_target);
    fq_current=getFrequency(node_id);
    printf("the proc freq after setting is:%lu \n",fq_current);
    return 0;
}
