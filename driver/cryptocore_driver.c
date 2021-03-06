#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>

#include "../include/cryptocore_ioctl_header.h"

#define GPIO1_BASE				0xFF709000
#define GPIO1_SPAN				0x00000078

#define H2F_BRIDGE_BASE			0xC0000000

#define MWMAC_RAM_BASE			0x00000000
#define MWMAC_RAM_SPAN			0x00000FFF

#define LW_H2F_BRIDGE_BASE		0xFF200000

#define LEDR_BASE          		0x00000000
#define LEDR_SPAN				0x0000000F

#define HEX3_HEX0_BASE			0x00000020
#define HEX3_HEX0_SPAN			0x0000000F

#define HEX5_HEX4_BASE			0x00000030
#define HEX5_HEX4_SPAN			0x0000000F

#define SW_BASE					0x00000040
#define SW_SPAN					0x0000000F

#define KEY_BASE				0x00000050
#define KEY_SPAN				0x0000000F

#define MWMAC_CMD_BASE			0x00000060
#define MWMAC_CMD_SPAN			0x00000007

#define MWMAC_IRQ_BASE         	0x00000070
#define MWMAC_IRQ_SPAN			0x0000000F

#define TRNG_CMD_BASE			0x00000080
#define TRNG_CMD_SPAN			0x0000000F

#define TRNG_CTR_BASE			0x00000090
#define TRNG_CTR_SPAN			0x0000000F

#define TRNG_TSTAB_BASE			0x000000A0
#define TRNG_TSTAB_SPAN			0x0000000F

#define TRNG_TSAMPLE_BASE		0x000000B0
#define TRNG_TSAMPLE_SPAN		0x0000000F

#define TRNG_IRQ_BASE			0x000000C0
#define TRNG_IRQ_SPAN			0x0000000F

#define TRNG_FIFO_BASE			0x00001000
#define TRNG_FIFO_SPAN			0x00000FFF

#define TIMER_BASE            	0x00002000
#define TIMER_SPAN				0x0000001F

#define KEY_IRQ	 			73
#define MWMAC_IRQ	 		74
#define TRNG_IRQ			75

#define DRIVER_NAME "cryptocore" /* Name des Moduls */

// CryptoCore Operations:

#define MONTMULT			0x0
#define MONTR				0x1
#define MONTR2				0x2
#define MONTEXP				0x3
#define MODADD				0x4
#define MODSUB				0x5
#define COPYH2V				0x6
#define COPYV2V				0x7
#define COPYH2H				0x8
#define COPYV2H				0x9
#define MONTMULT1			0xA
#define MONTEXPFULL			0xB

// CryptoCore RAM Locations

#define MWMAC_RAM_B1 			0x0
#define MWMAC_RAM_B2 			0x1
#define MWMAC_RAM_B3 			0x2
#define MWMAC_RAM_B4 			0x3
#define MWMAC_RAM_B5 			0x4
#define MWMAC_RAM_B6 			0x5
#define MWMAC_RAM_B7 			0x6
#define MWMAC_RAM_B8 			0x7

#define MWMAC_RAM_P1			0x8
#define MWMAC_RAM_P2			0x9
#define MWMAC_RAM_P3			0xA
#define MWMAC_RAM_P4			0xB
#define MWMAC_RAM_P5			0xC
#define MWMAC_RAM_P6			0xD
#define MWMAC_RAM_P7			0xE
#define MWMAC_RAM_P8			0xF

#define MWMAC_RAM_TS1			0x10
#define MWMAC_RAM_TS2			0x11
#define MWMAC_RAM_TS3			0x12
#define MWMAC_RAM_TS4			0x13
#define MWMAC_RAM_TS5			0x14
#define MWMAC_RAM_TS6			0x15
#define MWMAC_RAM_TS7			0x16
#define MWMAC_RAM_TS8			0x17

#define MWMAC_RAM_TC1			0x18
#define MWMAC_RAM_TC2			0x19
#define MWMAC_RAM_TC3			0x1A
#define MWMAC_RAM_TC4			0x1B
#define MWMAC_RAM_TC5			0x1C
#define MWMAC_RAM_TC6			0x1D
#define MWMAC_RAM_TC7			0x1E
#define MWMAC_RAM_TC8			0x1F

#define MWMAC_RAM_A1			0x0
#define MWMAC_RAM_A2			0x1
#define MWMAC_RAM_A3			0x2
#define MWMAC_RAM_A4			0x3
#define MWMAC_RAM_A5			0x4
#define MWMAC_RAM_A6			0x5
#define MWMAC_RAM_A7			0x6
#define MWMAC_RAM_A8			0x7

#define MWMAC_RAM_E1			0x8
#define MWMAC_RAM_E2			0x9
#define MWMAC_RAM_E3			0xA
#define MWMAC_RAM_E4			0xB
#define MWMAC_RAM_E5			0xC
#define MWMAC_RAM_E6			0xD
#define MWMAC_RAM_E7			0xE
#define MWMAC_RAM_E8			0xF

#define MWMAC_RAM_X1			0x10
#define MWMAC_RAM_X2			0x11
#define MWMAC_RAM_X3			0x12
#define MWMAC_RAM_X4			0x13
#define MWMAC_RAM_X5			0x14
#define MWMAC_RAM_X6			0x15
#define MWMAC_RAM_X7			0x16
#define MWMAC_RAM_X8			0x17

static dev_t cryptocore_dev_number;
static struct cdev *driver_object;
static struct class *cryptocore_class;
static struct device *cryptocore_dev;

volatile u32 *HPS_GPIO1_ptr;
volatile u32 *LEDR_ptr;
volatile u32 *KEY_ptr;
volatile u32 *MWMAC_RAM_ptr;
volatile u32 *MWMAC_CMD_ptr;
volatile u32 *MWMAC_IRQ_ptr;
volatile u32 *TRNG_CMD_ptr;
volatile u32 *TRNG_CTR_ptr;
volatile u32 *TRNG_TSTAB_ptr;
volatile u32 *TRNG_TSAMPLE_ptr;
volatile u32 *TRNG_IRQ_ptr;
volatile u32 *TRNG_FIFO_ptr;

volatile u32 mwmac_irq_var;

volatile u32 trng_words_available;

// CryptoCore Driver Supported Precisions:
static u32 PRIME_PRECISIONS[13][2]={ {192,0x0}, {224,0x1}, {256,0x2}, {320,0x3}, {384,0x4}, {512,0x5}, {768,0x6}, {1024,0x7}, {1536,0x8}, {2048,0x9}, {3072,0xA}, {4096,0xB}, {448, 0xD}};
static u32 BINARY_PRECISIONS[16][2]={ {131,0x0}, {163,0x1}, {176,0x2}, {191,0x3}, {193,0x4}, {208,0x5}, {233,0x6}, {239,0x7}, {272,0x8}, {283,0x9}, {304,0xA}, {359,0xB}, {368,0xC}, {409,0xD}, {431,0xE}, {571,0xF} };

// CryptoCore Driver Function Prototyps:
static void Clear_MWMAC_RAM(void);
static void MWMAC_MontMult(MontMult_params_t *MontMult_params_ptr);
static void MWMAC_MontR(MontR_params_t *MontR_params_ptr);
static void MWMAC_MontR2(MontR2_params_t *MontR2_params_ptr);
static void MWMAC_MontExp(MontExp_params_t *MontExp_params_ptr);
static void MWMAC_ModAdd(ModAdd_params_t *ModAdd_params_ptr);
static void MWMAC_ModSub(ModSub_params_t *ModSub_params_ptr);
static void MWMAC_CopyH2V(CopyH2V_params_t *CopyH2V_params_ptr);
static void MWMAC_CopyV2V(CopyV2V_params_t *CopyV2V_params_ptr);
static void MWMAC_CopyH2H(CopyH2H_params_t *CopyH2H_params_ptr);
static void MWMAC_CopyV2H(CopyV2H_params_t *CopyV2H_params_ptr);
static void MWMAC_MontMult1(MontMult1_params_t *MontMult1_params_ptr);
static void MWMAC_MontExpFull(MontExp_params_t *MontExp_params_ptr);
static void MWMAC_ModExp(ModExp_params_t *ModExp_params_ptr);
static void MWMAC_ModRed(ModRed_params_t *ModRed_params_ptr);
// Add further Function Prototypes here...
static void MWMAC_Prep(Prep_params_t *Prep_params_ptr);
static void MWMAC_PointAdd(PointAdd_params_t *PointAdd_params_ptr);
static void MWMAC_PointDouble(PointDouble_params_t *PointDouble_params_ptr);
static void MWMAC_PointMult(PointMult_params_t *PointMult_params_ptr);
static void MWMAC_PostOp(PostOp_params_t* PostOp_params_ptr);
irq_handler_t key_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	u32 led_val;
	led_val = ioread32(LEDR_ptr);
	iowrite32((led_val ^ 0x00000001), LEDR_ptr); // Toggle LEDR[0]
	
	iowrite32(0x0000000F, KEY_ptr+3);
    
	return (irq_handler_t) IRQ_HANDLED;
}

irq_handler_t mwmac_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	u32 led_val;
	led_val = ioread32(LEDR_ptr);
	iowrite32((led_val ^ 0x00000001), LEDR_ptr); // Toggle LEDR[0]
	
	iowrite32(0x00000001, MWMAC_IRQ_ptr+3);
	mwmac_irq_var = 1;
    
	return (irq_handler_t) IRQ_HANDLED;
}

irq_handler_t trng_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	iowrite32(0x00000001, TRNG_IRQ_ptr+3);

	trng_words_available = 1024;
    
	return (irq_handler_t) IRQ_HANDLED;
}

static int cryptocore_driver_open ( struct inode *inode, struct file *instance )
{
	dev_info( cryptocore_dev, "cryptocore_driver_open called\n" );
	return 0;
}

static int cryptocore_driver_close ( struct inode *inode, struct file *instance )
{
	dev_info( cryptocore_dev, "cryptocore_driver_close called\n" );
	return 0;
}
// attention ==>>
static long cryptocore_driver_ioctl( struct file *instance, unsigned int cmd, unsigned long arg)
{	
	// Add CryptoCore Structs here and allocate kernel memory...
	MontMult_params_t *MontMult_params_ptr = kmalloc(sizeof(MontMult_params_t), GFP_DMA);
	MontR_params_t *MontR_params_ptr = kmalloc(sizeof(MontR_params_t), GFP_DMA);
	MontR2_params_t *MontR2_params_ptr = kmalloc(sizeof(MontR2_params_t), GFP_DMA);
	MontExp_params_t *MontExp_params_ptr = kmalloc(sizeof(MontExp_params_t), GFP_DMA);
	ModAdd_params_t *ModAdd_params_ptr = kmalloc(sizeof(ModAdd_params_t), GFP_DMA);
	ModSub_params_t *ModSub_params_ptr = kmalloc(sizeof(ModSub_params_t), GFP_DMA);
	CopyH2V_params_t *CopyH2V_params_ptr = kmalloc(sizeof(CopyH2V_params_t), GFP_DMA);
	CopyV2V_params_t *CopyV2V_params_ptr = kmalloc(sizeof(CopyV2V_params_t), GFP_DMA);
	CopyH2H_params_t *CopyH2H_params_ptr = kmalloc(sizeof(CopyH2H_params_t), GFP_DMA);
	CopyV2H_params_t *CopyV2H_params_ptr = kmalloc(sizeof(CopyV2H_params_t), GFP_DMA);
	MontMult1_params_t *MontMult1_params_ptr = kmalloc(sizeof(MontMult1_params_t), GFP_DMA);
	ModExp_params_t *ModExp_params_ptr = kmalloc(sizeof(ModExp_params_t), GFP_DMA);
	ModRed_params_t *ModRed_params_ptr = kmalloc(sizeof(ModRed_params_t), GFP_DMA);
	
	// my
	Prep_params_t *Prep_params_ptr = kmalloc(sizeof(Prep_params_t), GFP_DMA);
	PointAdd_params_t *PointAdd_params_ptr = kmalloc(sizeof(PointAdd_params_t), GFP_DMA);
	PointDouble_params_t* PointDouble_params_ptr = kmalloc(sizeof(PointDouble_params_t), GFP_DMA);
	PointMult_params_t* PointMult_params_ptr = kmalloc(sizeof(PointMult_params_t), GFP_DMA);
	PostOp_params_t *PostOp_params_ptr = kmalloc(sizeof(PostOp_params_t), GFP_DMA);

	int rc;
	u32 i;
	u32 trng_val = 0;

	mwmac_irq_var = 0;

	dev_info( cryptocore_dev, "cryptocore_driver_ioctl called 0x%4.4x %p\n", cmd, (void *) arg );
// attention! ==>>
	switch(cmd) {
		case IOCTL_SET_TRNG_CMD:
			get_user(trng_val, (u32 *)arg);
			iowrite32(trng_val, TRNG_CMD_ptr);
			if(trng_val | 0x00000001) {
				for(i=0; i<60; i++){
					udelay(1000); // Give TRNG FIFO time to fill
				}
			}
			break;
		case IOCTL_SET_TRNG_CTR:
			get_user(trng_val, (u32 *)arg);
			iowrite32(trng_val, TRNG_CTR_ptr);
			break;
		case IOCTL_SET_TRNG_TSTAB:
			get_user(trng_val, (u32 *)arg);
			iowrite32(trng_val, TRNG_TSTAB_ptr);
			break;
		case IOCTL_SET_TRNG_TSAMPLE:
			get_user(trng_val, (u32 *)arg);
			iowrite32(trng_val, TRNG_TSAMPLE_ptr);
			break;
		case IOCTL_READ_TRNG_FIFO:
			trng_val = ioread32(TRNG_FIFO_ptr);
			trng_words_available--;
			put_user(trng_val, (u32 *)arg);
			if(trng_words_available == 0) {
				for(i=0; i<60; i++){
					udelay(1000); // Give TRNG FIFO time to fill
				}				
			}
			break;
		case IOCTL_MWMAC_MONTMULT:
			rc = copy_from_user(MontMult_params_ptr, (void *)arg, sizeof(MontMult_params_t));
			MWMAC_MontMult(MontMult_params_ptr);
			rc = copy_to_user((void *)arg, MontMult_params_ptr, sizeof(MontMult_params_t));
			break;
		case IOCTL_MWMAC_MONTR:
			rc = copy_from_user(MontR_params_ptr, (void *)arg, sizeof(MontR_params_t));
			MWMAC_MontR(MontR_params_ptr);
			rc = copy_to_user((void *)arg, MontR_params_ptr, sizeof(MontR_params_t));
			break;			
		case IOCTL_MWMAC_MONTR2:
			rc = copy_from_user(MontR2_params_ptr, (void *)arg, sizeof(MontR2_params_t));
			MWMAC_MontR2(MontR2_params_ptr);
			rc = copy_to_user((void *)arg, MontR2_params_ptr, sizeof(MontR2_params_t));
			break;			
		case IOCTL_MWMAC_MONTEXP:
			rc = copy_from_user(MontExp_params_ptr, (void *)arg, sizeof(MontExp_params_t));
			MWMAC_MontExp(MontExp_params_ptr);
			rc = copy_to_user((void *)arg, MontExp_params_ptr, sizeof(MontExp_params_t));
			break;			
		case IOCTL_MWMAC_MODADD:
			rc = copy_from_user(ModAdd_params_ptr, (void *)arg, sizeof(ModAdd_params_t));
			MWMAC_ModAdd(ModAdd_params_ptr);
			rc = copy_to_user((void *)arg, ModAdd_params_ptr, sizeof(ModAdd_params_t));
			break;
		case IOCTL_MWMAC_MODSUB:
			rc = copy_from_user(ModSub_params_ptr, (void *)arg, sizeof(ModSub_params_t));
			MWMAC_ModSub(ModSub_params_ptr);
			rc = copy_to_user((void *)arg, ModSub_params_ptr, sizeof(ModSub_params_t));
			break;
		case IOCTL_MWMAC_COPYH2V:
			rc = copy_from_user(CopyH2V_params_ptr, (void *)arg, sizeof(CopyH2V_params_t));
			MWMAC_CopyH2V(CopyH2V_params_ptr);
			rc = copy_to_user((void *)arg, CopyH2V_params_ptr, sizeof(CopyH2V_params_t));
			break;
		case IOCTL_MWMAC_COPYV2V:
			rc = copy_from_user(CopyV2V_params_ptr, (void *)arg, sizeof(CopyV2V_params_t));
			MWMAC_CopyV2V(CopyV2V_params_ptr);
			rc = copy_to_user((void *)arg, CopyV2V_params_ptr, sizeof(CopyV2V_params_t));
			break;
		case IOCTL_MWMAC_COPYH2H:
			rc = copy_from_user(CopyH2H_params_ptr, (void *)arg, sizeof(CopyH2H_params_t));
			MWMAC_CopyH2H(CopyH2H_params_ptr);
			rc = copy_to_user((void *)arg, CopyH2H_params_ptr, sizeof(CopyH2H_params_t));
			break;
		case IOCTL_MWMAC_COPYV2H:
			rc = copy_from_user(CopyV2H_params_ptr, (void *)arg, sizeof(CopyV2H_params_t));
			MWMAC_CopyV2H(CopyV2H_params_ptr);
			rc = copy_to_user((void *)arg, CopyV2H_params_ptr, sizeof(CopyV2H_params_t));
			break;
		case IOCTL_MWMAC_MONTMULT1:
			rc = copy_from_user(MontMult1_params_ptr, (void *)arg, sizeof(MontMult1_params_t));
			MWMAC_MontMult1(MontMult1_params_ptr);
			rc = copy_to_user((void *)arg, MontMult1_params_ptr, sizeof(MontMult1_params_t));
			break;
		case IOCTL_MWMAC_MONTEXP_FULL:
			rc = copy_from_user(MontExp_params_ptr, (void *)arg, sizeof(MontExp_params_t));
			MWMAC_MontExpFull(MontExp_params_ptr);
			rc = copy_to_user((void *)arg, MontExp_params_ptr, sizeof(MontExp_params_t));
			break;	
		case IOCTL_MWMAC_MODEXP:
			rc = copy_from_user(ModExp_params_ptr, (void *)arg, sizeof(ModExp_params_t));
			MWMAC_ModExp(ModExp_params_ptr);
			rc = copy_to_user((void *)arg, ModExp_params_ptr, sizeof(ModExp_params_t));
			break;
		case IOCTL_MWMAC_MODRED:
			rc = copy_from_user(ModRed_params_ptr, (void *)arg, sizeof(ModRed_params_t));
			MWMAC_ModRed(ModRed_params_ptr);
			rc = copy_to_user((void *)arg, ModRed_params_ptr, sizeof(ModRed_params_t));
			break;
		// Add further CryptoCore commands here

		case IOCTL_MWMAC_PREP:
			rc = copy_from_user(Prep_params_ptr, (void *)arg, sizeof(Prep_params_t));
			MWMAC_Prep(Prep_params_ptr);
			rc = copy_to_user((void *)arg, Prep_params_ptr, sizeof(Prep_params_t));
			break;
		
		case IOCTL_MWMAC_PADD:
			rc = copy_from_user(PointAdd_params_ptr, (void *)arg, sizeof(PointAdd_params_t));
			MWMAC_PointAdd(PointAdd_params_ptr);
			rc = copy_to_user((void *)arg, PointAdd_params_ptr, sizeof(PointAdd_params_t));
			break;

		case IOCTL_MWMAC_PDOUBLE:
			rc = copy_from_user(PointDouble_params_ptr, (void*)arg, sizeof(PointDouble_params_t));
			MWMAC_PointDouble(PointDouble_params_ptr);
			rc = copy_to_user((void*)arg, PointDouble_params_ptr, sizeof(PointDouble_params_t));
			break;

		case IOCTL_MWMAC_PMULT:
			rc = copy_from_user(PointMult_params_ptr, (void*)arg, sizeof(PointMult_params_t));
			MWMAC_PointMult(PointMult_params_ptr);
			rc = copy_to_user((void*)arg, PointMult_params_ptr, sizeof(PointMult_params_t));
			break;

		case IOCTL_MWMAC_POSTOP:
			rc = copy_from_user(PostOp_params_ptr, (void*)arg, sizeof(PostOp_params_t));
			MWMAC_PostOp(PostOp_params_ptr);
			rc = copy_to_user((void*)arg, PostOp_params_ptr, sizeof(PostOp_params_t));
			break;

		default:
			printk("unknown IOCTL 0x%x\n", cmd);

			// Free allocated kernel memory for defined CryptoCore Structs here...
			kfree(MontMult_params_ptr);
			kfree(MontR_params_ptr);
			kfree(MontR2_params_ptr);
			kfree(MontExp_params_ptr);
			kfree(ModAdd_params_ptr);
			kfree(ModSub_params_ptr);
			kfree(CopyH2V_params_ptr);
			kfree(CopyV2V_params_ptr);
			kfree(CopyH2H_params_ptr);
			kfree(CopyV2H_params_ptr);
			kfree(MontMult1_params_ptr);
			kfree(ModExp_params_ptr);
			kfree(ModRed_params_ptr);
			// my
			kfree(Prep_params_ptr);
			kfree(PointAdd_params_ptr);
			kfree(PointDouble_params_ptr);
			kfree(PointMult_params_ptr);
			kfree(PostOp_params_ptr);
			return -EINVAL;
	}
	
	// Free allocated kernel memory for defined CryptoCore Structs here...
	kfree(MontMult_params_ptr);
	kfree(MontR_params_ptr);
	kfree(MontR2_params_ptr);
	kfree(MontExp_params_ptr);
	kfree(ModAdd_params_ptr);
	kfree(ModSub_params_ptr);
	kfree(CopyH2V_params_ptr);
	kfree(CopyV2V_params_ptr);
	kfree(CopyH2H_params_ptr);
	kfree(CopyV2H_params_ptr);
	kfree(MontMult1_params_ptr);
	kfree(ModExp_params_ptr);
	kfree(ModRed_params_ptr);
	// my
	kfree(Prep_params_ptr);	
	kfree(PointAdd_params_ptr);
	kfree(PointDouble_params_ptr);
	kfree(PointMult_params_ptr);
	kfree(PostOp_params_ptr);
	return 0;
	
}

static struct file_operations fops = {
   .owner = THIS_MODULE,
   .open = cryptocore_driver_open,
   .release = cryptocore_driver_close,
   .unlocked_ioctl = cryptocore_driver_ioctl,
};

static int __init cryptocore_init( void )
{
   int value;

   if( alloc_chrdev_region(&cryptocore_dev_number, 0, 1, DRIVER_NAME) < 0 )
      return -EIO;

   driver_object = cdev_alloc(); /* Anmeldeobject reservieren */

   if( driver_object == NULL )
      goto free_device_number;

   driver_object->owner = THIS_MODULE;
   driver_object->ops = &fops;

   if( cdev_add(driver_object, cryptocore_dev_number, 1) )
      goto free_cdev;

   cryptocore_class = class_create( THIS_MODULE, DRIVER_NAME );

   if( IS_ERR( cryptocore_class ) ) {
      pr_err( "cryptocore: no udev support\n");
      goto free_cdev;
   }

   cryptocore_dev = device_create(cryptocore_class, NULL, cryptocore_dev_number, NULL, "%s", DRIVER_NAME );
   dev_info( cryptocore_dev, "cryptocore_init called\n" );

   if(!(request_mem_region(GPIO1_BASE, GPIO1_SPAN, DRIVER_NAME))) {
      pr_err( "timer: request mem_region (GPIO1) failed!\n");
      goto fail_request_mem_region_1;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + LEDR_BASE, LEDR_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (LEDR) failed!\n");
      goto fail_request_mem_region_2;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + KEY_BASE, KEY_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (KEY) failed!\n");
      goto fail_request_mem_region_3;
   }

   if(!(request_mem_region(H2F_BRIDGE_BASE + MWMAC_RAM_BASE, MWMAC_RAM_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (MWMAC_RAM) failed!\n");
      goto fail_request_mem_region_4;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_CMD_BASE, MWMAC_CMD_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (MWMAC_CMD) failed!\n");
      goto fail_request_mem_region_5;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_IRQ_BASE, MWMAC_IRQ_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (MWMAC_IRQ) failed!\n");
      goto fail_request_mem_region_6;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CMD_BASE, TRNG_CMD_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_CMD) failed!\n");
      goto fail_request_mem_region_7;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CTR_BASE, TRNG_CTR_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_CTR) failed!\n");
      goto fail_request_mem_region_8;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSTAB_BASE, TRNG_TSTAB_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_TSTAB) failed!\n");
      goto fail_request_mem_region_9;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSAMPLE_BASE, TRNG_TSAMPLE_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_TSAMPLE) failed!\n");
      goto fail_request_mem_region_10;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_IRQ_BASE, TRNG_IRQ_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_IRQ) failed!\n");
      goto fail_request_mem_region_11;
   }

   if(!(request_mem_region(LW_H2F_BRIDGE_BASE + TRNG_FIFO_BASE, TRNG_FIFO_SPAN, DRIVER_NAME))) {
      pr_err( "cryptocore: request mem_region (TRNG_FIFO) failed!\n");
      goto fail_request_mem_region_12;
   }

   if(!(HPS_GPIO1_ptr = ioremap(GPIO1_BASE, GPIO1_SPAN))) {
      pr_err( "cryptocore: ioremap (GPIO1) failed!\n");
      goto fail_ioremap_1;
   }

   if(!(LEDR_ptr = ioremap(LW_H2F_BRIDGE_BASE + LEDR_BASE, LEDR_SPAN))) {
      pr_err( "cryptocore: ioremap (LEDR) failed!\n");
      goto fail_ioremap_2;
   }

   if(!(KEY_ptr = ioremap(LW_H2F_BRIDGE_BASE + KEY_BASE, KEY_SPAN))) {
      pr_err( "cryptocore: ioremap (KEY) failed!\n");
      goto fail_ioremap_3;
   }

   if(!(MWMAC_RAM_ptr = ioremap(H2F_BRIDGE_BASE + MWMAC_RAM_BASE, MWMAC_RAM_SPAN))) {
      pr_err( "cryptocore: ioremap (MWMAC_RAM) failed!\n");
      goto fail_ioremap_4;
   }

   if(!(MWMAC_CMD_ptr = ioremap(LW_H2F_BRIDGE_BASE + MWMAC_CMD_BASE, MWMAC_CMD_SPAN))) {
      pr_err( "cryptocore: ioremap (MWMAC_CMD) failed!\n");
      goto fail_ioremap_5;
   }

   if(!(MWMAC_IRQ_ptr = ioremap(LW_H2F_BRIDGE_BASE + MWMAC_IRQ_BASE, MWMAC_IRQ_SPAN))) {
      pr_err( "cryptocore: ioremap (MWMAC_IRQ) failed!\n");
      goto fail_ioremap_6;
   }

   if(!(TRNG_CMD_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_CMD_BASE, TRNG_CMD_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_CMD) failed!\n");
      goto fail_ioremap_7;
   }

   if(!(TRNG_CTR_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_CTR_BASE, TRNG_CTR_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_CTR) failed!\n");
      goto fail_ioremap_8;
   }

   if(!(TRNG_TSTAB_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_TSTAB_BASE, TRNG_TSTAB_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_TSTAB) failed!\n");
      goto fail_ioremap_9;
   }

   if(!(TRNG_TSAMPLE_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_TSAMPLE_BASE, TRNG_TSAMPLE_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_TSAMPLE) failed!\n");
      goto fail_ioremap_10;
   }

   if(!(TRNG_IRQ_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_IRQ_BASE, TRNG_IRQ_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_IRQ) failed!\n");
      goto fail_ioremap_11;
   }

   if(!(TRNG_FIFO_ptr = ioremap(LW_H2F_BRIDGE_BASE + TRNG_FIFO_BASE, TRNG_FIFO_SPAN))) {
      pr_err( "cryptocore: ioremap (TRNG_FIFO) failed!\n");
      goto fail_ioremap_12;
   }

   // Clear the PIO edgecapture register (clear any pending interrupt)
   iowrite32(0x00000001, MWMAC_IRQ_ptr+3); 
   // Enable IRQ generation for the CryptoCore
   iowrite32(0x00000001, MWMAC_IRQ_ptr+2); 

   // Clear the PIO edgecapture register (clear any pending interrupt)
   iowrite32(0x00000001, TRNG_IRQ_ptr+3);
   // Enable IRQ generation for the CryptoCore TRNG
   iowrite32(0x00000001, TRNG_IRQ_ptr+2);

   
   // Clear the PIO edgecapture register (clear any pending interrupt)
   iowrite32(0x0000000F, KEY_ptr+3);
   // Enable IRQ generation for the 4 buttons
   iowrite32(0x0000000F, KEY_ptr+2);

   value = request_irq (KEY_IRQ, (irq_handler_t) key_irq_handler, IRQF_SHARED, "cryptocore_key_irq_handler", (void *) (key_irq_handler));
   if(value) {
      pr_err( "cryptocore: request_irq (KEY) failed!\n");
      goto fail_irq_1;
   }

   value = request_irq (MWMAC_IRQ, (irq_handler_t) mwmac_irq_handler, IRQF_SHARED, "cryptocore_mwmac_irq_handler", (void *) (mwmac_irq_handler));
   if(value) {
      pr_err( "cryptocore: request_irq (MWMAC) failed!\n");
      goto fail_irq_2;
   }

   value = request_irq (TRNG_IRQ, (irq_handler_t) trng_irq_handler, IRQF_SHARED, "cryptocore_trng_irq_handler", (void *) (trng_irq_handler));
   if(value) {
      pr_err( "cryptocore: request_irq (TRNG) failed!\n");
      goto fail_irq_3;
   } 

   // Turn on green User LED
   iowrite32(0x01000000, (HPS_GPIO1_ptr+1));
   iowrite32(0x01000000, (HPS_GPIO1_ptr));

   return 0;

fail_irq_3:
   free_irq (MWMAC_IRQ, (void*) mwmac_irq_handler);

fail_irq_2:
   free_irq (KEY_IRQ, (void*) key_irq_handler);

fail_irq_1:
   iounmap(TRNG_FIFO_ptr);

fail_ioremap_12:
   iounmap(TRNG_IRQ_ptr);

fail_ioremap_11:
   iounmap(TRNG_TSAMPLE_ptr);

fail_ioremap_10:
   iounmap(TRNG_TSTAB_ptr);

fail_ioremap_9:
   iounmap(TRNG_CTR_ptr);

fail_ioremap_8:
   iounmap(TRNG_CMD_ptr);

fail_ioremap_7:
   iounmap(MWMAC_IRQ_ptr);

fail_ioremap_6:
   iounmap(MWMAC_CMD_ptr);

fail_ioremap_5:
   iounmap(MWMAC_RAM_ptr);

fail_ioremap_4:
   iounmap(KEY_ptr);

fail_ioremap_3:
   iounmap(LEDR_ptr);

fail_ioremap_2:
   iounmap(HPS_GPIO1_ptr);

fail_ioremap_1:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_FIFO_BASE, TRNG_FIFO_SPAN);

fail_request_mem_region_12:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_IRQ_BASE, TRNG_IRQ_SPAN);

fail_request_mem_region_11:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSAMPLE_BASE, TRNG_TSAMPLE_SPAN);

fail_request_mem_region_10:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSTAB_BASE, TRNG_TSTAB_SPAN);

fail_request_mem_region_9:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CTR_BASE, TRNG_CTR_SPAN);

fail_request_mem_region_8:
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CMD_BASE, TRNG_CMD_SPAN);

fail_request_mem_region_7:
   release_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_IRQ_BASE, MWMAC_IRQ_SPAN);

fail_request_mem_region_6:
   release_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_CMD_BASE, MWMAC_CMD_SPAN);

fail_request_mem_region_5:
   release_mem_region(H2F_BRIDGE_BASE + MWMAC_RAM_BASE, MWMAC_RAM_SPAN);

fail_request_mem_region_4:
   release_mem_region(LW_H2F_BRIDGE_BASE + KEY_BASE, KEY_SPAN);

fail_request_mem_region_3:
   release_mem_region(LW_H2F_BRIDGE_BASE + LEDR_BASE, LEDR_SPAN);

fail_request_mem_region_2:
   release_mem_region(GPIO1_BASE, GPIO1_SPAN);

fail_request_mem_region_1:
   device_destroy( cryptocore_class, cryptocore_dev_number );
   class_destroy( cryptocore_class );

free_cdev:
   kobject_put( &driver_object->kobj );

free_device_number:
   unregister_chrdev_region( cryptocore_dev_number, 1 );
   return -EIO;
}

static void __exit cryptocore_exit( void )
{
   dev_info( cryptocore_dev, "cryptocore_exit called\n" );

   iowrite32(0x00000000, (LEDR_ptr));
   iowrite32(0x00000000, (HPS_GPIO1_ptr));
   iowrite32(0x00000000, (HPS_GPIO1_ptr+1));

   free_irq (KEY_IRQ, (void*) key_irq_handler);
   free_irq (MWMAC_IRQ, (void*) mwmac_irq_handler);
   free_irq (TRNG_IRQ, (void*) trng_irq_handler);

   iounmap(TRNG_FIFO_ptr);
   iounmap(TRNG_IRQ_ptr);
   iounmap(TRNG_TSAMPLE_ptr);
   iounmap(TRNG_TSTAB_ptr);
   iounmap(TRNG_CTR_ptr);
   iounmap(TRNG_CMD_ptr);
   iounmap(MWMAC_IRQ_ptr);
   iounmap(MWMAC_CMD_ptr);
   iounmap(MWMAC_RAM_ptr);
   iounmap(KEY_ptr);
   iounmap(LEDR_ptr);
   iounmap(HPS_GPIO1_ptr);

   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_FIFO_BASE, TRNG_FIFO_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_IRQ_BASE, TRNG_IRQ_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSAMPLE_BASE, TRNG_TSAMPLE_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_TSTAB_BASE, TRNG_TSTAB_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CTR_BASE, TRNG_CTR_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + TRNG_CMD_BASE, TRNG_CMD_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_IRQ_BASE, MWMAC_IRQ_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + MWMAC_CMD_BASE, MWMAC_CMD_SPAN);
   release_mem_region(H2F_BRIDGE_BASE + MWMAC_RAM_BASE, MWMAC_RAM_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + KEY_BASE, KEY_SPAN);
   release_mem_region(LW_H2F_BRIDGE_BASE + LEDR_BASE, LEDR_SPAN);
   release_mem_region(GPIO1_BASE, GPIO1_SPAN);

   device_destroy( cryptocore_class, cryptocore_dev_number );
   class_destroy( cryptocore_class );

   cdev_del( driver_object );
   unregister_chrdev_region( cryptocore_dev_number, 1 );

   return;
}

static void Clear_MWMAC_RAM(void)
{
	u32 value;
	u32 i;
	
	value = 0x00000000;
	
	for(i=0; i<128; i++){
		// Clear B
		iowrite32(value, (MWMAC_RAM_ptr+0x3+i*0x4));
		// Clear P
		iowrite32(value, (MWMAC_RAM_ptr+0x2+i*0x4));
		// Clear TS
		iowrite32(value, (MWMAC_RAM_ptr+0x1+i*0x4));
		// Clear TC
		iowrite32(value, (MWMAC_RAM_ptr+0x0+i*0x4));
		// Clear A
		iowrite32(value, (MWMAC_RAM_ptr+0x200+i));
		// Clear E
		iowrite32(value, (MWMAC_RAM_ptr+0x280+i));
		// Clear X
		iowrite32(value, (MWMAC_RAM_ptr+0x300+i));
	}
	
}

static void MWMAC_MontMult(MontMult_params_t *MontMult_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontMult_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontMult_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontMult_params_ptr->prec % 32) {
					rw_prec = (MontMult_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontMult_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontMult_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontMult_params_ptr->prec;
			}
		}
	}	
	
	if(MontMult_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}	
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontMult_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}
	
	// Write Parameter a to A Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontMult_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x200+i));
	}

	// Write Parameter b to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontMult_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}	
	
	// MontMult(A1, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontMult_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_MontR(MontR_params_t *MontR_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontR_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontR_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontR_params_ptr->prec % 32) {
					rw_prec = (MontR_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontR_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontR_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontR_params_ptr->prec;
			}
		}
	}	
	
	if(MontR_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontR_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// MontR(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result r from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontR_params_ptr->r[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_MontR2(MontR2_params_t *MontR2_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontR2_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontR2_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontR2_params_ptr->prec % 32) {
					rw_prec = (MontR2_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontR2_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontR2_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontR2_params_ptr->prec;
			}
		}
	}			
	
	if(MontR2_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontR2_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// MontR2(P1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR2 << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result r from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontR2_params_ptr->r2[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_MontExp(MontExp_params_t *MontExp_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontExp_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontExp_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontExp_params_ptr->prec % 32) {
					rw_prec = (MontExp_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontExp_params_ptr->prec;
				}	
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontExp_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontExp_params_ptr->prec;
			}
		}
	}	
	
	if(MontExp_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// Write Parameter e to E Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->e[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x280+i));
	}	

	// Write Parameter b to X Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x300+i));
	}
	
	// MontR(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// MontExp(A1, B1, E1, X1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTEXP << 8) 
	//			src_addr      			dest_addr    		src_addr_e   			src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (MWMAC_RAM_E1 << 22) | (MWMAC_RAM_X1 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontExp_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_ModAdd(ModAdd_params_t *ModAdd_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(ModAdd_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==ModAdd_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(ModAdd_params_ptr->prec % 32) {
					rw_prec = (ModAdd_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = ModAdd_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==ModAdd_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = ModAdd_params_ptr->prec;
			}
		}
	}		
	
	if(ModAdd_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
		
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModAdd_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}
	
	// Write Parameter a to TS Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModAdd_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x1+i*0x4));
	}	

	// Write Parameter b to TC Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModAdd_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x0+i*0x4));
	}	
	
	// ModAdd(TS1, TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		ModAdd_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_ModSub(ModSub_params_t *ModSub_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(ModSub_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==ModSub_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(ModSub_params_ptr->prec % 32) {
					rw_prec = (ModSub_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = ModSub_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==ModSub_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = ModSub_params_ptr->prec;
			}
		}
	}		
	
	if(ModSub_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModSub_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}
	
	// Write Parameter a to TS Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModSub_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x1+i*0x4));
	}	

	// Write Parameter b to TC Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModSub_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x0+i*0x4));
	}	
	
	// ModSub(TS1, TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODSUB << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		ModSub_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_CopyH2V(CopyH2V_params_t *CopyH2V_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(CopyH2V_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==CopyH2V_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(CopyH2V_params_ptr->prec % 32) {
					rw_prec = (CopyH2V_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = CopyH2V_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==CopyH2V_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = CopyH2V_params_ptr->prec;
			}
		}
	}	
	
	if(CopyH2V_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter a to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(CopyH2V_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}
	
	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result acopy from A Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		CopyH2V_params_ptr->acopy[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x200+i);
	} 
}

static void MWMAC_CopyV2V(CopyV2V_params_t *CopyV2V_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(CopyV2V_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==CopyV2V_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(CopyV2V_params_ptr->prec % 32) {
					rw_prec = (CopyV2V_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = CopyV2V_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==CopyV2V_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = CopyV2V_params_ptr->prec;
			}
		}
	}			
	
	if(CopyV2V_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
		
	Clear_MWMAC_RAM();

	// Write Parameter a to A Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(CopyV2V_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x200+i));
	}
	
	// CopyV2V(A1, X1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8) 
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_A1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result acopy from X Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		CopyV2V_params_ptr->acopy[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x300+i);
	} 
}

static void MWMAC_CopyH2H(CopyH2H_params_t *CopyH2H_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;
	
	if(CopyH2H_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==CopyH2H_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(CopyH2H_params_ptr->prec % 32) {
					rw_prec = (CopyH2H_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = CopyH2H_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==CopyH2H_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = CopyH2H_params_ptr->prec;
			}
		}
	}		
	
	if(CopyH2H_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter a to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(CopyH2H_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}
	
	// CopyH2H(B1, TS1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result acopy from TS Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		CopyH2H_params_ptr->acopy[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x1+i*0x4);
	} 	

}

static void MWMAC_CopyV2H(CopyV2H_params_t *CopyV2H_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;
	
	if(CopyV2H_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==CopyV2H_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(CopyV2H_params_ptr->prec % 32) {
					rw_prec = (CopyV2H_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = CopyV2H_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==CopyV2H_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = CopyV2H_params_ptr->prec;
			}
		}
	}		
	
	if(CopyV2H_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
		
	Clear_MWMAC_RAM();

	// Write Parameter a to A Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(CopyV2H_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x200+i));
	}
	
	// CopyV2H(A1, TS1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A1 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result acopy from TS Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		CopyV2H_params_ptr->acopy[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x1+i*0x4);
	} 
}

static void MWMAC_MontMult1(MontMult1_params_t *MontMult1_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontMult1_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontMult1_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontMult1_params_ptr->prec % 32) {
					rw_prec = (MontMult1_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontMult1_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontMult1_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontMult1_params_ptr->prec;
			}
		}
	}			
	
	if(MontMult1_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
		
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontMult1_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}
	
	// Write Parameter b to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontMult1_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}	
	
	// MontMult1(B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT1 << 8) 
	//			src_addr      dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;		
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontMult1_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_MontExpFull(MontExp_params_t *MontExp_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(MontExp_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==MontExp_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(MontExp_params_ptr->prec % 32) {
					rw_prec = (MontExp_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = MontExp_params_ptr->prec;
				}	
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==MontExp_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = MontExp_params_ptr->prec;
			}
		}
	}	
	
	if(MontExp_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// Write Parameter e to E Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->e[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x280+i));
	}	

	// Write Parameter b to X Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(MontExp_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x300+i));
	}
	
	// MontR(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// MontExp(A1, B1, E1, X1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTEXPFULL << 8) 
	//			src_addr      			dest_addr    		src_addr_e   			src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (MWMAC_RAM_E1 << 22) | (MWMAC_RAM_X1 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		MontExp_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_ModExp(ModExp_params_t *ModExp_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;
	
	if(ModExp_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==ModExp_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(ModExp_params_ptr->prec % 32) {
					rw_prec = (ModExp_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = ModExp_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==ModExp_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = ModExp_params_ptr->prec;
			}
		}
	}	
	
	if(ModExp_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
		
	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModExp_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// MontR2(P1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR2 << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// Write Parameter b to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModExp_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}
	
	// Write Parameter e to E Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(ModExp_params_ptr->e[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x280+i));
	}	
	
	// MontMult(A1, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// CopyH2V(B1, X1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// MontR(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// MontExp(A1, B1, E1, X1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTEXP << 8) 
	//			src_addr      			dest_addr    		src_addr_e   			src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (MWMAC_RAM_E1 << 22) | (MWMAC_RAM_X1 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// MontMult1(B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT1 << 8) 
	//			src_addr      dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_prec/32; i++){
		ModExp_params_ptr->c[rw_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}

static void MWMAC_ModRed(ModRed_params_t *ModRed_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_full_prec = 0;
	u32 mwmac_cmd_half_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_full_prec = 0;
	u32 rw_half_prec = 0;

	if(ModRed_params_ptr->f_sel == 0) {
		// Modular Reduction is not supported in GF(2^m)
		mwmac_f_sel = 0;
//		for(i=0; i<16; i++){
//			if(BINARY_PRECISIONS[i][0]==ModRed_params_ptr->prec){
//				mwmac_cmd_full_prec = BINARY_PRECISIONS[i][1];
//			}
//			if(BINARY_PRECISIONS[i][0]==ModRed_params_ptr->prec/2){
//				mwmac_cmd_half_prec = BINARY_PRECISIONS[i][1];
//			}
//		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==ModRed_params_ptr->prec){
				mwmac_cmd_full_prec = PRIME_PRECISIONS[i][1];
				rw_full_prec = ModRed_params_ptr->prec;
			}
			if(PRIME_PRECISIONS[i][0]==ModRed_params_ptr->prec/2){
				mwmac_cmd_half_prec = PRIME_PRECISIONS[i][1];
				rw_half_prec = ModRed_params_ptr->prec/2;
			}
		}
	}		
	
	if(ModRed_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}

	Clear_MWMAC_RAM();

	// Write Parameter n to P Register Memory
	for(i=0; i<rw_half_prec/32; i++){
		iowrite32(ModRed_params_ptr->n[rw_half_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}

	// MontR2(P1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_half_prec << 4) | (MONTR2 << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1, A1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_half_prec << 4) | (COPYH2V << 8) 
	//			src_addr      				dest_addr   	 src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	
	// Write Parameter a to B Register Memory
	for(i=0; i<rw_full_prec/32; i++){
		iowrite32(ModRed_params_ptr->a[rw_full_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}
	
	// MontMult(A1, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_full_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// Read Result c from B Register Memory 
 	for(i=0; i<rw_full_prec/32; i++){
		ModRed_params_ptr->c[rw_full_prec/32-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4);
	} 
}


// Add further CryptoCore Functions here...

static void MWMAC_Prep(Prep_params_t *Prep_params_ptr)
{
	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(Prep_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==Prep_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(Prep_params_ptr->prec % 32) {
					rw_prec = (Prep_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = Prep_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==Prep_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = Prep_params_ptr->prec;
			}
		}
	}	
	
	if(Prep_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
	Clear_MWMAC_RAM();
	
// Write Parameter n to P Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(Prep_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
	}
	// copy P1 to P3
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// Copy P1 to P5
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// Copy P1 to P7
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// MontR(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	
	// CopyH2V(B1, A5)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// MontR2(P1, B1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR2 << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	
	// CopyH2V(B1, A7)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
// Write Parameter a to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(Prep_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}

//Montgomerizing a
	// MontMult(A7, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1,E3) a to E3
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// Write Parameter b to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(Prep_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}	
	//Montgomerizing b
	// MontMult(A7, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1,E1) b to E1
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	

// Write Parameter x to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(Prep_params_ptr->x[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}

	//Montgomerizing x
	// MontMult(A7, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// Copying Xmont to X1 and X7
	
	// CopyH2V(B1,X1) Xmont to X1
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,X7) Xmont to X7
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// Write Parameter y to B Register Memory
	for(i=0; i<rw_prec/32; i++){
		iowrite32(Prep_params_ptr->y[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
	}

//Montgomerizing y
	// MontMult(A7, B1, P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// Copying Ymont to E7 and X5
	
	// CopyH2V(B1,E7) Ymont to E7
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,X5) Ymont to X5
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// Copy R as Zmont to E5 and X3

	// CopyV2V(A5,E5) Zmont to E5
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(A5,X3) Zmont to X3
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// Checking the R value
	// CopyV2H(A5, B1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

}

static void MWMAC_PointAdd(PointAdd_params_t *PointAdd_params_ptr)
{

	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if(PointAdd_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for(i=0; i<16; i++){
			if(BINARY_PRECISIONS[i][0]==PointAdd_params_ptr->prec){
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if(PointAdd_params_ptr->prec % 32) {
					rw_prec = (PointAdd_params_ptr->prec/32 + 1) * 32;
				} else {
					rw_prec = PointAdd_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for(i=0; i<13; i++){
			if(PRIME_PRECISIONS[i][0]==PointAdd_params_ptr->prec){
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = PointAdd_params_ptr->prec;
			}
		}
	}	
	
	if(PointAdd_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	
// 	Clear_MWMAC_RAM();

// 	// settings values to appropriate locations. (in real this portion will not be needed 
// 	// since we will already have values set in the memory)
	
	
// // Write Parameter n to P Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->n[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x2+i*0x4));
// 	}
// 	// copy P1 to P3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copy P1 to P5
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copy P1 to P7
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // creating R

// 	// MontR(P1, B1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
// 	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // copying R to A5	
// 	// CopyH2V(B1, A5)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Creating R2
// 	// MontR2(P1, B1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR2 << 8)
// 	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 			| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | 	(0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;
	
// // Copying R2 to A7
// 	// CopyH2V(B1, A7)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Write Parameter a to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->a[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}

// //Montgomerizing a
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying mont a to E3
// 	// CopyH2V(B1,E3) a to E3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Write Parameter b to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->b[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}	

// //Montgomerizing b
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying Mont b to E1
// 	// CopyH2V(B1,E1) b to E1
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;	

// /*
// 	xP=X1 ; yP=E7 ; zP=E5
// 	xQ=X7 ; yQ=X5 ; zQ=X3
// */

// // Write Parameter xP to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->x_p[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}

// //Montgomerizing xP
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying xP mont to X1
// 	// CopyH2V(B1,X1)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;
	
	
// // Write Parameter yP to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->y_p[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}

// //Montgomerizing yP
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying yP mont to E7
// 	// CopyH2V(B1,E7)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copy R as zP mont to E5

// 	// CopyV2V(A5,E5) zP mont to E5
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;
	
	
// // CopyH2V(A5,X3) zQ mont to X3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;




// // xQ=X7 ; yQ=X5 ; zQ=X3

// // Write Parameter xQ to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->x_q[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}

// //Montgomerizing xQ
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying xQ mont to X1
// 	// CopyH2V(B1,X1)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;
	
	
// // Write Parameter yQ to B Register Memory
// 	for(i=0; i<rw_prec/32; i++){
// 		iowrite32(PointAdd_params_ptr->y_q[rw_prec/32-1-i], (MWMAC_RAM_ptr+0x3+i*0x4));
// 	}

// //Montgomerizing yQ
// 	// MontMult(A7, B1, P1)
// 	//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
// 	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// // Copying yQ mont to X5
// 	// CopyH2V(B1,E7)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
// 	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while(!mwmac_irq_var);
// 	mwmac_irq_var = 0;
// /*
// 	// Read Result r_b from B Register Memory 
//  	for(i=0; i<128; i++){
// 		PointAdd_params_ptr->r_b[128-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4); //changed rw_prec/32 --> 128
// 	}
// 	// Read Result r_p from P Register Memory 
//  	for(i=0; i<128; i++){
// 		PointAdd_params_ptr->r_p[128-1-i] = ioread32(MWMAC_RAM_ptr+0x2+i*0x4);
// 	}
// 	// Read Result r_e from E Register Memory 
//  	for(i=0; i<128; i++){
// 		PointAdd_params_ptr->r_e[128-1-i] = ioread32(MWMAC_RAM_ptr+0x280+i);
// 	}
// 	// Read Result r_x from X Register Memory 
//  	for(i=0; i<128; i++){
// 		PointAdd_params_ptr->r_x[128-1-i] = ioread32(MWMAC_RAM_ptr+0x300+i);
// 	} 
// 	// Read Result r_a from A Register Memory 
//  	for(i=0; i<128; i++){
// 		PointAdd_params_ptr->r_a[128-1-i] = ioread32(MWMAC_RAM_ptr+0x200+i);
// 	}
// */


// // // // Addition // // // /

// O1 = Z1*Z1*rinvpoly

	// CopyV2H(E5,B1) Copy Z1 to B1
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_E5 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// MontMult(E5, B1, P1) 

	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1,A3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// O2 = Z2*Z2*rinvpoly
	//CopyV2H(X3,B7)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_B7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	//MontMult(X3,B7,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
// A = X1*O2*rinvpoly

	//CopyH2H(B7,B1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	//MontMult(X1,B1,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	//CopyH2H(B1,B5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// B = X2*O1*rinvpoly
	// CopyV2H(X7,B3)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_X7 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// MontMult(A3,B3,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A3 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// T1 = Y1*O2*rinvpoly
	//MontMult(E7,B7,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_E7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// C = T1*Z2*rinvpoly
	// MontMult(X3,B7,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// T2 = Y2*O1*rinvpoly
	// CopyV2H(A3, B1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A3 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// MontMult(X5,B1,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	//CopyH2V(B1,A3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
// D = T2*Z1*rinvpoly
	// CopyV2H(A3, B1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A3 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// MontMult(E5,B1,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2V(B1,A3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


// F = C+D
	// CopyH2H(B7,TS7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_TS7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyV2H(A3,TC7)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A3 << 12) | (MWMAC_RAM_TC7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	//ModAdd(TS7,TC7)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS7 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


// E = A+B

	// CopyH2H(B5,TS5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_TS5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2H(B3,TC5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_TC5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// ModAdd(TS5,TC5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS5 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	

// G = E*Z1*rinvpoly
	//CopyV2H(E5,B3)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//MontMult(E5,B3,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


// T3 = F*X2*rinvpoly
	// CopyH2H(B7,B1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// MontMult(X7,B1,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,A3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// T4 = G*Y2*rinvpoly
	// CopyH2H(B3,B1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// MontMult(X5,B1,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,A5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// H = T3+T4
	// CopyV2H(A3, TS1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A3 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyV2H(A5, TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// ModAdd(TS1,TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,A3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// Z3 = G*Z2*rinvpoly
	//	CopyH2H(B3,B1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// MontMult(X3,B1,P1)

	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	//CopyH2V(B1,X3)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// I = F+Z3
	// CopyH2H(B7,TS1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyV2H(X3,TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// ModAdd(TS1,TC1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;	
	// CopyH2V(B1,A5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// T5 = Z3*Z3*rinvpoly
	// CopyV2H(X3,B1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// MontMult(X3,B1,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// CopyH2V(B1,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

//T6 = apoly_mont*T5*rinvpoly
	//CopyV2H(A7,B1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A7 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//MontMult(E3,B1,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2V(B1,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

//T7 = F*I*rinvpoly
	// MontMult(A5,B7,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


// T8 = T6+T7
	// CopyV2H(A7,TS1)
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A7 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	// CopyH2H(B7,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

	// ModAdd(TS1,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	// CopyH2V(B1,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

// T9 = E*E*rinvpoly
	/*//CopyH2V(B5,A1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;*/
	//CopyH2V(B5,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2H(B5,B7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_B7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	
	//MontMult(A7,B7,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
//T10 = T9*E*rinvpoly
	//MontMult(B7,A7,P1)
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


//X3 = T8+T10

	//CopyH2H(B1,TS1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2H(B7,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//ModAdd(TS1,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2V(B1,X7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

//T11 = I*X3*rinvpoly
	//CopyV2H(A7,B1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8) 
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//MontMult(X7,B1,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2V(B1,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2H(B1,B7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


//T12 = G*G*rinvpoly
	//CopyH2V(B3,A7)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//MontMult(B3,B7,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


//T13 = T12*H*rinvpoly
	//MontMult(A3,B3,P1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8) 
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;

//Y3 = T11+T13
	
	//CopyH2H(B7,TS1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2H(B3,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//ModAdd(TS1,TC1)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8) 
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
			| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;
	//CopyH2V(B1,X5)
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8) 
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
			| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while(!mwmac_irq_var);
	mwmac_irq_var = 0;


	//  // Read Result r_b from B Register Memory 
 	// for(i=0; i<128; i++){
	// 	PointAdd_params_ptr->r_b[128-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4); //changed rw_prec/32 --> 128
	// }
	// // Read Result r_p from P Register Memory 
 	// for(i=0; i<128; i++){
	// 	PointAdd_params_ptr->r_p[128-1-i] = ioread32(MWMAC_RAM_ptr+0x2+i*0x4);
	// }
	// // Read Result r_e from E Register Memory 
 	// for(i=0; i<128; i++){
	// 	PointAdd_params_ptr->r_e[128-1-i] = ioread32(MWMAC_RAM_ptr+0x280+i);
	// }
	// // Read Result r_x from X Register Memory 
 	// for(i=0; i<128; i++){
	// 	PointAdd_params_ptr->r_x[128-1-i] = ioread32(MWMAC_RAM_ptr+0x300+i);
	// } 
	// // Read Result r_a from A Register Memory 
 	// for(i=0; i<128; i++){
	// 	PointAdd_params_ptr->r_a[128-1-i] = ioread32(MWMAC_RAM_ptr+0x200+i);
	// }




}

static void MWMAC_PointDouble(PointDouble_params_t *PointDouble_params_ptr)
{

	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if (PointDouble_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for (i = 0; i < 16; i++) {
			if (BINARY_PRECISIONS[i][0] == PointDouble_params_ptr->prec) {
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if (PointDouble_params_ptr->prec % 32) {
					rw_prec = (PointDouble_params_ptr->prec / 32 + 1) * 32;
				}
				else {
					rw_prec = PointDouble_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for (i = 0; i < 13; i++) {
			if (PRIME_PRECISIONS[i][0] == PointDouble_params_ptr->prec) {
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = PointDouble_params_ptr->prec;
			}
		}
	}

	if (PointDouble_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}

// 	Clear_MWMAC_RAM();

// 	// settings values to appropriate locations. (in real this portion will not be needed 
// 	// since we will already have values set in the memory)


// // Write Parameter n to P Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->n[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x2 + i * 0x4));
// 	}
// 	// copy P1 to P3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copy P1 to P5
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copy P1 to P7
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_P7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// creating R

// 		// MontR(P1, B1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
// 		//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// copying R to A5	
// 		// CopyH2V(B1, A5)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Creating R2
// 		// MontR2(P1, B1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR2 << 8)
// 		//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying R2 to A7
// 		// CopyH2V(B1, A7)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Write Parameter a to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->a[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing a
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying mont a to E3
// 		// CopyH2V(B1,E3) a to E3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Write Parameter b to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->b[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing b
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying Mont b to E1
// 		// CopyH2V(B1,E1) b to E1
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	/*
// 		xP=X1 ; yP=E7 ; zP=E5
// 		xQ=X7 ; yQ=X5 ; zQ=X3
// 	*/

// 	// Write Parameter xP to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->x_p[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing xP
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying xP mont to X1
// 		// CopyH2V(B1,X1)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X1 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;


// 	// Write Parameter yP to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->y_p[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing yP
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying yP mont to E7
// 		// CopyH2V(B1,E7)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copy R as zP mont to E5

// 		// CopyV2V(A5,E5) zP mont to E5
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_E5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;


// 	// CopyH2V(A5,X3) zQ mont to X3
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;




// 	// xQ=X7 ; yQ=X5 ; zQ=X3

// 	// Write Parameter xQ to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->x_q[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing xQ
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying xQ mont to X1
// 		// CopyH2V(B1,X1)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;


// 	// Write Parameter yQ to B Register Memory
// 	for (i = 0; i < rw_prec / 32; i++) {
// 		iowrite32(PointDouble_params_ptr->y_q[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
// 	}

// 	//Montgomerizing yQ
// 		// MontMult(A7, B1, P1)
// 		//            Start     Abort       f_sel     sec_calc        precision         operation
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
// 		//			src_addr     			dest_addr    		src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// 	// Copying yQ mont to X5
// 		// CopyH2V(B1,E7)
// 	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
// 		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
// 		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
// 	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
// 	while (!mwmac_irq_var);
// 	mwmac_irq_var = 0;

// /*	
// 		// Read Result r_b from B Register Memory
// 		for(i=0; i<128; i++){
// 			PointDouble_params_ptr->r_b[128-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4); //changed rw_prec/32 --> 128
// 		}
// 		// Read Result r_p from P Register Memory
// 		for(i=0; i<128; i++){
// 			PointDouble_params_ptr->r_p[128-1-i] = ioread32(MWMAC_RAM_ptr+0x2+i*0x4);
// 		}
// 		// Read Result r_e from E Register Memory
// 		for(i=0; i<128; i++){
// 			PointDouble_params_ptr->r_e[128-1-i] = ioread32(MWMAC_RAM_ptr+0x280+i);
// 		}
// 		// Read Result r_x from X Register Memory
// 		for(i=0; i<128; i++){
// 			PointDouble_params_ptr->r_x[128-1-i] = ioread32(MWMAC_RAM_ptr+0x300+i);
// 		}
// 		// Read Result r_a from A Register Memory
// 		for(i=0; i<128; i++){
// 			PointDouble_params_ptr->r_a[128-1-i] = ioread32(MWMAC_RAM_ptr+0x200+i);
// 		}
// */	


	// // // // Doubling // // // /
	//===============================================================================
	//	1		t_a = X_i * X_i * rinvpoly
	//
	//	copyV2H		X_i[X7]		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_X7 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		t_a			eq		MontMult(X_i[X7], X_i[B1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	2		t_b = t_a * t_a * rinvpoly
	//
	//	copyH2V		t_a[B1]		to		[A7]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		t_b			eq		MontMult(t_a[A7], t_a[B1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2V		t_b[B1]		to		[A5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	3		t_c = Z_i * Z_i * rinvpoly
	//
	//	copyV2H		Z_i[X3]		to		[B7]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_B7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyV2H		Z_i[X3]		to		[B5]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_B5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		t_c			eq		MontMult(Z_i[X3], Z_i[B5])		to		[B5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	4		t_d = t_c * t_c * rinvpoly
	//
	//	copyH2V		t_c[B5]		to		[A1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2H		t_c[B5]		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		t_d			eq		MontMult(t_c[A1], t_c[B1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	5		t_1 = t_d * t_d * rinvpoly
	//
	//	copyH2V		t_d[B1]		to		[A1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		t_1			eq		MontMult(t_d[A1], t_d[B1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	6		t_2 = B * t_1 * rinvpoly
	//
	//	calc		t_2			eq		MontMult(B[E1], t_1[B1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_E1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2H		t_2[B1]		to		[B3]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	7		t_3 = Y_i * Z_i * rinvpoly
	//
	//	calc		t_3			eq		MontMult(Y_i[X5], Z_i[B7])		to		[B7]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	8		Z_R = X_i * t_c * rinvpoly
	//
	//	calc		Z_R			eq		MontMult(X_i[X7], t_c[B5])		to		[B5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2V		Z_R[B5]		to		[X3]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
		//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_X3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	9		X_R = t_b + t_2
	//
	//	copyV2H		t_b[A5]		to		[TS1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_A5 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//	copyH2H		t_2[B3]		to		[TC1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		X_R			eq		MontAdd(t_b[TS1], t_2[TC1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8)
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
		| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//	copyH2H		X_R[B1]		to		[B3]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2V		X_R[B1]		to		[X7]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	10		t_4 = t_b * Z_R * rinvpoly
	//
	//	calc		t_4			eq		MontMult(t_b[A5], Z_R[B5])		to		[B5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2V		t_4[B5]		to		[A5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_A5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	11		t_5 = t_a + t_3 * rinvpoly
	//
	//	copyV2H		t_a[A7]		to		[TS1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_A7 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2H		t_3[B7]		to		[TC1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B7 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//   
	//	calc		t_5			eq		MontAdd(t_a[TS1], t_3[TC2])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8)
		//			src_addr      			dest_addr    src_addr_e   src_addr_x
		| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2H		t_5[B1]		to		[B7]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	12		t_6 = t_5 + Z_R
	//
	//	copyH2H		t_5[B1]		to		[TS1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyV2H		Z_R[X3]		to		[TC1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYV2H << 8)
	//			src_addr      			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_X3 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//	calc		t_6			eq		MontAdd(t_5[TS1], Z_R[TC1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8)
	//			src_addr      			dest_addr    src_addr_e   src_addr_x
		| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	13		t_7 = t_6 * X_R * rinvpoly
	//
	//	copyH2V		t_6[B1]		to		[A1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//	calc		t_7			eq		MontMult(t_6[A1], X_R[B3])		to		[B3]
	//
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    		src_addr_e   src_addr_x
		| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//===============================================================================
	//	14		Y_R = t_4 + t_7
	//
	//	copyH2H		t_4[B5]		to		[TS1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B5 << 12) | (MWMAC_RAM_TS1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2H		t_7[B3]		to		[TC1]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_TC1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	calc		Y_R			eq		MontAdd(t_4[TS1], t_7[TC1])		to		[B1]
	//            Start     Abort       f_sel     sec_calc        precision              operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MODADD << 8)
		//			src_addr      			dest_addr    src_addr_e   src_addr_x
		| (MWMAC_RAM_TS1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//	copyH2V		Y_R[B1]		to		[X5]
	//            Start     Abort       f_sel     sec_calc        precision         operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    			src_addr_e   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;

	// // Read Result r_b from B Register Memory 
	// for (i = 0; i < 128; i++) {
	// 	PointDouble_params_ptr->r_b[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x3 + i * 0x4); //changed rw_prec/32 --> 128
	// }
	// // Read Result r_p from P Register Memory 
	// for (i = 0; i < 128; i++) {
	// 	PointDouble_params_ptr->r_p[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x2 + i * 0x4);
	// }
	// // Read Result r_e from E Register Memory 
	// for (i = 0; i < 128; i++) {
	// 	PointDouble_params_ptr->r_e[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x280 + i);
	// }
	// // Read Result r_x from X Register Memory 
	// for (i = 0; i < 128; i++) {
	// 	PointDouble_params_ptr->r_x[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x300 + i);
	// }
	// // Read Result r_a from A Register Memory 
	// for (i = 0; i < 128; i++) {
	// 	PointDouble_params_ptr->r_a[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x200 + i);
	// }




}

static void MWMAC_PostOp(PostOp_params_t* PostOp_params_ptr)
{

	u32 i;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;

	if (PostOp_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for (i = 0; i < 16; i++) {
			if (BINARY_PRECISIONS[i][0] == PostOp_params_ptr->prec) {
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if (PostOp_params_ptr->prec % 32) {
					rw_prec = (PostOp_params_ptr->prec / 32 + 1) * 32;
				}
				else {
					rw_prec = PostOp_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for (i = 0; i < 13; i++) {
			if (PRIME_PRECISIONS[i][0] == PostOp_params_ptr->prec) {
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = PostOp_params_ptr->prec;
			}
		}
	}

	if (PostOp_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}

	// // Post Operation // // /
	//
	//========================================================================================================================
	//	1	write		exp			to		B1
	for (i = 0; i < rw_prec / 32; i++) {
		iowrite32(PostOp_params_ptr->exp[rw_prec / 32 - 1 - i], (MWMAC_RAM_ptr + 0x3 + i * 0x4));
	}
	//
	//========================================================================================================================
	//	1	copyH2V		exp[B1]		to		A1
	//
	//		      Start     Abort			 f_sel				 sec_calc				precision            operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//			src_addr      			dest_addr    		src_addr_e	  src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	2	calc		R 			eq		MontR(n(x))									to		B1
	//
	//    	      Start      Abort			f_sel				sec_calc				precision			operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTR << 8)
	//			src_addr      			dest_addr    		src_addr_e   	src_addr_x
		| (MWMAC_RAM_P1 << 12) | (MWMAC_RAM_B1 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	2	copyH2V		R[B1] 			to		A3
	//
	//		      Start     Abort			 f_sel				 sec_calc				precision            operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//		src_addr      			dest_addr    		src_addr_e	   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	3	calc		Z_Ri		eq		MontExp(R[A3], R[B1], exp[A1], Z_R[X3])		to		B1 and A3
	//
	//			 Start      Abort       f_sel					 sec_calc				 precision			 operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTEXP << 8)
	//			src_addr      			dest_addr    		src_addr_e   			src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (MWMAC_RAM_A1 << 22) | (MWMAC_RAM_X3 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
// *************++++++++++++***********
	//========================================================================================================================
	//	4	copyH2H		Z_Ri[B1]	to		B3
	//
	//			  Start      Abort			 f_sel				 sec_calc				 precision			 operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	//		src_addr      			dest_addr    		 src_addr_e    src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//

	//========================================================================================================================
	//	4	calc		Z_Ri2		eq		MontMult(Z_Ri[A3], Z_Ri[B1])				to		B1
	//
	//			  Start      Abort			f_sel				 sec_calc				precision			operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    	 src_addr_e    src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	// //========================================================================================================================
	// //	4	copyH2H		Z_Ri2[B1]	to		B3
	// //
	// //			  Start      Abort			 f_sel				 sec_calc				 precision			 operation
	// mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2H << 8)
	// //		src_addr      			dest_addr    		 src_addr_e    src_addr_x
	// 	| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_B3 << 17) | (0x0 << 22) | (0x0 << 27);
	// iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	// while (!mwmac_irq_var);
	// mwmac_irq_var = 0;
	// //
	//========================================================================================================================
	//	4	copyH2V		Z_Ri2[B1]	to		A3
	//
	//		      Start     Abort			 f_sel				 sec_calc				precision            operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (COPYH2V << 8)
	//		src_addr      			dest_addr    		src_addr_e	   src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	5	calc		Z_Ri3		eq		MontMult(Z_Ri2[A3], Z_Ri2[B3])				to		B3
	//
	//			  Start      Abort			f_sel				 sec_calc				precision			operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    	 src_addr_e    src_addr_x
		| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_A3 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	6	calc		X_Ra		eq		MontMult(X_R[X7], Z_Ri2[B1])				to		B1
	//
	//			  Start      Abort			f_sel				 sec_calc				precision			operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    	 src_addr_e    src_addr_x
		| (MWMAC_RAM_B1 << 12) | (MWMAC_RAM_X7 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	7	calc		Y_Ra		eq		MontMult(Y_R[X5], Z_Ri3[B3])				to		B3
	//
	//			  Start      Abort			f_sel				 sec_calc				precision			operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT << 8)
	//			src_addr     			dest_addr    	 src_addr_e    src_addr_x
		| (MWMAC_RAM_B3 << 12) | (MWMAC_RAM_X5 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	8	calc		X_fin		eq		MontMult1(X_Ra[B1])							to 		B1
	//
	//			  Start      Abort			 f_sel				sec_calc				precision			  operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT1 << 8)
	//			src_addr		  dest_addr     src_addr_e    src_addr_x
		| (MWMAC_RAM_B1 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;
	//
	//========================================================================================================================
	//	9	calc		Y_fin		eq 		MontMult1(Y_Ra[B3])							to		B3
	//			  Start      Abort			 f_sel				sec_calc				precision			  operation
	mwmac_cmd = (1 << 0) | (0 << 1) | (mwmac_f_sel << 2) | (mwmac_sec_calc << 3) | (mwmac_cmd_prec << 4) | (MONTMULT1 << 8)
	//			src_addr		  dest_addr     src_addr_e    src_addr_x
		| (MWMAC_RAM_B3 << 12) | (0x0 << 17) | (0x0 << 22) | (0x0 << 27);
	iowrite32(mwmac_cmd, MWMAC_CMD_ptr);
	while (!mwmac_irq_var);
	mwmac_irq_var = 0;


	// Read Result r_b from B Register Memory 
	for (i = 0; i < 128; i++) {
		PostOp_params_ptr->r_b[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x3 + i * 0x4); //changed rw_prec/32 --> 128
	}
	// Read Result r_p from P Register Memory 
	for (i = 0; i < 128; i++) {
		PostOp_params_ptr->r_p[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x2 + i * 0x4);
	}
	// Read Result r_e from E Register Memory 
	for (i = 0; i < 128; i++) {
		PostOp_params_ptr->r_e[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x280 + i);
	}
	// Read Result r_x from X Register Memory 
	for (i = 0; i < 128; i++) {
		PostOp_params_ptr->r_x[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x300 + i);
	}
	// Read Result r_a from A Register Memory 
	for (i = 0; i < 128; i++) {
		PostOp_params_ptr->r_a[128 - 1 - i] = ioread32(MWMAC_RAM_ptr + 0x200 + i);
	}
}

static void MWMAC_PointMult(PointMult_params_t *PointMult_params_ptr)
{

	u32 i,j,pos,m;
	u32 mwmac_cmd = 0;
	u32 mwmac_cmd_prec = 0;
	u32 mwmac_f_sel = 1;
	u32 mwmac_sec_calc = 0;
	u32 rw_prec = 0;
	u32 word = 0x0;
    u32 byte = 0x0;
    u32 bit = 0x0;
    

	if (PointMult_params_ptr->f_sel == 0) {
		mwmac_f_sel = 0;
		for (i = 0; i < 16; i++) {
			if (BINARY_PRECISIONS[i][0] == PointMult_params_ptr->prec) {
				mwmac_cmd_prec = BINARY_PRECISIONS[i][1];
				if (PointMult_params_ptr->prec % 32) {
					rw_prec = (PointMult_params_ptr->prec / 32 + 1) * 32;
				}
				else {
					rw_prec = PointMult_params_ptr->prec;
				}
			}
		}
	}
	else {
		mwmac_f_sel = 1;
		for (i = 0; i < 13; i++) {
			if (PRIME_PRECISIONS[i][0] == PointMult_params_ptr->prec) {
				mwmac_cmd_prec = PRIME_PRECISIONS[i][1];
				rw_prec = PointMult_params_ptr->prec;
			}
		}
	}

	if (PointMult_params_ptr->sec_calc == 0) {
		mwmac_sec_calc = 0;
	}
	else {
		mwmac_sec_calc = 1;
	}
	u32 bits[rw_prec];

// calling functions!!!

	MWMAC_Prep(PointMult_params_ptr);
	// MWMAC_PointDouble(PointMult_params_ptr);
	// MWMAC_PointAdd(PointMult_params_ptr);
	// MWMAC_PointDouble(PointMult_params_ptr);
	// MWMAC_PointDouble(PointMult_params_ptr);
	m = rw_prec-1;
	for(i=0; i<rw_prec/32; i++){
        word = PointMult_params_ptr->kk[rw_prec/32-1-i];
        for(j=0;j<8;j++)
        {   
            byte = word & 0xf;
            for(pos=0;pos<4;pos++)
            {
                bit = byte & 0x1;
                bits[m] = bit;
                byte = byte >> 1;
                m--;
            }
            word = word >> 4;
        }
    }
	pos = 0;
    while(pos<rw_prec)
    {
        if(bits[pos]==1)
        {
            break;
        }
        pos++;
    }
    for(i=pos+1;i<rw_prec;i++)
    {
		MWMAC_PointDouble(PointMult_params_ptr);
        if(bits[i]==1)
		{
			MWMAC_PointAdd(PointMult_params_ptr);
		}
    }
	// for(i=0; i<rw_prec; i++){
	// 	PointMult_params_ptr->kk[i] = bits[i]; //changed rw_prec/32 --> 128
	// }
	
	
	//MWMAC_PostOp(PointMult_params_ptr);
	
	
	//Read Result r_b from B Register Memory 
 	for(i=0; i<128; i++){
		PointMult_params_ptr->r_b[128-1-i] = ioread32(MWMAC_RAM_ptr+0x3+i*0x4); //changed rw_prec/32 --> 128
	}
	// Read Result r_p from P Register Memory 
 	for(i=0; i<128; i++){
		PointMult_params_ptr->r_p[128-1-i] = ioread32(MWMAC_RAM_ptr+0x2+i*0x4);
	}
	// Read Result r_e from E Register Memory 
 	for(i=0; i<128; i++){
		PointMult_params_ptr->r_e[128-1-i] = ioread32(MWMAC_RAM_ptr+0x280+i);
	}
	// Read Result r_x from X Register Memory 
 	for(i=0; i<128; i++){
		PointMult_params_ptr->r_x[128-1-i] = ioread32(MWMAC_RAM_ptr+0x300+i);
	} 
	// Read Result r_a from A Register Memory 
 	for(i=0; i<128; i++){
		PointMult_params_ptr->r_a[128-1-i] = ioread32(MWMAC_RAM_ptr+0x200+i);
	}

}



module_init( cryptocore_init );
module_exit( cryptocore_exit );

MODULE_AUTHOR("MAI - Selected Topics of Embedded Software Development II");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("The driver for the FPGA-based CryptoCore");
MODULE_SUPPORTED_DEVICE("CryptoCore");

