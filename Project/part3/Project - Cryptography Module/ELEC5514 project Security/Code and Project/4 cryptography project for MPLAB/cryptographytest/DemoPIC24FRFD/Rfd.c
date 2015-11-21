/*
    Microchip ZigBee2006 Residential Stack

    Demo Reduced Function Device

    This demonstration shows how a ZigBee coordinator can be set up.  This demo allows
    the PICDEM Z/Explorer 16 Demostration Board to act as ZigBee protocol Coordinator
    It is designed to interact with other ZigBee protocol devices - Routers and End Devices.
    
    Switch and LED functionality are as follows:

    RB4/RD6:  Adds a node to, or removes a node from Group 4.  
    RA0/D10:  Used to indicate that a node is a member of Group 4 (Lit – yes; unlit – no) 

    At startup the devices are not in any group, and the lit LEDs just indicate they are ready and on the network.
    
    RB5/RD7:   Is used to send a messages to nodes in Group 4.    
               The actual message is a request for all the Group 4 nodes to send 
               back to the requester 10 bytes.
    RA1/D09:   Toggles to indicate that the receiving node is in Group 4 
               and received requests for data.  


    NOTE: To speed network formation, ALLOWED_CHANNELS has been set to
    channel 21 only.  
    Please consult the PICDEM Z Zigbee2006 Residential StackQTGuide.pdf for how to 
    run this sample application.

 *********************************************************************
 * FileName:        Rfd.c
 * Dependencies:
 * Processor:       PIC18F/PIC24F
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
 * DL                   08/01/08 Microchip ZigBee Stack v2.0-2.6
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

#if defined(MCHP_C18) && defined(__18F4620)
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

    #pragma romdata
#elif defined(__PIC24F__)
	// Explorer 16 board
	_CONFIG2(FNOSC_PRI & POSCMOD_XT)	// Primary XT OSC with 4X PLL
	_CONFIG1(JTAGEN_OFF & FWDTEN_OFF & WDTPS_PS512 )	// JTAG off, watchdog timer off, 
	                                                    //prescale 512(~2secs timeout on WDT)
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

void HardwareInit( void );

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

/* Menu System */
ROM char * const menu =
    "\r\n     2: Request Data From Another Device"
    "\r\n     3: Request Data From a Group of Devices"
    "\r\n     4: Send Data To Another Device"
    "\r\n     5: Send Data To a Group of Devices"
    "\r\n     6: Add/Remove Device to/from a Group"
    "\r\n     7: Dump Neighborhood Information"
    ;
	
// enums to execute menu options.
SHORT_ADDR          discoveredAddress;
BYTE                routeDiscovery;

WORD_VAL MSGPacketCount;

//******************************************************************************
// Constants
//******************************************************************************

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
BYTE AllowJoin = 1;

//******************************************************************************
// Function Prototypes
//******************************************************************************
void ProcessMenu( void );
void PrintMenu( void );
BYTE GetHexDigit( void );
BYTE GetMACByte( void );
WORD GetShortAddressVal( void );
void HardwareInit(void);
void SendLightMessage( WORD destVal, BYTE message, BYTE rd );

void SendCountedPacket(BYTE seqNumber, BYTE len);

void ProcessNONZigBeeTasks(void);
void ProcessZigBeePrimitives(void);

extern void     RemoveAllGroups(void);
extern void     RemoveAllBindings(SHORT_ADDR);
extern      BYTE APSCounter;
extern      NWK_STATUS  nwkStatus;
extern      BYTE    SendingEDBRequest;
extern      BYTE    macLongAddrByte[8];

BYTE        loopCount, byteMSB, byteLSB, i;
TICK        transmitInterval, currentTime;

BOOL     gettingAcks = FALSE;
BOOL     toggle = TRUE;
BYTE     temp2;

static TICK startPollingTime;

BOOL    group_state_ID5 = TRUE;
BOOL    group_state_ID4 = TRUE;
BYTE    GROUP_ID4       = 0x04;

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
BOOL PB_LEFT_pressed    = FALSE;        /* RB5/RD6 */
BOOL PB_RIGHT_pressed   = FALSE;        /* RB4/RD7 */

TICK PB_LEFT_press_time;
TICK PB_RIGHT_press_time;
TICK tickDifference;
TICK tick2Difference;

/* determines whether both the PIC and radio sleep, or just simulated in 
 * order to debug and use the menus with the uart
 */
//#define BOTH_MICRO_TRANSV_SLEEP
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
    
    ConsolePutROMString( (ROM char *)"\r\n\r\n\r\n********************************" );
    ConsolePutROMString( (ROM char *)"\r\nZigBee End Device - v2.0-2.6.0a\r\n\r\n" );
    startPollingTime.Val = 0x00;
    
    // *************************************************************************
    // Perform any other initialization here
    // *************************************************************************

    NWKClearNeighborTable();
    #if defined(I_SUPPORT_BINDING)
        ClearBindingTable();
    #endif
    /* Clear the Group Table */
    RemoveAllGroups();
    
    #if defined(I_SUPPORT_BINDING)
        RemoveAllBindings(macPIB.macShortAddress);
    #endif

    discoveredAddress.Val = 0x0000;
    routeDiscovery = 0;

    // Enable interrupts to get everything going.
    #if !defined(__C30__)
    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    #endif
    /* Set LEDs to a known state */
    GROUP_INDICATION	    = 0;
	MESSAGE_INDICATION		= 0;

    
    /* Initialize network status bits */
    nwkStatus.flags.Val = 0x00;
    ZigBeeStatus.flags.bits.bRadioIsSleeping = 0;
    
    while (1)
    {
        CLRWDT();
        /* Determine which is the next ZigBee Primitive operate on*/
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
                        params.NLME_JOIN_request.JoinAsRouter   = FALSE;
                        params.NLME_JOIN_request.RejoinNetwork  = FALSE;
                        params.NLME_JOIN_request.PowerSource    = NOT_MAINS_POWERED;
                        params.NLME_JOIN_request.RxOnWhenIdle   = FALSE;
                        params.NLME_JOIN_request.MACSecurity    = FALSE;
                        currentPrimitive = NLME_JOIN_request;

                        ConsolePutROMString( (ROM char *)"Network(s) found. Trying to join " );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.MSB );
                        PrintChar( params.NLME_JOIN_request.PANId.byte.LSB );
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
                    
                    /* package and send an end device announcement */
                    {
                        LONG_ADDR myLongAddress;
                        GetMACAddress(&myLongAddress);
                    
                        ConsolePutROMString( (ROM char *)"Announcing I am on the network\r\n" );
                        
                        ZigBeeBlockTx();
                        TxBuffer[TxData++] = APSCounter++;
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
                        /* Set LEDs to the joined the network state */
                        GROUP_INDICATION	    = 1;
	                    MESSAGE_INDICATION		= 1;
                    
                        currentPrimitive = APSDE_DATA_request;
                    }

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
                            // We ran out of descriptors.  Free the network descriptor list, and fall
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
                    currentPrimitive = NO_PRIMITIVE;
                    break;

            case NLME_RESET_confirm:
                ConsolePutROMString( (ROM char *)"ZigBee Stack has been reset.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;

            case NLME_START_ROUTER_confirm:
                if (!params.NLME_START_ROUTER_confirm.Status)
                {
                    ConsolePutROMString( (ROM char *)"Router Started!\r\n" );
                }
                else
                {
                    PrintChar( params.NLME_JOIN_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Router start unsuccessful. We cannot route frames.\r\n" );
                }

                // We are now ready to do ZigBee related tasks.
                currentPrimitive = NO_PRIMITIVE;

                break;

            case APSDE_DATA_indication:
                {
                    WORD_VAL    attributeId;
                    BYTE        command;
                    BYTE        data;
                    BYTE        frameHeader;
                    BYTE        sequenceNumber;
                    BYTE        transaction;
                    BYTE        transByte;
                    
                    BYTE        frameHeaderIndex  = TxData;
                    currentPrimitive = NO_PRIMITIVE;

                    switch (params.APSDE_DATA_indication.DstEndpoint)
                    {
                        
                        case EP_ZDO:
                            #define dataLength command
                            
                                frameHeader = 1;
                                for (transaction=0; transaction<frameHeader; transaction++)
                                {
                                    sequenceNumber          = APLGet();
                                    transByte               = 1;    // Account for status byte

                                    switch( params.APSDE_DATA_indication.ClusterId.Val )
                                    {

                                        // ********************************************************
                                        // Put a case here to handle each ZDO response that we requested.
                                        // ********************************************************

                                        case NWK_ADDR_rsp:
                                            if (APLGet() == SUCCESS)
                                            {
                                                ConsolePutROMString( (ROM char *)"  Receiving NWK_ADDR_rsp.\r\n" );

                                                // Skip over the IEEE address of the responder.
                                                for (data=0; data<8; data++)
                                                {
                                                    APLGet();
                                                    transByte++;
                                                }
                                                discoveredAddress.byte.LSB = APLGet();
                                                discoveredAddress.byte.MSB = APLGet();
                                                transByte += 2;
                                            }
                                            break;
                                            
                                            
                                       #ifdef SUPPORT_END_DEVICE_BINDING
                                        case END_DEVICE_BIND_rsp:
                                            switch( APLGet() )
                                            {
                                                case SUCCESS:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind successful!\r\n" );
                                                    
                                                    break;
                                                case ZDO_NOT_SUPPORTED:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind not supported.\r\n" );
                                                    break;
                                                case END_DEVICE_BIND_TIMEOUT:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind time out.\r\n" );
                                                    break;
                                                case END_DEVICE_BIND_NO_MATCH:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind failed - no match.\r\n" );
                                                    break;
                                                default:
                                                    ConsolePutROMString( (ROM char *)"End device bind/unbind invalid response.\r\n" );
                                                    break;
                                            }
                                            
                                            /* Go back to slower poll rate */
                                            SendingEDBRequest = 0;
                                            currentPrimitive = NO_PRIMITIVE;
                                            break;
                                        #endif     
                                       
                                       
                                            

                                        default:
                                            printf(" Got message on EndPoint zero ..");
                                            break;
                                    }

                                }
                            #undef dataLength
                            break;

                        // ************************************************************************
                        // Place a case for each user defined endpoint.
                        // ************************************************************************

                        default:
                           
                            {
	                            WORD_VAL clusterID = params.APSDE_DATA_indication.ClusterId;
	                        	frameHeader = 1;   
	                        	for(transaction=0; transaction<frameHeader; transaction++)
	                        	{
		                        	BYTE PacketLen;
		                        	BYTE transactionNumber;
		                        	switch( clusterID.Val )
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
			                        	
			                        	case RESET_PACKET_COUNT_CLUSTER:
			                        		MSGPacketCount.Val = 0;
			                        		break;
			                        		
			                        	case RETRIEVE_PACKET_COUNT_CLUSTER:			                        		
			                        		TxBuffer[TxData++] = sequenceNumber;
			                        		TxBuffer[TxData++] = 2;
			                        		TxBuffer[TxData++] = MSGPacketCount.v[0];
			                        		TxBuffer[TxData++] = MSGPacketCount.v[1];
			                        		MSGPacketCount.Val++;
			                        		transactionNumber = TxBuffer[frameHeaderIndex] & APL_FRAME_COUNT_MASK;
			                        		TxBuffer[frameHeaderIndex] &= APL_FRAME_TYPE_MASK;
			                        		TxBuffer[frameHeaderIndex] |= (transactionNumber+1);
			                        		
											if( transactionNumber == 0 )
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
                        	                	params.APSDE_DATA_request.ClusterId.Val = PACKET_COUNT_RESPONSE_CLUSTER;
                            	            	currentPrimitive = APSDE_DATA_request;
                            	            }
			                        		
			                    			break;  
			                    			
			                    		case PACKET_COUNT_RESPONSE_CLUSTER:
			                    		{
				                    		BYTE PC_LSB = APLGet();
				                    		BYTE PC_MSB = APLGet();
				                    		
				                    		ConsolePutROMString((ROM char *)"Packet Count Response: ");
				                    		PrintChar(PC_MSB);
				                    		PrintChar(PC_LSB);
				                    		ConsolePutROMString((ROM char *)"\r\n");
				                    	}  	
				                    	break;
				                    	
				                    	case BUFFER_TEST_REQUEST_CLUSTER:
				                    	{
  					                    	BYTE SeqLen = APLGet();
  					                    	   printf("\r\n Internal message - I am in same group!\r\n");
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
							                } else {
	                            	            TxBuffer[TxData++] = SeqLen;
	                            	            TxBuffer[TxData++] = 0x01;
	                            	        }
	                            	            /* don't bother sending data to myself */
	                            	            if(params.APSDE_DATA_indication.SrcAddress.ShortAddr.Val==(macPIB.macShortAddress.Val))
    			                        		{
        			                        	    APSDiscardRx();
        			                        	    currentPrimitive = NO_PRIMITIVE;
        			                        	    break;
        			                            } 
		                	        		    /* package and send response */
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
					                    }
					                    /* Group 4 uses End Point 4 so here that application
					                     * is Toggling LED1 it received a  request from Group 4 
					                    */
					                    if(params.APSDE_DATA_indication.DstEndpoint == GROUP_ID4)
					                        MESSAGE_INDICATION = !MESSAGE_INDICATION;
					                    break;
					                    
					                    case BUFFER_TEST_RESPONSE_CLUSTER:
					                    {
						                    BYTE len = APLGet();
						                    printf("\r\nLen: ");
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
					                    	TxBuffer[TxData++] = requestType;  /* return it */
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
			                        		/* send response */
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
				                    	
				                    	case FREEFORM_MSG_RESPONSE_CLUSTER:
				                    	{
					                    	BYTE len = APLGet();
					                    	for(i = 0; i < len; i++)
					                    		APLGet();
				                    		break;
				                    	}
				                    	
				                    	default:
				                    		break;
			                        }
		                        		
		                        }
	                        	if( currentPrimitive != APSDE_DATA_request )
		                        	TxData = TX_DATA_START;
	                        	
	                        }
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
                //ConsolePutROMString( (ROM char *)" Performed Group Operation.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;    

            case NO_PRIMITIVE:
                
                if (!ZigBeeStatus.flags.bits.bNetworkJoined)
                {
                    if (!ZigBeeStatus.flags.bits.bTryingToJoinNetwork)
                    {
                        if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                        {
                            ConsolePutROMString( (ROM char *)"Trying to join network as an orphan...\r\n" );
                            params.NLME_JOIN_request.ScanDuration       = 8;
                            params.NLME_JOIN_request.ScanChannels.Val   = ALLOWED_CHANNELS;
                            params.NLME_JOIN_request.JoinAsRouter       = FALSE;
                            params.NLME_JOIN_request.RejoinNetwork      = TRUE;
                            params.NLME_JOIN_request.PowerSource        = NOT_MAINS_POWERED;
                            params.NLME_JOIN_request.RxOnWhenIdle       = FALSE;
                            params.NLME_JOIN_request.MACSecurity        = FALSE;
                            currentPrimitive = NLME_JOIN_request;
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
                    // Process the pushbutton interrupts 
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
                        if(PB_LEFT_SWITCH == 0 /* && (toggle == TRUE ) */ )
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

            #if !defined(BOTH_MICRO_TRANSV_SLEEP)
                        /* check the menu to see if anything is there to process */
                        if (!ZigBeeStatus.flags.bits.bHasBackgroundTasks)
                        {
                            if ( ConsoleIsGetReady())
                            {
                                ProcessMenu();
                            }
                            
                        }
            #endif
                    }
                    /* Process when there are no primitives */
                    if (currentPrimitive == NO_PRIMITIVE)
                    {
                        if (!ZigBeeStatus.flags.bits.bDataRequestComplete)
                        {
                            // We have not received all data from our parent.  If we are not waiting
                            // for an answer from a data request, send a data request.
                            if (!ZigBeeStatus.flags.bits.bRequestingData)
                            {
                                if (ZigBeeReady())
                                {
                                    // Our parent still may have data for us.
                                    params.NLME_SYNC_request.Track = FALSE;
                                    currentPrimitive = NLME_SYNC_request;
                                }
                            }
                        }
                        else
                        {
                            if (!ZigBeeStatus.flags.bits.bHasBackgroundTasks )
                            {
                                // There is no primitive to execute,  all messages extracted
                                // from parent, the stack has no background tasks,
                                // and all application-specific processes are complete.  Now 
                                // go to sleep.  Make sure that the UART is finished, turn off the transceiver,
                                
                 #if defined(BOTH_MICRO_TRANSV_SLEEP)       
                                if(!ZigBeeStatus.flags.bits.bRadioIsSleeping)  
                                {
	                                while (!ConsoleIsPutReady());
                                    #if defined(__18F4620)
    	                                INTCONbits.RBIE = 1;
    	                            #endif
    	                            #if defined(__PIC24F__)
    	                                IEC1bits.CNIE   = 1;
    	                            #endif
    	                            MRF24J40Sleep();
	                                SLEEP();
        	                        NOP();

	                                /* Woken up by WDT, so request data from parent */
	                                MRF24J40Wake();
	                                params.NLME_SYNC_request.Track = FALSE;
	                                currentPrimitive = NLME_SYNC_request;
	                                
	                                /* Toggle LEDs at Polling rate */
                                    GROUP_INDICATION    = !GROUP_INDICATION;
                                    MESSAGE_INDICATION  = !MESSAGE_INDICATION;
	                             }
			     #endif
			    #if !defined(BOTH_MICRO_TRANSV_SLEEP)              
			                    /* To enable menu system, while still polling use this block */
								/* During End Device Bind speed up Polling, else go back to reqular polling rate */
								{
	                                TICK currentTime;
	                                /*	Capture the current time for initiating end device polling	*/
	                                
	                                currentTime = TickGet();
	                                /*	if the diff between current and the start time is greater than
	        	                    polling rate than initiate sync request	*/
                                    /* Speed things up during EDB */
                                    if(SendingEDBRequest == 0)
                                    {
	                                    if( ( TickGetDiff( currentTime, startPollingTime ) ) > (RFD_POLL_RATE))
	                                    {   
        		                                params.NLME_SYNC_request.Track = FALSE;
		                                        currentPrimitive                = NLME_SYNC_request;
		                                    /* Capture the start polling time for the next cycle	*/
		                                    startPollingTime = TickGet();
	                                    }
	                                }
	                                else
	                                {
    	                                /* Speed things up during End Device Binding */
    	                                if( ( TickGetDiff( currentTime, startPollingTime ) ) > (ONE_SECOND)/4)
	                                    {   
		                                    params.NLME_SYNC_request.Track = FALSE;
		                                    currentPrimitive = NLME_SYNC_request;
		                                    /* Toggle LEDs at Polling rate: faster during Binding */
                                            GROUP_INDICATION    = !GROUP_INDICATION;
                                            MESSAGE_INDICATION  = !MESSAGE_INDICATION;
		                                    /*Capture the start polling time for the next cycle	*/
		                                    startPollingTime = TickGet();
	                                    }   
    	                                
    	                                
    	                            }
    	                                

                                }
								
							#endif	
								
								
								
                            }
                        }
                    }
                    
                    /* end  block */
                    
                //}
                break;

            default:
                //ConsolePutROMString( (ROM char *)" Unhandled primitive.\r\n" );
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

BYTE extendedResponse = 0;

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
                    PrintChar(currentNeighborRecord.shortAddr.byte.MSB);
                    PrintChar(currentNeighborRecord.shortAddr.byte.LSB);
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
    ConsolePutROMString( (ROM char * const) "\r\n\r\nEnter a menu choice: " );
}


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

WORD GetShortAddressVal( void )
{
    SHORT_ADDR  address;

    ConsolePutROMString( (ROM char * const) "\r\n\r\nEnter target short address in hex: " );
    address.byte.MSB = GetHexDigit() << 4;
    address.byte.MSB += GetHexDigit();
    address.byte.LSB = GetHexDigit() << 4;
    address.byte.LSB += GetHexDigit();
    ConsolePutROMString( (ROM char * const) "\r\n\r\n" );

    return address.Val;
}

#if defined(__C30__)
/*********************************************************************
 * Function:        void HardwareInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 *
 * Overview:        This function initializes the ZigBee hardware and is required
 *                  before any stack operations can be performed
 ********************************************************************
All port directioning and SPI must be initialized before calling ZigBeeInit().
*******************************************************************************/
void HardwareInit(void)
{
    SPI1CON1 = 0b0000000100111110;
    SPI1STAT = 0x8000;
    
    SPI2CON1 = 0b0000000100111110;
    SPI2STAT = 0x8000;
    
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
    
    PHY_WAKE        = 1;
    PHY_WAKE_TRIS = 0;
    	   
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

    // PIC24 - Initialize the SPI module
//    SPI2CON1 = 0b0000000100111110;
//    SPI2STAT = 0x8000;

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
    #endif
    
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB5 = 1;
    PHY_WAKE        = 1;
    PHY_WAKE_TRIS = 0;
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
void SendCountedPacket(BYTE seqNumber, BYTE len)
{
    		
	TxBuffer[TxData++] = 0x02 + len;   /*  Packet Length */
   	TxBuffer[TxData++] = 0;             /* MSB of Sequence Counter */
   	TxBuffer[TxData++] = seqNumber;     /* LSB Sequence Coounter */
   			
   	/* transmit a 10-byte octet sequence */		
    for(i = 0; i < len; i++)
    {
        TxBuffer[TxData++] = i;
    }

    params.APSDE_DATA_request.DstAddrMode = APS_ADDRESS_16_BIT;
       		
    params.APSDE_DATA_request.DstAddress.ShortAddr.v[1] = byteMSB;
    params.APSDE_DATA_request.DstAddress.ShortAddr.v[0] = byteLSB;
    params.APSDE_DATA_request.SrcEndpoint    =  1;
    params.APSDE_DATA_request.DstEndpoint    =  240;
    params.APSDE_DATA_request.ProfileId.Val  = MY_PROFILE_ID;
    	    
    //params.APSDE_DATA_request.asduLength; TxData
	params.APSDE_DATA_request.RadiusCounter = DEFAULT_RADIUS;
	params.APSDE_DATA_request.DiscoverRoute = TRUE;
	params.APSDE_DATA_request.DiscoverRoute = ROUTE_DISCOVERY_ENABLE;
	   	    
	#ifdef I_SUPPORT_SECURITY
	    params.APSDE_DATA_request.TxOptions.Val = 1;
    #else
        params.APSDE_DATA_request.TxOptions.Val = 0;
    #endif
            
    params.APSDE_DATA_request.TxOptions.bits.acknowledged = 1;			        
    params.APSDE_DATA_request.ClusterId.Val = TRANSMIT_COUNTED_PACKETS_CLUSTER;	            
    				
    ZigBeeBlockTx();
    currentPrimitive = APSDE_DATA_request;	        
    
}
