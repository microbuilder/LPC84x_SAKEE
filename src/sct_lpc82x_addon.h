#ifndef __SCT_LPC82X_ADDON_H
#define __SCT_LPC82X_ADDON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "core_cm0plus.h"                           /*!< Cortex-M0PLUS processor and core peripherals                          */

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif
	
/**
  * @brief State Configurable Timer0 (SCT0)
  */

#define CONFIG_SCT0_nEV   (8)       /* Number of events */
#define CONFIG_SCT0_nRG   (8)       /* Number of match/compare registers */
#define CONFIG_SCT0_nOU   (8)       /* Number of outputs */

typedef struct
{
    __IO  uint32_t CONFIG;              /* 0x000 Configuration Register */
    union {
        __IO uint32_t CTRL_U;           /* 0x004 Control Register */
        __IO uint32_t CTRL;           	/* 0x004 Control Register */
        struct {
            __IO uint16_t CTRL_L;       /* 0x004 low control register */
            __IO uint16_t CTRL_H;       /* 0x006 high control register */
        };
    };
		union {
			__IO uint16_t LIMIT_U;					 	/* 0x008 limit register for counter U */
			__IO uint16_t LIMIT;					 		/* 0x008 limit register for counter U */
			struct {
					__IO uint16_t LIMIT_L;			 	/* 0x008 limit register for counter L */
					__IO uint16_t LIMIT_H;       	/* 0x00A limit register for counter H */
			};
		};
		
		
    union {
				__IO uint32_t HALT_U;       		/* 0x00C halt register for counter U */
				__IO uint32_t HALT;       			/* 0x00C halt register for counter U */
        struct {
						__IO uint16_t HALT_L;       /* 0x00C halt register for counter L */
						__IO uint16_t HALT_H;       /* 0x00E halt register for counter H */
        };
    };
		
    union {
				__IO uint32_t STOP_U;       		/* 0x010 stop register for counter U */
				__IO uint32_t STOP;       			/* 0x010 stop register for counter U */
        struct {
						__IO uint16_t STOP_L;       /* 0x010 stop register for counter L */
						__IO uint16_t STOP_H;       /* 0x012 stop register for counter H */
        };
    };
		
    union {
				__IO uint32_t START_U;      		/* 0x014 start register for counter U */
				__IO uint32_t START;      			/* 0x014 start register for counter U */
        struct {
						__IO uint16_t START_L;      /* 0x014 start register for counter L */
						__IO uint16_t START_H;      /* 0x016 start register for counter H */
        };
    };

         uint32_t RESERVED1[10];        /* 0x018-0x03C reserved */
		
    union {
        __IO uint32_t COUNT_U;          /* 0x040 counter register */
        __IO uint32_t COUNT;          	/* 0x040 counter register */
        struct {
            __IO uint16_t COUNT_L;      /* 0x040 counter register for counter L */
            __IO uint16_t COUNT_H;      /* 0x042 counter register for counter H */
        };
    };
		
    union {
				__IO uint32_t STATE_U;          /* 0x044 state register for counter U */
				__IO uint32_t STATE;          	/* 0x044 state register for counter U */
        struct {
						__IO uint16_t STATE_L;      /* 0x044 state register for counter L */
						__IO uint16_t STATE_H;      /* 0x046 state register for counter H */
        };
    };

    __I  uint32_t INPUT;                /* 0x048 input register */
		

    union {
				__IO uint32_t REGMODE_U;    		/* 0x04C match - capture registers mode register U */
				__IO uint32_t REGMODE;    			/* 0x04C match - capture registers mode register U */
        struct {
						__IO uint16_t REGMODE_L;    /* 0x04C match - capture registers mode register L */
						__IO uint16_t REGMODE_H;    /* 0x04E match - capture registers mode register H */
        };
    };
		
    __IO uint32_t OUTPUT;               /* 0x050 output register */
    __IO uint32_t OUTPUTDIRCTRL;        /* 0x054 Output counter direction Control Register */
    __IO uint32_t RES;                  /* 0x058 conflict resolution register */
         uint32_t RESERVED2[37];        /* 0x05C-0x0EC reserved */
    __IO uint32_t EVEN;                 /* 0x0F0 event enable register */
    __IO uint32_t EVFLAG;               /* 0x0F4 event flag register */
    __IO uint32_t CONEN;                /* 0x0F8 conflict enable register */
    __IO uint32_t CONFLAG;              /* 0x0FC conflict flag register */

    union {
        __IO union {                    /* 0x100-... Match / Capture value */
            uint32_t U;                 /*       SCTMATCH[i].U  Unified 32-bit register */
            struct {
                uint16_t L;             /*       SCTMATCH[i].L  Access to L value */
                uint16_t H;             /*       SCTMATCH[i].H  Access to H value */
            };
        } MATCH[CONFIG_SCT0_nRG];
        __I union {
            uint32_t U;                 /*       SCTCAP[i].U  Unified 32-bit register */
            struct {
                uint16_t L;             /*       SCTCAP[i].L  Access to H value */
                uint16_t H;             /*       SCTCAP[i].H  Access to H value */
            };
        } CAP[CONFIG_SCT0_nRG];
    };


         uint32_t RESERVED3[32-CONFIG_SCT0_nRG];      /* ...-0x17C reserved */

    union {
        __IO uint16_t MATCH_L[CONFIG_SCT0_nRG];       /* 0x180-... Match Value L counter */
        __I  uint16_t CAP_L[CONFIG_SCT0_nRG];         /* 0x180-... Capture Value L counter */
    };
         uint16_t RESERVED4[32-CONFIG_SCT0_nRG];      /* ...-0x1BE reserved */
    union {
        __IO uint16_t MATCH_H[CONFIG_SCT0_nRG];       /* 0x1C0-... Match Value H counter */
        __I  uint16_t CAP_H[CONFIG_SCT0_nRG];         /* 0x1C0-... Capture Value H counter */
    };

         uint16_t RESERVED5[32-CONFIG_SCT0_nRG];      /* ...-0x1FE reserved */


    union {
        __IO union {                    /* 0x200-... Match Reload / Capture Control value */
            uint32_t U;                 /*       SCTMATCHREL[i].U  Unified 32-bit register */
            struct {
                uint16_t L;             /*       SCTMATCHREL[i].L  Access to L value */
                uint16_t H;             /*       SCTMATCHREL[i].H  Access to H value */
            };
        } MATCHREL[CONFIG_SCT0_nRG];
        __IO union {
            uint32_t U;                 /*       SCTCAPCTRL[i].U  Unified 32-bit register */
            struct {
                uint16_t L;             /*       SCTCAPCTRL[i].L  Access to H value */
                uint16_t H;             /*       SCTCAPCTRL[i].H  Access to H value */
            };
        } CAPCTRL[CONFIG_SCT0_nRG];
    };

         uint32_t RESERVED6[32-CONFIG_SCT0_nRG];      /* ...-0x27C reserved */

    union {
        __IO uint16_t MATCHREL_L[CONFIG_SCT0_nRG];    /* 0x280-... Match Reload value L counter */
        __IO uint16_t CAPCTRL_L[CONFIG_SCT0_nRG];     /* 0x280-... Capture Control value L counter */
    };
         uint16_t RESERVED7[32-CONFIG_SCT0_nRG];      /* ...-0x2BE reserved */
    union {
        __IO uint16_t MATCHREL_H[CONFIG_SCT0_nRG];    /* 0x2C0-... Match Reload value H counter */
        __IO uint16_t CAPCTRL_H[CONFIG_SCT0_nRG];     /* 0x2C0-... Capture Control value H counter */
    };
         uint16_t RESERVED8[32-CONFIG_SCT0_nRG];      /* ...-0x2FE reserved */

    __IO struct {                       /* 0x300-0x3FC  SCTEVENT[i].STATE / SCTEVENT[i].CTRL*/
        uint32_t STATE;                 /* Event State Register */
        uint32_t CTRL;                  /* Event Control Register */
    } EVENT[CONFIG_SCT0_nEV];

         uint32_t RESERVED9[128-2*CONFIG_SCT0_nEV];   /* ...-0x4FC reserved */

    __IO struct {                       /* 0x500-0x57C  SCTOUT[i].SET / SCTOUT[i].CLR */
        uint32_t SET;                   /* Output n Set Register */
        uint32_t CLR;                   /* Output n Clear Register */
    } OUT[CONFIG_SCT0_nOU];

         uint32_t RESERVED10[191-2*CONFIG_SCT0_nOU];  /* ...-0x7F8 reserved */

    __I  uint32_t MODULECONTENT;        /* 0x7FC Module Content */

} LPC_SCT0_Type;
/*@}*/ /* end of group LPC8xx_SCT0 */

#define LPC_SCT0_BASE                    0x50004000UL
#define LPC_SCT0                        ((LPC_SCT0_Type           *) LPC_SCT0_BASE)

#ifdef __cplusplus
}
#endif

#endif /* __SCT_LPC82X_ADDON_H */
