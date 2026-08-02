/**
 * @file uni-STC/chipid.h
 * 
 * SFR definitions for the chip ID and other special parameters.
 */

#ifndef _UNISTC_CHIPID_H
#define _UNISTC_CHIPID_H

#if MCU_FAMILY == 8
	#if defined(CHIPID_IN_XDATA)
		/*
		 * This is the most reliable and comprehensive API to access 
		 * these parameters, but only a few MCU series support it: 
		 * 
		 * - STC8A8K64D4
		 * - STC8H1K17T
		 * - STC8H2K17U
		 * - STC8H4K64TLCD
		 * - STC8H8K64U
		 */
		
		// Chip globally unique ID
		__xdata const uint8_t __at(0xFDE0) CHIP_GUID[7];
		
		__xdata const uint8_t __at(0xFDE7) __OTHER_INFO[4];
		
		// Internal bandgap reference voltage in mV (normally 1190).
		#define INTERNAL_VOLTAGE_REF_mV ((uint16_t) ((__OTHER_INFO[0] << 8) | __OTHER_INFO[1]))
		
		// Power-down wake-up timer frequency in Hz (normally 32768).
		#define WKT_FREQ_Hz ((uint16_t) ((__OTHER_INFO[2] << 8) | __OTHER_INFO[3]))
		
		// IRTRIM values for predefined IRC frequencies ----------------
		// 27MHz band
		__xdata const uint8_t __at(0xFDEB) IRTRIM_22_1184MHz;
		__xdata const uint8_t __at(0xFDEC) IRTRIM_24MHz;
		
		#ifdef MCU_HAS_USB
			// 27MHz band
			__xdata const uint8_t __at(0xFDED) IRTRIM_27MHz;
			__xdata const uint8_t __at(0xFDEE) IRTRIM_30MHz;
			__xdata const uint8_t __at(0xFDEF) IRTRIM_33_1776MHz;
			// 44MHz band
			__xdata const uint8_t __at(0xFDF0) IRTRIM_35MHz;
			__xdata const uint8_t __at(0xFDF1) IRTRIM_36_864MHz;
			__xdata const uint8_t __at(0xFDF2) IRTRIM_40MHz;
			__xdata const uint8_t __at(0xFDF3) IRTRIM_44_2368MHz;
			__xdata const uint8_t __at(0xFDF4) IRTRIM_48MHz;
		#else
			// 27MHz band
			__xdata const uint8_t __at(0xFDED) IRTRIM_20MHz;
			__xdata const uint8_t __at(0xFDEE) IRTRIM_27MHz;
			__xdata const uint8_t __at(0xFDEF) IRTRIM_30MHz;
			__xdata const uint8_t __at(0xFDF0) IRTRIM_33_1776MHz;
			// 44MHz band
			__xdata const uint8_t __at(0xFDF1) IRTRIM_35MHz;
			__xdata const uint8_t __at(0xFDF2) IRTRIM_36_864MHz;
			__xdata const uint8_t __at(0xFDF3) IRTRIM_40MHz;
			__xdata const uint8_t __at(0xFDF4) IRTRIM_45MHz;
		#endif
		
		// VRTRIM values for the different frequency bands -------------
		__xdata const uint8_t __at(0xFDF5) VRTRIM_6MHz;
		__xdata const uint8_t __at(0xFDF6) VRTRIM_10MHz;
		__xdata const uint8_t __at(0xFDF7) VRTRIM_27MHz;
		__xdata const uint8_t __at(0xFDF8) VRTRIM_44MHz;
		
		/*
		 * The IRC is normally configured during firmware download.
		 * However, if you want to change it's frequency at run time,
		 * you can use the above predefined values. For instance, if 
		 * you want to set the IRC frequency to 35 MHz, you will do: 
		 *     IRTRIM = IRTRIM_35MHz;
		 *     VRTRIM = VRTRIM_44MHz;
		 * (because 35MHz is in the 44MHz band).
		 */
		
		// Size of the flash area that can be used for program storage.
		// Also corresponds to the program/EEPROM split address when 
		// the MCU supports IAP.
		__xdata const uint16_t __at(0xFDF9) PROGRAM_AREA_SIZE;
		
		// Date of the chip's test, BCD-coded.
		__xdata const uint8_t __at(0xFDFB) CHIP_TEST_YEAR_BCD;
		__xdata const uint8_t __at(0xFDFC) CHIP_TEST_MONTH_BCD;
		__xdata const uint8_t __at(0xFDFD) CHIP_TEST_DAY_BCD;
		
		typedef enum {
			PACKAGE_DIP8 = 0x00,
			PACKAGE_SOP8 = 0x01,
			PACKAGE_DFN8 = 0x02,
			PACKAGE_DIP16 = 0x10,
			PACKAGE_SOP16 = 0x11,
			PACKAGE_DIP18 = 0x20,
			PACKAGE_SOP18 = 0x21,
			PACKAGE_DIP20 = 0x30,
			PACKAGE_SOP20 = 0x31,
			PACKAGE_TSSOP20 = 0x32,
			PACKAGE_LSSOP20 = 0x33,
			PACKAGE_QFN20 = 0x34,
			PACKAGE_SKDIP28 = 0x40,
			PACKAGE_SOP28 = 0x41,
			PACKAGE_TSSOP28 = 0x42,
			PACKAGE_QFN28 = 0x43,
			PACKAGE_SOP32 = 0x50,
			PACKAGE_LQFP32 = 0x51,
			PACKAGE_QFN32 = 0x52,
			PACKAGE_PLCC32 = 0x53,
			PACKAGE_QFN32S = 0x54,
			PACKAGE_PDIP40 = 0x60,
			PACKAGE_LQFP44 = 0x70,
			PACKAGE_PLCC44 = 0x71,
			PACKAGE_PQFP44 = 0x72,
			PACKAGE_LQFP48 = 0x80,
			PACKAGE_QFN48 = 0x81,
			PACKAGE_LQFP64 = 0x90,
			PACKAGE_LQFP64S = 0x91,
			PACKAGE_LQFP64L = 0x92,
			PACKAGE_LQFP64M = 0x93,
			PACKAGE_QFN64 = 0x94,
		} PackageType;
		
		// Chip package type.
		__xdata const PackageType __at(0xFDFE) CHIP_PACKAGE_TYPE;
	#elif defined(CHIPID_IN_CODE)
		/*
		 * If we know the flash size, we can access these parameters at 
		 * the end of the __code segment. However, by default, only the 
		 * chip GUID is set. A specific setting must be used when flashing 
		 * the MCU with STC-ISP in order to get the other parameters, 
		 * otherwise they'll default to 0xFF.
		 */
		
		#ifdef FLASH_SIZE
			__code const uint8_t __at(FLASH_SIZE - 7) CHIP_GUID[7];
			__code const uint8_t __at(FLASH_SIZE - 11) __OTHER_INFO[4];
			#define INTERNAL_VOLTAGE_REF_mV ((uint16_t) ((__OTHER_INFO[2] << 8) | __OTHER_INFO[3]))
			#define WKT_FREQ_Hz ((uint16_t) ((__OTHER_INFO[0] << 8) | __OTHER_INFO[1]))
			
			// 20/27MHz band
			__code const uint8_t __at(FLASH_SIZE - 12) IRTRIM_22_1184MHz;
			__code const uint8_t __at(FLASH_SIZE - 13) IRTRIM_24MHz;
			
			#ifndef MCU_MAX_FREQ_MHZ
				#error "The MCU_MAX_FREQ_MHZ macro must be defined."
			#endif
			
			#if MCU_MAX_FREQ_MHZ >= 45
				#ifdef MCU_HAS_USB
					// 27MHz band
					__code const uint8_t __at(FLASH_SIZE - 14) IRTRIM_27MHz;
					__code const uint8_t __at(FLASH_SIZE - 15) IRTRIM_30MHz;
					__code const uint8_t __at(FLASH_SIZE - 16) IRTRIM_33_1776MHz;
					// 44MHz band
					__code const uint8_t __at(FLASH_SIZE - 17) IRTRIM_35MHz;
					__code const uint8_t __at(FLASH_SIZE - 18) IRTRIM_36_864MHz;
					__code const uint8_t __at(FLASH_SIZE - 19) IRTRIM_40MHz;
					__code const uint8_t __at(FLASH_SIZE - 20) IRTRIM_44_2368MHz;
					__code const uint8_t __at(FLASH_SIZE - 21) IRTRIM_48MHz;
				#else
					// 27MHz band
					__code const uint8_t __at(FLASH_SIZE - 14) IRTRIM_20MHz;
					__code const uint8_t __at(FLASH_SIZE - 15) IRTRIM_27MHz;
					__code const uint8_t __at(FLASH_SIZE - 16) IRTRIM_30MHz;
					__code const uint8_t __at(FLASH_SIZE - 17) IRTRIM_33_1776MHz;
					// 44MHz band
					__code const uint8_t __at(FLASH_SIZE - 18) IRTRIM_35MHz;
					__code const uint8_t __at(FLASH_SIZE - 19) IRTRIM_36_864MHz;
					__code const uint8_t __at(FLASH_SIZE - 20) IRTRIM_40MHz;
					__code const uint8_t __at(FLASH_SIZE - 21) IRTRIM_45MHz;
				#endif
				
				// VRTRIM values for the different frequency bands -------------
				__code const uint8_t __at(FLASH_SIZE - 22) VRTRIM_6MHz;
				__code const uint8_t __at(FLASH_SIZE - 23) VRTRIM_10MHz;
				__code const uint8_t __at(FLASH_SIZE - 24) VRTRIM_27MHz;
				__code const uint8_t __at(FLASH_SIZE - 25) VRTRIM_44MHz;
			#elif MCU_MAX_FREQ_MHZ > 30
				// 20MHz band
				__code const uint8_t __at(FLASH_SIZE - 14) IRTRIM_20MHz;
				__code const uint8_t __at(FLASH_SIZE - 15) IRTRIM_27MHz;
				__code const uint8_t __at(FLASH_SIZE - 16) IRTRIM_30MHz;
				__code const uint8_t __at(FLASH_SIZE - 17) IRTRIM_33_1776MHz;
				// 35MHz band
				__code const uint8_t __at(FLASH_SIZE - 18) IRTRIM_35MHz;
				__code const uint8_t __at(FLASH_SIZE - 19) IRTRIM_36_864MHz;
				
				// VRTRIM values for the different frequency bands -------------
				__code const uint8_t __at(FLASH_SIZE - 22) VRTRIM_20MHz;
				__code const uint8_t __at(FLASH_SIZE - 23) VRTRIM_35MHz;
			#endif
		#else
			#warning "The FLASH_SIZE macro must be defined to allow access to the chip ID."
			#warning "See comments starting at chipid.h:196 to calculate its value."
			/*
			 * On an MCU with less than 64K flash, FLASH_SIZE must be set 
			 * to the entire flash value, e.g. 28672 for an STC8H1K28.
			 * 
			 * (It looks like an MCU supporting IAP with less than 64K 
			 * flash has its default EEPROM size set to 0, instead of 
			 * 512 bytes on models with 64K flash.)
			 * 
			 * On an MCU with 64K flash, FLASH_SIZE must be set to the 
			 * entire flash minus the EEPROM size, e.g. 65024 for an 
			 * STC8H3K64S2 with the default flash/EEPROM split.
			 */
		#endif
	#endif
	
	// The first 2 bytes of CHIP_GUID represent the part ID, e.g. 
	// F784 for STC8H8K64U, F847 for STC8H1K17T, etc.
	#define CHIP_PART_ID ((uint16_t) ((CHIP_GUID[0] << 8) | CHIP_GUID[1]))
#endif // MCU_FAMILY == 8

#endif // _UNISTC_CHIPID_H
