#include "kernel.h"
#include <circle/types.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/usb/dwhci.h>
#include "hw.h"
#include "gadget.h"

#include "core.h"
#include "my_core.h"


//-----------------------------------------------
// "main"
//-----------------------------------------------


static struct dwc2_core_params params_bcm2835 = {
	.otg_cap						= 0,	/* HNP/SRP capable */
	.otg_ver						= 0,	/* 1.3 */
	.dma_enable						= 1,
	.dma_desc_enable				= 0,
	.speed							= 0,	/* High Speed */
	.enable_dynamic_fifo			= 1,
	.en_multiple_tx_fifo			= 1,
	.host_rx_fifo_size				= 774,	/* 774 DWORDs */
	.host_nperio_tx_fifo_size		= 256,	/* 256 DWORDs */
	.host_perio_tx_fifo_size		= 512,	/* 512 DWORDs */
	.max_transfer_size				= 65535,
	.max_packet_count				= 511,
	.host_channels					= 8,
	.phy_type						= 1,	/* UTMI */
	.phy_utmi_width					= 8,	/* 8 bits */
	.phy_ulpi_ddr					= 0,	/* Single */
	.phy_ulpi_ext_vbus				= 0,
	.i2c_enable						= 0,
	.ulpi_fs_ls						= 0,
	.host_support_fs_ls_low_power	= 0,
	.host_ls_low_power_phy_clk		= 0,	/* 48 MHz */
	.ts_dline						= 0,
	.reload_ctl						= 0,
	.ahbcfg							= 0x10,
	.uframe_sched					= 0,
	.external_id_pin_ctl			= -1,
	.hibernation					= -1,
};


struct dwc2_hsotg hsotg;

void CKernel::usbStuff(int what)
{
	if (what == 0)   // init
	{
		hsotg.dr_mode = USB_DR_MODE_PERIPHERAL;
		hsotg.core_params = &params_bcm2835;
	}
	else if (what == 1)	// task
	{
		
	}
	
	//--------------------------------
	
	if (what == 2)	// 'a'
	{
		debugHWParams();
		debugDumpUsbRegisters();
	}

	if (what == 3)	// 'b'
	{
		printf("\n");
		initPeripheralConfig();
		initPeripheral();
		printf("\n");
	}
	if (what == 4)	// 'c'
	{
		debugDumpUsbRegisters();
	}
	
	//------------------
	
	if (what == 5)  // 'd'			// initializing
	{
		printf("\nSTARTING GADGET\n\n");
		dwc2_gadget_init(&hsotg,0);
			// init gadget with no irq
		dwc2_hsotg_pullup(&hsotg,1);
		printf("\nGADGET STARTED\n\n");
	}

	if (what == 6)	// 'e'		    // process an interrupt
	{
		// clear_ep_interrupt(0,false);
		// clear_ep_interrupt(0,true);
		dwc2_hsotg_irq(&hsotg,0);
	}

	if (what == 7)	// 'f'
	{
		dwc2_hsotg_dump(&hsotg);
	}
	
}	// usbStuff()



