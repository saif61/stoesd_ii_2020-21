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

    //for converting kk to bin
    int j,pos,m;
    __u32 word = 0x0;
    __u32 byte = 0x0;
    __u32 bit = 0x0;
    __u32 bits[rw_prec];
    
	double seconds;
	struct timespec tstart={0,0}, tend={0,0};

	if ((dd = open_physical (dd)) == -1)
      return (-1);

    usleep(10);

    
    // Prep_params_t Prep_163_test = { 163,
	// 0,
	// 0,
	// { 0x08, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000107 },
	// { 0x07, 0x2546b543, 0x5234a422, 0xe0789675, 0xf432c894, 0x35de5242 },
	// { 0x00, 0xc9517d06, 0xd5240d3c, 0xff38c74b, 0x20b6cd4d, 0x6f9dd4d9 },
	// { 0x00000000, 0x5973f2ce, 0x9481d196, 0x43a811a8, 0x97599e67, 0x52b62834 }, // xP : 0x5973f2ce9481d19643a811a897599e6752b62834
	// { 0x00000006, 0x55ebc626, 0xa6197260, 0x18b03f84, 0x730e81a6, 0x19142c94 }, // yP : 0x655ebc626a619726018b03f84730e81a619142c94

	// {  }, // r_p
	// {  }, // r_b
	// {  }, // r_a
	// {  }, // r_e
	// {  }, // r_x
	// };

    PointMult_params_t PointMult_163_test = { 163,
	0,
	0,
	{ 0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000107 }, // n  : 0x080000000000000000000000000000000000000107
	{ 0x00000007, 0x2546b543, 0x5234a422, 0xe0789675, 0xf432c894, 0x35de5242 }, // a  : 0x072546b5435234a422e0789675f432c89435de5242
	{ 0x00000000, 0xc9517d06, 0xd5240d3c, 0xff38c74b, 0x20b6cd4d, 0x6f9dd4d9 }, // b  : 0x00c9517d06d5240d3cff38c74b20b6cd4d6f9dd4d9
	{ 0x00000000, 0x5973f2ce, 0x9481d196, 0x43a811a8, 0x97599e67, 0x52b62834 }, // xP : 0x5973f2ce9481d19643a811a897599e6752b62834
	{ 0x00000006, 0x55ebc626, 0xa6197260, 0x18b03f84, 0x730e81a6, 0x19142c94 }, // yP : 0x655ebc626a619726018b03f84730e81a619142c94
	{  }, // r_p
	{  }, // r_b
	{  }, // r_a
	{  }, // r_e
	{  }, // r_x
    { 0x00000001, 0x4fa32024, 0x125ada45, 0x679c3ed8, 0x49eec551, 0x7313e4c6 }, //kk 0x14fa32024125ada45679c3ed849eec5517313e4c6 0b10100111110100011001000000010010000010010010110101101101001000101011001111001110000111110110110000100100111101110110001010101000101110011000100111110010011000110
	{ 0x00000007, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe }, // exp:	0x07fffffffffffffffffffffffffffffffffffffffe

	};

    PostOp_params_t PostOp_params_163_test = { 163,
	0,
	0,
	{  }, // n  : 0x080000000000000000000000000000000000000107
	{  }, // a  : 0x072546b5435234a422e0789675f432c89435de5242
	{  }, // b  : 0x00c9517d06d5240d3cff38c74b20b6cd4d6f9dd4d9
	{  }, // xP : 0x5973f2ce9481d19643a811a897599e6752b62834
	{  }, // yP : 0x655ebc626a619726018b03f84730e81a619142c94
	{  }, //zp
	{ 0x00000007, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfffffffe }, // exp:	0x07fffffffffffffffffffffffffffffffffffffffe
    {  }, // r_p
	{  }, // r_b
	{  }, // r_a
	{  }, // r_e
	{  }, // r_x
     //kk 0x14fa32024125ada45679c3ed849eec5517313e4c6 0b10100111110100011001000000010010000010010010110101101101001000101011001111001110000111110110110000100100111101110110001010101000101110011000100111110010011000110
	};

    if(PointMult_163_test.prec % 32) {
		rw_prec = (PointMult_163_test.prec/32 + 1) * 32;
	} else {
		rw_prec = PointMult_163_test.prec;
	}


//Calling PointMult function

	clock_gettime(CLOCK_MONOTONIC, &tstart);

    ret_val = ioctl(dd, IOCTL_MWMAC_PMULT, &PointMult_163_test);
	if(ret_val != 0) {
		printf("Error occured 11\n");
	}
	clock_gettime(CLOCK_MONOTONIC, &tend);

	seconds = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
	if (seconds*1000000.0 > 1000.0)
		printf("\n\nPointMult took about %.5f ms\n\n", seconds*1000.0);
	else 
		printf("\n\nPointMult took about %.5f us\n\n", seconds*1000000.0);	
	
//Printing all memory location before post operation
    
pos=0;
	for(i=0;i<8;i++)
	{
		printf("P%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%x", PointMult_163_test.r_p[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%08x", PointMult_163_test.r_p[pos],pos);
			}
			else{
				printf("");
			}
			pos++;		
		}
		printf("\n");
	}
	pos=0;
	for(i=0;i<8;i++)
	{
		printf("B%d: ",8-i);
		for(j=0;j<16;j++)
		{
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PointMult_163_test.r_b[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("A%d: ",8-i);
		for(j=0;j<16;j++)
		{
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PointMult_163_test.r_a[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("E%d: ",8-i);
		for(j=0;j<16;j++)
		{
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PointMult_163_test.r_e[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("X%d: ",8-i);
		for(j=0;j<16;j++)
		{
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PointMult_163_test.r_x[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}

//Calling Post operation
	clock_gettime(CLOCK_MONOTONIC, &tstart);

    ret_val = ioctl(dd, IOCTL_MWMAC_POSTOP, &PostOp_params_163_test);
	if(ret_val != 0) {
		printf("Error occured 13\n");
	}

	clock_gettime(CLOCK_MONOTONIC, &tend);

	seconds = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
	if (seconds*1000000.0 > 1000.0)
		printf("\n\nPost-Operation took about %.5f ms\n\n", seconds*1000.0);
	else 
		printf("\n\nPost-Operation 191 took about %.5f us\n\n", seconds*1000000.0);
//Printing all memory locations
	pos=0;

	for(i=0;i<8;i++)
	{
		printf("P%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%x", PostOp_params_163_test.r_p[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%08x", PostOp_params_163_test.r_p[pos],pos);
			}
			else{
				printf("");
			}
			
			pos++;		
		}
		printf("\n");
	}
	pos=0;
	for(i=0;i<8;i++)
	{
		printf("B%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%08x", PostOp_params_163_test.r_b[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PostOp_params_163_test.r_b[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("A%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%08x", PostOp_params_163_test.r_a[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PostOp_params_163_test.r_a[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("E%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%08x", PostOp_params_163_test.r_e[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PostOp_params_163_test.r_e[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}
	pos = 0;
	for(i=0;i<8;i++)
	{
		printf("X%d: ",8-i);
		for(j=0;j<16;j++)
		{
			//printf("%08x", PostOp_params_163_test.r_x[pos]);
			if(j>(16-rw_prec/32-1))
			{
				printf("%x", PostOp_params_163_test.r_x[pos],pos);
			}
			else{
				printf("");
			}
			pos++;
		}
		printf("\n");
	}



//////////////////////////////////////////////	\\\\\\\\\\\\\\\\\\\\\
/////////////////////////////////////////////	 \\\\\\\\\\\\\\\\\\\\\




////////////////////////////////////////////	  \\\\\\\\\\\\\\\\\\\\\
///////////////////////////////////////////		   \\\\\\\\\\\\\\\\\\\\\	
    
    close_physical (dd);   // close /dev/cryptocore
	return 0;


}

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