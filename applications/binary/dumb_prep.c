#include <asm-generic/fcntl.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <inttypes.h>

#include "../../include/cryptocore_ioctl_header.h"

/* Prototypes for functions used to access physical memory addresses */
int open_physical (int);
void close_physical (int);

int main(void)
{
	int dd = -1;
	int ret_val;

	__u32 trng_val = 0;
	__u32 i = 0;
	__u32 rw_prec = 0;

	double seconds;
	struct timespec tstart={0,0}, tend={0,0};

	if ((dd = open_physical (dd)) == -1)
      return (-1);

    usleep(10);

    Prep_params_t Prep_163_test = { 163,
	0,
	0,
	{ 0x00000080, 0x00000000, 0x00000000, 0x00000000,
	  0x00000000, 0x00000107                         },
	{ 12 },
	{ 24 },
	{  },
	{  },
	{  },
	};

    if(Prep_163_test.prec % 32) {
		rw_prec = (Prep_163_test.prec/32 + 1) * 32;
	} else {
		rw_prec = Prep_163_test.prec;
	}

    printf("A: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", Prep_163_test.a[i]);
	}
	printf("\n\n");

    printf("B: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", Prep_163_test.b[i]);
	}
	printf("\n\n");

    printf("N: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", Prep_163_test.n[i]);
	}
	printf("\n\n");	

    ret_val = ioctl(dd, IOCTL_MWMAC_PREP, &Prep_163_test);
	if(ret_val != 0) {
		printf("Error occured 10\n");
	}

    printf("C = Prep(A,B,N): 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", Prep_163_test.c[i]);
	}
    printf("\n\n");
    close_physical (dd);   // close /dev/cryptocore
	return 0;
}

// Open /dev/cryptocore, if not already done, to give access to physical addresses
int open_physical (int dd)
{
   if (dd == -1)
      if ((dd = open( "/dev/cryptocore", (O_RDWR | O_SYNC))) == -1)
      {
         printf ("ERROR: could not open \"/dev/cryptocore\"...\n");
         return (-1);
      }
   return dd;
}

// Close /dev/cryptocore to give access to physical addresses
void close_physical (int dd)
{
   close (dd);
}
