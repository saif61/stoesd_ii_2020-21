/* 
* cryptocore_ioctl_header.h - the header file with the ioctl definitions.
* The declarations here have to be in a header file, because
* they need to be known both the kernel module in *_driver.c
* and the application *_app.c 
*/

#include <linux/ioctl.h>

// CryptoCore Struct Declarations:
typedef struct MontMult_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 c[128];
} MontMult_params_t;

typedef struct MontR_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 r[128];
} MontR_params_t;

typedef struct MontR2_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 r2[128];
} MontR2_params_t;

typedef struct MontExp_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 b[128];
	__u32 e[128];
	__u32 c[128];
} MontExp_params_t;

typedef struct ModAdd_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 c[128];
} ModAdd_params_t;

typedef struct ModSub_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 c[128];
} ModSub_params_t;

typedef struct CopyH2V_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 a[128];
	__u32 acopy[128];
} CopyH2V_params_t;

typedef struct CopyV2V_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 a[128];
	__u32 acopy[128];
} CopyV2V_params_t;

typedef struct CopyH2H_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 a[128];
	__u32 acopy[128];
} CopyH2H_params_t;

typedef struct CopyV2H_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 a[128];
	__u32 acopy[128];
} CopyV2H_params_t;

typedef struct MontMult1_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 b[128];
	__u32 c[128];
} MontMult1_params_t;

typedef struct ModExp_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 b[128];
	__u32 e[128];
	__u32 c[128];
} ModExp_params_t;

typedef struct ModRed_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[64];
	__u32 a[128];
	__u32 c[128];
} ModRed_params_t ;

// Add CryptoCore Struct Declarations here...
typedef struct Prep_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 x[128];
	__u32 y[128];
	__u32 r_p[128];
	__u32 r_b[128];
	__u32 r_a[128];
	__u32 r_e[128];
	__u32 r_x[128];

} Prep_params_t ;

typedef struct PointAdd_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 x_p[128];
	__u32 y_p[128];
	__u32 z_p[128];
	__u32 x_q[128];
	__u32 y_q[128];
	__u32 z_q[128];
	__u32 r_p[128];
	__u32 r_b[128];
	__u32 r_a[128];
	__u32 r_e[128];
	__u32 r_x[128];

} PointAdd_params_t ;

typedef struct PointDouble_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 x_p[128];
	__u32 y_p[128];
	__u32 z_p[128];
	__u32 x_q[128];
	__u32 y_q[128];
	__u32 z_q[128];
	__u32 r_p[128];
	__u32 r_b[128];
	__u32 r_a[128];
	__u32 r_e[128];
	__u32 r_x[128];

} PointDouble_params_t;

typedef struct PointMult_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];
	__u32 a[128];
	__u32 b[128];
	__u32 x[128];
	__u32 y[128];
	__u32 r_p[128];
	__u32 r_b[128];
	__u32 r_a[128];
	__u32 r_e[128];
	__u32 r_x[128];
	__u32 kk[192];
	__u32 exp[128];

} PointMult_params_t ;

typedef struct PostOp_params{
	__u32 prec;
	__u32 f_sel;
	__u32 sec_calc;
	__u32 n[128];			
	__u32 a[128];
	__u32 b[128];
	__u32 x_p[128];
	__u32 y_p[128];
	__u32 z_p[128];
	__u32 exp[128];
	__u32 r_p[128];
	__u32 r_b[128];
	__u32 r_a[128];
	__u32 r_e[128];
	__u32 r_x[128];
} PostOp_params_t;

#define IOCTL_BASE 'k' 					// magic number

// NOTE: magic | cmdnumber | size of data to pass
#define 	IOCTL_SET_TRNG_CMD			_IOW(IOCTL_BASE,   1, __u32)
#define 	IOCTL_SET_TRNG_CTR			_IOW(IOCTL_BASE,   2, __u32)
#define 	IOCTL_SET_TRNG_TSTAB		_IOW(IOCTL_BASE,   3, __u32)
#define 	IOCTL_SET_TRNG_TSAMPLE		_IOW(IOCTL_BASE,   4, __u32)
#define 	IOCTL_READ_TRNG_FIFO		_IOR(IOCTL_BASE,   5, __u32)

#define		IOCTL_MWMAC_MONTMULT		_IOWR(IOCTL_BASE,  6, MontMult_params_t)
#define		IOCTL_MWMAC_MONTR			_IOWR(IOCTL_BASE,  7, MontR_params_t)
#define		IOCTL_MWMAC_MONTR2			_IOWR(IOCTL_BASE,  8, MontR2_params_t)
#define		IOCTL_MWMAC_MONTEXP			_IOWR(IOCTL_BASE,  9, MontExp_params_t)
#define		IOCTL_MWMAC_MODADD			_IOWR(IOCTL_BASE, 10, ModAdd_params_t)
#define		IOCTL_MWMAC_MODSUB			_IOWR(IOCTL_BASE, 11, ModSub_params_t)
#define		IOCTL_MWMAC_COPYH2V			_IOWR(IOCTL_BASE, 12, CopyH2V_params_t)
#define		IOCTL_MWMAC_COPYV2V			_IOWR(IOCTL_BASE, 13, CopyV2V_params_t)
#define		IOCTL_MWMAC_COPYH2H			_IOWR(IOCTL_BASE, 14, CopyH2H_params_t)
#define		IOCTL_MWMAC_COPYV2H			_IOWR(IOCTL_BASE, 15, CopyV2H_params_t)
#define		IOCTL_MWMAC_MONTMULT1		_IOWR(IOCTL_BASE, 16, MontMult1_params_t)
#define		IOCTL_MWMAC_MONTEXP_FULL	_IOWR(IOCTL_BASE, 17, MontExp_params_t)
#define		IOCTL_MWMAC_MODEXP			_IOWR(IOCTL_BASE, 18, ModExp_params_t)
#define		IOCTL_MWMAC_MODRED			_IOWR(IOCTL_BASE, 19, ModRed_params_t)

// Define further IOCTL commands here...

#define		IOCTL_MWMAC_PREP			_IOWR(IOCTL_BASE, 20, Prep_params_t)
#define		IOCTL_MWMAC_PADD			_IOWR(IOCTL_BASE, 21, PointAdd_params_t)
#define		IOCTL_MWMAC_PDOUBLE			_IOWR(IOCTL_BASE, 22, PointDouble_params_t)
#define		IOCTL_MWMAC_PMULT			_IOWR(IOCTL_BASE, 23, PointMult_params_t)
#define		IOCTL_MWMAC_POSTOP			_IOWR(IOCTL_BASE, 24, PostOp_params_t)