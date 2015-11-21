/*********************************************************************

    Microchip ZigBee Stack

    Sample Demo Router for the PIC18/PIC24 ZigBee 2006 Residential Stack
    This demonstration shows how a ZigBee Router can be set up.  
    This demo allows the PICDEM Z/Explorer 16 Demostration Board to act as ZigBee
    protocol Router. It is designed to interact with other ZigBee protocol 
    devices - Routers and End Devices.
    
    Switch and LED functionality are as follows:

    RB4/RD6:  Adds a node to, or removes a node from Group 4.  
    RA0/D10:  Used to indicate if a node is a member of Group 4 (Lit – yes; unlit – no) 

    At startup the devices do not belong to a group, and the lit LEDs just indicate
    they are on the network.
    
    RB5/RD7:   Is used to send a messages to nodes in Group 4.    
               The actual message is a request for all the Group 4 nodes to end 
               back to the requester 10 bytes.
    RA1/D09:   Toggles to indicate that the receiving node is in Group 4 
               and received requests for data.  


    NOTE: To speed network formation, ALLOWED_CHANNELS has been set to
    channel 21 only.  
    Please consult the PICDEM Z Zigbee2006 Residential StackQTGuide.pdf for how to 
    run this sample application.
 *********************************************************************
 * FileName:        Router.c
 *  Dependencies:
 * Processor:       PIC18F/PIC24f
 * Complier:        MCC18 v3.20 or higher
 * Complier:        MCC30 v3.10 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright © 2004-2007 Microchip Technology Inc.  All rights reserved.
 *
 * Microchip licenses to you the right to use, copy and distribute Software 
 * only when embedded on a Microchip microcontroller or digital signal 
 * controller and used with a Microchip radio frequency transceiver, which 
 * are integrated into your product or third party product (pursuant to the 
 * sublicense terms in the accompanying license agreement).  You may NOT 
 * modify or create derivative works of the Software.  
 *
 * If you intend to use this Software in the development of a product for 
 * sale, you must be a member of the ZigBee Alliance.  For more information, 
 * go to www.zigbee.org.
 *
 * You should refer to the license agreement accompanying this Software for 
 * additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY 
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR 
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED 
 * UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF 
 * WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR 
 * EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF 
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY 
 * THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER 
 * SIMILAR COSTS.
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DPL                  08/01/08 Microchip ZigBee Stack v2.0-2.6
 ********************************************************************/

//******************************************************************************
// Header Files
//******************************************************************************

// Include the main ZigBee header file.
#include "zAPL.h"
#ifdef I_SUPPORT_SECURITY
	#include "zSecurity.h"
#endif

// If you are going to send data to a terminal, include this file.
#include "console.h"



//******************************************************************************
// Configuration Bits
//******************************************************************************

#if defined(MCHP_C18)
    #if defined(__18F4620)
        #pragma romdata CONFIG1H = 0x300001
        const rom unsigned char config1H = 0b00000110;      // HSPLL oscillator
    
        #pragma romdata CONFIG2L = 0x300002
        const rom unsigned char config2L = 0b00011111;      // Brown-out Reset Enabled in hardware @ 2.0V, PWRTEN disabled
    
        #pragma romdata CONFIG2H = 0x300003
        const rom unsigned char config2H = 0b00010010;      // HW WD disabled, 1:512 prescaler
    
        #pragma romdata CONFIG3H = 0x300005
        const rom unsigned char config3H = 0b10000000;      // PORTB digital on RESET
    
        #pragma romdata CONFIG4L = 0x300006
        const rom unsigned char config4L = 0b10000001;      // DEBUG disabled,
                                                            // XINST disabled
                                                            // LVP disabled
                                                            // STVREN enabled
    #elif defined(__18F87J10)
        #pragma romdata CONFIG1L = 0x1FFF8
        const rom unsigned char config1L = 0b10111110;      
    
        #pragma romdata CONFIG1H = 0x1FFF9
        const rom unsigned char config1H = 0b11111111;      
    
        #pragma romdata CONFIG2L = 0x1FFFA
        const rom unsigned char config2L = 0b11111101;      
    
        #pragma romdata CONFIG2H = 0x1FFFB
        const rom unsigned char config2H = 0b11111010;      
    
        #pragma romdata CONFIG3L = 0x1FFFC
        const rom unsigned char config3L = 0b11111111;

        #pragma romdata CONFIG3H = 0x1FFFD
        const rom unsigned char config3H = 0b11111111;
        
    #endif
    #pragma romdata
#elif defined(__PIC24F__)
	// Explorer 16 board
	_CONFIG2(FNOSC_PRI & POSCMOD_XT)	// Primary XT OSC with 4X PLL
	_CONFIG1(JTAGEN_OFF & FWDTEN_OFF)	// JTAG off, watchdog timer off
#elif defined(__dsPIC33F__) || defined(__PIC24H__)
	// Explorer 16 board
	_FOSCSEL(FNOSC_PRI)				// primary osc
	_FOSC(OSCIOFNC_OFF & POSCMD_XT)	// XT Osc
	_FWDT(FWDTEN_OFF)				// Disable Watchdog timer
	// JTAG should be disabled as well
#elif defined(__dsPIC30F__)
	// dsPICDEM 1.1 board
	_FOSC(XT_PLL16)		// XT Osc + 16X PLL
	_FWDT(WDT_OFF)		// Disable Watchdog timer
	_FBORPOR(MCLR_EN & PBOR_OFF & PWRT_OFF)
#else
    #error Other compilers are not yet supported.
#endif		
	

//******************************************************************************
// Function Prototypes
//******************************************************************************

void HardwareInit( void );
void ProcessNONZigBeeTasks(void);
void ProcessZigBeePrimitives(void);
#if defined(I_SUPPORT_GROUP_ADDRESSING)
    extern GROUP_ADDRESS_RECORD currentGroupAddressRecord;
    extern BYTE    GetEndPointsFromGroup(INPUT SHORT_ADDR GroupAddr);
    extern BYTE    GetEmptyGroup(void);
    extern BYTE    AddGroup(INPUT WORD_VAL GroupAddress, INPUT BYTE EndPoint);
    extern BYTE    RemoveGroup(INPUT WORD GroupAddress, INPUT BYTE EndPoint);
#endif

extern void     RemoveAllGroups(void);
extern void     RemoveAllBindings(SHORT_ADDR);
//******************************************************************************
// Application Variables
//******************************************************************************
NETWORK_DESCRIPTOR  *currentNetworkDescriptor;
ZIGBEE_PRIMITIVE    currentPrimitive;
NETWORK_DESCRIPTOR  *NetworkDescriptor;
BYTE                orphanTries;
#ifdef I_SUPPORT_SECURITY
	extern KEY_VAL	KeyVal;
	#ifdef USE_EXTERNAL_NVM
		extern WORD trustCenterLongAddr;
		extern NETWORK_KEY_INFO plainSecurityKey[2];
	#else
		extern ROM LONG_ADDR trustCenterLongAddr;
	#endif
#endif


BYTE AllowJoin = 1;
BYTE loopCount;

extern BYTE ZDOCounter;
BYTE    i;

#if defined(__C30__)
	#define PB_LEFT_SWITCH				PORTDbits.RD6
	#define PB_RIGHT_SWITCH				PORTDbits.RD7
	#define GROUP_INDICATION			LATAbits.LATA6
	#define MESSAGE_INDICATION			LATAbits.LATA7
#else
	#define PB_LEFT_SWITCH            	PORTBbits.RB5
	#define PB_RIGHT_SWITCH             PORTBbits.RB4
	#if defined(__18F4620)
		#define GROUP_INDICATION        LATAbits.LATA0
		#define MESSAGE_INDICATION      LATAbits.LATA1
	#else
		#define GROUP_INDICATION	    LATDbits.LATD0
		#define MESSAGE_INDICATION		LATDbits.LATD1
	#endif
#endif

#define DEBOUNCE_TIME 0x00008FF


#define TRANSMIT_COUNTED_PACKETS_CLUSTER 0x0001


static union
{
    struct
    {
        BYTE    bBroadcastSwitchToggled    : 1;
        BYTE    bLightSwitchToggled        : 1;
        BYTE    bTryingToBind              : 1;
        BYTE    bIsBound                   : 1;
        BYTE    bDestinationAddressKnown   : 1;
        BYTE    bBindSwitchToggled         : 1;
    } bits;
    BYTE Val;
} myStatusFlags;

LONG_ADDR myLongAddress;

BYTE                routeDiscovery;
extern BYTE				MACDataTransmission;
extern BYTE             APSCounter;

/* Menu System */
ROM char * const menu =
    "\r\n     1: Enable/Disable Joining by Other Devices"
    "\r\n     2: Request Data From Another Device"
    "\r\n     3: Request Data From a Group of Devices"
    "\r\n     4: Send Data To Another Device"
    "\r\n     5: Send Data To a Group of Devices"
    "\r\n     6: Add/Remove Device to/from a Group"
    "\r\n     7: Dump Neighborhood Information"
    ;
    
void ProcessMenu( void );
void PrintMenu( void );
BYTE GetHexDigit( void );
BYTE GetMACByte( void );

WORD_VAL MSGPacketCount;

BYTE RX_On = 1;

BYTE        i;
BYTE        loopTest, byteMSB, byteLSB;
BOOL        gettingAcks = FALSE;

extern BOOL bDisableShortAddress;

extern BOOL APSSaveAPSAddress(APS_ADDRESS_MAP *AddressMap);

extern NWK_STATUS nwkStatus;
extern BYTE macLongAddrByte[8];


BOOL    group_state_ID5 = TRUE;
BOOL    group_state_ID4 = TRUE;
BYTE    GROUP_ID4       = 0x04;
BYTE    GROUP_ID5       = 0x05;

BOOL PB_LEFT_pressed    = FALSE;        /* RB5/RD6 */
BOOL PB_RIGHT_pressed   = FALSE;        /* RB4/RD7 */

TICK PB_LEFT_press_time;
TICK PB_RIGHT_press_time;
TICK tickDifference;
TICK tick2Difference;

BYTE       temp2;
//******************************************************************************
//******************************************************************************
//******************************************************************************
#if defined(__C30__)
    int main(void)
#else
    void main(void)
#endif
{
	#if defined(__18F87J10)
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        OSCTUNEbits.PLLEN = 1;
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
    #endif
    
    /* Initialize both Hardware and Stack */
    CLRWDT();
    ENABLE_WDT();

    currentPrimitive = NO_PRIMITIVE;
    NetworkDescriptor = NULL;
    orphanTries = 3;

    // If you are going to send data to a terminal, initialize the UART.
    ConsoleInit();

    // Initialize the hardware - must be done before initializing ZigBee.
    HardwareInit();


    // Initialize the ZigBee Stack.
    ZigBeeInit();

    

    // *************************************************************************
    // Perform any other initialization here
    // *************************************************************************

    ConsolePutROMString( (ROM char *)"\r\n\r\n\r\n********************************" );
    ConsolePutROMString( (ROM char *)"\r\nZigBee Router - v2.0-2.6.0a\r\n\r\n" );
    #if (RF_CHIP == MRF24J40)
        ConsolePutROMString( (ROM char *)"Transceiver-MRF24J40\r\n\r\n" );
    #else
        ConsolePutROMString( (ROM char *)"Transceiver-Unknown\r\n\r\n" );
    #endif

    // Enable interrupts to get everything going.
    #if defined(__C30__)
        RFIE = 1;
    #endif
    #if !defined(__C30__)
        INTCONbits.RBIE = 1;
        RCONbits.IPEN = 1;
        INTCONbits.GIEH = 1;
    #endif

    /* Set LEDs to a known state */
    GROUP_INDICATION	    = 0;
	MESSAGE_INDICATION		= 0;
    myStatusFlags.Val = 0x00;

    /* Start the network anew each time Node is booted up */
    NWKClearNeighborTable();
    #if defined(I_SUPPORT_BINDING)
        ClearBindingTable();
    #endif
    
    /* Clear the Group Table */
    RemoveAllGroups();
    
    #if defined(I_SUPPORT_BINDING)
        RemoveAllBindings(macPIB.macShortAddress);
    #endif
    
    /* Clearing nwk status flags */
    nwkStatus.flags.Val = 0x00;
    
    while (1)
    {
        CLRWDT();
        /* Determine which is the next ZigBee Primitive to operate on */
        ZigBeeTasks( &currentPrimitive );
        /* Process the next ZigBee Primitive */
        ProcessZigBeePrimitives();
        /* do any non ZigBee related tasks and then go back to ZigBee tasks */
        ProcessNONZigBeeTasks();  
    }
}     

void ProcessZigBeePrimitives(void)
{
        switch (currentPrimitive)
        {
            case NLME_NETWORK_DISCOVERY_confirm:
                currentPrimitive = NO_PRIMITIVE;
                if (!params.NLME_NETWORK_DISCOVERY_confirm.Status)
                {
                    if (!params.NLME_NETWORK_DISCOVERY_confirm.NetworkCount)
                    {
                        ConsolePutROMString( (ROM char *)"No networks found.  Trying again...\r\n" );
                    }
                    else
                    {
                        // Save the descriptor list pointer so we can destroy it later.
                        NetworkDescriptor = params.NLME_NETWORK_DISCOVERY_confirm.NetworkDescriptor;

                        // Select a network to try to join.  We're not going to be picky right now...
                        currentNetworkDescriptor = NetworkDescriptor;

SubmitJoinRequest:
                        params.NLME_JOIN_request.PANId          = currentNetworkDescriptor->PanID;
                        params.NLME_JOIN_request.JoinAsRouter   = TRUE;
                        params.NLME_JOIN_request.RejoinNetwork  = FALSE;
                        params.NLME_JOIN_request.PowerSource    = MAINS_POWERED;
                        params.NLME_JOIN_request.RxOnWhenIdle   = TRUE;
                        params.NLME_JOIN_request.MACSecurity    = FALSE;
                        params.NLME_JOIN_request.ExtendedPANID = currentNetworkDescriptor->ExtendedPANID;
                        currentPrimitive = NLME_JOIN_request;

                        ConsolePutROMString( (ROM char *)"Network(s) found. Trying to join " );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.MSB );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.LSB );
                        printf(" | ");
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[7]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[6]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[5]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[4]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[3]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[2]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[1]);
                        PrintChar(currentNetworkDescriptor->ExtendedPANID.v[0]);
                        ConsolePutROMString( (ROM char *)".\r\n" );
                    }
                }
                else
                {
                    PrintChar( params.NLME_NETWORK_DISCOVERY_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Error finding network.  Trying again...\r\n" );
                }
                break;

            case NLME_JOIN_confirm:
                if (!params.NLME_JOIN_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Join successful!\r\n" );

                    // Free the network descriptor list, if it exists. If we joined as an orphan, it will be NULL.
                    while (NetworkDescriptor)
                    {
                        currentNetworkDescriptor = NetworkDescriptor->next;
                        nfree( NetworkDescriptor );
                        NetworkDescriptor = currentNetworkDescriptor;
                    }

                    // Start routing capability.
                    params.NLME_START_ROUTER_request.BeaconOrder = MAC_PIB_macBeaconOrder;
                    params.NLME_START_ROUTER_request.SuperframeOrder = MAC_PIB_macSuperframeOrder;
                    params.NLME_START_ROUTER_request.BatteryLifeExtension = FALSE;
                    currentPrimitive = NLME_START_ROUTER_request;
                }
                else
                {
                    currentPrimitive = NO_PRIMITIVE;
                    PrintChar( params.NLME_JOIN_confirm.Status );

                    // If we were trying as an orphan, see if we have some more orphan attempts.
                    if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                    {
                        // If we tried to join as an orphan, we do not have NetworkDescriptor, so we do
                        // not have to free it.

                        ConsolePutROMString( (ROM char *)" Could not join as orphan. " );
                        orphanTries--;
                        if (orphanTries == 0)
                        {
                            ConsolePutROMString( (ROM char *)"Must try as new node...\r\n" );
                            ZigBeeStatus.flags.bits.bTryOrphanJoin = 0;
                        }
                        else
                        {
                            ConsolePutROMString( (ROM char *)"Trying again...\r\n" );
                        }
                    }
                    else
                    {
                        ConsolePutROMString( (ROM char *)" Could not join selected network. " );
                        currentNetworkDescriptor = currentNetworkDescriptor->next;
                        if (currentNetworkDescriptor)
                        {
                            ConsolePutROMString( (ROM char *)"Trying next discovered network...\r\n" );
                            goto SubmitJoinRequest;
                        }
                        else
                        {
                            //Ran out of descriptors.  Free the network descriptor list, and fall
                            // through to try discovery again.
                            ConsolePutROMString( (ROM char *)"Cleaning up and retrying discovery...\r\n" );
                            while (NetworkDescriptor)
                            {
                                currentNetworkDescriptor = NetworkDescriptor->next;
                                nfree( NetworkDescriptor );
                                NetworkDescriptor = currentNetworkDescriptor;
                            }
                        }
                    }
                }
                break;

            case NLME_START_ROUTER_confirm:
                if (!params.NLME_START_ROUTER_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Router Started! Enabling joins...\r\n" );
                    params.NLME_PERMIT_JOINING_request.PermitDuration = 0xFF;   // No Timeout
                    params.NLME_PERMIT_JOINING_request._updatePayload = TRUE;
                    currentPrimitive = NLME_PERMIT_JOINING_request;
                }
                else
                {
                    PrintChar( params.NLME_JOIN_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Router start unsuccessful. We cannot route frames.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_PERMIT_JOINING_confirm:
                if (!params.NLME_PERMIT_JOINING_confirm.Status)
                {
                    LONG_ADDR myLongAddress;
                    GetMACAddress(&myLongAddress);
                    
                    ConsolePutROMString( (ROM char *)"Joining permitted.\r\n" );
                    
                    ZigBeeBlockTx();
                    TxBuffer[TxData++] = ZDOCounter++;
                    /* package and send device announcement */
                    TxBuffer[TxData++] = macPIB.macShortAddress.v[0];
                    TxBuffer[TxData++] = macPIB.macShortAddress.v[1];
                    
                    TxBuffer[TxData++] = myLongAddress.v[0];
                    TxBuffer[TxData++] = myLongAddress.v[1];
                    TxBuffer[TxData++] = myLongAddress.v[2];
                    TxBuffer[TxData++] = myLongAddress.v[3];
                    TxBuffer[TxData++] = myLongAddress.v[4];
                    TxBuffer[TxData++] = myLongAddress.v[5];
                    TxBuffer[TxData++] = myLongAddress.v[6];
                    TxBuffer[TxData++] = myLongAddress.v[7];
                    
                    TxBuffer[TxData++] = MY_CAPABILITY_INFO;
                    
                    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
                    params.APSDE_DATA_request.DstEndpoint = EP_ZDO;
                    params.APSDE_DATA_request.DstAddress.ShortAddr.Val = 0xFFFF; //destinationAddress;

                    params.APSDE_DATA_request.ProfileId.Val = ZDO_PROFILE_ID;
                    params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                    params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_SUPPRESS;						                            
                    params.APSDE_DATA_request.TxOptions.Val = 0;                          
                    params.APSDE_DATA_request.SrcEndpoint = EP_ZDO;
                    params.APSDE_DATA_request.ClusterId.Val = END_DEVICE_annce;
                    /* Set LED to up and running ON state */
                    GROUP_INDICATION	    = 1;
	                MESSAGE_INDICATION		= 1;
                    
                    currentPrimitive = APSDE_DATA_request;
                    
                }
                else
                {
                    PrintChar( params.NLME_PERMIT_JOINING_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Join permission unsuccessful. We cannot allow joins.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_JOIN_indication:
                ConsolePutROMString( (ROM char *)"\r\nNode " );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.MSB );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.LSB );
                ConsolePutROMString( (ROM char *)" With MAC Address " );
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[7]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[6]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[5]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[4]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[3]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[2]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[1]);
                PrintChar(params.NLME_JOIN_indication.ExtendedAddress.v[0]);
                ConsolePutROMString( (ROM char *)" just joined.\r\n" );

                /* For Zigbee 2006: If a new device with the same old longAddress address
                * joins the PAN, then make sure the old short address is no longer used and is 
                * overwritten with the new shortAddress & longAddress combination 
                */
                {  /* same long address check block */
                    APS_ADDRESS_MAP currentAPSAddress1; 
                    currentAPSAddress1.shortAddr   =   params.NLME_JOIN_indication.ShortAddress;
                    currentAPSAddress1.longAddr    =   params.NLME_JOIN_indication.ExtendedAddress;
                   
                    if(LookupAPSAddress(&params.NLME_JOIN_indication.ExtendedAddress) )
                    {
                     
                        for( i = 0; i < apscMaxAddrMapEntries; i++)
                        {
                            #ifdef USE_EXTERNAL_NVM
                                GetAPSAddress( &currentAPSAddress,  apsAddressMap + i * sizeof(APS_ADDRESS_MAP) );
                            #else
                                GetAPSAddress( &currentAPSAddress,  &apsAddressMap[i] );
                            #endif
                            if (!memcmp( (void *)&currentAPSAddress.longAddr, (void *)&currentAPSAddress1.longAddr, 8 ))
                            {
                                /* overwrite the old with the new short/long address combo  */
                                #ifdef USE_EXTERNAL_NVM
                                    PutAPSAddress( apsAddressMap + i * sizeof(APS_ADDRESS_MAP), &currentAPSAddress1);
                                #else
                                    PutAPSAddress( &apsAddressMap[i], &currentAPSAddress1 );
                                #endif
                            }
                        }   /* end for loop */
                                    
                    }       /* end if */
 
                }           /* end address check block */
				#ifdef I_SUPPORT_SECURITY

					#ifdef I_AM_TRUST_CENTER
  		 			{
			  		  	BOOL AllowJoin = TRUE;
						// decide if you allow this device to join
			  		  	if( !AllowJoin )
  		  				{
			  		  		// no need to set deviceAddress, since it is overlap with NLME_JOIN_indication
		  					//params.NLME_LEAVE_request.DeviceAddress = params.NLME_JOIN_indication.ExtendedAddress;
					  		params.NLME_LEAVE_request.RemoveChildren = TRUE;
  		  					currentPrimitive = NLME_LEAVE_request;
							break;
  		  				}
  		  	
						#ifdef I_SUPPORT_SECURITY_SPEC
							if( params.NLME_JOIN_indication.secureJoin )
							{
								BYTE i;
								for(i = 0; i < 16; i++)
								{
									KeyVal.v[i] = 0;
								}
								params.APSME_TRANSPORT_KEY_request.Key = &KeyVal;
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = 0;
				
							} else {
								BYTE i;
								GetNwkActiveKeyNumber(&i);
				#ifdef USE_EXTERNAL_NVM
					currentNetworkKeyInfo = plainSecurityKey[i-1];
				#else
								GetNwkKeyInfo(&currentNetworkKeyInfo, (ROM void *)&(NetworkKeyInfo[i-1]));
				#endif
								params.APSME_TRANSPORT_KEY_request.Key = &(currentNetworkKeyInfo.NetKey);
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = currentNetworkKeyInfo.SeqNumber.v[0];

							}
						#else
							#ifdef PRECONFIGURE_KEY
							{
								BYTE i;
								for(i = 0; i < 16; i++)
								{
									KeyVal.v[i] = 0;
								}
								params.APSME_TRANSPORT_KEY_request.Key = &KeyVal;
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = 0;
				params.APSME_TRANSPORT_KEY_request._UseSecurity = TRUE;
							}
							#else
			if( params.NLME_JOIN_indication.secureJoin )
							{
								BYTE i;
				for(i = 0; i < 16; i++)
				{
					KeyVal.v[i] = 0;
				}
				params.APSME_TRANSPORT_KEY_request.Key = &KeyVal;
				params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = 0;
				params.APSME_TRANSPORT_KEY_request._UseSecurity = TRUE;	
    	    }
    	    else 
			{
				BYTE i;
								GetNwkActiveKeyNumber(&i);
				#ifdef USE_EXTERNAL_NVM
					currentNetworkKeyInfo = plainSecurityKey[i-1];
				#else
								GetNwkKeyInfo(&currentNetworkKeyInfo, (ROM void *)&(NetworkKeyInfo[i-1]));
				#endif
								params.APSME_TRANSPORT_KEY_request.Key = &(currentNetworkKeyInfo.NetKey);
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = currentNetworkKeyInfo.SeqNumber.v[0];		
				params.APSME_TRANSPORT_KEY_request._UseSecurity = FALSE;
							}	
							#endif
			
						#endif
						params.APSME_TRANSPORT_KEY_request.KeyType = ID_NetworkKey;
						params.APSME_TRANSPORT_KEY_request.DestinationAddress = params.NLME_JOIN_indication.ExtendedAddress;
						params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.UseParent = FALSE;
						currentPrimitive = APSME_TRANSPORT_KEY_request;
					}
  					#else
						#ifdef I_SUPPORT_SECURITY_SPEC
							params.APSME_UPDATE_DEVICE_request.Status = (params.NLME_JOIN_indication.secureJoin ) ? 0x00 : 0x01;
						#else
							#ifdef PRECONFIGURE_KEY
								params.APSME_UPDATE_DEVICE_request.Status = 0x00;
							#else
								params.APSME_UPDATE_DEVICE_request.Status = 0x01;
							#endif
						#endif
						params.APSME_UPDATE_DEVICE_request.DeviceShortAddress = params.NLME_JOIN_indication.ShortAddress;
						params.APSME_UPDATE_DEVICE_request.DeviceAddress = params.NLME_JOIN_indication.ExtendedAddress;
						GetTrustCenterAddress(&params.APSME_UPDATE_DEVICE_request.DestAddress);
						for(i=0; i < 8; i++)
						    params.APSME_UPDATE_DEVICE_request.DestAddress.v[i] = 0xaa;
						        
						currentPrimitive = APSME_UPDATE_DEVICE_request;
					#endif
				#else
                	currentPrimitive = NO_PRIMITIVE;
				#endif				
                break;

            case NLME_LEAVE_indication:
                {
                    #if defined(__C30__)
                        LONG_ADDR myLongAddr;
                    
                        GetMACAddress(&myLongAddr);
                    if(!memcmppgm2ram( &params.NLME_LEAVE_indication.DeviceAddress, &myLongAddr, 8 ))   
                #else
                    if (!memcmppgm2ram( &params.NLME_LEAVE_indication.DeviceAddress, (ROM void *)&macLongAddr, 8 ))
                #endif
                {
                    ConsolePutROMString( (ROM char *)"We have left the network.\r\n" );
                }
                else
                {
                    ConsolePutROMString( (ROM char *)"Another node has left the network.\r\n" );
                }
              }
				#ifdef I_SUPPORT_SECURITY
		 			{
						SHORT_ADDR		LeavingChildShortAddress;
						if( !APSFromLongToShort(&params.NLME_LEAVE_indication.DeviceAddress) ) 
						{
							currentPrimitive = NO_PRIMITIVE;
							break;
						}
						LeavingChildShortAddress = currentAPSAddress.shortAddr;

					#ifdef I_AM_TRUST_CENTER
						params.APSME_UPDATE_DEVICE_indication.Status = 0x02;
						params.APSME_UPDATE_DEVICE_indication.DeviceAddress = params.NLME_LEAVE_indication.DeviceAddress;
						GetMACAddress(&params.APSME_UPDATE_DEVICE_indication.SrcAddress);
						params.APSME_UPDATE_DEVICE_indication.DeviceShortAddress = LeavingChildShortAddress;
						currentPrimitive = APSME_UPDATE_DEVICE_indication;
						break;
					#else
						params.APSME_UPDATE_DEVICE_request.Status = 0x02;
						GetTrustCenterAddress(&params.APSME_UPDATE_DEVICE_request.DestAddress);
						params.APSME_UPDATE_DEVICE_request.DeviceShortAddress = LeavingChildShortAddress;
						currentPrimitive = APSME_UPDATE_DEVICE_request;
						break;
					#endif
					}

				#else			
                	currentPrimitive = NO_PRIMITIVE;
				#endif
                break;

            case NLME_RESET_confirm:
                ConsolePutROMString( (ROM char *)"ZigBee Stack has been reset.\r\n" );
                /* For Zigbee 2006 The Specs require that node needs to 
                 * try joining as an orphan first after each reset, 
                 * see Mandatory test 3.9  
                */
                ZigBeeStatus.flags.bits.bTryOrphanJoin = 1;
                
                currentPrimitive = NO_PRIMITIVE;
                break;
                
            case NLME_LEAVE_confirm:
                PrintChar(params.NLME_LEAVE_confirm.Status); 
                ConsolePutROMString( (ROM char *)" Leaving the Zigbee network.\r\n" );
 
                               
                currentPrimitive = NO_PRIMITIVE;
                break;

            case APSDE_DATA_indication:
                {
                    WORD_VAL    attributeId;
                    BYTE        command;
                    BYTE        data;
                    BYTE        dataLength;
                    BYTE        frameHeader;
                    BYTE        sequenceNumber;
                    BYTE        transaction;
                    BYTE        transByte;

                    currentPrimitive = NO_PRIMITIVE;

                    switch (params.APSDE_DATA_indication.DstEndpoint)
                    {
                        case EP_ZDO:
                            if ((frameHeader & APL_FRAME_TYPE_MASK) == APL_FRAME_TYPE_MSG)
                            {
                                frameHeader &= APL_FRAME_COUNT_MASK;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    sequenceNumber          = APLGet();
                                    dataLength              = APLGet();
                                    transByte               = 1;    // Account for status byte

                                    switch( params.APSDE_DATA_indication.ClusterId.Val )
                                    {

                                        // ********************************************************
                                        // Put a case here to handle each ZDO response that we requested.
                                        // ********************************************************

                                        default:
                                            break;
                                    }

                                    // Read out the rest of the MSG in case there is another transaction.
                                    for (; transByte<dataLength; transByte++)
                                    {
                                        APLGet();
                                    }
                                }
                            }
                            break;

                        // ************************************************************************
                        // Place a case for each user defined endpoint.
                        // ************************************************************************
                        case 0xF0:
                        case 0x01:
                        case 0x04:
                        case 0x05:
                            {
                                BYTE i;
	                            BYTE frameHeaderIndex = TxData;
	                            WORD_VAL clusterID = params.APSDE_DATA_indication.ClusterId;
                                frameHeader = 1;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    BYTE PacketLen;
		                        	BYTE transactionNumber;
                                    transByte               = 1;    // Account for status byte

                                    switch(clusterID.Val)
                                    {
                                       
                                        case TRANSMIT_COUNTED_PACKETS_CLUSTER:
			                        	{
				                        	WORD_VAL Seq;
				                        	PacketLen = APLGet();
				                        	
				                        	Seq.v[0] = APLGet();
				                        	Seq.v[1] = APLGet();
				                        	if( Seq.Val > MSGPacketCount.Val )
				                        	{
					                        	MSGPacketCount.Val = Seq.Val;
											}
											for(i = 0; i < PacketLen-2; i++)
												APLGet();
										}
			                        	break;
			                        	
			                      
			                        	case BUFFER_TEST_REQUEST_CLUSTER:
				                    	{
					                    	BYTE SeqLen = APLGet();
					                    #ifdef I_SUPPORT_SECURITY
					                    	if( SeqLen < 66 )
					                    #else
					                    	if( SeqLen < 84 )
					                    #endif
					                    	{
						                    	TxBuffer[TxData++] = SeqLen;
						                    	TxBuffer[TxData++] = SUCCESS;
						                    	for(i = 0; i < SeqLen; i++)
						                    	{
							                    	TxBuffer[TxData++] = i;
							                    }
							                } 
							                else
							                {
	                            	            TxBuffer[TxData++] = SeqLen;
	                            	            TxBuffer[TxData++] = 0x01;
	                            	        }
			                        			
		                	        	    /* package the response */
			                        		{
    			                        		/* don't bother sending data to myself */
	                            	            if(params.APSDE_DATA_indication.SrcAddress.ShortAddr.Val==(macPIB.macShortAddress.Val))
    			                        		{
        			                        	    APSDiscardRx();
        			                        	    currentPrimitive = NO_PRIMITIVE;
        			                        	    break;
        			                            }
					                    		ZigBeeBlockTx();
	
   	                                    		params.APSDE_DATA_request.DstAddrMode = params.APSDE_DATA_indication.SrcAddrMode;
   		                                    	params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;
       		                                	params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
           		                            	params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_SUPPRESS;
                                            #ifdef I_SUPPORT_SECURITY
												params.APSDE_DATA_request.TxOptions.Val = 1;
                                            #else
                   		                    	params.APSDE_DATA_request.TxOptions.Val = 0;
                                            #endif
												i = params.APSDE_DATA_indication.SrcEndpoint;
												params.APSDE_DATA_request.SrcEndpoint = params.APSDE_DATA_indication.DstEndpoint;
                       	        	        	params.APSDE_DATA_request.DstEndpoint =  i;
                       	            	    	params.APSDE_DATA_request.ClusterId.Val = BUFFER_TEST_RESPONSE_CLUSTER;
                           	            		currentPrimitive = APSDE_DATA_request;
                           	            		/* Toggle LED1 when you recieve a request from group 4 */
                           	            		if(params.APSDE_DATA_request.DstEndpoint == GROUP_ID4)
                           	            		{
                               	            		MESSAGE_INDICATION = !MESSAGE_INDICATION;
                               	                }                      
                           	            	}	                            	            
					                    }
					                    break;
                                        
                                        case BUFFER_TEST_RESPONSE_CLUSTER:
					                    {
						                    BYTE len = APLGet();
						                    printf("\r\n Len: ");
						                    PrintChar(len);
						                    printf("\r\n");
						                    printf("From Address: ");
						                    PrintChar(params.APSDE_DATA_indication.SrcAddress.ShortAddr.byte.MSB);
						                    PrintChar(params.APSDE_DATA_indication.SrcAddress.ShortAddr.byte.LSB);
						                    printf("\r\n");
						                    for(i = 0; i < len+1; i++) {
						                    	PrintChar(APLGet());
						                    }
						                    printf("\r\n");	
						                }
					                    	break;
					                    
                                        case FREEFORM_MSG_REQUEST_CLUSTER:
				                    	{
					                    	BYTE requestType = APLGet();
					                    	TxBuffer[TxData++] = requestType; /* put request type back */
					                    	switch(requestType)
					                    	{
						                    	case 0x00:
						                    		TxBuffer[TxData++] = 0x42;
						                    		break;
						                    		
						                    	case 0x01:
						                    		TxBuffer[TxData++] = 0x5a;
						                    		TxBuffer[TxData++] = 0x69;
						                    		TxBuffer[TxData++] = 0x67;
						                    		TxBuffer[TxData++] = 0x42;
						                    		TxBuffer[TxData++] = 0x65;
						                    		TxBuffer[TxData++] = 0x65;
						                    		break;
						                    		
						                    	case 0x02:
						                    		TxBuffer[TxData++] = 0x12;
						                    		TxBuffer[TxData++] = 0x34;
						                    		TxBuffer[TxData++] = 0x56;
						                    		TxBuffer[TxData++] = 0x78;
						                    		break;
						                    }
			                        			
			                        		/* package the response */
			                        		{					                    
					                    		ZigBeeBlockTx();

	                                        	params.APSDE_DATA_request.DstAddrMode = params.APSDE_DATA_indication.SrcAddrMode;
    		                                    params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;
        		                                params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
            		                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
                                            #ifdef I_SUPPORT_SECURITY
												params.APSDE_DATA_request.TxOptions.Val = 1;
                                            #else
                    		                    params.APSDE_DATA_request.TxOptions.Val = 0;
                                            #endif
												i = params.APSDE_DATA_indication.SrcEndpoint;
												params.APSDE_DATA_request.SrcEndpoint = params.APSDE_DATA_indication.DstEndpoint;
                        	    	            params.APSDE_DATA_request.DstEndpoint =  i;
                        	        	        params.APSDE_DATA_request.ClusterId.Val = FREEFORM_MSG_RESPONSE_CLUSTER;
                            	        	    currentPrimitive = APSDE_DATA_request;					                    	
                            	        	}
					                    }
					                    break;
                                        
                                        /* Place other Cluster.ID cases here */
                                        
                                        default:
                                            printf(" \r\n Router recieved a response: \r\n ");
                                            for (transByte = 0; transByte<dataLength; transByte++)
                                            {
                                                PrintChar( APLGet() );
                                            }
                                            printf("  \r\n");
                                            break;
                                    }   /* switch 1*/
                            
                                }       /* for */
                                if( currentPrimitive != APSDE_DATA_request )
		                        	TxData = TX_DATA_START;
                            }           /* if msg */
      
                        break;
                            
                        /* Example of where to put other End Point Processing */   
                        case 0x90: 
                        case 0x91:
                        
                            printf("Got Message on an End Point\r\n");
                            
                            TxBuffer[TxData++] = params.APSDE_DATA_indication.DstEndpoint;

                            // Send back an application level acknowledge.
                            ZigBeeBlockTx();

                            params.APSDE_DATA_request.DstEndpoint = 0x90;
                            if( params.APSDE_DATA_indication.SrcAddrMode == APS_ADDRESS_NOT_PRESENT )
                            {
                                params.APSDE_DATA_request.DstAddrMode = 0x02;
                                params.APSDE_DATA_request.DstAddress.ShortAddr.Val = 0x0000;
                            } 
                            else
                            {
                                params.APSDE_DATA_request.DstAddrMode = params.APSDE_DATA_indication.SrcAddrMode;
                                params.APSDE_DATA_request.DstAddress.ShortAddr = params.APSDE_DATA_indication.SrcAddress.ShortAddr;
                            }

                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_SUPPRESS;										                                            
                            params.APSDE_DATA_request.TxOptions.Val = 0;
                                          
                            params.APSDE_DATA_request.SrcEndpoint = 0x91;

                            currentPrimitive = APSDE_DATA_request;
                            break;
                            
                        
                        default:
                            break;
                    }
                    APLDiscardRx();
                }
                break;

            case APSDE_DATA_confirm:
                if (params.APSDE_DATA_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Error " );
                    PrintChar( params.APSDE_DATA_confirm.Status );
                    ConsolePutROMString( (ROM char *)" sending message.\r\n" );
                }
                else
                {
                    ConsolePutROMString( (ROM char *)" Message sent successfully.\r\n" );
                    
                }
                currentPrimitive = NO_PRIMITIVE;
                break;

            case APSME_ADD_GROUP_confirm:
            case APSME_REMOVE_GROUP_confirm:
            case APSME_REMOVE_ALL_GROUPS_confirm:
                ConsolePutROMString( (ROM char *)" Perform Group Operation.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;
       
            case NO_PRIMITIVE:
                if (AllowJoin &&!ZigBeeStatus.flags.bits.bNetworkJoined)
                {
                    if (!ZigBeeStatus.flags.bits.bTryingToJoinNetwork)
                    {
                        if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                        {
                            ConsolePutROMString( (ROM char *)"Trying to join network as an orphan...\r\n" );
                            params.NLME_JOIN_request.ScanDuration     = 8;
                            params.NLME_JOIN_request.ScanChannels.Val = ALLOWED_CHANNELS;
                            params.NLME_JOIN_request.JoinAsRouter     = TRUE;
                            params.NLME_JOIN_request.RejoinNetwork    = 0x01;
                            params.NLME_JOIN_request.PowerSource      = MAINS_POWERED;
                            params.NLME_JOIN_request.RxOnWhenIdle     = TRUE;
                            params.NLME_JOIN_request.MACSecurity      = FALSE;
                            
                            params.NLME_JOIN_request.ExtendedPANID = currentNetworkDescriptor->ExtendedPANID;
                            currentPrimitive = NLME_JOIN_request;
                            
                            /* For Zigbee 2006 09/17/07, we now need to do an orphan join after a reset, but 
                             * we choose not to do this forever, and quit after a few of retries
                            */
                            orphanTries--;
                            if (orphanTries == 0)
                            {
                                orphanTries = 3;
                                ConsolePutROMString( (ROM char *)" Must now try as a new node...\r\n" );
                                ZigBeeStatus.flags.bits.bTryOrphanJoin = 0;
                            }
                        }
                        else
                        {
                            ConsolePutROMString( (ROM char *)"Trying to join network as a new device...\r\n" );
                            params.NLME_NETWORK_DISCOVERY_request.ScanDuration          = 8;
                            params.NLME_NETWORK_DISCOVERY_request.ScanChannels.Val      = ALLOWED_CHANNELS;
                            currentPrimitive = NLME_NETWORK_DISCOVERY_request;
                        }
                    }
                }
                else
                {
                    if (ZigBeeReady())
                    {
                        if (!ZigBeeStatus.flags.bits.bHasBackgroundTasks)
                        {
                            if ( ConsoleIsGetReady() )
                            {
                                loopTest = 0;
                                ProcessMenu();
                            }
                        }    

                        // ************************************************************************
                        // Place all processes that can send messages here.  Be sure to call
                        // ZigBeeBlockTx() when currentPrimitive is set to APSDE_DATA_request.
                        // ************************************************************************
                        /* Add/Remove Device to/from Group 4 */                      
                        if( PB_RIGHT_SWITCH == 0 )
                        {
                            /* wait debounce time before taking any action again */
                            if(PB_RIGHT_pressed == FALSE)
                            {
                                /* release to capture another button press */
                                PB_RIGHT_pressed = TRUE;
                                
                                /* determine if we should add or remove group */
                                if(group_state_ID4)
                                {
                                    params.APSME_ADD_GROUP_request.Endpoint          = GROUP_ID4; 
	                                params.APSME_ADD_GROUP_request.GroupAddress.v[1] = 0x00;
	                                params.APSME_ADD_GROUP_request.GroupAddress.v[0] = GROUP_ID4;
	                        
                                    currentPrimitive    = APSME_ADD_GROUP_request;
                                    GROUP_INDICATION = 1;
                                    printf("   \r\nAdded node to group 4\r\n");
                                }
                                else
                                {
                                    params.APSME_REMOVE_GROUP_request.Endpoint          = GROUP_ID4; 
	                                params.APSME_REMOVE_GROUP_request.GroupAddress.v[1] = 0x00;
	                                params.APSME_REMOVE_GROUP_request.GroupAddress.v[0] = GROUP_ID4;
	                        
                                    currentPrimitive = APSME_REMOVE_GROUP_request;
                                    GROUP_INDICATION = 0;
                                    printf("   \r\nRemoved node from group 4\r\n");
                                }
                                group_state_ID4     = !group_state_ID4;
                            
                                break;
                             }
                        }
                        else        /* Debounce Timeout period calculation */
                        {
                            TICK t = TickGet();
                            tick2Difference.Val = TickGetDiff(t,PB_RIGHT_press_time);
                
                            if(tick2Difference.Val > DEBOUNCE_TIME)
                            {
                                PB_RIGHT_pressed = FALSE;
                            }    
                      
                        }
                        
                        /* Send Message for Group 4 Nodes */
                        if(PB_LEFT_SWITCH == 0)
                        {
                            /* wait debounce time before taking any action again */
                            if(PB_LEFT_pressed == FALSE)
                            {
                                PB_LEFT_pressed = TRUE;
                                
                                /* Send group message to all the devices */
                                ZigBeeBlockTx();
                                TxBuffer[TxData++] = 0x0a;      /* request 10-bytes */

                                /* Use group addressing mode to send request */
                                params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_GROUP;
                    
                                /* GroupID MSB is zero in this example  */
                                params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = 0x00;
                                params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] = GROUP_ID4;
                            
                                params.APSDE_DATA_request.SrcEndpoint               = GROUP_ID4;
                    
	                            params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
                                params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_SUPPRESS;
                                #ifdef I_SUPPORT_SECURITY
                                    params.APSDE_DATA_request.TxOptions.Val = 1;
                                #else
	                            params.APSDE_DATA_request.TxOptions.Val = 0;
                                #endif
                    
                                /* no acknowledgement since this is a broadcast request */
	                            params.APSDE_DATA_request.TxOptions.bits.acknowledged = 0;
	                            params.APSDE_DATA_request.ProfileId.Val = 0x7f01;
                                params.APSDE_DATA_request.ClusterId.Val = BUFFER_TEST_REQUEST_CLUSTER;
                                currentPrimitive = APSDE_DATA_request;
                                
                                /* debounce lockout time start */
                                PB_LEFT_press_time = TickGet();
                                	 
                                break;
                            }   
                        }
                        else        /* Debounce Timeout period calculation */
                        {
                            TICK t = TickGet();
                            tickDifference.Val = TickGetDiff(t,PB_LEFT_press_time);
                
                            if(tickDifference.Val > DEBOUNCE_TIME)
                            {
                                PB_LEFT_pressed = FALSE;
                            }    
                      
                        }
                        
                        
                    }
                }
                break;

            default:
                PrintChar( currentPrimitive );
                ConsolePutROMString( (ROM char *)" Unhandled primitive.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;
        }
}

void ProcessNONZigBeeTasks(void)
{

        // *********************************************************************
        // Place any non-ZigBee related processing here.  Be sure that the code
        // will loop back and execute ZigBeeTasks() in a timely manner.
        // *********************************************************************
    {
        
    }
}

/*******************************************************************************
HardwareInit

All port directioning and SPI must be initialized before calling ZigBeeInit().

For demonstration purposes, required signals are configured individually.
*******************************************************************************/
#if defined(__C30__)
void HardwareInit(void)
{
    #if(CLOCK_FREQ < 1000000)
        SPI1CON1 = 0b0000000100111111;      // CLOCK_FREQ as SPI CLOCK
        SPI1STAT = 0x8000;
       
        SPI2CON1 = 0b0000000100111111;      // CLOCK_FREQ as SPI CLOCK
        SPI2STAT = 0x8000;
    #else
        SPI1CON1 = 0b0000000100111110;      // CLOCK_FREQ/4 as SPI CLOCK
        SPI1STAT = 0x8000;
        
        SPI2CON1 = 0b0000000100111110;      // CLOCK_FREQ/4 as SPI CLOCK
        SPI2STAT = 0x8000;
    #endif
    
    #ifdef USE_EXTERNAL_NVM
    	EEPROM_nCS		= 1;
    	EEPROM_nCS_TRIS	= 0;
    	IFS2bits.SPI2IF = 1;
    #endif
    
    PHY_RESETn = 0;
    PHY_RESETn_TRIS = 0;
    PHY_CS = 1;
    PHY_CS_TRIS = 0;
    
    TRISAbits.TRISA6 = 0;
    TRISAbits.TRISA7 = 0;
    
    RFIF = 0;
    RFIE = 1;
    
    if(RF_INT_PIN == 0)
    {
        RFIF = 1;
    }
    
    TRISDbits.TRISD6 = 1;
    TRISDbits.TRISD7 = 1;
  
    CNEN1bits.CN15IE = 1;
    CNEN2bits.CN16IE = 1;
    CNPU1bits.CN15PUE = 1;
    CNPU2bits.CN16PUE = 1;
    
    IFS1bits.CNIF = 0;
    IEC1bits.CNIE = 1;
    	   
}

#else
void HardwareInit(void)
{


    #ifdef USE_EXTERNAL_NVM
        EEPROM_nCS          = 1;
        EEPROM_nCS_TRIS     = 0;
    #endif

    #if defined(USE_EXTERNAL_NVM) && !defined(EE_AND_RF_SHARE_SPI)
        RF_SPIInit();
        EE_SPIInit();
    #else
        SPIInit();
    #endif

    #if (RF_CHIP == MRF24J40)
        // Start with MRF24J40 disabled and not selected
        PHY_CS              = 1;
        PHY_RESETn          = 1;

        // Set the directioning for the MRF24J40 pin connections.
        PHY_CS_TRIS         = 0;
        PHY_RESETn_TRIS     = 0;

        // Initialize the interrupt.
        INTCON2bits.INTEDG0 = 0;
    #else
        #error Unknown transceiver selected
    #endif

    #if defined(USE_EXTERNAL_NVM) && !defined(EE_AND_RF_SHARE_SPI)
        // Initialize the SPI1 pins and directions
        LATCbits.LATC3               = 0;    // SCK
        LATCbits.LATC5               = 1;    // SDO
        TRISCbits.TRISC3             = 0;    // SCK
        TRISCbits.TRISC4             = 1;    // SDI
        TRISCbits.TRISC5             = 0;    // SDO
    
        // Initialize the SPI2 pins and directions
        LATDbits.LATD6               = 0;    // SCK
        LATDbits.LATD4               = 1;    // SDO
        TRISDbits.TRISD6             = 0;    // SCK
        TRISDbits.TRISD5             = 1;    // SDI
        TRISDbits.TRISD4             = 0;    // SDO
    
        RF_SSPSTAT_REG = 0x40;
        RF_SSPCON1_REG = 0x21;
        EE_SSPSTAT_REG = 0x40;
        EE_SSPCON1_REG = 0x21;
    #else
        // Initialize the SPI pins and directions
        LATCbits.LATC3               = 0;    // SCK
        LATCbits.LATC5               = 1;    // SDO
        TRISCbits.TRISC3             = 0;    // SCK
        TRISCbits.TRISC4             = 1;    // SDI
        TRISCbits.TRISC5             = 0;    // SDO
    
        SSPSTAT_REG = 0x40;
        SSPCON1_REG = 0x20;
    #endif

    //-------------------------------------------------------------------------
    // This section is required for application-specific hardware
    // initialization.
    //-------------------------------------------------------------------------

    #if defined (__18F4620)
        // D1 and D2 are on RA0 and RA1 respectively, and CS of the TC77 is on RA2.
        // Make PORTA digital I/O.
        ADCON1 = 0x0F;
    
        // Deselect the TC77 temperature sensor (RA2)
        LATA = 0x04;
    
        // Make RA0, RA1, RA2 and RA4 outputs.
        TRISA = 0xE0;
    #endif

    // Clear the RBIF flag (INTCONbits.RBIF)
    INTCONbits.RBIF = 0;

    // Enable PORTB pull-ups (INTCON2bits.RBPU)
    INTCON2bits.RBPU = 0;

    // Make the PORTB switch connections inputs.
    #if !defined(__18F4620)
        TRISDbits.TRISD7 = 0;
        TRISBbits.TRISB3 = 1;
        TRISDbits.TRISD0 = 0;
        TRISDbits.TRISD1 = 0;        
    #endif
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB5 = 1;
}
#endif

/*******************************************************************************
User Interrupt Handler

The stack uses some interrupts for its internal processing.  Once it is done
checking for its interrupts, the stack calls this function to allow for any
additional interrupt processing.
*******************************************************************************/
#if defined(__C30__)
void UserInterruptHandler(void)
{
}

void _ISRFAST __attribute__((interrupt, auto_psv)) _CNInterrupt(void)
{
    // *************************************************************************
    // Place any application-specific interrupt processing here
    // *************************************************************************
    // Is this an interrupt-on-change interrupt?
    if ( IFS1bits.CNIF == 1 )
    {
        // Disable further RBIF until we process it
        IEC1bits.CNIE = 0;

        // Clear mis-match condition and reset the interrupt flag
        LATD = PORTD;

        IFS1bits.CNIF = 0;
    }	
}

#else
void UserInterruptHandler(void)
{

    // *************************************************************************
    // Place any application-specific interrupt processing here
    // *************************************************************************
    // Is this an interrupt-on-change interrupt?
    if ( INTCONbits.RBIF == 1 )
    {

        // Disable further RBIF until we process it
        INTCONbits.RBIE = 0;

        // Clear mis-match condition and reset the interrupt flag
        LATB = PORTB;

        INTCONbits.RBIF = 0;
    }
}
#endif

BYTE GetHexDigit( void )
{
    BYTE    c;

    while (!ConsoleIsGetReady());
    c = ConsoleGet();
    ConsolePut(c);

    if (('0' <= c) && (c <= '9'))
        c -= '0';
    else if (('a' <= c) && (c <= 'f'))
        c = c - 'a' + 10;
    else if (('A' <= c) && (c <= 'F'))
        c = c - 'A' + 10;
    else
        c = 0;

    return c;
}

BYTE GetMACByte( void )
{
    BYTE    oneByte;

    //ConsolePutROMString( (ROM char * const) "\r\n\r\nEnter last MAC byte in hex: " );
    oneByte = GetHexDigit() << 4;
    oneByte += GetHexDigit();
    //ConsolePutROMString( (ROM char * const) "\r\n\r\n" );

    return oneByte;
}


void ProcessMenu( void )
{

    BYTE        c;
    SHORT_ADDR  shortAddress;

    DISABLE_WDT();

    /* Get the user's input from the keyboard. */
    c = ConsoleGet();
    ConsolePut( c );
    switch (c)
    { 
        /* Enable or Disable Joining by other devices */       
		case '1':  
        	ConsolePutROMString((ROM char * const)"\r\n0=Disable Join, 1=Enable Join : ");
        	while( !ConsoleIsGetReady());
        	c = ConsoleGet();
        	ConsolePut(c);
        	switch(c)
        	{
        		case '0':
        			params.NLME_PERMIT_JOINING_request.PermitDuration   = 0x00;
        			params.NLME_PERMIT_JOINING_request._updatePayload          = FALSE;
        			currentPrimitive = NLME_PERMIT_JOINING_request;
        			break;
        		case '1':
        			params.NLME_PERMIT_JOINING_request.PermitDuration   = 0xFF;
        			params.NLME_PERMIT_JOINING_request._updatePayload          = FALSE;
        			currentPrimitive = NLME_PERMIT_JOINING_request;
        			break;
        		
        	}   
            break;
            
        /* Request 16-bytes of data from Another Device */    			
		case '2':
	 		printf("\r\nHow many bytes are you requesting(hex): ");
	 		TxBuffer[TxData++] = GetMACByte();
	 		ZigBeeBlockTx();
	 		
            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
            printf("\r\nWhat is the short address of device you want data from: ");
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = GetMACByte();
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] =  GetMACByte();
        	params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
        	
            params.APSDE_DATA_request.DiscoverRoute =  ROUTE_DISCOVERY_SUPPRESS;
            
            
        #ifdef I_SUPPORT_SECURITY
            params.APSDE_DATA_request.TxOptions.Val = 1;
        #else
        	params.APSDE_DATA_request.TxOptions.Val = 0;
        #endif
			params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;
			params.APSDE_DATA_request.SrcEndpoint = 1;
            params.APSDE_DATA_request.DstEndpoint =  240;
            params.APSDE_DATA_request.ProfileId.Val = 0x7f01;
            params.APSDE_DATA_request.ClusterId.Val = BUFFER_TEST_REQUEST_CLUSTER;
            currentPrimitive = APSDE_DATA_request;	 
            break;		 	

		/* Request Data From a group of devices */
        case '3':
        		 		
	 		TxBuffer[TxData++] = 0x0a;      /* request 10-bytes */
	 		
			ZigBeeBlockTx();

            params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_GROUP;
            printf("\r\nPlease enter the Group ID of the Data Request: ");
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = GetMACByte();
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] =  GetMACByte();
        	params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
            params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_SUPPRESS;
        #ifdef I_SUPPORT_SECURITY
            params.APSDE_DATA_request.TxOptions.Val = 1;
        #else
        	params.APSDE_DATA_request.TxOptions.Val = 0;
        #endif
			params.APSDE_DATA_request.TxOptions.bits.acknowledged = 0;
			params.APSDE_DATA_request.ProfileId.Val = 0x7f01;
            params.APSDE_DATA_request.ClusterId.Val = BUFFER_TEST_REQUEST_CLUSTER;
            currentPrimitive = APSDE_DATA_request;	 
            break;		 			
			
    
	    /* Send data directly to another device */		
		case '4':
		    printf("\r\nPlease enter the number of bytes to send (hex): ");
		    temp2 = GetMACByte();
	 		if(temp2 > 0x52)
	 		    temp2 = 0x52;  

			/* send buffer test to device with short address 0002 */
			TxBuffer[TxData++]   = temp2 + 2; // Length
			TxBuffer[TxData++]   = 0x00;                    // octet sequence 
			TxBuffer[TxData++]   = 0x00;
   			
   			/* Load the transmit buffer with the data to send */		
            for(i = 0; i < temp2; i++)
		    {
  		        TxBuffer[TxData++] = i;
			}

    	    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
       		printf("\r\nPlease enter the short address of the destination device: ");
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = GetMACByte();
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] = GetMACByte();
    	    
    	    params.APSDE_DATA_request.SrcEndpoint    = 1;
            params.APSDE_DATA_request.DstEndpoint    = 240;
            params.APSDE_DATA_request.ProfileId.Val  = MY_PROFILE_ID;
    	    
       		//params.APSDE_DATA_request.asduLength; TxData
			params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
	   	    params.APSDE_DATA_request.DiscoverRoute = TRUE;
	   	    params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;
	   	    
	   	    
            params.APSDE_DATA_request.DiscoverRoute =  ROUTE_DISCOVERY_SUPPRESS;
            
	   	    #ifdef I_SUPPORT_SECURITY
			    params.APSDE_DATA_request.TxOptions.Val = 1;
            #else
                params.APSDE_DATA_request.TxOptions.Val = 0;
            #endif
			params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;			        
    		params.APSDE_DATA_request.ClusterId.Val = TRANSMIT_COUNTED_PACKETS_CLUSTER;	            
    				
    		ZigBeeBlockTx();
		    currentPrimitive = APSDE_DATA_request;
		    break;
		    

		case '5':
		    printf("\r\nPlease enter the number of bytes to send (hex): ");
		    temp2 = GetMACByte();
	 		if(temp2 > 0x52)
	 		    temp2 = 0x52;  

			/* send buffer test to device with short address 0002 */
			TxBuffer[TxData++]   = temp2 + 2; // Length
			TxBuffer[TxData++]   = 0x00;                    // octet sequence 
			TxBuffer[TxData++]   = 0x00;
   			
   			/* Load the transmit buffer with the data to send */		
            for(i = 0; i < temp2; i++)
		    {
  		        TxBuffer[TxData++] = i;
			}

    	    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_GROUP;
       		printf("\r\nEnter the GroupID of devices to send data: ");
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = GetMACByte();
    	    params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] = GetMACByte();
    	    
    	    params.APSDE_DATA_request.SrcEndpoint    = 1;
            params.APSDE_DATA_request.DstEndpoint    = 240;
            params.APSDE_DATA_request.ProfileId.Val  = MY_PROFILE_ID;
    	    
			params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
	   	    params.APSDE_DATA_request.DiscoverRoute = TRUE;
	   	    params.APSDE_DATA_request.TxOptions.bits.acknowledged = 0;
	   	    
	   	    
            params.APSDE_DATA_request.DiscoverRoute =  ROUTE_DISCOVERY_SUPPRESS;
            
	   	    #ifdef I_SUPPORT_SECURITY
			    params.APSDE_DATA_request.TxOptions.Val = 1;
            #else
                params.APSDE_DATA_request.TxOptions.Val = 0;
            #endif
	        
    		params.APSDE_DATA_request.ClusterId.Val = TRANSMIT_COUNTED_PACKETS_CLUSTER;	            
    				
    		ZigBeeBlockTx();
		    currentPrimitive = APSDE_DATA_request;
		    break;

        
        /* Add/Remove device to/from a Group */
		case '6':
		    
		    printf("\r\n\n What Group Activity do you want to do ? \r\n"); 
		    printf("0=Add Device to a Group \r\n1=Remove Device from a Group\r\n2=Remove Device from All Groups: ");
		    
			while( !ConsoleIsGetReady());
 			c = ConsoleGet();
 			ConsolePut(c);
 			
 			if( c < '0' || c > '2' )
		        break;
		        
		        
		    switch(c)
		    {
    		    case '0':
    		    case '1':
		            printf("\r\nEnter 16-bit Group ID (Hex): ");    
		            params.APSME_ADD_GROUP_request.GroupAddress.v[1] = GetMACByte();
		            params.APSME_ADD_GROUP_request.GroupAddress.v[0] = GetMACByte();
		            
		            if(c == '0')
		            {
    		            /* Using a Fixed endpoint here to simplify things in this application */
		                params.APSME_ADD_GROUP_request.Endpoint = GROUP_ID4; 
                        currentPrimitive = APSME_ADD_GROUP_request;
                        GROUP_INDICATION    = 1;
                    }
                    else
                    {
                        params.APSME_REMOVE_GROUP_request.Endpoint = GROUP_ID4;
                        currentPrimitive = APSME_REMOVE_GROUP_request;
                        GROUP_INDICATION    = 0;
                    }    
		            break;
		            
		        case '2':
		            params.APSME_REMOVE_ALL_GROUPS_request.Endpoint = GROUP_ID4;
		            currentPrimitive = APSME_REMOVE_ALL_GROUPS_request;
		            GROUP_INDICATION    = 0;
		            break;
		            
		     }
		     break;

		
		case '7':
		    #ifdef USE_EXTERNAL_NVM
                    pCurrentNeighborRecord = neighborTable;   //+ (WORD)neighborIndex * (WORD)sizeof(NEIGHBOR_RECORD);
            #else
                    pCurrentNeighborRecord = &(neighborTable[0]);
            #endif
            printf("\r\nShort              MAC                 Type        Rntlship ");
		    for ( i=0; i < MAX_NEIGHBORS; i++ )
            {
                
                GetNeighborRecord( &currentNeighborRecord, pCurrentNeighborRecord );
                if ((currentNeighborRecord.deviceInfo.bits.bInUse))
                {
                    BYTE z;
                    printf("\r\n");
                    PrintChar(currentNeighborRecord.shortAddr.byte.MSB);                    PrintChar(currentNeighborRecord.shortAddr.byte.LSB);
                    printf("    |    ");
                    
                    for(z=0; z < 8; z++)
                       PrintChar(currentNeighborRecord.longAddr.v[7-z]);
                    
                    printf("    |     ");
                    
                    if((currentNeighborRecord.deviceInfo.bits.deviceType == 0x01))
                        printf("RTR");

                    else if((currentNeighborRecord.deviceInfo.bits.deviceType == 0x02))
                        printf("RFD");
                        
                    else if((currentNeighborRecord.deviceInfo.bits.deviceType == 0x00))
                        printf("CRD");
                       
                    else
                        printf("UKN");
                        
                    printf("    |     ");
                    if(currentNeighborRecord.deviceInfo.bits.Relationship == 0x01)
                        printf("CHILD ");
                    else if(currentNeighborRecord.deviceInfo.bits.Relationship == 0x00)
                        printf("PARENT");
                    else
                        printf("UNKWN ");      
     
                }
                #ifdef USE_EXTERNAL_NVM
                    pCurrentNeighborRecord += (WORD)sizeof(NEIGHBOR_RECORD);
                #else
                    pCurrentNeighborRecord++;
                #endif
            }
            printf("\r\n");	
		    break;
		
		/* Request the route to a device */            
		case '9':
                    params.NLME_ROUTE_DISCOVERY_request.DstAddrMode = APS_ADDRESS_16_BIT;
                    printf("\r\nPlease enter the short address of the destination device(hex): ");
    	            params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = GetMACByte();
    	            params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] = GetMACByte();
                    
                    params.NLME_ROUTE_DISCOVERY_request.Radius      = DEFAULT_RADIUS;
                   
					ZigBeeBlockTx();
					currentPrimitive = NLME_ROUTE_DISCOVERY_request;
	      		    break;	
        default:
            break;
    }

    PrintMenu();

    ENABLE_WDT();

}


void PrintMenu( void )
{
    ConsolePutROMString(menu);
    ConsolePutROMString( (ROM char * const) "\r\nEnter a menu choice: " );
}

