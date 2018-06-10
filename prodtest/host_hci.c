/**
 ****************************************************************************************
 *
 * @file host_hci.c
 *
 * @brief Connection Manager HCI library.
 *
 * Copyright (C) 2013. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <windows.h>
#include <conio.h>
#include <stdint.h>         

#include "host_hci.h"
#include "uart.h"
#include "queue.h"




/* HCI TEST MODE */

void send_hci_command(hci_cmd_t *cmd)
{
#ifdef DEVELOPMENT_MESSAGES
	int kk;

	// log command in stderr
	fprintf(stderr, "==== Tx ====> \n");
	fprintf(stderr, "opcode   : 0x%04x\n", cmd->opcode);
	fprintf(stderr, "length   : 0x%02x\n", cmd->length);
	fprintf(stderr, "Payload  : ");
	for (kk = 0; kk < (cmd->length + 3/*sizeof(hci_cmd_header_t)*/); kk++)
	{
		fprintf(stderr, "%02x ", *((unsigned char *)cmd+ kk));
	}
	fprintf(stderr, "\n");
#endif //DEVELOPMENT_MESSAGES

	UARTSend(0x01, cmd->length + 3/*sizeof(hci_cmd_header_t)*/, (unsigned char *) cmd);

	free(cmd);
}

void *alloc_hci_command(unsigned short opcode, unsigned char length)
{
    hci_cmd_t *cmd = (hci_cmd_t *) malloc(sizeof(hci_cmd_t) + length - sizeof (unsigned char));

    cmd->opcode = opcode;
	cmd->length = length;

	if (length)
		memset(cmd->parameters, 0, length);

    return cmd;
}

hci_evt_t *hci_recv_event_wait(unsigned int millis)
{	
	QueueElement *qe;
	DWORD dw;
	hci_evt_t *evt;
	
	dw = WaitForSingleObject(QueueHasAvailableData, millis); // wait until elements are available
	if (dw != WAIT_OBJECT_0)	
	{
		return 0;
	}

	WaitForSingleObject(UARTRxQueueSem, INFINITE);
	qe = (QueueElement *) DeQueue(&UARTRxQueue); 
	ReleaseMutex(UARTRxQueueSem);

	evt = (hci_evt_t *) qe->payload;

	free(qe);

	return evt;
};



void handle_hci_event( hci_evt_t * evt)
{
#ifdef DEVELOPMENT_MESSAGES
	unsigned int kk;
	// log event in stderr
	//evt->length = 28;
	fprintf(stderr, "<==== Rx ===== \n");	
	fprintf(stderr, "event     : 0x%02X \n", evt->event);
	fprintf(stderr, "length    : 0x%02X \n", evt->length);
	fprintf(stderr, "parameters: ");
	for (kk = 0; kk < evt->length; kk++)
	{
		fprintf(stderr, "0x%02X ", evt->parameters[kk]);
		//fprintf(stderr, "%c", evt->parameters[kk]);
	}
	fprintf(stderr, "\n");

	// log payload
	fprintf(stderr, "Payload   : ");
	for (kk = 0; kk < (evt->length + sizeof(hci_evt_header_t)); kk++)
	{
		fprintf(stderr, "%02X ",*((unsigned char *)evt + kk));
	}
	fprintf(stderr, "\n");

	// log snlog
	fprintf(stderr, "LOG      : ");
	for (kk = 4; kk < (evt->length + sizeof(hci_evt_header_t)); kk++)
	{
		fprintf(stderr, "%c",evt->parameters[kk]);
	}
	fprintf(stderr, "\n");

#endif // DEVELOPMENT_MESSAGES
	
}


/*
 * HCI TEST MODE
 */

bool __stdcall hci_reset()
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x0C03, 0);

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_tx_test(uint8_t frequency, uint8_t length, uint8_t payload)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x201E, 3);

    cmd->parameters[0] = frequency;
    cmd->parameters[1] = length;
    cmd->parameters[2] = payload;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_rx_test(uint8_t frequency)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x201D, 1);

    cmd->parameters[0] = frequency;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_test_end()
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x201F, 0);
    
    send_hci_command((void *)cmd);
    
    return(true);
}

bool __stdcall hci_dialog_tx_test(uint8_t frequency, uint8_t length, uint8_t payload, uint16_t number_of_packets)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x201E, 5); // same with default tx test but with 5 parameters

    cmd->parameters[0] = frequency;
    cmd->parameters[1] = length;
    cmd->parameters[2] = payload;
    cmd->parameters[3] = number_of_packets & 0xFF;
    cmd->parameters[4] = number_of_packets >> 8;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_rx_readback_test(uint8_t frequency)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4020, 1);

    cmd->parameters[0] = frequency;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_rx_readback_test_end()
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4030, 0);

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_unmodulated_rx_tx(uint8_t mode, uint8_t frequency)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4010, 2); 

    cmd->parameters[0] = mode;
    cmd->parameters[1] = frequency;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_tx_continuous_start(uint8_t frequency, uint8_t payload_type)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4050, 2); 

    cmd->parameters[0] = frequency;
    cmd->parameters[1] = payload_type;

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_tx_continuous_end()
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4060, 0); 

    send_hci_command((void *)cmd);

    return(true);
}

bool __stdcall hci_dialog_sleep(uint8_t sleep_type, uint8_t minutes, uint8_t seconds)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4070, 3); 

    cmd->parameters[0] = sleep_type;
    cmd->parameters[1] = minutes;
    cmd->parameters[2] = seconds;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_xtal_trimming(uint8_t operation, uint16_t trim_value_or_delta)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4080, 3); 

    cmd->parameters[0] = operation;
    cmd->parameters[1] = (trim_value_or_delta ) & 0xFF;
    cmd->parameters[2] = (trim_value_or_delta >> 8 ) & 0xFF;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_rd_xtrim(void)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_RD_XTRIM;
    cmd->parameters[1] = 0;
    cmd->parameters[2] = 0;
    cmd->parameters[3] = 0;
    cmd->parameters[4] = 0;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_wr_xtrim (uint16_t trim_value)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_WR_XTRIM ;
    cmd->parameters[1] = trim_value & 0xFF;
    cmd->parameters[2] = (trim_value >> 8 ) & 0xFF;
    cmd->parameters[3] = 0;
    cmd->parameters[4] = 0;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_re_xtrim (void)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_RE_XTRIM ;
    cmd->parameters[1] = 0;
    cmd->parameters[2] = 0;
    cmd->parameters[3] = 0;
    cmd->parameters[4] = 0;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;

    send_hci_command(cmd);

    return(true);
}
bool __stdcall hci_dialog_otp_we_xtrim (void)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_WE_XTRIM ;
    cmd->parameters[1] = 0x10;
    cmd->parameters[2] = 0;
    cmd->parameters[3] = 0;
    cmd->parameters[4] = 0;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_rd_bdaddr(void)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_RD_BDADDR;
    cmd->parameters[1] = 0;
    cmd->parameters[2] = 0;
    cmd->parameters[3] = 0;
    cmd->parameters[4] = 0;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_wr_bdaddr(uint8_t bdaddr[6])
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x4090, 7);

    cmd->parameters[0] = CMD__OTP_OP_WR_BDADDR;
    cmd->parameters[1] = bdaddr[0]; // LSB first
    cmd->parameters[2] = bdaddr[1];
    cmd->parameters[3] = bdaddr[2];
    cmd->parameters[4] = bdaddr[3];
    cmd->parameters[5] = bdaddr[4];
    cmd->parameters[6] = bdaddr[5];

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_read(uint16_t otp_address, uint8_t word_count)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x40A0, 3);

    cmd->parameters[0] = otp_address & 0xFF;
    cmd->parameters[1] = otp_address >> 8;
    cmd->parameters[2] = word_count;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_otp_write(uint16_t otp_address, uint32_t *words, uint8_t word_count)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (0x40B0, 3 + 4 * word_count);
    int pos = 0, i = 0;

    cmd->parameters[0] = otp_address & 0xFF;
    cmd->parameters[1] = otp_address >> 8;
    cmd->parameters[2] = word_count;

    for (pos = 3, i = 0; i < word_count; ++i, pos += 4)
    {
        cmd->parameters[pos + 0] = (words[i]      ) & 0xFF;
        cmd->parameters[pos + 1] = (words[i] >>  8) & 0xFF;
        cmd->parameters[pos + 2] = (words[i] >> 16) & 0xFF;
        cmd->parameters[pos + 3] = (words[i] >> 24) & 0xFF;
    }

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_read_reg32(uint32_t reg_addr)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_REGISTER_RW_CMD_OPCODE, 9);

    cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_REG32;
    cmd->parameters[1] = reg_addr       & 0xFF; // LSB first
    cmd->parameters[2] = reg_addr >> 8  & 0xFF;
    cmd->parameters[3] = reg_addr >> 16 & 0xFF;
    cmd->parameters[4] = reg_addr >> 24 & 0xFF;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;
    cmd->parameters[7] = 0;
    cmd->parameters[8] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_write_reg32(uint32_t reg_addr, uint32_t value)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_REGISTER_RW_CMD_OPCODE, 9);

    cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_REG32;
    cmd->parameters[1] = reg_addr       & 0xFF; // LSB first
    cmd->parameters[2] = reg_addr >> 8  & 0xFF;
    cmd->parameters[3] = reg_addr >> 16 & 0xFF;
    cmd->parameters[4] = reg_addr >> 24 & 0xFF;
    cmd->parameters[5] = value       & 0xFF; // LSB first
    cmd->parameters[6] = value >> 8  & 0xFF;
    cmd->parameters[7] = value >> 16 & 0xFF;
    cmd->parameters[8] = value >> 24 & 0xFF;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_read_reg16(uint32_t reg_addr)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_REGISTER_RW_CMD_OPCODE, 9);

    cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_REG16;
    cmd->parameters[1] = reg_addr       & 0xFF; // LSB first
    cmd->parameters[2] = reg_addr >> 8  & 0xFF;
    cmd->parameters[3] = reg_addr >> 16 & 0xFF;
    cmd->parameters[4] = reg_addr >> 24 & 0xFF;
    cmd->parameters[5] = 0;
    cmd->parameters[6] = 0;
    cmd->parameters[7] = 0;
    cmd->parameters[8] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_write_reg16(uint32_t reg_addr, uint16_t value)
{
    hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_REGISTER_RW_CMD_OPCODE, 9);

    cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_REG16;
    cmd->parameters[1] = reg_addr       & 0xFF; // LSB first
    cmd->parameters[2] = reg_addr >> 8  & 0xFF;
    cmd->parameters[3] = reg_addr >> 16 & 0xFF;
    cmd->parameters[4] = reg_addr >> 24 & 0xFF;
    cmd->parameters[5] = value       & 0xFF; // LSB first
    cmd->parameters[6] = value >> 8  & 0xFF;
    cmd->parameters[7] = 0;
    cmd->parameters[8] = 0;

    send_hci_command(cmd);

    return(true);
}

bool __stdcall hci_dialog_write_SN(uint32_t reg_addr, char* value)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 25);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_SN;
	
	cmd->parameters[1] = value[0] ; 
	cmd->parameters[2] = value[1] ;
	cmd->parameters[3] = value[2] ;
	cmd->parameters[4] = value[3] ;
	cmd->parameters[5] = value[4] ;   
	cmd->parameters[6] = value[5] ;
	cmd->parameters[7] = value[6] ;
	cmd->parameters[8] = value[7] ;
	cmd->parameters[9] = value[8] ; 
	cmd->parameters[10] = value[9] ;
	cmd->parameters[11] = value[10] ;
	cmd->parameters[12] = value[11] ;
	cmd->parameters[13] = value[12] ;
	cmd->parameters[14] = value[13] ;
	cmd->parameters[15] = value[14] ;
	cmd->parameters[16] = 0 ;
	cmd->parameters[17] = 0 ;
	cmd->parameters[18] = 0 ;
	cmd->parameters[19] = 0 ;
	cmd->parameters[20] = 0 ;
	cmd->parameters[21] = 0 ;
	cmd->parameters[22] = 0 ;
	cmd->parameters[23] = 0 ;
	cmd->parameters[24] = 0 ;
	cmd->parameters[25] = 0 ;
	cmd->parameters[26] = 0 ;
	cmd->parameters[27] = 0 ;
	cmd->parameters[28] = 0 ;
	cmd->parameters[29] = 0 ;
	cmd->parameters[30] = 0 ;

	send_hci_command(cmd);

	return(true);
}


/*
bool __stdcall hci_dialog_write_SN(uint32_t reg_addr, uint16_t value)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 17);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_SN;

	
	cmd->parameters[1] = value		 & 0xFF; // LSB first 
	cmd->parameters[2] = value	>> 8 	& 0xFF;;
	cmd->parameters[3] = value	>> 16 	 & 0xFF;;
	cmd->parameters[4] = value	>> 24 	& 0xFF;;
	cmd->parameters[5] = value	>> 32 	& 0xFF;;   
	cmd->parameters[6] = value	>> 40 	 & 0xFF;
	
	cmd->parameters[7] = value	>> 48 	 & 0xFF;
	cmd->parameters[8] = value	>> 56 	& 0xFF;

	cmd->parameters[9] = value >> 64 	& 0xFF; 
	cmd->parameters[10] = value >> 72 	& 0xFF;
	cmd->parameters[11] = value >> 80 	& 0xFF;
	cmd->parameters[12] = value >> 88 	& 0xFF;
	cmd->parameters[13] = value >> 96 	& 0xFF;
	cmd->parameters[14] = value >> 104 	& 0xFF;
	cmd->parameters[15] = value >> 112 	& 0xFF;
	cmd->parameters[16] = value >> 120 	& 0xFF;

	send_hci_command(cmd);

	return(true);
}
*/

bool __stdcall hci_dialog_read_SN(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_SN;
	/*
	cmd->parameters[1] = reg_addr       & 0xFF; // LSB first
	cmd->parameters[2] = reg_addr >> 8  & 0xFF;
	cmd->parameters[3] = reg_addr >> 16 & 0xFF;
	cmd->parameters[4] = reg_addr >> 24 & 0xFF;
	cmd->parameters[5] = 0;
	cmd->parameters[6] = 0;
	cmd->parameters[7] = 0;
	cmd->parameters[8] = 0;
	*/
	send_hci_command(cmd);

	return(true);
}




bool __stdcall hci_dialog_write_swversion(uint32_t reg_addr, char* value)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_SWVERSION;
	
	cmd->parameters[1] = value[0] ; 
	cmd->parameters[2] = value[1] ;
	cmd->parameters[3] = value[2] ;
	cmd->parameters[4] = value[3] ;
	cmd->parameters[5] = value[4] ;   
	cmd->parameters[6] = value[5] ;
	cmd->parameters[7] = value[6] ;
	cmd->parameters[8] = value[7] ;
	
	cmd->parameters[9] = value[8] ; 
	cmd->parameters[10] = value[9] ;
	cmd->parameters[11] = value[10] ;
	cmd->parameters[12] = value[11] ;
	cmd->parameters[13] = value[12] ;
	cmd->parameters[14] = value[13] ;
	cmd->parameters[15] = value[14] ;
	//cmd->parameters[16] = value[15] ;

	cmd->parameters[16] = 0 ;
	cmd->parameters[17] = 0 ;
	cmd->parameters[18] = 0 ;
	cmd->parameters[19] = 0 ;
	cmd->parameters[20] = 0 ;
	cmd->parameters[21] = 0 ;
	cmd->parameters[22] = 0 ;
	cmd->parameters[23] = 0 ;
	cmd->parameters[24] = 0 ;
	cmd->parameters[25] = 0 ;
	cmd->parameters[26] = 0 ;
	cmd->parameters[27] = 0 ;
	cmd->parameters[28] = 0 ;
	cmd->parameters[29] = 0 ;
	cmd->parameters[30] = 0 ;
	
	send_hci_command(cmd);

	return(true);
}


bool __stdcall hci_dialog_read_swversion(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_SWVERSION;
	
	send_hci_command(cmd);

	return(true);
}




bool __stdcall hci_dialog_write_flag(uint32_t reg_addr, char* value)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_FLAG;

	cmd->parameters[1] = value[0] ; 
	cmd->parameters[2] = value[1] ;
	cmd->parameters[3] = value[2] ;
	cmd->parameters[4] = value[3] ;
	cmd->parameters[5] = value[4] ;   
	cmd->parameters[6] = value[5] ;
	cmd->parameters[7] = value[6] ;
	cmd->parameters[8] = value[7] ;

	cmd->parameters[9] = value[8] ; 
	cmd->parameters[10] = value[9] ;
	cmd->parameters[11] = value[10] ;
	cmd->parameters[12] = value[11] ;
	cmd->parameters[13] = value[12] ;
	cmd->parameters[14] = value[13] ;
	cmd->parameters[15] = value[14] ;
	cmd->parameters[16] = 0 ;
	cmd->parameters[17] = 0 ;
	cmd->parameters[18] = 0 ;
	cmd->parameters[19] = 0 ;
	cmd->parameters[20] = 0 ;
	cmd->parameters[21] = 0 ;
	cmd->parameters[22] = 0 ;
	cmd->parameters[23] = 0 ;
	cmd->parameters[24] = 0 ;
	cmd->parameters[25] = 0 ;
	cmd->parameters[26] = 0 ;
	cmd->parameters[27] = 0 ;
	cmd->parameters[28] = 0 ;
	cmd->parameters[29] = 0 ;
	cmd->parameters[30] = 0 ;

	send_hci_command(cmd);

	return(true);
}


bool __stdcall hci_dialog_read_flag(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_FLAG;

	send_hci_command(cmd);

	return(true);
}



bool __stdcall hci_dialog_write_PSN(uint32_t reg_addr, char* value)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_PSN;

	cmd->parameters[1] = value[0] ; 
	cmd->parameters[2] = value[1] ;
	cmd->parameters[3] = value[2] ;
	cmd->parameters[4] = value[3] ;
	cmd->parameters[5] = value[4] ;   
	cmd->parameters[6] = value[5] ;
	cmd->parameters[7] = value[6] ;
	cmd->parameters[8] = value[7] ;
	
	cmd->parameters[9] = value[8] ; 
	cmd->parameters[10] = value[9] ;
	cmd->parameters[11] = value[10] ;
	cmd->parameters[12] = value[11] ;
	cmd->parameters[13] = value[12] ;
	cmd->parameters[14] = value[13] ;
	cmd->parameters[15] = value[14] ;
	cmd->parameters[16] = 0 ;
	cmd->parameters[17] = 0 ;
	cmd->parameters[18] = 0 ;
	cmd->parameters[19] = 0 ;
	cmd->parameters[20] = 0 ;
	cmd->parameters[21] = 0 ;
	cmd->parameters[22] = 0 ;
	cmd->parameters[23] = 0 ;
	cmd->parameters[24] = 0 ;
	cmd->parameters[25] = 0 ;
	cmd->parameters[26] = 0 ;
	cmd->parameters[27] = 0 ;
	cmd->parameters[28] = 0 ;
	cmd->parameters[29] = 0 ;
	cmd->parameters[30] = 0 ;

	send_hci_command(cmd);

	return(true);
}


bool __stdcall hci_dialog_read_PSN(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_PSN;

	send_hci_command(cmd);

	return(true);
}
/*doco lixiping fix for ticket/1 20180607 begin*/
bool __stdcall hci_dialog_read_MAC(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_MAC;

	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_go_sleep(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_GO_SLEEP;

	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_read_vbat(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 1);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_READ_VBAT;

	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_write_fpsenser_zero(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_FPSENSER_ZERO;
	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_write_bpsenser_zero(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_BPSENSER_ZERO;
	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_write_fpsenser_work(uint32_t reg_addr)
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_FPSENSER_WORK;


	send_hci_command(cmd);

	return(true);
}
bool __stdcall hci_dialog_write_bpsenser_work(uint32_t reg_addr )
{
	hci_cmd_t *cmd = (hci_cmd_t *) alloc_hci_command (HCI_CUSTOM_ACTION_CMD_OPCODE, 30);

	cmd->parameters[0] = CMD__REGISTER_RW_OP_WRITE_BPSENSER_WORK;

	send_hci_command(cmd);

	return(true);
}
/*doco lixiping fix for ticket/1 20180607 end*/
