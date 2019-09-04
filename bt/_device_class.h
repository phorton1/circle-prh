#ifndef __DEVICE_CLASS_H_
#define __DEVICE_CLASS_H_

// https://www.bluetooth.com/specifications/assigned-numbers/baseband/

#define DC_MAJOR_SERVICE_DISCOVERABLE       (1 << 13)
#define DC_MAJOR_SERVICE_RESERVED1          (1 << 14)
#define DC_MAJOR_SERVICE_RESERVED2          (1 << 15)
#define DC_MAJOR_SERVICE_POSITIONING        (1 << 16)      // LOCATION IDENTIFICATION
#define DC_MAJOR_SERVICE_NETWORKING         (1 << 17)      // LAN, AD HOC, …      
#define DC_MAJOR_SERVICE_RENDERING          (1 << 18)      // PRINTING, SPEAKERS, …      
#define DC_MAJOR_SERVICE_CAPTURING          (1 << 19)      // SCANNER, MICROPHONE, …
#define DC_MAJOR_SERVICE_OBJECT_TRANSFER    (1 << 20)      // V-INBOX, V-FOLDER, …
#define DC_MAJOR_SERVICE_AUDIO              (1 << 21)      // SPEAKER, MICROPHONE, HEADSET SERVICE, …
#define DC_MAJOR_SERVICE_TELEPHONY          (1 << 22)      // CORDLESS TELEPHONY, MODEM, HEADSET SERVICE, …) 
#define DC_MAJOR_SERVICE_INFORMATION        (1 << 23)      // WEB-SERVER, WAP-SERVER, …
    

#define DC_MAJOR_CLASS_COMPUTER             (0x1 << 8)    // (desktop, notebook, PDA, organizer, … )
#define DC_MAJOR_CLASS_PHONE                (0x2 << 8)    // (cellular, cordless, pay phone, modem, …)
#define DC_MAJOR_CLASS_LAN                  (0x3 << 8)    // Network Access point
#define DC_MAJOR_CLASS_AUDIO_VIDEO          (0x4 << 8)    // (headset, speaker, stereo, video display, VCR, …
#define DC_MAJOR_CLASS_PERIPHERAL           (0x5 << 8)    // (mouse, joystick, keyboard, … )
#define DC_MAJOR_CLASS_IMAGING              (0x6 << 8)    // (printer, scanner, camera, display, …)
#define DC_MAJOR_CLASS_WEARABLE             (0x7 << 8)    // 
#define DC_MAJOR_CLASS_TOY                  (0x8 << 8)    // 
#define DC_MAJOR_CLASS_HEALTH               (0x9 << 8)    // 
#define DC_MAJOR_CLASS_UNCATEGORIZED        (0xf << 8)    // : device code not specified

#define DC_COMPUTER_DESKTOP         (1 << 2)   // workstation
#define DC_COMPUTER_SERVER          (2 << 2)   // class computer
#define DC_COMPUTER_LAPTOP          (3 << 2)   //
#define DC_COMPUTER_HANDHELD        (4 << 2)   // PC/PDA (clamshell)
#define DC_COMPUTER_PALM            (5 << 2)   // -size PC/PDA
#define DC_COMPUTER_WEARABLE        (6 << 2)   // computer (watch size)
#define DC_COMPUTER_TABLET          (7 << 2)

#define DC_PHONE_CELLULAR           (1 << 2)    
#define DC_PHONE_CORDLESS           (2 << 2)    
#define DC_PHONE_SMARTPHONE         (3 << 2)    
#define DC_PHONE_WIRED              (4 << 2)    //  modem or voice gateway
#define DC_PHONE_COMMON             (5 << 2)    // ISDN access

#define DC_NETWORK_FULL             (0 << 2)    // Fully available
#define DC_NETWORK_1                (1 << 2)    // 1% to 17% utilized
#define DC_NETWORK_17               (2 << 2)    // 17% to 33% utilized
#define DC_NETWORK_33               (3 << 2)    // 33% to 50% utilized
#define DC_NETWORK_50               (4 << 2)    // 50% to 67% utilized
#define DC_NETWORK_67               (5 << 2)    // 67% to 83% utilized
#define DC_NETWORK_83               (6 << 2)    // 83% to 99% utilized
#define DC_NETWORK_UNAVAILABLE      (7 << 2)    // No service available

#define DC_AUDIO_WEARABLE           (0x01 << 2)
#define DC_AUDIO_HANDS              (0x02 << 2)
#define DC_AUDIO_RESERVED1          (0x03 << 2)
#define DC_AUDIO_MICROPHONE         (0x04 << 2)
#define DC_AUDIO_LOUDSPEAKER        (0x05 << 2)
#define DC_AUDIO_HEADPHONES         (0x06 << 2)
#define DC_AUDIO_PORTABLE           (0x07 << 2)
#define DC_AUDIO_CAR                (0x08 << 2)
#define DC_AUDIO_SETTOPBOX          (0x09 << 2)
#define DC_AUDIO_HIFI               (0x0a << 2)
#define DC_AUDIO_VCR                (0x0b << 2)
#define DC_AUDIO_CAMERA             (0x0c << 2)
#define DC_AUDIO_CAMCORDER          (0x0d << 2)
#define DC_AUDIO_MONITOR            (0x0e << 2)
#define DC_AUDIO_DISPLAYANDSPEAKER  (0x0f << 2)
#define DC_AUDIO_CONFERENCING       (0x10 << 2)
#define DC_AUDIO_RESERVED2          (0x11 << 2)
#define DC_AUDIO_GTOY               (0x12 << 2)


#define DC_PERIP_MAJOR_KEYBOARD     (1 << 6)
#define DC_PERIP_MAJOR_POINTING     (2 << 6)
#define DC_PERIP_MAJOR_COMBO        (3 << 6)

#define DC_PERIPH_JOYSTICK          (1 << 2)
#define DC_PERIPH_GAMEPAD           (2 << 2)
#define DC_PERIPH_REMOTE            (3 << 2)      // control
#define DC_PERIPH_SENSING           (4 << 2)      // device
#define DC_PERIPH_DIGITIZER         (5 << 2)      // tablet
#define DC_PERIPH_CARDREADER        (6 << 2)      // e.g. SIM Card Reader
#define DC_PERIPH_PEN               (7 << 2)
#define DC_PERIPH_SCANNER           (8 << 2)      // for bar-codes, RFID, etc.
#define DC_PERIPH_GESTURAL          (9 << 2)      // e.g., “wand” form factor

#define DC_IMAGING_DISPLAY          (1 << 2)
#define DC_IMAGING_CAMERA           (2 << 2)
#define DC_IMAGING_SCANNER          (4 << 2)
#define DC_IMAGING_PRINTER          (8 << 2)

#define DC_WEARABLE_WRISTWATCH      (1 << 2)
#define DC_WEARABLE_PAGER           (2 << 2)
#define DC_WEARABLE_JACKET          (3 << 2)
#define DC_WEARABLE_HELMET          (4 << 2)
#define DC_WEARABLE_GLASSES         (5 << 2)

#define DC_TOY_Robot               (1 << 2)
#define DC_TOY_Vehicle             (2 << 2)
#define DC_TOY_Doll                (3 << 2)     // Action figure
#define DC_TOY_Controller          (4 << 2)
#define DC_TOY_Game                (5 << 2)

#define DC_HEALTH_BP_Monitor               (0x1 << 2)
#define DC_HEALTH_Thermometer              (0x2 << 2)
#define DC_HEALTH_Weighing_Scale           (0x3 << 2)
#define DC_HEALTH_Glucose_Meter            (0x4 << 2)
#define DC_HEALTH_Pulse_Oximeter           (0x5 << 2)
#define DC_HEALTH_Heart_Pulse_Monitor      (0x6 << 2)
#define DC_HEALTH_Health_Data_Display      (0x7 << 2)
#define DC_HEALTH_Step_Counter             (0x8 << 2)
#define DC_HEALTH_Body_Analyzer            (0x9 << 2)
#define DC_HEALTH_Peak_Flow_Monitor        (0xa << 2)
#define DC_HEALTH_Medication_Monitor       (0xb << 2)
#define DC_HEALTH_Knee_Prosthesis          (0xc << 2)
#define DC_HEALTH_Ankle_Prosthesis         (0xd << 2)
#define DC_HEALTH_Health_Manager           (0xe << 2)
#define DC_HEALTH_Personal_Mobility        (0xf << 2)

#endif
