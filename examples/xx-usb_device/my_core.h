#ifndef __MY_COREH__
#define __MY_COREH__

// stuff to replace linux/drivers/usb/core 


#define MAX_EPS  16

#define DWC2_CTRL_BUFF_SIZE 8

#define DWC2_SPEED_PARAM_HIGH	0
#define DWC2_SPEED_PARAM_FULL	1

#define DWC2_PHY_TYPE_PARAM_FS		0	// Full Speed PHY (default)
#define DWC2_PHY_TYPE_PARAM_UTMI	1	// UTMI+ 
#define DWC2_PHY_TYPE_PARAM_ULPI	2	// ULPI

// my common definitions of behavior

#define MY_USB_SPEED            	DWC2_SPEED_PARAM_HIGH			//
#define MY_USB_AHBCFG			   -1								// don't understand this
#define MY_USB_PHY_TYPE         	DWC2_PHY_TYPE_PARAM_FS			//
#define MY_USB_I2C_ENABLE  			0								//
#define MY_USB_ULPI_FS_LS           0								//
#define MY_USB_DMA_ENABLE  			0								//
#define MY_USB_DMA_DESC_ENABLE  	0								// requires MY_USB_DMA_ENAMBE
#define MY_USB_PHY_ULPI_DDR         0								// don't understand this
#define MY_USB_PHY_UTMI_WIDTH       0								//
#define MY_USB_EXTERNAL_PIN_CTRL	0								//


typedef struct
	// there is one of these for each endpoint-direction
	// the same endpoint could have two ... in and out
{
	// struct usb_ep           ep;					// The gadget layer representation of the endpoint.
	// struct list_head        queue;               // Queue of requests for this endpoint.
	// struct dwc2_hsotg_req   *req;                // The current request that the endpoint is processing

	u8   index;                 // index into "all eps"
	u8	 ep_num;
	u16	 ep_max;				// maximum packet size
	bool dir_in;                // Set to true if this endpoint is of the IN direction,
	char name[10];				// The name for the endpoint
	
	u32	total_data;				// The total number of data bytes done
	u16 size_loaded;            // The last loaded size for DxEPTSIZE for periodic IN
	u16 last_load;              // The offset of data for the last start of request
	u16 fifo_load;              // The amount of data loaded into the FIFO (periodic IN)
	u16	fifo_index;             //
	
	u8    mc;                   // Multi Count - number of transactions per microframe
	u8    interval;             // Interval for periodic endpoints
	bool  halted;               // Set if the endpoint has been halted
	bool  periodic;             // Set if this is a periodic ep, such as Interrupt
	bool  isochronous;          // Set if this is a isochronous ep
	bool  send_zlp;             // Set if we need to send a zero-length packet
	bool  has_correct_parity;

} peripheral_ep_t;



typedef struct 
{
	const char *gadget_name;

	u8 max_speed;
	u8 num_eps;
	u8 num_in_eps;
	u8 num_out_eps;
	
	peripheral_ep_t ep0;
	peripheral_ep_t *all_eps[MAX_EPS];		// includes ep0, twice
	peripheral_ep_t *eps_in[MAX_EPS];		// does not include ep0
	peripheral_ep_t *eps_out[MAX_EPS];		// does not include ep0
	
	u8 *ep0_buff;
	u8 *ctrl_buff;
	
	u16 g_rx_fifo_sz;
	u16 g_np_g_tx_fifo_sz;
	u8  phyif;
	
} usb_peripheral_device_t;




typedef struct 
{
	u32 snpsid;
	u32 op_mode;
	u32 arch;
	u32 enable_dynamic_fifo;
	u32 host_channels;
	u32 hs_phy_type;
	u32 fs_phy_type;
	u32 num_dev_ep;
	u32 nperio_tx_q_depth;
	u32 host_perio_tx_q_depth;
	u32 dev_token_q_depth;
	u32 max_transfer_size;
	u32 max_packet_count;
	u32 i2c_enable;
	u32 total_fifo_size;
	u32 en_multiple_tx_fifo;
	u32 num_dev_perio_in_ep;
	u32 dma_desc_enable;
	u32 power_optimized;
	u32 utmi_phy_data_width;
	u32 host_rx_fifo_size;
	u32 host_nperio_tx_fifo_size;
	u32 host_perio_tx_fifo_size;
	
} usb_hardware_t;





//-----------------------------------------------
// api
//-----------------------------------------------

#define dwc2_writel(val,a)  	dbg_writel(val,a,#a)
#define dwc2_readl(a)  			readl(a)

extern usb_hardware_t *hw;
extern usb_peripheral_device_t *pd;

extern void dbg_writel(u32 val, u32 addr, const char *name);
extern u32 readl(u32 addr);

extern void initPeripheralConfig();
extern void initPeripheral();
extern void debugDumpUsbRegisters();
extern void debugHWParams();

extern void enable_common_interrupts();
extern void clear_common_interrupts();
extern void enable_ep_interrupt(u8 ep_num, bool in);
extern void disable_ep_interrupt(u8 ep_num, bool in);
extern void clear_ep_interrupt(u8 ep_num, bool in);


#endif // __MY_COREH__
