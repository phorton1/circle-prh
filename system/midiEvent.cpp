#include "midiEvent.h"
#include <utils/myUtils.h>
#include <circle/logger.h>
#include <circle/usb/usbmidi.h>
#include <circle/devicenameservice.h>
#include <circle/usb/usbstring.h>


#define log_name  "midi"


midiSystem *midiSystem::s_pMidiSystem = 0;


CString *getDeviceString(CUSBDevice *usb_device, u8 which)
{
	const TUSBDeviceDescriptor *pDesc = usb_device->GetDeviceDescriptor();
	u8 id =
		which == 0 ? pDesc->iManufacturer	:
		which == 1 ? pDesc->iProduct		:
		which == 2 ? pDesc->iSerialNumber	: 0;
	if (!id) return 0;
	if (id == 0xff) return 0;

	CUSBString usb_string(usb_device);
	if (usb_string.GetFromDescriptor(id, usb_string.GetLanguageID()))
	{
		return new CString(usb_string.Get());
	}
	return 0;
}


void midiSystem::Initialize(CSerialDevice *pSerial)
{
	LOG("initialize(%08x)",(u32)pSerial);
	m_pSerial = pSerial;

	unsigned dev_num = 1;
	boolean found = 1;
	while (found)
	{
		CString dev_name;
		dev_name.Format("umidi%d",dev_num);
		CUSBMIDIDevice *pMidiDevice = (CUSBMIDIDevice *) // : public CUSBFunction : public CDevice
			CDeviceNameService::Get()->GetDevice (dev_name, FALSE);

		if (pMidiDevice)
		{
			CUSBDevice *usb_device = pMidiDevice->GetDevice();
			LOG("found MIDI DEVICE[%d]: %08x",dev_num,pMidiDevice);
			CString *vendor_name = usb_device->GetName(DeviceNameVendor);
			CString *device_name = usb_device->GetName(DeviceNameDevice);
			LOG("    usb_device(%08x) addr(%d) vendor_name(%s) device_name(%s)",
				(u32)usb_device,
				usb_device->GetAddress(),
				vendor_name ? ((const char *)*vendor_name) : "null",
				device_name ? ((const char *)*device_name) : "null");
			if (vendor_name)
				delete vendor_name;
			if (device_name)
				delete device_name;

			// this is how you get to the device strings
			// which requies access to specific members of
			// the TUSBDeviceDescriptor ...

			CString *p_str0 = getDeviceString(usb_device,0);
			CString *p_str1 = getDeviceString(usb_device,1);
			CString *p_str2 = getDeviceString(usb_device,2);
			LOG("    manuf(%s) product(%s) serial(%s)",
				p_str0 ? ((const char *)*p_str0) : "empty",
				p_str1 ? ((const char *)*p_str1) : "empty",
				p_str2 ? ((const char *)*p_str2) : "empty");
			delete p_str0;
			delete p_str1;
			delete p_str2;

			dev_num++;
			pMidiDevice->RegisterPacketHandler(staticMidiPacketHandler);
		}
		else
		{
			found = 0;
		}
	}
}



void midiSystem::registerMidiHandler(
	void *pObject,
	handleMidiEventFxn pMethod,
	s16 cable,
	s16 channel,
	midiEventType_t type,
	s16 value1,
	s16 value2)
{
	#if 0
		LOG("registerMidiHandler(0x%08X,0x%02X,0x%02X,%d,0x%02X,0x%02x)",
			(u32)pObject,
			(cable&0xff),channel&0xff,type,value1&0xff,value2&0xff);
	#endif

	midiEventHandler *handler = new midiEventHandler(pObject,pMethod,cable,channel,type,value1,value2);

	if (m_pLastHandler)
	{
		handler->m_pPrevHandler = m_pLastHandler;
		m_pLastHandler->m_pNextHandler = handler;
	}
	else
	{
		m_pFirstHandler = handler;
	}
	m_pLastHandler = handler;

	int count = 0;
	midiEventHandler *cur = m_pFirstHandler;
	while (cur)
	{
		count++;
		cur = cur->m_pNextHandler;
	}
	// LOG("    numHandlers=%d",count);
}


void midiSystem::unRegisterMidiHandler(
	void *pObject,
	handleMidiEventFxn pMethod,
	s16 cable,
	s16 channel,
	midiEventType_t type,
	s16 value1,
	s16 value2)
{
	#if 0
		LOG("unRegisterMidiHandler(0x%08X,0x%02X,0x%02X,%d,0x%02X,0x%02X)",
			(u32)pObject,
			(cable&0xff),channel&0xff,type,value1&0xff,value2&0xff);
	#endif

	midiEventHandler *found = 0;
	midiEventHandler *cur = m_pFirstHandler;
	while (cur)
	{
		if (pObject == cur->m_pObject &&
			pMethod == cur->m_pMethod &&
			cable == cur->m_cable &&
			channel == cur->m_channel &&
			type == cur->m_type &&
			value1 == cur->m_value1 &&
			value2 == cur->m_value2)
		{
			found = cur;
			break;
		}
		cur = cur->m_pNextHandler;
	}
	if (found)
	{
		LOG("    deleting midi handler",0);
		if (found == m_pLastHandler)
		{
			m_pLastHandler = found->m_pPrevHandler;
		}
		if (found == m_pFirstHandler)
		{
			m_pFirstHandler = found->m_pNextHandler;
		}
		if (found->m_pNextHandler)
		{
			found->m_pNextHandler->m_pPrevHandler = found->m_pPrevHandler;
		}
		if (found->m_pPrevHandler)
		{
			found->m_pPrevHandler->m_pNextHandler = found->m_pNextHandler;
		}
		delete found;
	}
	else
	{
		LOG_WARNING("   could not find midi handler to unRegister",0);
	}
}





void midiSystem::dispatchEvents()
{
	m_spinlock.Acquire();
	midiEvent *pEvent = m_pFirstEvent;
	midiEvent *pLast  = m_pLastEvent;
	m_pFirstEvent = 0;
	m_pLastEvent = 0;
	m_spinlock.Release();

	if (m_pSerial)
	{
		u8 buf[6];
		int num_read = m_pSerial->Read(buf,5);
		while (num_read)
		{
			// display_bytes("midi",buf,num_read);
			if (num_read == 5)	// add a midi event
			{
				midiEvent *pNewEvent = new midiEvent(
					buf[0],
					buf[1],
					buf[2],
					buf[3],
					buf[4],
					0,
					0);

				// m_spinlock.Acquire();

				if (!pEvent)
					pEvent = pNewEvent;
				if (pLast)
					pLast->m_pNextEvent = pNewEvent;
				pLast = pNewEvent;

				// m_spinlock.Release();

			}
			else
			{
				LOG_ERROR("wrong_midi %d",num_read);
			}
			num_read = m_pSerial->Read(buf,5);
		}
	}


	while (pEvent)
	{
		#if 0
			LOG("dispatching(0x%02X,0x%02X,0x%02x,0x%02X,0x%02X)",
				(pEvent->m_cable 	& 0xff),
				(pEvent->m_channel	& 0xff),
				(pEvent->m_type 	& 0xff),
				(pEvent->m_value1)  & 0xff),
				(pEvent->m_value1() & 0xff));
		#endif

		midiEventHandler *cur = m_pFirstHandler;
		while (cur)
		{
			#if 0
				LOG("    checking cur(0x%02X,0x%02X,%d,0x%02X,0x%02X)",
					(cur->m_cable & 0xff),
					(cur->m_channel & 0xff),
					cur->m_type,
					(cur->m_value1) & 0xff),
					(cur->m_value1() & 0xff));
			#endif

			if (((cur->m_cable   == -1)  || (cur->m_cable   == pEvent->m_channel  )) &&
				((cur->m_channel == -1)  || (cur->m_channel == pEvent->m_channel  )) )
			{
				s16 msg = pEvent->m_msg;
				s16 p1 = pEvent->m_value1;
				s16 p2 = pEvent->m_value2;

				if ((cur->m_type == MIDI_EVENT_TYPE_NOTE) &&
					(msg == MIDI_EVENT_NOTE_ON ||
					 msg == MIDI_EVENT_NOTE_OFF) &&
					(cur->m_value1 == -1 || cur->m_value1 == p1) &&
					(cur->m_value2 == -1 || cur->m_value2 == p2))
				{
					(cur->m_pMethod)(cur->m_pObject,pEvent);
				}
				else
				if ((cur->m_type == MIDI_EVENT_TYPE_CC) &&
					(msg == MIDI_EVENT_CC) &&
					(cur->m_value1 == -1 || cur->m_value1 == p1) &&
					(cur->m_value2 == -1 || cur->m_value2 == p2))
				{
					(cur->m_pMethod)(cur->m_pObject,pEvent);
				}
				else
				if ((cur->m_type == MIDI_EVENT_TYPE_INC_DEC1) &&
					(msg == MIDI_EVENT_CC) &&
					(p1 == MIDI_CC_DECREMENT ||
					 p1 == MIDI_CC_INCREMENT) &&
					(cur->m_value1 == -1 || cur->m_value1 == pEvent->m_nrpn0) &&
					(cur->m_value2 == -1 || cur->m_value2 == pEvent->m_nrpn1))
				{
					(cur->m_pMethod)(cur->m_pObject,pEvent);
				}
				else
				if ((cur->m_type == MIDI_EVENT_TYPE_INC_DEC2) &&
					(msg == MIDI_EVENT_CC) &&
					(cur->m_value1 == -1 || cur->m_value1 == p1))
				{
					// CREATE A NEW EVENT !!!

					midiEvent *newEvent = new midiEvent(pEvent);
					newEvent->m_value1 = p2 >= 0x40 ? MIDI_CC_DECREMENT : MIDI_CC_INCREMENT;
					newEvent->m_value2 = p2 >= 0x40 ? 0x80 - p2 : p2;
					(cur->m_pMethod)(cur->m_pObject,newEvent);
					delete newEvent;

				}
			}
			cur = cur->m_pNextHandler;
		}

		midiEvent *ptr = pEvent;
		pEvent = pEvent->m_pNextEvent;
		delete ptr;

	}
}




void midiSystem::midiPacketHandler(unsigned cable, u8 *pPacket, unsigned length)
{
	// The packet contents are just normal MIDI data - see
	// https://www.midi.org/specifications/item/table-1-summary-of-midi-message

	// MPD218 note packats have a length of 2
	// control packets have a length of 3
	// if (length < 3)
	// {
	// 	return;
	// }

	u8 status  = pPacket[0];
	u8 channel = status & 0x0F;
	u8 msg     = status >> 4;
	u8 param1  = pPacket[1];			// the key for note on and off events
	u8 param2  = pPacket[2];			// velocity for note on and off events

	#if 0
		LOG("midiPacketHandler(0x%02X,0x%02X,0x%02X,0x%02X,0x%02X)",
			(cable&0xff),channel&0xff,msg&0xff,param1&0xff,param2&0xff);
	#endif




	static u8 nrpn[2] = {0x7f,0x7f};

	if (msg == MIDI_EVENT_CC &&
		param1 == MIDI_EVENT_MSB)
	{
		nrpn[0] = param2;
	}
	else if (msg == MIDI_EVENT_CC &&
			 param1 == MIDI_EVENT_LSB)
	{
		nrpn[1] = param2;
	}
	else
	{
		// LOG("midPacket(length=%d cable=%d channel=%d msg=%d param1=%d param2=%d)",length,cable,channel,msg,param1,param2);

		midiEvent *pEvent = new midiEvent(
			cable,
			channel,
			msg,
			param1,
			param2,
			nrpn[0],
			nrpn[1]);

		m_spinlock.Acquire();

		if (!m_pFirstEvent)
			m_pFirstEvent = pEvent;
		if (m_pLastEvent)
			m_pLastEvent->m_pNextEvent = pEvent;
		m_pLastEvent = pEvent;

		m_spinlock.Release();
	}
}
