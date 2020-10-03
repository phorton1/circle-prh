#include "kernel.h"
#include <circle/types.h>
#include <circle/util.h>
#include <circle/alloc.h>
#include <circle/usb/dwhci.h>
#include "hw.h"
#include "gadget.h"
#include "my_core.h"

usb_hardware_t hw_params;
usb_hardware_t *hw = &hw_params;
usb_peripheral_device_t peripheral_device;
usb_peripheral_device_t *pd = &peripheral_device;


//------------------------
// primitives
//------------------------


void dbg_writel(u32 val, u32 addr, const char *name)
{
	printf("    %-12s [%08lx] <= %08lX\n",name,addr,val);
	*((volatile u32 *) addr) = val;
}

u32 readl(u32 addr)
{
	return *((u32 *) addr);
}


//----------------------------------
// methods
//----------------------------------

peripheral_ep_t *create_ep(u8 ep_num, bool in, u16 ep_max, const char *name)
{
	printf("    create_ep(%d,%d,%d,%s)\n",ep_num,in,ep_max,name);
	peripheral_ep_t *ep = new peripheral_ep_t;
	memset(ep,0,sizeof(peripheral_ep_t));
	
	ep->dir_in = in;
	ep->ep_num = ep_num;
	ep->ep_max = ep_max;
	strcpy(ep->name,name);

	ep->index = pd->num_eps++;
	pd->all_eps[ep->index] = ep;
	if (in)
		pd->eps_in[pd->num_in_eps++] = ep;
	else
		pd->eps_out[pd->num_out_eps++] = ep;
	
	return ep;
}



//-----------------------------------------------
// debugging
//-----------------------------------------------

void dbg_reg(int idx, const char* dwc_name, u32 addr, const char *name)
{
	volatile int vol_i = 0;
	static u32 save_reg[50];
	u32 val = *(u32 *) addr;
	if (1)	// *dwc_name)
	{
		int alt = 0;
		if (save_reg[idx] != val)
		{
			save_reg[idx] = val;
			alt = 1;
		}
		printf("%s%-15s  %-30s  0x%08lx : 0x%08X\n",alt?"!":"",dwc_name,name,addr,val);
		for (vol_i=0; vol_i<1000000; vol_i++);
	}
}


void debugDumpUsbRegisters()
{
	printf("\n----------------Core Global Registers\n");
	
	dbg_reg( 1,"GOTGCTL",	DWHCI_CORE_OTG_CTRL, 			"DWHCI_CORE_OTG_CTRL");
	dbg_reg( 2,"GOTGINT",	DWHCI_CORE_OTG_INT,  			"DWHCI_CORE_OTG_INT");
	dbg_reg( 3,"GAHBCFG",	DWHCI_CORE_AHB_CFG,  			"DWHCI_CORE_AHB_CFG");
	dbg_reg( 4,"GUSBCFG",	DWHCI_CORE_USB_CFG, 			"DWHCI_CORE_USB_CFG");
	dbg_reg( 5,"GRSTCTL",	DWHCI_CORE_RESET, 				"DWHCI_CORE_RESET");
	dbg_reg( 6,"GINTSTS",	DWHCI_CORE_INT_STAT, 			"DWHCI_CORE_INT_STAT");
	dbg_reg( 7,"GINTMSK",	DWHCI_CORE_INT_MASK, 			"DWHCI_CORE_INT_MASK");
	dbg_reg( 8,"GRXSTSR",	DWHCI_CORE_RX_STAT_RD, 			"DWHCI_CORE_RX_STAT_RD");
	
	// should pop the tx status during a debug probe
	// dbg_reg( 9,"GRXSTSP",	DWHCI_CORE_RX_STAT_POP,	 		"DWHCI_CORE_RX_STAT_POP");
	
	dbg_reg(10,"GRXFSIZ",	DWHCI_CORE_RX_FIFO_SIZ, 		"DWHCI_CORE_RX_FIFO_SIZ");
	dbg_reg(11,"GNPTXFSIZ",	DWHCI_CORE_NPER_TX_FIFO_SIZ,	"DWHCI_CORE_NPER_TX_FIFO_SIZ");
	dbg_reg(12,"GNPTXSTS",	DWHCI_CORE_NPER_TX_STAT, 		"DWHCI_CORE_NPER_TX_STAT");
	dbg_reg(13,"GI2CCTL",	DWHCI_CORE_I2C_CTRL, 			"DWHCI_CORE_I2C_CTRL");
	dbg_reg(14,"GPVNDCTL",	DWHCI_CORE_PHY_VENDOR_CTRL, 	"DWHCI_CORE_PHY_VENDOR_CTRL");
	dbg_reg(15,"GGPIO",		DWHCI_CORE_GPIO, 				"DWHCI_CORE_GPIO");
	dbg_reg(16,"GUID",		DWHCI_CORE_USER_ID, 			"DWHCI_CORE_USER_ID");
	dbg_reg(17,"GSNPSID",	DWHCI_CORE_VENDOR_ID, 			"DWHCI_CORE_VENDOR_ID");
	dbg_reg(18,"GHWCFG1",	DWHCI_CORE_HW_CFG1, 			"DWHCI_CORE_HW_CFG1");
	dbg_reg(19,"GHWCFG2",	DWHCI_CORE_HW_CFG2, 			"DWHCI_CORE_HW_CFG2");
	dbg_reg(20,"GHWCFG3",	DWHCI_CORE_HW_CFG3, 			"DWHCI_CORE_HW_CFG3");
	dbg_reg(21,"GHWCFG4",	DWHCI_CORE_HW_CFG4, 			"DWHCI_CORE_HW_CFG4");
	dbg_reg(22,"GLPMCFG",	DWHCI_CORE_LPM_CFG, 			"DWHCI_CORE_LPM_CFG");
	dbg_reg(23,"GPWRDN",	DWHCI_CORE_POWER_DOWN, 			"DWHCI_CORE_POWER_DOWN");
	dbg_reg(24,"GDFIFOCFG", DWHCI_CORE_DFIFO_CFG, 			"DWHCI_CORE_DFIFO_CFG");
	dbg_reg(25,"",			DWHCI_CORE_ADP_CTRL, 			"DWHCI_CORE_ADP_CTRL");
	dbg_reg(26,"",			DWHCI_VENDOR_MDIO_CTRL, 		"DWHCI_VENDOR_MDIO_CTRL");
	dbg_reg(27,"",			DWHCI_VENDOR_MDIO_DATA, 		"DWHCI_VENDOR_MDIO_DATA");
	dbg_reg(28,"",			DWHCI_VENDOR_VBUS_DRV, 			"DWHCI_VENDOR_VBUS_DRV");
	dbg_reg(29,"",			DWHCI_CORE_HOST_PER_TX_FIFO_SIZ,"DWHCI_CORE_HOST_PER_TX_FIFO_SIZ");

	// should not read fifos during a debug probe!
	// dbg_reg(30,"HPTXFSIZ",	DWHCI_CORE_DEV_PER_TX_FIFO(0), 	"DWHCI_CORE_DEV_PER_TX_FIFO");
	// dbg_reg(31,"",			DWHCI_CORE_DEV_TX_FIFO(0),		"DWHCI_CORE_DEV_TX_FIFO");

	printf("---------------- Device Mode Registers\n");
	
	dbg_reg(32,"DCFG",			DCFG,		"");
	dbg_reg(33,"DCTL",			DCTL,		"");
	dbg_reg(34,"DSTS",			DSTS,		"");
	dbg_reg(35,"DIEPMSK",		DIEPMSK,	"");
	dbg_reg(36,"DOEPMSK",		DOEPMSK,	"");
	dbg_reg(37,"DAINT",			DAINT,		"");
	dbg_reg(38,"DAINTMSK",		DAINTMSK,	"");
	dbg_reg(39,"DTKNQR1",		DTKNQR1,	"");
	dbg_reg(40,"DTKNQR2",		DTKNQR2,	"");
	dbg_reg(41,"DTKNQR3",		DTKNQR3,	"");
	dbg_reg(42,"DTKNQR4",		DTKNQR4,	"");
	dbg_reg(43,"DVBUSDIS",		DVBUSDIS,	"");
	dbg_reg(44,"DVBUSPULSE",	DVBUSPULSE,	"");
	dbg_reg(45,"DIEPCTL0",		DIEPCTL0,	"");
	dbg_reg(46,"DOEPCTL0",		DOEPCTL0,	"");
	// should not read fifos during a debug probe!
	// dbg_reg(47,"EPFIFO(0)",		EPFIFO(0),	"");
	dbg_reg(48,"PCGCTL",	    PCGCTL,		"PCGCTL");

}

void debugHWParams()
{
	unsigned width;
	u32 hwcfg1, hwcfg2, hwcfg3, hwcfg4, grxfsiz;
	memset(hw,0,sizeof(usb_hardware_t));
	
	// Attempt to ensure this device is really a DWC_otg Controller.
	// Read and verify the GSNPSID register contents. The value should be
	// 0x45f42xxx or 0x45f43xxx, which corresponds to either "OT2" or "OT3",
	// as in "OTG version 2.xx" or "OTG version 3.xx".
	
	hw->snpsid = dwc2_readl(GSNPSID);
	if ((hw->snpsid & 0xfffff000) != 0x4f542000 &&
	    (hw->snpsid & 0xfffff000) != 0x4f543000)
	{
		printf("Bad value for GSNPSID: 0x%08x\n",
			hw->snpsid);
		return;
	}

	printf("\nCore Release: %1x.%1x%1x%1x (snpsid=%x)\n",
		hw->snpsid >> 12 & 0xf, hw->snpsid >> 8 & 0xf,
		hw->snpsid >> 4 & 0xf, hw->snpsid & 0xf, hw->snpsid);

	hwcfg1 = dwc2_readl(GHWCFG1);
	hwcfg2 = dwc2_readl(GHWCFG2);
	hwcfg3 = dwc2_readl(GHWCFG3);
	hwcfg4 = dwc2_readl(GHWCFG4);
	grxfsiz = dwc2_readl(GRXFSIZ);

	printf("hwcfg1=%08x\n", hwcfg1);
	printf("hwcfg2=%08x\n", hwcfg2);
	printf("hwcfg3=%08x\n", hwcfg3);
	printf("hwcfg4=%08x\n", hwcfg4);
	printf("grxfsiz=%08x\n",grxfsiz);

	#if 0
		// Force host mode to get HPTXFSIZ / GNPTXFSIZ exact power on value 
		
		u32 gusbcfg = dwc2_readl(GUSBCFG);
		gusbcfg |= GUSBCFG_FORCEHOSTMODE;
		dwc2_writel(gusbcfg,GUSBCFG);
		CKernel::Get()->m_Timer.MsDelay(100);
	
		u32 gnptxfsiz = dwc2_readl(GNPTXFSIZ);
		u32 hptxfsiz = dwc2_readl(HPTXFSIZ);
		printf("gnptxfsiz=%08x\n", gnptxfsiz);
		printf("hptxfsiz=%08x\n", hptxfsiz);
		gusbcfg = dwc2_readl(GUSBCFG);
		gusbcfg &= ~GUSBCFG_FORCEHOSTMODE;
		dwc2_writel(gusbcfg,GUSBCFG);
		CKernel::Get()->m_Timer.MsDelay(100);
	#endif
	
	/* hwcfg2 */
	
	hw->op_mode = (hwcfg2 & GHWCFG2_OP_MODE_MASK) >>
		      GHWCFG2_OP_MODE_SHIFT;  // mask = 1, shift = 0, sigh
			  
	hw->arch = (hwcfg2 & GHWCFG2_ARCHITECTURE_MASK) >>
		   GHWCFG2_ARCHITECTURE_SHIFT;
	hw->enable_dynamic_fifo = !!(hwcfg2 & GHWCFG2_DYNAMIC_FIFO);
	hw->host_channels = 1 + ((hwcfg2 & GHWCFG2_NUM_HOST_CHAN_MASK) >>
				GHWCFG2_NUM_HOST_CHAN_SHIFT);
	hw->hs_phy_type = (hwcfg2 & GHWCFG2_HS_PHY_TYPE_MASK) >>
			  GHWCFG2_HS_PHY_TYPE_SHIFT;

	hw->fs_phy_type = (hwcfg2 & GHWCFG2_FS_PHY_TYPE_MASK) >>
			  GHWCFG2_FS_PHY_TYPE_SHIFT;
	hw->num_dev_ep = (hwcfg2 & GHWCFG2_NUM_DEV_EP_MASK) >>
			 GHWCFG2_NUM_DEV_EP_SHIFT;
	hw->nperio_tx_q_depth =
		(hwcfg2 & GHWCFG2_NONPERIO_TX_Q_DEPTH_MASK) >>
		GHWCFG2_NONPERIO_TX_Q_DEPTH_SHIFT << 1;
	hw->host_perio_tx_q_depth =
		(hwcfg2 & GHWCFG2_HOST_PERIO_TX_Q_DEPTH_MASK) >>
		GHWCFG2_HOST_PERIO_TX_Q_DEPTH_SHIFT << 1;
	hw->dev_token_q_depth =
		(hwcfg2 & GHWCFG2_DEV_TOKEN_Q_DEPTH_MASK) >>
		GHWCFG2_DEV_TOKEN_Q_DEPTH_SHIFT;

	/* hwcfg3 */
	
	width = (hwcfg3 & GHWCFG3_XFER_SIZE_CNTR_WIDTH_MASK) >>
		GHWCFG3_XFER_SIZE_CNTR_WIDTH_SHIFT;
	hw->max_transfer_size = (1 << (width + 11)) - 1;
	/*
	 * Clip max_transfer_size to 65535. dwc2_hc_setup_align_buf() allocates
	 * coherent buffers with this size, and if it's too large we can
	 * exhaust the coherent DMA pool.
	 */
	if (hw->max_transfer_size > 65535)
		hw->max_transfer_size = 65535;
	width = (hwcfg3 & GHWCFG3_PACKET_SIZE_CNTR_WIDTH_MASK) >>
		GHWCFG3_PACKET_SIZE_CNTR_WIDTH_SHIFT;
	hw->max_packet_count = (1 << (width + 4)) - 1;
	hw->i2c_enable = !!(hwcfg3 & GHWCFG3_I2C);
	hw->total_fifo_size = (hwcfg3 & GHWCFG3_DFIFO_DEPTH_MASK) >>
			      GHWCFG3_DFIFO_DEPTH_SHIFT;

	/* hwcfg4 */
	hw->en_multiple_tx_fifo = !!(hwcfg4 & GHWCFG4_DED_FIFO_EN);
	hw->num_dev_perio_in_ep = (hwcfg4 & GHWCFG4_NUM_DEV_PERIO_IN_EP_MASK) >>
				  GHWCFG4_NUM_DEV_PERIO_IN_EP_SHIFT;
	hw->dma_desc_enable = !!(hwcfg4 & GHWCFG4_DESC_DMA);
	hw->power_optimized = !!(hwcfg4 & GHWCFG4_POWER_OPTIMIZ);
	hw->utmi_phy_data_width = (hwcfg4 & GHWCFG4_UTMI_PHY_DATA_WIDTH_MASK) >>
				  GHWCFG4_UTMI_PHY_DATA_WIDTH_SHIFT;

	/* fifo sizes */
	hw->host_rx_fifo_size = (grxfsiz & GRXFSIZ_DEPTH_MASK) >>
				GRXFSIZ_DEPTH_SHIFT;
				
	// hw->host_nperio_tx_fifo_size = (gnptxfsiz & FIFOSIZE_DEPTH_MASK) >>
	// 			       FIFOSIZE_DEPTH_SHIFT;
	// hw->host_perio_tx_fifo_size = (hptxfsiz & FIFOSIZE_DEPTH_MASK) >>
	// 			      FIFOSIZE_DEPTH_SHIFT;

	printf("\nDetected values from hardware:\n");
	printf("  op_mode=%d %s\n",
		hw->op_mode,hw->op_mode & 1 ? "HOST" : "DEVICE");
	printf("  arch=%d\n",
		hw->arch);
	printf("  dma_desc_enable=%d\n",
		hw->dma_desc_enable);
	printf("  power_optimized=%d\n",
		hw->power_optimized);
	printf("  i2c_enable=%d\n",
		hw->i2c_enable);
	printf("  hs_phy_type=%d\n",
		hw->hs_phy_type);
	printf("  fs_phy_type=%d\n",
		hw->fs_phy_type);
	printf("  utmi_phy_data_width=%d\n",
		hw->utmi_phy_data_width);
	printf("  num_dev_ep=%d\n",
		hw->num_dev_ep);
	printf("  num_dev_perio_in_ep=%d\n",
		hw->num_dev_perio_in_ep);
	printf("  host_channels=%d\n",
		hw->host_channels);
	printf("  max_transfer_size=%d\n",
		hw->max_transfer_size);
	printf("  max_packet_count=%d\n",
		hw->max_packet_count);
	printf("  nperio_tx_q_depth=0x%0x\n",
		hw->nperio_tx_q_depth);
	printf("  host_perio_tx_q_depth=0x%0x\n",
		hw->host_perio_tx_q_depth);
	printf("  dev_token_q_depth=0x%0x\n",
		hw->dev_token_q_depth);
	printf("  enable_dynamic_fifo=%d\n",
		hw->enable_dynamic_fifo);
	printf("  en_multiple_tx_fifo=%d\n",
		hw->en_multiple_tx_fifo);
	printf("  total_fifo_size=%d\n",
		hw->total_fifo_size);
	printf("  host_rx_fifo_size=%d\n",
		hw->host_rx_fifo_size);
	printf("  host_nperio_tx_fifo_size=%d\n",
		hw->host_nperio_tx_fifo_size);
	printf("  host_perio_tx_fifo_size=%d\n",
		hw->host_perio_tx_fifo_size);
	printf("\n");

	return;
}



//-----------------------------------------------
// init methods
//-----------------------------------------------


void enable_ep_interrupt(u8 ep_num, bool in)
{
	u32 daintmsk = dwc2_readl(DAINTMSK);
	u32 bit = 1 << ep_num;
	if (in) bit <<= 16;
	daintmsk |= bit;
	dwc2_writel(daintmsk,DAINTMSK);
	
	u32 ep_ctl_reg = in ? DIEPCTL(ep_num) : DOEPCTL(ep_num);
	u32 ctl = dwc2_readl(ep_ctl_reg);
	ctl |= DXEPCTL_EPENA;
	dwc2_writel(ctl,ep_ctl_reg);
}


void disable_ep_interrupt(u8 ep_num, bool in)
{
	u32 daintmsk = dwc2_readl(DAINTMSK);
	u32 bit = 1 << ep_num;
	if (in) bit <<= 16;
	daintmsk &= ~bit;
	dwc2_writel(daintmsk,DAINTMSK);

	u32 ep_ctl_reg = in ? DIEPCTL(ep_num) : DOEPCTL(ep_num);
	u32 ctl = dwc2_readl(ep_ctl_reg);
	ctl |= DXEPCTL_EPDIS;
	dwc2_writel(ctl,ep_ctl_reg);
	
}

void clear_ep_interrupt(u8 ep_num, bool in)
{
	//u32 daint = dwc2_readl(DAINT);
	u32 bit = 1 << ep_num;
	// if (in) bit <<= 16;
	// daint &= ~bit;
	dwc2_writel(bit,DAINT); // daint);
	
}


void clear_common_interrupts()
{
	printf("clear_common_interrupts()\n");
	dwc2_writel(0xffffffff,GOTGINT);
	dwc2_writel(0xffffffff,GINTSTS);
}

void enable_common_interrupts()
{
	// Clear any pending OTG Interrupts 
	// Enable the interrupts in the GINTMSK */
	// Clear any pending interrupts 

	clear_common_interrupts();
	printf("enable_common_interrupts()\n");
	
	u32 intmsk =
		GINTSTS_MODEMIS |
		GINTSTS_OTGINT |
		GINTSTS_WKUPINT |
		GINTSTS_USBSUSP |
		GINTSTS_SESSREQINT;
		
	if (!MY_USB_DMA_ENABLE)
		intmsk |= GINTSTS_RXFLVL;
		
	if (!MY_USB_EXTERNAL_PIN_CTRL)
		intmsk |= GINTSTS_CONIDSTSCHNG;

	dwc2_writel(intmsk,GINTMSK);
}



void resetPeripheral()
{
	printf("resetPeripheral() ...\n");

	/* Wait for AHB master IDLE state */
	
	int count = 0;
	u32 greset = dwc2_readl(GRSTCTL);
	while (!(greset & GRSTCTL_AHBIDLE))	// 0x80000000
	{
		CKernel::Get()->m_Timer.MsDelay(30);
		if (++count > 30)
		{
			printf("timed out waiting for AHB Idle GRSTCTL=%0x\n",greset);
			return;
		}
	} 

	/* Core Soft Reset */

	count = 0;
	greset |= GRSTCTL_CSFTRST;
	dwc2_writel(greset,GRSTCTL);
	while (dwc2_readl(GRSTCTL) & GRSTCTL_CSFTRST)	// 0x00000001 
	{
		CKernel::Get()->m_Timer.MsDelay(30);
		if (++count > 30)
		{
			printf("timed out waiting for AHB Reset GRSTCTL=%0x\n",greset);
			return;
		}
	} 

	CKernel::Get()->m_Timer.MsDelay(100);
	printf("resetPeripheral() finished\n");	
}



void initPeripheral()
{
	printf("initPeripheral()..\n");

	// Basic configuration
	// i don't understand this
	// u32 trdtim = (pd->phyif == GUSBCFG_PHYIF8) ? 9 : 5;
	// usbcfg |= pd->phyif | GUSBCFG_TOUTCAL(7) |
	// 	(trdtim << GUSBCFG_USBTRDTIM_SHIFT));
	
	resetPeripheral();
	u32 usbcfg = dwc2_readl(GUSBCFG);				// starts as 0x00001400 which is USBTRDTIM masked to b0101 == 5, which we don't change
	usbcfg &= ~GUSBCFG_HNPCAP;                      // clear     0x00000200 does nothing
	usbcfg &= ~GUSBCFG_SRPCAP;						// clear 	 0x00000100 does nothing
	usbcfg &= ~GUSBCFG_FORCEHOSTMODE;				// clear     0x20000000 does nothing
	usbcfg &= ~GUSBCFG_TERMSELDLPULSE;				// clear     0x00400000 does nothing
	usbcfg &= ~GUSBCFG_ULPI_EXT_VBUS_DRV;			// clear     0x00100000 does nothing
	usbcfg |= GUSBCFG_PHYSEL;					    // add 		 0x00000040
	usbcfg |= GUSBCFG_FORCEDEVMODE;					// add       0x40000000
	dwc2_writel(usbcfg, GUSBCFG);                   // ends as   0x40001440 looks right
	resetPeripheral();

	// Be in disconnected state until gadget is registered 
	
	u32 dctl = dwc2_readl(DCTL);	// starts as 0x00000000
	dctl |= DCTL_SFTDISCON;         // add 		 0x00000002
	dwc2_writel(dctl,DCTL);			// is        0x00000002 here
		// we clear this later, so it's back to zero
	
	// DMA control (off to begin)
	
	u32 ahbcfg = dwc2_readl(GAHBCFG);		// starts as 0x00000000			
	ahbcfg &= GAHBCFG_CTRL_MASK;			// and with  0x000001A1 does nothing
	ahbcfg &= ~GAHBCFG_DMA_EN;              // clear     0x00000020 does nothing
	// ahbcfg |= MY_USB_AHBCFG & ~GAHBCFG_CTRL_MASK;  
	// ahbcfg |= GAHBCFG_DMA_EN;
	dwc2_writel(ahbcfg,GAHBCFG);			// ends as   0x00000000 write the zero back

	// Device Registers
	
	dwc2_writel(pd->g_rx_fifo_sz,	// starts as 0x00001000 == 4096
		GRXFSIZ);                   // we change 0x00000400 == 1024 
	dwc2_writel(									// starts as 0x00201000 == 0x20 (flag) bytes at address 0x1000
		(pd->g_rx_fifo_sz << FIFOSIZE_STARTADDR_SHIFT) |    // shift is zero, our size is 0x400 = 1024 
		(pd->g_np_g_tx_fifo_sz << FIFOSIZE_DEPTH_SHIFT),    // shift is 16, our addr is 0x400 = 1024
		GNPTXFSIZ);
	
			// ends as 0x04000400
			// note that GNPTXSTS gets a value after init 0x00080400
			// which appears to be saying "you have room in the TX FIFO to write stuff"
			
	// This is where it gets confusing for me.
	//
	// The gadget.c code appeared to READ GHWCFG2 and get the information from the chip,
	// My attempts to change this register are not working.
	// I thought we would  TELL IT how many end points we have,
	// But it's not working
	//
	// starts as 0x228DDD50
	// should change to 0x228DD150 but it doesn't!!
	//
	// binary 								    2    2    8    D    D    D    5    0
	//        							   b 0010 0010 1000 1101 1101 1101 0101 0000
	// GHWCFG2_OP_MODE_MASK                                                      000	0 = GHWCFG2_OP_MODE_HNP_SRP_CAPABLE
	// GHWCFG2_ARCHITECTURE_MASK                                              1 0       2 = GHWCFG2_INT_DMA_ARCH 
	// GHWCFG2_POINT2POINT                                                   0          0 = NO POINT TO POINT
	// GHWCFG2_HS_PHY_TYPE_MASK                                            01			1 = GHWCFG2_HS_PHY_TYPE_UTMI
	// GHWCFG2_FS_PHY_TYPE_MASK                                         01  			1 = GHWCFG2_FS_PHY_TYPE_DEDICATED       
	// GHWCFG2_NUM_DEV_EP_MASK                                     01 11                7 = num Dev EP's                 
	// GHWCFG2_NUM_HOST_CHAN_MASK                             01 11                     7 = num host channels
	// GHWCFG2_PERIO_EP_SUPPORTED                            1						    1 = GHWCFG2_PERIO_EP_SUPPORTED
	// GHWCFG2_DYNAMIC_FIFO                                 1                           1 = GHWCFG2_DYNAMIC_FIFO supported
	// GHWCFG2_MULTI_PROC_INT                             0								0 = !GHWCFG2_MULTI_PROC_INT
	// unused bit                                        X
	// GHWCFG2_NONPERIO_TX_Q_DEPTH_MASK                10							    2 = GHWCFG2_NONPERIO_TX_Q_DEPTH
	// GHWCFG2_HOST_PERIO_TX_Q_DEPTH_MASK           10									2 = GHWCFG2_HOST_PERIO_TX_Q_DEPTH
	// GHWCFG2_DEV_TOKEN_Q_DEPTH_MASK         010 00									8 = GHWCFG2_DEV_TOKEN_Q_DEPTH
	// GHWCFG2_OTG_ENABLE_IC_USB             0										    0 = GHWCFG2_OTG_ENABLE_IC_USB
	//
	// seems like the default is device mode.
	// I should not have to enable "OTG" as that's just for host mode
	// eveything else looks "sort of" ok ...
	
	u32 cfg2 = dwc2_readl(GHWCFG2);								// 0x228DDD50		 
	cfg2 &= ~GHWCFG2_NUM_DEV_EP_MASK;						
	cfg2 |= (pd->num_eps << GHWCFG2_NUM_DEV_EP_SHIFT);    	
	dwc2_writel(cfg2,GHWCFG2);									// 0x228DD150 not changing anything!
		
	// Set the TX Fifo's according to the max packet 
	// size in the peripheral configuration.  They
	// begin following the RX_FIFO and PERIODIC_TX_FIFO
	// and must fit within the total hw->fifo_mem.
	
	// Start with looop clearing all of them ...
	
	u32 addr = pd->g_rx_fifo_sz + pd->g_np_g_tx_fifo_sz;
	for (int i = 1; i < MAX_EPS; i++)
		dwc2_writel(0,DPTXFSIZN(i));

	// Then set the OUT fifo sizes
	
	for (int i = 1; i < pd->num_out_eps; i++)
	{
		peripheral_ep_t *ep = pd->eps_out[i];
		u8 ep_num = ep->ep_num;
		u32 size = ep->ep_max;
		if (addr + size > hw->total_fifo_size)
		{
			printf("insufficient fifo memory\n");
			size = 0;
		}
		size <<= FIFOSIZE_DEPTH_SHIFT;
		size |= addr;
		addr += size;
		dwc2_writel(size,DPTXFSIZN(ep_num));
	}

	// Set EP direction bits into GHWCFG1
	// dwc2_writel(0,GHWCFG1);
	
	// 1 = OUT
	// 2 = IN
	
	u32 cfg1 = 0;
	for (u8 i=0; i<pd->num_eps; i++)
	{
		peripheral_ep_t *ep = pd->all_eps[i];
		u8 ep_num = ep->ep_num;
		
		u32 val = ep->dir_in ? 2 : 1;
		val <<= 2 * ep_num;
		cfg1 |= val;
	}
	dwc2_writel(cfg1,GHWCFG1);
	
	// debugging to show internal state
	
	u32 cfg3 = dwc2_readl(GHWCFG3);
	u32 dbg_fifo_mem = (cfg3 >> GHWCFG3_DFIFO_DEPTH_SHIFT);
	u32 cfg4 = dwc2_readl(GHWCFG4);
	u32 dbg_dedicated_fifos = (cfg4 >> GHWCFG4_DED_FIFO_SHIFT) & 1;

	printf("EPS all(%d) in(%d) out(%d) %s fifo_mem=%d\n",
		 pd->num_eps,
		 pd->num_in_eps,
		 pd->num_out_eps,
		 dbg_dedicated_fifos ? "dedicated" : "shared",
		 dbg_fifo_mem);

	// according to p428 of the design guide, we need to ensure that
	// all fifos are flushed before continuing

	dwc2_writel(GRSTCTL,
		GRSTCTL_TXFNUM(0x10) |
		GRSTCTL_TXFFLSH |
	    GRSTCTL_RXFFLSH);

	// wait until the fifos are both flushed 
	
	u32 timeout = 100;
	while (1)
	{
		u32 val = dwc2_readl(GRSTCTL);
		if ((val & (GRSTCTL_TXFFLSH | GRSTCTL_RXFFLSH)) == 0)
			break;
		if (--timeout == 0)
		{
			printf("%s: timeout flushing fifos (GRSTCTL=%08x)\n",__func__, val);
			break;
		}
		CKernel::Get()->m_Timer.usDelay(1);	// udelay(1);
	}

	// make sure OTGCTL is turned off

	u32 otgctl = dwc2_readl(GOTGCTL);
	otgctl &= ~GOTGCTL_OTGVER;
	dwc2_writel(otgctl,GOTGCTL);

	// unmask a subset of device endpoint interrupts 

	dwc2_writel(
		DIEPMSK_TIMEOUTMSK | DIEPMSK_AHBERRMSK |
		DIEPMSK_EPDISBLDMSK | DIEPMSK_XFERCOMPLMSK,
		DIEPMSK);

	dwc2_writel(
		DOEPMSK_SETUPMSK | DOEPMSK_AHBERRMSK |
		DOEPMSK_EPDISBLDMSK | DOEPMSK_XFERCOMPLMSK,
		DOEPMSK);
	
	dwc2_writel(0,DAINTMSK);

	// Enable common interrupts 
	// Enable EP0 endpoints
	
	enable_common_interrupts();
	enable_ep_interrupt(0,true);
	enable_ep_interrupt(0,false);
	
	// wait a bit then remove the soft disconnect bit
	// should now start getting EP0 enumeration interrupts
	
	CKernel::Get()->m_Timer.MsDelay(200);
	dctl = dwc2_readl(DCTL);
	dctl &= ~DCTL_SFTDISCON;
	dwc2_writel(dctl,DCTL);
	
	
	printf("initPeripheral() finished\n");
}


void initPeripheralConfig()
{
	printf("initPeripheralConfig() ...\n");
	for (u8 i=0; i<pd->num_eps; i++)
	{
		if (pd->all_eps[i])
			free(pd->all_eps[i]);
	}
	memset(pd,0,sizeof(usb_peripheral_device_t));
	
	pd->gadget_name = "PRH DEVICE";	
	pd->max_speed = USB_SPEED_HIGH;
	printf("initPerpheralConfig(%s)\n",pd->gadget_name);
	printf("max_speed=%d\n",pd->max_speed);

	pd->g_rx_fifo_sz = 1024;
	pd->g_np_g_tx_fifo_sz = 1024;
	printf("RXFIFO size: %d\n", pd->g_rx_fifo_sz);
	printf("NonPeriodic TXFIFO size: %d\n",pd->g_np_g_tx_fifo_sz);

	create_ep(0,true,64,"EP0_IN");
	create_ep(0,false,64,"EP0_OUT");
	create_ep(1,true,64,"EP2_IN");
	create_ep(2,false,64,"EP1_OUT");
		
	pd->ctrl_buff = new u8[DWC2_CTRL_BUFF_SIZE];
	pd->ep0_buff = new u8[DWC2_CTRL_BUFF_SIZE];

	
	#if 0 	// prh starting without IRQs
	
		u32 ret = devm_request_irq(pd->dev, irq, dwc2_hsotg_irq, IRQF_SHARED,dev_name(pd->dev), hsotg);
		if (ret < 0)
		{
			dev_err(dev, "cannot claim IRQ for gadget\n");
			return ret;
		}
	#endif
	

	// PRH !!!
	
	#if 0	

		INIT_LIST_HEAD(&pd->gadget.ep_list);
		pd->gadget.ep0 = &pd->eps_out[0]->ep;
		pd->ctrl_req = dwc2_hsotg_ep_alloc_request(&pd->eps_out[0]->ep, GFP_KERNEL);
		if (!pd->ctrl_req)
		{
			printf("failed to allocate ctrl req\n");
			return;
		}
	
		for (epnum = 0; epnum < pd->num_of_eps; epnum++)
		{
			if (pd->eps_in[epnum])
				dwc2_hsotg_initep(hsotg, pd->eps_in[epnum], epnum, 1);
			if (pd->eps_out[epnum])
				dwc2_hsotg_initep(hsotg, pd->eps_out[epnum],	epnum, 0);
		}
	
		ret = usb_add_gadget_udc(dev, &pd->gadget);
		if (ret)
			return ret;

		dwc2_hsotg_dump(hsotg);
	
	#endif
	
	printf("initPeripheralConfig() finished\n");
}


