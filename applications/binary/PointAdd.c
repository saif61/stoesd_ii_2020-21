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

    PointAdd_params_t PointAdd_163_test = { 163,
	0,
	0,
	{ 0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000107 }, // n  : 0x080000000000000000000000000000000000000107
	{ 0x00000007, 0x2546b543, 0x5234a422, 0xe0789675, 0xf432c894, 0x35de5242 }, // a  : 0x072546b5435234a422e0789675f432c89435de5242
	{ 0x00000000, 0xc9517d06, 0xd5240d3c, 0xff38c74b, 0x20b6cd4d, 0x6f9dd4d9 }, // b  : 0x00c9517d06d5240d3cff38c74b20b6cd4d6f9dd4d9
	{ 0x00000000, 0x5973f2ce, 0x9481d196, 0x43a811a8, 0x97599e67, 0x52b62834 }, // xP : 0x5973f2ce9481d19643a811a897599e6752b62834
	{ 0x00000006, 0x55ebc626, 0xa6197260, 0x18b03f84, 0x730e81a6, 0x19142c94 }, // yP : 0x655ebc626a619726018b03f84730e81a619142c94
	{ }, // zP : 1, montgomerized and will be copied from R
	{ 0x00000005, 0x25bd3cdd, 0xa7c989f2, 0x1652da0d, 0xe9c7a5d3, 0x5ae62f1f }, // xQ : 0x525bd3cdda7c989f21652da0de9c7a5d35ae62f1f
	{ 0x00000005, 0x4972dfe4, 0x2d888584, 0xd8cfce07, 0x57f7b0a6, 0x4801ae68 }, // yQ : 0x54972dfe42d888584d8cfce0757f7b0a64801ae68
	{ }, // zQ : 1, montgomerized and will be copied from R
	{  }, // r_p
	{  }, // r_b
	{  }, // r_a
	{  }, // r_e
	{  }, // r_x
	};


    if(PointAdd_163_test.prec % 32) {
		rw_prec = (PointAdd_163_test.prec/32 + 1) * 32;
	} else {
		rw_prec = PointAdd_163_test.prec;
	}

    printf("A: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.a[i]);
		//printf("I: %x",i);

	}
	printf("\n\n");

    printf("B: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.b[i]);
	}
	printf("\n\n");

    printf("N: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.n[i]);
	}
	printf("\n\n");	

	printf("X_P: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.x_p[i]);
	}
	printf("\n\n");

	printf("Y_P: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.y_p[i]);
	}
	printf("\n\n");
	
	printf("X_Q: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.x_q[i]);
	}
	printf("\n\n");

	printf("Y_Q: 0x");
	for(i=0; i<rw_prec/32; i++){
		printf("%08x", PointAdd_163_test.y_q[i]);
	}
	printf("\n\n");

    ret_val = ioctl(dd, IOCTL_MWMAC_PADD, &PointAdd_163_test);
	if(ret_val != 0) {
		printf("Error occured 10\n");
	}




// Printing RAM
    printf("r_b: 0x");
	for(i=0; i<128; i++){	// change: rw_prec/32 --> 128
		printf("%x", PointAdd_163_test.r_b[i]);
	}
    printf("\n\n");

	printf("r_p: 0x");
	for(i=0; i<128; i++){
		printf("%x", PointAdd_163_test.r_p[i]);
	}
    printf("\n\n");

	printf("r_a: 0x");
	for(i=0; i<128; i++){
		printf("%x", PointAdd_163_test.r_a[i]);
	}
    printf("\n\n");

	printf("r_e: 0x");
	for(i=0; i<128; i++){
		printf("%x", PointAdd_163_test.r_e[i]);
	}
    printf("\n\n");

	printf("r_x: 0x");
	for(i=0; i<128; i++){
		printf("%x", PointAdd_163_test.r_x[i]);
	}
    printf("\n\n");
	

/*
	//all memory locations
	for(i=0; i<128; i++){
		printf(" %x \t %x \t %x \t %x \n", 0x0+i*0x4, 0x1+i*0x4, 0x2+i*0x4, 0x3+i*0x4);
	}
	printf("\n\n");
	for(i=0; i<128; i++){
		printf(" %x \t %x \t %x \n", 0x200+i, 0x280+i, 0x300+i);
	}
*/


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
