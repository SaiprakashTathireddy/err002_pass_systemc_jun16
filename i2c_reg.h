#include<systemc.h>
#include<iostream>

// I2c_CR1 register

#define CR1_PE  ( 1u<<0 )
#define CR1_ENGC  ( 1u<<6 )
#define CR1_START ( 1u<<8 )
#define CR1_STOP  ( 1u<<9 )
#define CR1_ACK   ( 1u<<10 )
#define CR1_SWRST ( 1u<<15 )
#define CR1_RW_MASK ( CR1_PE | CR1_ACK | CR1_ENGC )
#define CR1_RES ( 0xbffb )

// I2C_CR2 register

#define CR2_FREQ_MASK ( 0x3Fu )
#define CR2_ITERREN   ( 1u<<8 )
#define CR2_ITEVTEN   ( 1u<<9 )
#define CR2_ITBUFEN   ( 1u<<10 )
#define CR2_DMAEN     ( 1u<<11 )
#define CR2_LAST      ( 1u<<12 )
#define CR2_RW_MASK ( CR2_FREQ_MASK | CR2_ITERREN | CR2_ITEVTEN | CR2_ITBUFEN | CR2_DMAEN | CR2_LAST )
#define CR2_RES  ( 0x1f3f )

// I2C_SR1 register 

#define SR1_SB    ( 1u<<0 )
#define SR1_ADDR  ( 1u<<1 )
#define SR1_BTF   ( 1u<<2 )
#define SR1_ADD10 ( 1u<<3 )
#define SR1_STOPF ( 1u<<4 )
#define SR1_RXNE  ( 1u<<6 )
#define SR1_TXE   ( 1u<<7 )
#define SR1_BERR  ( 1u<<8 )
#define SR1_ARLO  ( 1u<<9 )
#define SR1_AF    ( 1u<<10 )
#define SR1_OVR   ( 1u<<11 )
#define SR1_PECERR   ( 1u<<12 )
#define SR1_TIMEOUT  ( 1u<<14 )
#define SR1_SMBALERT ( 1u<<15 )
#define SR1_ERR_MASK ( SR1_BERR | SR1_ARLO | SR1_AF | SR1_OVR | SR1_PECERR | SR1_TIMEOUT | SR1_SMBALERT )
#define SR1_RES ( 0xdfdfu )

// I2C_SR2 register

#define SR2_MSL  ( 1u<<0 )
#define SR2_BUSY ( 1u<<1 )
#define SR2_TRA  ( 1u<<2 )
#define SR2_GENCALL ( 1u<<4 )
#define SR2_SMBDEFAULT ( 1u<<5 )
#define SR2_SMBHOST ( 1u<<6 )
#define SR2_DUALF (1u<<7)
#define SR2_RES ( 0xfff7u )

// I2C_OAR1 register

#define OAR1_ADDMODE ( 1u<<15 )
#define OAR1_RW_MASK ( OAR1_ADDMODE | 0x3FFu )
#define OAR1_RES ( 0x83ff )

// I2C_OAR2 register 

#define OAR2_ENDUAL ( 1u<<0 )
#define OAR2_RW_MASK ( 0xFFu )
#define OAR2_RES  ( 0xffu )

// I2C_DR register

#define DR_RES ( 0xffu )

// I2C_CCR register 

#define CCR_DUTY ( 1u<<14 )
#define CCR_FS   ( 1u<<15 )
#define CCR_RES ( 0xcfffu )

// I2c_TRISE register

#define TRISE_RES ( 0x003f)


