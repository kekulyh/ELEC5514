/*********************************************************************

    Microchip ZigBee Stack

    ZigBee Router Template

 *********************************************************************
 * FileName:        Router Template.c
 * Dependencies:
 * Processor:       PIC18F
 * Complier:        MCC18 v3.00 or higher
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
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DF/KO                01/09/06 Microchip ZigBee Stack v1.0-3.5
 * DF/KO                08/31/06 Microchip ZigBee Stack v1.0-3.6
 * DF/KO/YY				11/27/06 Microchip ZigBee Stack v1.0-3.7
 * DF/KO/YY				01/12/07 Microchip ZigBee Stack v1.0-3.8
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

#else
    #error Other compilers are not yet supported.
#endif

//******************************************************************************
// Function Prototypes
//******************************************************************************

void HardwareInit( void );

//******************************************************************************
// Application Variables
//******************************************************************************

NETWORK_DESCRIPTOR  *currentNetworkDescriptor;
ZIGBEE_PRIMITIVE    currentPrimitive;
NETWORK_DESCRIPTOR  *NetworkDescriptor;
BYTE                orphanTries;

//******************************************************************************
//******************************************************************************
//******************************************************************************

void main(void)
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


    // Enable interrupts to get everything going.
    IPEN = 1;
    GIEH = 1;

    while (1)
    {
        CLRWDT();
        ZigBeeTasks( &currentPrimitive );

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
                        // not needed for new join params.NLME_JOIN_request.ScanDuration = ;
                        // not needed for new join params.NLME_JOIN_request.ScanChannels = ;
                        params.NLME_JOIN_request.PANId          = currentNetworkDescriptor->PanID;
                        params.NLME_JOIN_request.JoinAsRouter   = TRUE;
                        params.NLME_JOIN_request.RejoinNetwork  = FALSE;
                        params.NLME_JOIN_request.PowerSource    = MAINS_POWERED;
                        params.NLME_JOIN_request.RxOnWhenIdle   = TRUE;
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
                        free( NetworkDescriptor );
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
                            // We ran out of descriptors.  Free the network descriptor list, and fall
                            // through to try discovery again.
                            ConsolePutROMString( (ROM char *)"Cleaning up and retrying discovery...\r\n" );
                            while (NetworkDescriptor)
                            {
                                currentNetworkDescriptor = NetworkDescriptor->next;
                                free( NetworkDescriptor );
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
                    ConsolePutROMString( (ROM char *)"Joining permitted.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                else
                {
                    PrintChar( params.NLME_PERMIT_JOINING_confirm.Status );
                    ConsolePutROMString( (ROM char *)" Join permission unsuccessful. We cannot allow joins.\r\n" );
                    currentPrimitive = NO_PRIMITIVE;
                }
                break;

            case NLME_JOIN_indication:
                ConsolePutROMString( (ROM char *)"Node " );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.MSB );
                PrintChar( params.NLME_JOIN_indication.ShortAddress.byte.LSB );
                ConsolePutROMString( (ROM char *)" just joined.\r\n" );
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
								//ConsolePutROMString((ROM char *)"secure join\r\n");
								for(i = 0; i < 16; i++)
								{
									KeyVal.v[i] = 0;
								}
								params.APSME_TRANSPORT_KEY_request.Key = &KeyVal;
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = 0;
				
							} else {
								BYTE i;
								GetNwkActiveKeyNumber(&i);
								GetNwkKeyInfo(&currentNetworkKeyInfo, (ROM void *)&(NetworkKeyInfo[i-1]));
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
							}
							#else
							{
								BYTE i;
								GetNwkActiveKeyNumber(&i);
								GetNwkKeyInfo(&currentNetworkKeyInfo, (ROM void *)&(NetworkKeyInfo[i-1]));
								params.APSME_TRANSPORT_KEY_request.Key = &(currentNetworkKeyInfo.NetKey);
								params.APSME_TRANSPORT_KEY_request.TransportKeyData.NetworkKey.KeySeqNumber = currentNetworkKeyInfo.SeqNumber.v[0];		
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
//		  				PrintChar(params.APSME_UPDATE_DEVICE_request.Status);
						params.APSME_UPDATE_DEVICE_request.DeviceShortAddress = params.NLME_JOIN_indication.ShortAddress;
						params.APSME_UPDATE_DEVICE_request.DeviceAddress = params.NLME_JOIN_indication.ExtendedAddress;
						GetTrustCenterAddress(&params.APSME_UPDATE_DEVICE_request.DestAddress);
						currentPrimitive = APSME_UPDATE_DEVICE_request;
					#endif
				#else
                	currentPrimitive = NO_PRIMITIVE;
				#endif			
                break;

            case NLME_LEAVE_indication:
                if (!memcmppgm2ram( &params.NLME_LEAVE_indication.DeviceAddress, (ROM void *)&macLongAddr, 8 ))
                {
                    ConsolePutROMString( (ROM char *)"We have left the network.\r\n" );
                }
                else
                {
                    ConsolePutROMString( (ROM char *)"Another node has left the network.\r\n" );
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
						// omit to do this since the address overlap
						//params.APSME_UPDATE_DEVICE_request.DeviceAddress = params.NLME_LEAVE_indication.DeviceAddress;
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
                currentPrimitive = NO_PRIMITIVE;
                break;

            case APSDE_DATA_indication:
                {
                    WORD_VAL    attributeId;
                    BYTE        command;
                    BYTE        data;
                    BYTE        dataLength;
                    //BYTE        dataType;
                    BYTE        frameHeader;
                    BYTE        sequenceNumber;
                    BYTE        transaction;
                    BYTE        transByte;

                    currentPrimitive = NO_PRIMITIVE;
                    frameHeader = APLGet();

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
                                    transByte               = 0;

                                    switch( params.APSDE_DATA_indication.ClusterId )
                                    {

                                        // ********************************************************
                                        // Put a case here to handle each ZDO response that we requested.
                                        // Be sure to increment transByte for each APLGet().
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

                        default:
                            // If the command type was something that requested an acknowledge, we could send back
                            // KVP_INVALID_ENDPOINT here.
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

            case NO_PRIMITIVE:
                if (!ZigBeeStatus.flags.bits.bNetworkJoined)
                {
                    if (!ZigBeeStatus.flags.bits.bTryingToJoinNetwork)
                    {
                        if (ZigBeeStatus.flags.bits.bTryOrphanJoin)
                        {
                            ConsolePutROMString( (ROM char *)"Trying to join network as an orphan...\r\n" );
                            params.NLME_JOIN_request.ScanDuration     = 8;
                            params.NLME_JOIN_request.ScanChannels.Val = ALLOWED_CHANNELS;
                            // not needed for orphan join - params.NLME_JOIN_request.PANId
                            params.NLME_JOIN_request.JoinAsRouter     = TRUE;
                            params.NLME_JOIN_request.RejoinNetwork    = TRUE;
                            params.NLME_JOIN_request.PowerSource      = MAINS_POWERED;
                            params.NLME_JOIN_request.RxOnWhenIdle     = TRUE;
                            params.NLME_JOIN_request.MACSecurity      = FALSE;
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
                    if (ZigBeeReady())
                    {

                        // ************************************************************************
                        // Place all processes that can send messages here.  Be sure to call
                        // ZigBeeBlockTx() when currentPrimitive is set to APSDE_DATA_request.
                        // ************************************************************************

                    }
                }
                break;

            default:
                PrintChar( currentPrimitive );
                ConsolePutROMString( (ROM char *)" Unhandled primitive.\r\n" );
                currentPrimitive = NO_PRIMITIVE;
                break;
        }

        // *********************************************************************
        // Place any non-ZigBee related processing here.  Be sure that the code
        // will loop back and execute ZigBeeTasks() in a timely manner.
        // *********************************************************************

    }
}

/*******************************************************************************
HardwareInit

All port directioning and SPI must be initialized before calling ZigBeeInit().

For demonstration purposes, required signals are configured individually.
*******************************************************************************/
void HardwareInit(void)
{

    //-------------------------------------------------------------------------
    // This section is required to initialize the PICDEM Z for the CC2420
    // and the ZigBee Stack.
    //-------------------------------------------------------------------------

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
    #elif (RF_CHIP==UZ2400)
        // Start with UZ2400 disabled and not selected
        PHY_SEN             = 1;
        PHY_RESETn          = 1;

        // Set the directioning for the UZ2400 pin connections.
        PHY_SEN_TRIS        = 0;
        PHY_RESETn_TRIS     = 0;

        // Initialize the interrupt.
        INTCON2bits.INTEDG0 = 0;
    #elif (RF_CHIP==CC2420)
        // CC2420 I/O assignments with respect to PIC:
        //NOTE: User must make sure that pin is capable of correct digital operation.
        //      This may require modificaiton of which pins are digital and analog.
        //NOTE: The stack requires that the SPI interface be located on LATC3 (SCK),
        //      RC4 (SO), and LATC5 (SI).
        //NOTE: The appropriate config bit must be set such that FIFOP is the CCP2
        //      input pin. The stack uses the CCP2 interrupt.

        // Start with CC2420 disabled and not selected
        PHY_CSn             = 1;
        PHY_VREG_EN         = 0;
        PHY_RESETn          = 1;

        // Set the directioning for the CC2420 pin connections.
        PHY_FIFO_TRIS       = 1;    // FIFO      (Input)
        PHY_SFD_TRIS        = 1;    // SFD       (Input - Generates interrupt on falling edge)
        PHY_FIFOP_TRIS      = 1;    // FIFOP     (Input - Used to detect overflow, CCP2 interrupt)
        PHY_CSn_TRIS        = 0;    // CSn       (Output - to select CC2420 SPI slave)
        PHY_VREG_EN_TRIS    = 0;    // VREG_EN   (Output - to enable CC2420 voltage regulator)
        PHY_RESETn_TRIS     = 0;    // RESETn    (Output - to reset CC2420)
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
    #endif
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB5 = 1;
}

/*******************************************************************************
User Interrupt Handler

The stack uses some interrupts for its internal processing.  Once it is done
checking for its interrupts, the stack calls this function to allow for any
additional interrupt processing.
*******************************************************************************/

void UserInterruptHandler(void)
{

    // *************************************************************************
    // Place any application-specific interrupt processing here
    // *************************************************************************

}
