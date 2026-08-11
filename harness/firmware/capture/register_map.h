/*
 * register_map.h - minimal STM32F411CEU6 register map.
 *
 * Only the peripherals this firmware touches: RCC, GPIOA, GPIOB, GPIOC,
 * I2C1, I2C3, TIM2, FLASH interface, and the Cortex-M4 DWT cycle counter.
 * No vendor CMSIS/HAL, per
 * docs/adr/2026-08-12-capture-engine-firmware-is-bare-metal.md.
 *
 * Base addresses and struct offsets checked against DocID026289 Rev 4 and
 * the STM32F4 peripheral memory map. The AHB3 reset and enable registers
 * are folded into the RESERVED arrays below rather than named, since this
 * part has no AHB3 peripherals; the offsets they occupy are still
 * accounted for, which is what the struct layout depends on.
 *
 * Bit positions here are from RM0383. Where a value encodes a field
 * rather than a single bit, the field's position and width are given so a
 * reader can check it without opening the manual.
 */
#ifndef CAPTURE_ENGINE_REGISTER_MAP_H
#define CAPTURE_ENGINE_REGISTER_MAP_H

#include <stdint.h>

#define __IO volatile
#define __I  volatile const
#define __O  volatile

/* ---- Peripheral base addresses ---- */
#define PERIPH_BASE         0x40000000UL
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define APB1PERIPH_BASE     (PERIPH_BASE)

#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800UL)
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE        (AHB1PERIPH_BASE + 0x3C00UL)

#define TIM2_BASE           (APB1PERIPH_BASE + 0x0000UL)
#define I2C1_BASE            (APB1PERIPH_BASE + 0x5400UL)
#define I2C3_BASE            (APB1PERIPH_BASE + 0x5C00UL)

#define DWT_BASE            0xE0001000UL
#define DEMCR_ADDR          0xE000EDFCUL

/* ---- GPIO ---- */
typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFR[2]; /* AFR[0] = AFRL, AFR[1] = AFRH */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIOC_BASE)

/* ---- RCC ---- */
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t PLLCFGR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    uint32_t      RESERVED0[2];
    __IO uint32_t APB1RSTR;
    __IO uint32_t APB2RSTR;
    uint32_t      RESERVED1[2];
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    uint32_t      RESERVED2[2];
    __IO uint32_t APB1ENR;
    __IO uint32_t APB2ENR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *) RCC_BASE)

#define RCC_CR_HSION        (1UL << 0)
#define RCC_CR_HSIRDY       (1UL << 1)
#define RCC_CR_PLLON        (1UL << 24)
#define RCC_CR_PLLRDY       (1UL << 25)

/* PLLCFGR field positions and widths, RM0383 section 6.3.2. The register
 * holds five separate fields plus reserved bits whose reset values must
 * survive; write it read-modify-write with these masks, never wholesale.
 *
 * PLLM  bits  5:0   division for the PLL input clock
 * PLLN  bits 14:6   multiplication for the VCO
 * PLLP  bits 17:16  division for the main system clock, encoded (P/2)-1
 * PLLSRC bit  22    0 = HSI, 1 = HSE
 * PLLQ  bits 27:24  division for the 48 MHz USB clock
 */
#define RCC_PLLCFGR_PLLM_Pos   0UL
#define RCC_PLLCFGR_PLLM_Msk   (0x3FUL << RCC_PLLCFGR_PLLM_Pos)
#define RCC_PLLCFGR_PLLN_Pos   6UL
#define RCC_PLLCFGR_PLLN_Msk   (0x1FFUL << RCC_PLLCFGR_PLLN_Pos)
#define RCC_PLLCFGR_PLLP_Pos   16UL
#define RCC_PLLCFGR_PLLP_Msk   (0x3UL << RCC_PLLCFGR_PLLP_Pos)
#define RCC_PLLCFGR_PLLSRC_Msk (1UL << 22)
#define RCC_PLLCFGR_PLLSRC_HSI (0UL << 22)
#define RCC_PLLCFGR_PLLQ_Pos   24UL
#define RCC_PLLCFGR_PLLQ_Msk   (0xFUL << RCC_PLLCFGR_PLLQ_Pos)

#define RCC_CFGR_SW_Msk      (3UL << 0)
#define RCC_CFGR_SW_PLL      (2UL << 0)
#define RCC_CFGR_SWS_Msk     (3UL << 2)
#define RCC_CFGR_SWS_PLL     (2UL << 2)
#define RCC_CFGR_HPRE_Msk    (0xFUL << 4)
#define RCC_CFGR_HPRE_DIV1   (0UL << 4)  /* AHB = SYSCLK */
#define RCC_CFGR_PPRE1_Msk   (7UL << 10)
#define RCC_CFGR_PPRE1_DIV2  (4UL << 10) /* APB1 = HCLK/2 */

#define RCC_AHB1ENR_GPIOAEN  (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN  (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN  (1UL << 2)

#define RCC_APB1ENR_TIM2EN   (1UL << 0)
#define RCC_APB1ENR_I2C1EN   (1UL << 21)
#define RCC_APB1ENR_I2C3EN   (1UL << 23)

/* ---- FLASH interface (wait states) ---- */
typedef struct {
    __IO uint32_t ACR;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *) FLASH_R_BASE)
#define FLASH_ACR_LATENCY_3WS (3UL << 0)
#define FLASH_ACR_PRFTEN      (1UL << 8)
#define FLASH_ACR_ICEN        (1UL << 9)
#define FLASH_ACR_DCEN        (1UL << 10)

/* ---- I2C ---- */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t OAR1;
    __IO uint32_t OAR2;
    __IO uint32_t DR;
    __IO uint32_t SR1;
    __IO uint32_t SR2;
    __IO uint32_t CCR;
    __IO uint32_t TRISE;
    __IO uint32_t FLTR;
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *) I2C1_BASE)
#define I2C3 ((I2C_TypeDef *) I2C3_BASE)

#define I2C_CR1_PE           (1UL << 0)
#define I2C_CR1_START        (1UL << 8)
/* CR2 FREQ is bits 5:0, carrying the APB1 rate in whole MHz (2..50). The
 * value is derived from the clock plan in timing_budget.h, not defined
 * here, so it cannot drift out of agreement with PCLK1. */
#define I2C_CR2_FREQ_Msk     (0x3FUL << 0)

/* ---- TIM2 (32-bit general purpose timer) ---- */
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *) TIM2_BASE)
#define TIM_CR1_CEN          (1UL << 0)

/* ---- Cortex-M4 DWT cycle counter ---- */
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t CYCCNT;
} DWT_TypeDef;

#define DWT ((DWT_TypeDef *) DWT_BASE)
#define DEMCR ((__IO uint32_t *) DEMCR_ADDR)
#define DEMCR_TRCENA         (1UL << 24)
#define DWT_CTRL_CYCCNTENA   (1UL << 0)

#endif /* CAPTURE_ENGINE_REGISTER_MAP_H */
