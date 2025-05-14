/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Placeholder wrapper to allow addressing Allwinner A64 (and later) sun50i
 * CPU based devices separately. Please do not add anything in here.
 */
#ifndef __CONFIG_H
#define __CONFIG_H

// #include <configs/sunxi-common.h>

#define CFG_SYS_SDRAM_BASE		0x40000000
#define CFG_SYS_INIT_RAM_ADDR	CONFIG_SUNXI_SRAM_ADDRESS
#define CFG_SYS_INIT_RAM_SIZE	0x8000 /* 32 KiB */
#define CFG_SYS_NS16550_CLK		24000000

#if !CONFIG_IS_ENABLED(DM_SERIAL)
#include <asm/arch/serial.h>
#define CFG_SYS_NS16550_COM1		SUNXI_UART0_BASE
#define CFG_SYS_NS16550_COM2		SUNXI_UART1_BASE
#define CFG_SYS_NS16550_COM3		SUNXI_UART2_BASE
#define CFG_SYS_NS16550_COM4		SUNXI_UART3_BASE
#define CFG_SYS_NS16550_COM5		SUNXI_R_UART_BASE
#endif

#define PHYS_SDRAM_0			CFG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		0x60000000 /* 1.5 GiB */

#define CFG_EXTRA_ENV_SETTINGS \
	"arch=arm\0" \
	"baudrate=115200\0" \
	"board=sunxi\0" \
	"cpu=armv8\0" \
	"console=ttyS0,115200\0" \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0" \
	"scriptaddr=0x4FF00000\0" \
	"bootcmd=mmc dev 0; " \
		"load mmc 0:1 ${scriptaddr} /boot/boot.scr; " \
		"source ${scriptaddr}\0"

#endif /* __CONFIG_H */
