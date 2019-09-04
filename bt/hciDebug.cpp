//
// hciBase.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "_hci_defs.h"
#include "_gap_defs.h"
#include "_lcap_defs.h"
#include "hciBase.h"
#include "hciLayer.h"
#include <assert.h>
#include <circle/util.h>
#include <circle/logger.h>


void showSupportedCommand(u16 byte_num, u8 byte)
{
    switch (byte_num)
    {
        case(0) :
            if (byte & (1 << 0)) printf("HCI_Inquiry\n");
            if (byte & (1 << 1)) printf("HCI_Inquiry_Cancel\n");
            if (byte & (1 << 2)) printf("HCI_Periodic_Inquiry_Mode\n");
            if (byte & (1 << 3)) printf("HCI_Exit_Periodic_Inquiry_Mode\n");
            if (byte & (1 << 4)) printf("HCI_Create_Connection\n");
            if (byte & (1 << 5)) printf("HCI_Disconnect\n");
            if (byte & (1 << 6)) printf("HCI_Add_SCO_Connection\n");
            if (byte & (1 << 7)) printf("HCI_Create_Connection_Cancel\n");
            break;
        case (1) :
            if (byte & (1 << 0)) printf("HCI_Accept_Connection_Request\n");
            if (byte & (1 << 1)) printf("HCI_Reject_Connection_Request\n");
            if (byte & (1 << 2)) printf("HCI_Link_Key_Request_Reply\n");
            if (byte & (1 << 3)) printf("HCI_Link_Key_Request_Negative_Reply\n");
            if (byte & (1 << 4)) printf("HCI_PIN_Code_Request_Reply\n");
            if (byte & (1 << 5)) printf("HCI_PIN_Code_Request_Negative_Reply\n");
            if (byte & (1 << 6)) printf("HCI_Change_Connection_Packet_Type\n");
            if (byte & (1 << 7)) printf("HCI_Authentication_Requested\n");
            break;
        case (2) :
            if (byte & (1 << 0)) printf("HCI_Set_Connection_Encryption\n");
            if (byte & (1 << 1)) printf("HCI_Change_Connection_Link_Key\n");
            if (byte & (1 << 2)) printf("HCI_Master_Link_Key\n");
            if (byte & (1 << 3)) printf("HCI_Remote_Name_Request\n");
            if (byte & (1 << 4)) printf("HCI_Remote_Name_Request_Cancel\n");
            if (byte & (1 << 5)) printf("HCI_Read_Remote_Supported_Features\n");
            if (byte & (1 << 6)) printf("HCI_Read_Remote_Extended_Features\n");
            if (byte & (1 << 7)) printf("HCI_Read_Remote_Version_Information\n");
            break;
        case (3) :
            if (byte & (1 << 0)) printf("HCI_Read_Clock_Offset\n");
            if (byte & (1 << 1)) printf("HCI_Read_LMP_Handle\n");
            break;
        case (4) :
            if (byte & (1 << 1)) printf("HCI_Hold_Mode\n");
            if (byte & (1 << 2)) printf("HCI_Sniff_Mode\n");
            if (byte & (1 << 3)) printf("HCI_Exit_Sniff_Mode\n");
            if (byte & (1 << 4)) printf("reserved for future use\n");
            if (byte & (1 << 5)) printf("reserved for future use\n");
            if (byte & (1 << 6)) printf("HCI_QoS_Setup\n");
            if (byte & (1 << 7)) printf("HCI_Role_Discovery\n");
            break;
        case (5) :
            if (byte & (1 << 0)) printf("HCI_Switch_Role\n");
            if (byte & (1 << 1)) printf("HCI_Read_Link_Policy_Settings\n");
            if (byte & (1 << 2)) printf("HCI_Write_Link_Policy_Settings\n");
            if (byte & (1 << 3)) printf("HCI_Read_Default_Link_Policy_Settings\n");
            if (byte & (1 << 4)) printf("HCI_Write_Default_Link_Policy_Settings\n");
            if (byte & (1 << 5)) printf("HCI_Flow_Specification\n");
            if (byte & (1 << 6)) printf("HCI_Set_Event_Mask\n");
            if (byte & (1 << 7)) printf("HCI_Reset\n");
            break;
        case (6) :
            if (byte & (1 << 0)) printf("HCI_Set_Event_Filter\n");
            if (byte & (1 << 1)) printf("HCI_Flush\n");
            if (byte & (1 << 2)) printf("HCI_Read_PIN_Type\n");
            if (byte & (1 << 3)) printf("HCI_Write_PIN_Type\n");
            if (byte & (1 << 5)) printf("HCI_Read_Stored_Link_Key\n");
            if (byte & (1 << 6)) printf("HCI_Write_Stored_Link_Key\n");
            if (byte & (1 << 7)) printf("HCI_Delete_Stored_Link_Key\n");
            break;
        case (7) :
            if (byte & (1 << 0)) printf("HCI_Write_Local_Name\n");
            if (byte & (1 << 1)) printf("HCI_Read_Local_Name\n");
            if (byte & (1 << 2)) printf("HCI_Read_Connection_Accept_Timeout\n");
            if (byte & (1 << 3)) printf("HCI_Write_Connection_Accept_Timeout\n");
            if (byte & (1 << 4)) printf("HCI_Read_Page_Timeout\n");
            if (byte & (1 << 5)) printf("HCI_Write_Page_Timeout\n");
            if (byte & (1 << 6)) printf("HCI_Read_Scan_Enable\n");
            if (byte & (1 << 7)) printf("HCI_Write_Scan_Enable\n");
            break;
        case (8) :
            if (byte & (1 << 0)) printf("HCI_Read_Page_Scan_Activity\n");
            if (byte & (1 << 1)) printf("HCI_Write_Page_Scan_Activity\n");
            if (byte & (1 << 2)) printf("HCI_Read_Inquiry_Scan_Activity\n");
            if (byte & (1 << 3)) printf("HCI_Write_Inquiry_Scan_Activity\n");
            if (byte & (1 << 4)) printf("HCI_Read_Authentication_Enable\n");
            if (byte & (1 << 5)) printf("HCI_Write_Authentication_Enable\n");
            if (byte & (1 << 6)) printf("HCI_Read_Encryption_Mode (deprecated)\n");
            if (byte & (1 << 7)) printf("HCI_Write_Encryption_Mode (deprecated)\n");
            break;
        case (9) :
            if (byte & (1 << 0)) printf("HCI_Read_Class_Of_Device\n");
            if (byte & (1 << 1)) printf("HCI_Write_Class_Of_Device\n");
            if (byte & (1 << 2)) printf("HCI_Read_Voice_Setting\n");
            if (byte & (1 << 3)) printf("HCI_Write_Voice_Setting\n");
            if (byte & (1 << 4)) printf("HCI_Read_Automatic_Flush_Timeout\n");
            if (byte & (1 << 5)) printf("HCI_Write_Automatic_Flush_Timeout\n");
            if (byte & (1 << 6)) printf("HCI_Read_Num_Broadcast_Retransmissions\n");
            if (byte & (1 << 7)) printf("HCI_Write_Num_Broadcast_Retransmissions\n");
            break;
        case (10) :
            if (byte & (1 << 0)) printf("HCI_Read_Hold_Mode_Activity\n");
            if (byte & (1 << 1)) printf("HCI_Write_Hold_Mode_Activity\n");
            if (byte & (1 << 2)) printf("HCI_Read_Transmit_Power_Level\n");
            if (byte & (1 << 3)) printf("HCI_Read_Synchronous_Flow_Control_Enable\n");
            if (byte & (1 << 4)) printf("HCI_Write_Synchronous_Flow_Control_Enable\n");
            if (byte & (1 << 5)) printf("HCI_Set_Controller_To_Host_Flow_Control\n");
            if (byte & (1 << 6)) printf("HCI_Host_Buffer_Size\n");
            if (byte & (1 << 7)) printf("HCI_Host_Number_Of_Completed_Packets\n");
            break;
        case (11) :
            if (byte & (1 << 0)) printf("HCI_Read_Link_Supervision_Timeout\n");
            if (byte & (1 << 1)) printf("HCI_Write_Link_Supervision_Timeout\n");
            if (byte & (1 << 2)) printf("HCI_Read_Number_Of_Supported_IAC\n");
            if (byte & (1 << 3)) printf("HCI_Read_Current_IAC_LAP\n");
            if (byte & (1 << 4)) printf("HCI_Write_Current_IAC_LAP\n");
            if (byte & (1 << 5)) printf("HCI_Read_Page_Scan_Mode_Period (deprecated)\n");
            if (byte & (1 << 6)) printf("HCI_Write_Page_Scan_Mode_Period (deprecated)\n");
            if (byte & (1 << 7)) printf("HCI_Read_Page_Scan_Mode (deprecated)\n");
            break;
        case (12) :
            if (byte & (1 << 0)) printf("HCI_Write_Page_Scan_Mode (deprecated)\n");
            if (byte & (1 << 1)) printf("HCI_Set_AFH_Host_Channel_Classification\n");
            if (byte & (1 << 4)) printf("HCI_Read_Inquiry_Scan_Type\n");
            if (byte & (1 << 5)) printf("HCI_Write_Inquiry_Scan_Type\n");
            if (byte & (1 << 6)) printf("HCI_Read_Inquiry_Mode\n");
            if (byte & (1 << 7)) printf("HCI_Write_Inquiry_Mode\n");
            break;
        case (13) :
            if (byte & (1 << 0)) printf("HCI_Read_Page_Scan_Type\n");
            if (byte & (1 << 1)) printf("HCI_Write_Page_Scan_Type\n");
            if (byte & (1 << 2)) printf("HCI_Read_AFH_Channel_Assessment_Mode\n");
            if (byte & (1 << 3)) printf("HCI_Write_AFH_Channel_Assessment_Mode\n");
            break;
        case (14) :
            if (byte & (1 << 3)) printf("HCI_Read_Local_Version_Information\n");
            if (byte & (1 << 5)) printf("HCI_Read_Local_Supported_Features\n");
            if (byte & (1 << 6)) printf("HCI_Read_Local_Extended_Features\n");
            if (byte & (1 << 7)) printf("HCI_Read_Buffer_Size\n");
            break;
        case (15) :
            if (byte & (1 << 0)) printf("HCI_Read_Country_Code (deprecated)\n");
            if (byte & (1 << 1)) printf("HCI_Read_BD_ADDR\n");
            if (byte & (1 << 2)) printf("HCI_Read_Failed_Contact_Counter\n");
            if (byte & (1 << 3)) printf("HCI_Reset_Failed_Contact_Counter\n");
            if (byte & (1 << 4)) printf("HCI_Read_Link_Quality\n");
            if (byte & (1 << 5)) printf("HCI_Read_RSSI\n");
            if (byte & (1 << 6)) printf("HCI_Read_AFH_Channel_Map\n");
            if (byte & (1 << 7)) printf("HCI_Read_Clock\n");
            break;
        case (16) :
            if (byte & (1 << 0)) printf("HCI_Read_Loopback_Mode\n");
            if (byte & (1 << 1)) printf("HCI_Write_Loopback_Mode\n");
            if (byte & (1 << 2)) printf("HCI_Enable_Device_Under_Test_Mode\n");
            if (byte & (1 << 3)) printf("HCI_Setup_Synchronous_Connection_Request\n");
            if (byte & (1 << 4)) printf("HCI_Accept_Synchronous_Connection_Request\n");
            if (byte & (1 << 5)) printf("HCI_Reject_Synchronous_Connection_Request\n");
            break;
        case (17) :
            if (byte & (1 << 0)) printf("HCI_Read_Extended_Inquiry_Response\n");
            if (byte & (1 << 1)) printf("HCI_Write_Extended_Inquiry_Response\n");
            if (byte & (1 << 2)) printf("HCI_Refresh_Encryption_Key\n");
            if (byte & (1 << 4)) printf("HCI_Sniff_Subrating\n");
            if (byte & (1 << 5)) printf("HCI_Read_Simple_Pairing_Mode\n");
            if (byte & (1 << 6)) printf("HCI_Write_Simple_Pairing_Mode\n");
            if (byte & (1 << 7)) printf("HCI_Read_Local_OOB_Data\n");
            break;
        case (18) :
            if (byte & (1 << 0)) printf("HCI_Read_Inquiry_Response_Transmit_Power_Level\n");
            if (byte & (1 << 1)) printf("HCI_Write_Inquiry_Transmit_Power_Level\n");
            if (byte & (1 << 2)) printf("HCI_Read_Default_Erroneous_Data_Reporting\n");
            if (byte & (1 << 3)) printf("HCI_Write_Default_Erroneous_Data_Reporting\n");
            if (byte & (1 << 7)) printf("HCI_IO_Capability_Request_Reply\n");
            break;
        case (19) :
            if (byte & (1 << 0)) printf("HCI_User_Confirmation_Request_Reply\n");
            if (byte & (1 << 1)) printf("HCI_User_Confirmation_Request_Negative_Reply\n");
            if (byte & (1 << 2)) printf("HCI_User_Passkey_Request_Reply\n");
            if (byte & (1 << 3)) printf("HCI_User_Passkey_Request_Negative_Reply\n");
            if (byte & (1 << 4)) printf("HCI_Remote_OOB_Data_Request_Reply\n");
            if (byte & (1 << 5)) printf("HCI_Write_Simple_Pairing_Debug_Mode\n");
            if (byte & (1 << 6)) printf("HCI_Enhanced_Flush\n");
            if (byte & (1 << 7)) printf("HCI_Remote_OOB_Data_Request_Negative_Reply\n");
            break;
        case (20) :
            if (byte & (1 << 2)) printf("HCI_Send_Keypress_Notification\n");
            if (byte & (1 << 3)) printf("HCI_IO_Capability_Request_Negative_Reply\n");
            if (byte & (1 << 4)) printf("HCI_Read_Encryption_Key_Size\n");
            break;
        case (21) :
            if (byte & (1 << 0)) printf("HCI_Create_Physical_Link\n");
            if (byte & (1 << 1)) printf("HCI_Accept_Physical_Link\n");
            if (byte & (1 << 2)) printf("HCI_Disconnect_Physical_Link\n");
            if (byte & (1 << 3)) printf("HCI_Create_Logical_Link\n");
            if (byte & (1 << 4)) printf("HCI_Accept_Logical_Link\n");
            if (byte & (1 << 5)) printf("HCI_Disconnect_Logical_Link\n");
            if (byte & (1 << 6)) printf("HCI_Logical_Link_Cancel\n");
            if (byte & (1 << 7)) printf("HCI_Flow_Spec_Modify\n");
            break;
        case (22) :
            if (byte & (1 << 0)) printf("HCI_Read_Logical_Link_Accept_Timeout\n");
            if (byte & (1 << 1)) printf("HCI_Write_Logical_Link_Accept_Timeout\n");
            if (byte & (1 << 2)) printf("HCI_Set_Event_Mask_Page_2\n");
            if (byte & (1 << 3)) printf("HCI_Read_Location_Data\n");
            if (byte & (1 << 4)) printf("HCI_Write_Location_Data\n");
            if (byte & (1 << 5)) printf("HCI_Read_Local_AMP_Info\n");
            if (byte & (1 << 6)) printf("HCI_Read_Local_AMP_ASSOC\n");
            if (byte & (1 << 7)) printf("HCI_Write_Remote_AMP_ASSOC\n");
            break;
        case (23) :
            if (byte & (1 << 0)) printf("HCI_Read_Flow_Control_Mode\n");
            if (byte & (1 << 1)) printf("HCI_Write_Flow_Control_Mode\n");
            if (byte & (1 << 2)) printf("HCI_Read_Data_Block_Size\n");
            if (byte & (1 << 5)) printf("HCI_Enable_AMP_Receiver_Reports\n");
            if (byte & (1 << 6)) printf("HCI_AMP_Test_End\n");
            if (byte & (1 << 7)) printf("HCI_AMP_Test\n");
            break;
        case (24) :
            if (byte & (1 << 0)) printf("HCI_Read_Enhanced_Transmit_Power_Level\n");
            if (byte & (1 << 2)) printf("HCI_Read_Best_Effort_Flush_Timeout\n");
            if (byte & (1 << 3)) printf("HCI_Write_Best_Effort_Flush_Timeout\n");
            if (byte & (1 << 4)) printf("HCI_Short_Range_Mode\n");
            if (byte & (1 << 5)) printf("HCI_Read_LE_Host_Support\n");
            if (byte & (1 << 6)) printf("HCI_Write_LE_Host_Support\n");
            break;
        case (25) :
            if (byte & (1 << 0)) printf("HCI_LE_Set_Event_Mask\n");
            if (byte & (1 << 1)) printf("HCI_LE_Read_Buffer_Size\n");
            if (byte & (1 << 2)) printf("HCI_LE_Read_Local_Supported_Features\n");
            if (byte & (1 << 4)) printf("HCI_LE_Set_Random_Address\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Advertising_Parameters\n");
            if (byte & (1 << 6)) printf("HCI_LE_Read_Advertising_Physical_Channel_Tx_Power\n");
            if (byte & (1 << 7)) printf("HCI_LE_Set_Advertising_Data\n");
            break;
        case (26) :
            if (byte & (1 << 0)) printf("HCI_LE_Set_Scan_Response_Data\n");
            if (byte & (1 << 1)) printf("HCI_LE_Set_Advertising_Enable\n");
            if (byte & (1 << 2)) printf("HCI_LE_Set_Scan_Parameters\n");
            if (byte & (1 << 3)) printf("HCI_LE_Set_Scan_Enable\n");
            if (byte & (1 << 4)) printf("HCI_LE_Create_Connection\n");
            if (byte & (1 << 5)) printf("HCI_LE_Create_Connection_Cancel\n");
            if (byte & (1 << 6)) printf("HCI_LE_Read_White_List_Size\n");
            if (byte & (1 << 7)) printf("HCI_LE_Clear_White_List\n");
            break;
        case (27) :
            if (byte & (1 << 0)) printf("HCI_LE_Add_Device_To_White_List\n");
            if (byte & (1 << 1)) printf("HCI_LE_Remove_Device_From_White_List\n");
            if (byte & (1 << 2)) printf("HCI_LE_Connection_Update\n");
            if (byte & (1 << 3)) printf("HCI_LE_Set_Host_Channel_Classification\n");
            if (byte & (1 << 4)) printf("HCI_LE_Read_Channel_Map\n");
            if (byte & (1 << 5)) printf("HCI_LE_Read_Remote_Features\n");
            if (byte & (1 << 6)) printf("HCI_LE_Encrypt\n");
            if (byte & (1 << 7)) printf("HCI_LE_Rand\n");
            break;
        case (28) :
            if (byte & (1 << 0)) printf("HCI_LE_Start_Encryption\n");
            if (byte & (1 << 1)) printf("HCI_LE_Long_Term_Key_Request_Reply\n");
            if (byte & (1 << 2)) printf("HCI_LE_Long_Term_Key_Request_Negative_Reply\n");
            if (byte & (1 << 3)) printf("HCI_LE_Read_Supported_States\n");
            if (byte & (1 << 4)) printf("HCI_LE_Receiver_Test [v1]\n");
            if (byte & (1 << 5)) printf("HCI_LE_Transmitter_Test [v1]\n");
            if (byte & (1 << 6)) printf("HCI_LE_Test_End\n");
            break;
        case (29) :
            if (byte & (1 << 3)) printf("HCI_Enhanced_Setup_Synchronous_Connection\n");
            if (byte & (1 << 4)) printf("HCI_Enhanced_Accept_Synchronous_Connection\n");
            if (byte & (1 << 5)) printf("HCI_Read_Local_Supported_Codecs\n");
            if (byte & (1 << 6)) printf("HCI_Set_MWS_Channel_Parameters\n");
            if (byte & (1 << 7)) printf("HCI_Set_External_Frame_Configuration\n");
            break;
        case (30) :
            if (byte & (1 << 0)) printf("HCI_Set_MWS_Signaling\n");
            if (byte & (1 << 1)) printf("HCI_Set_MWS_Transport_Layer\n");
            if (byte & (1 << 2)) printf("HCI_Set_MWS_Scan_Frequency_Table\n");
            if (byte & (1 << 3)) printf("HCI_Get_MWS_Transport_Layer_Configuration\n");
            if (byte & (1 << 4)) printf("HCI_Set_MWS_PATTERN_Configuration\n");
            if (byte & (1 << 5)) printf("HCI_Set_Triggered_Clock_Capture\n");
            if (byte & (1 << 6)) printf("HCI_Truncated_Page\n");
            if (byte & (1 << 7)) printf("HCI_Truncated_Page_Cancel\n");
            break;
        case (31) :
            if (byte & (1 << 0)) printf("HCI_Set_Connectionless_Slave_Broadcast\n");
            if (byte & (1 << 1)) printf("HCI_Set_Connectionless_Slave_Broadcast_Receive\n");
            if (byte & (1 << 2)) printf("HCI_Start_Synchronization_Train\n");
            if (byte & (1 << 3)) printf("HCI_Receive_Synchronization_Train\n");
            if (byte & (1 << 4)) printf("HCI_Set_reserved_LT_ADDR\n");
            if (byte & (1 << 5)) printf("HCI_Delete_reserved_LT_ADDR\n");
            if (byte & (1 << 6)) printf("HCI_Set_Connectionless_Slave_Broadcast_Data\n");
            if (byte & (1 << 7)) printf("HCI_Read_Synchronization_Train_Parameters\n");
            break;
        case (32) :
            if (byte & (1 << 0)) printf("HCI_Write_Synchronization_Train_Parameters\n");
            if (byte & (1 << 1)) printf("HCI_Remote_OOB_Extended_Data_Request_Reply\n");
            if (byte & (1 << 2)) printf("HCI_Read_Secure_Connections_Host_Support\n");
            if (byte & (1 << 3)) printf("HCI_Write_Secure_Connections_Host_Support\n");
            if (byte & (1 << 4)) printf("HCI_Read_Authenticated_Payload_Timeout\n");
            if (byte & (1 << 5)) printf("HCI_Write_Authenticated_Payload_Timeout\n");
            if (byte & (1 << 6)) printf("HCI_Read_Local_OOB_Extended_Data\n");
            if (byte & (1 << 7)) printf("HCI_Write_Secure_Connections_Test_Mode\n");
            break;
        case (33) :
            if (byte & (1 << 0)) printf("HCI_Read_Extended_Page_Timeout\n");
            if (byte & (1 << 1)) printf("HCI_Write_Extended_Page_Timeout\n");
            if (byte & (1 << 2)) printf("HCI_Read_Extended_Inquiry_Length\n");
            if (byte & (1 << 3)) printf("HCI_Write_Extended_Inquiry_Length\n");
            if (byte & (1 << 4)) printf("HCI_LE_Remote_Connection_Parameter_Request_Reply\n");
            if (byte & (1 << 5)) printf("HCI_LE_Remote_Connection_Parameter_Request_Negative_Reply\n");
            if (byte & (1 << 6)) printf("HCI_LE_Set_Data_Length\n");
            if (byte & (1 << 7)) printf("HCI_LE_Read_Suggested_Default_Data_Length\n");
            break;
        case (34) :
            if (byte & (1 << 0)) printf("HCI_LE_Write_Suggested_Default_Data_Length\n");
            if (byte & (1 << 1)) printf("HCI_LE_Read_Local_P-256_Public_Key\n");
            if (byte & (1 << 2)) printf("HCI_LE_Generate_DHKey [v1]\n");
            if (byte & (1 << 3)) printf("HCI_LE_Add_Device_To_Resolving_List\n");
            if (byte & (1 << 4)) printf("HCI_LE_Remove_Device_From_Resolving_List\n");
            if (byte & (1 << 5)) printf("HCI_LE_Clear_Resolving_List\n");
            if (byte & (1 << 6)) printf("HCI_LE_Read_Resolving_List_Size\n");
            if (byte & (1 << 7)) printf("HCI_LE_Read_Peer_Resolvable_Address\n");
            break;
        case (35) :
            if (byte & (1 << 0)) printf("HCI_LE_Read_Local_Resolvable_Address\n");
            if (byte & (1 << 1)) printf("HCI_LE_Set_Address_Resolution_Enable\n");
            if (byte & (1 << 2)) printf("HCI_LE_Set_Resolvable_Private_Address_Timeout\n");
            if (byte & (1 << 3)) printf("HCI_LE_Read_Maximum_Data_Length\n");
            if (byte & (1 << 4)) printf("HCI_LE_Read_PHY\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Default_PHY\n");
            if (byte & (1 << 6)) printf("HCI_LE_Set_PHY\n");
            if (byte & (1 << 7)) printf("HCI_LE_Receiver_Test [v2]\n");
            break;
        case (36) :
            if (byte & (1 << 0)) printf("HCI_LE_Transmitter_Test [v2]\n");
            if (byte & (1 << 1)) printf("HCI_LE_Set_Advertising_Set_Random_Address\n");
            if (byte & (1 << 2)) printf("HCI_LE_Set_Extended_Advertising_Parameters\n");
            if (byte & (1 << 3)) printf("HCI_LE_Set_Extended_Advertising_Data\n");
            if (byte & (1 << 4)) printf("HCI_LE_Set_Extended_Scan_Response_Data\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Extended_Advertising_Enable\n");
            if (byte & (1 << 6)) printf("HCI_LE_Read_Maximum_Advertising_Data_Length\n");
            if (byte & (1 << 7)) printf("HCI_LE_Read_Number_of_Supported_Advertising_Sets\n");
            break;
        case (37) :
            if (byte & (1 << 0)) printf("HCI_LE_Remove_Advertising_Set\n");
            if (byte & (1 << 1)) printf("HCI_LE_Clear_Advertising_Sets\n");
            if (byte & (1 << 2)) printf("HCI_LE_Set_Periodic_Advertising_Parameters\n");
            if (byte & (1 << 3)) printf("HCI_LE_Set_Periodic_Advertising_Data\n");
            if (byte & (1 << 4)) printf("HCI_LE_Set_Periodic_Advertising_Enable\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Extended_Scan_Parameters\n");
            if (byte & (1 << 6)) printf("HCI_LE_Set_Extended_Scan_Enable\n");
            if (byte & (1 << 7)) printf("HCI_LE_Extended_Create_Connection\n");
            break;
        case (38) :
            if (byte & (1 << 0)) printf("HCI_LE_Periodic_Advertising_Create_Sync\n");
            if (byte & (1 << 1)) printf("HCI_LE_Periodic_Advertising_Create_Sync_Cancel\n");
            if (byte & (1 << 2)) printf("HCI_LE_Periodic_Advertising_Terminate_Sync\n");
            if (byte & (1 << 3)) printf("HCI_LE_Add_Device_To_Periodic_Advertiser_List\n");
            if (byte & (1 << 4)) printf("HCI_LE_Remove_Device_From_Periodic_Advertiser_List\n");
            if (byte & (1 << 5)) printf("HCI_LE_Clear_Periodic_Advertiser_List\n");
            if (byte & (1 << 6)) printf("HCI_LE_Read_Periodic_Advertiser_List_Size\n");
            if (byte & (1 << 7)) printf("HCI_LE_Read_Transmit_Power\n");
            break;
        case (39) :
            if (byte & (1 << 0)) printf("HCI_LE_Read_RF_Path_Compensation\n");
            if (byte & (1 << 1)) printf("HCI_LE_Write_RF_Path_Compensation\n");
            if (byte & (1 << 2)) printf("HCI_LE_Set_Privacy_Mode\n");
            if (byte & (1 << 3)) printf("HCI_LE_Receiver_Test [v3]\n");
            if (byte & (1 << 4)) printf("HCI_LE_Transmitter_Test [v3]\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Connectionless_CTE_Transmit_Parameters\n");
            if (byte & (1 << 6)) printf("HCI_LE_Set_Connectionless_CTE_Transmit_Enable\n");
            if (byte & (1 << 7)) printf("HCI_LE_Set_Connectionless_IQ_Sampling_Enable\n");
            break;
        case (40) :
            if (byte & (1 << 0)) printf("HCI_LE_Set_Connection_CTE_Receive_Parameters\n");
            if (byte & (1 << 1)) printf("HCI_LE_Set_Connection_CTE_Transmit_Parameters\n");
            if (byte & (1 << 2)) printf("HCI_LE_Connection_CTE_Request_Enable\n");
            if (byte & (1 << 3)) printf("HCI_LE_Connection_CTE_Response_Enable\n");
            if (byte & (1 << 4)) printf("HCI_LE_Read_Antenna_Information\n");
            if (byte & (1 << 5)) printf("HCI_LE_Set_Periodic_Advertising_Receive_Enable\n");
            if (byte & (1 << 6)) printf("HCI_LE_Periodic_Advertising_Sync_Transfer\n");
            if (byte & (1 << 7)) printf("HCI_LE_Periodic_Advertising_Set_Info_Transfer\n");
            break;
        case (41) :
            if (byte & (1 << 0)) printf("HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters\n");
            if (byte & (1 << 1)) printf("HCI_LE_Set_Default_Periodic_Advertising_Sync_Transfer_Parameters\n");
            if (byte & (1 << 2)) printf("HCI_LE_Generate_DHKey [v2]\n");
            if (byte & (1 << 3)) printf("HCI_Read_Local_Simple_Pairing_Options\n");
            if (byte & (1 << 4)) printf("HCI_LE_Modify_Sleep_Clock_Accuracy\n");
            break;
    }
}



void showSupportedFeatures(u16 byte_num, u8 byte)
{
    
}
