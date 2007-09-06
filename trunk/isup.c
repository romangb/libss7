/*
libss7: An implementation of Signalling System 7

Written by Matthew Fredrickson <creslin@digium.com>

Copyright Digium, Inc. (C) 2006
All Rights Reserved.

This program is free software; you can redistribute it under the
terms of the GNU General Public License as published by the Free
Software Foundation

Contains definitions and data structurs for the ISUP portion of SS7
*/

#include <string.h>
#include <stdlib.h>
#include "libss7.h"
#include "isup.h"
#include "ss7_internal.h"
#include "mtp3.h"

#define FUNC_DUMP(name) int ((name))(struct ss7 *ss7, int messagetype, unsigned char *parm, int len)
/* Length here is paramter length */
#define FUNC_RECV(name) int ((name))(struct ss7 *ss7, struct isup_call *c, int messagetype, unsigned char *parm, int len)
/* Length here is maximum length */
#define FUNC_SEND(name) int ((name))(struct ss7 *ss7, struct isup_call *c, int messagetype, unsigned char *parm, int len)

#define PARM_TYPE_FIXED 0x01
#define PARM_TYPE_VARIABLE 0x02
#define PARM_TYPE_OPTIONAL 0x03

#define CODE_CCITT 0x0

#define LOC_PRIV_NET_LOCAL_USER 0x1

struct parm_func {
	int parm;
	char *name;
	FUNC_DUMP(*dump);
	FUNC_RECV(*receive);
	FUNC_SEND(*transmit);
};

static int iam_params[] = {ISUP_PARM_NATURE_OF_CONNECTION_IND, ISUP_PARM_FORWARD_CALL_IND, ISUP_PARM_CALLING_PARTY_CAT,
	ISUP_PARM_TRANSMISSION_MEDIUM_REQS, ISUP_PARM_CALLED_PARTY_NUM, ISUP_PARM_CALLING_PARTY_NUM, -1}; /* Don't have optional IEs */

static int ansi_iam_params[] = {ISUP_PARM_NATURE_OF_CONNECTION_IND, ISUP_PARM_FORWARD_CALL_IND, ISUP_PARM_CALLING_PARTY_CAT,
	ISUP_PARM_USER_SERVICE_INFO, ISUP_PARM_CALLED_PARTY_NUM, ISUP_PARM_CALLING_PARTY_NUM, ISUP_PARM_CHARGE_NUMBER, ISUP_PARM_ORIG_LINE_INFO, -1}; /* Include Charge number Don't have optional IEs */


static int acm_params[] = {ISUP_PARM_BACKWARD_CALL_IND, -1};

static int anm_params[] = { -1};

static int con_params[] = { ISUP_PARM_BACKWARD_CALL_IND, -1};

static int rel_params[] = { ISUP_PARM_CAUSE, -1};

static int greset_params[] = { ISUP_PARM_RANGE_AND_STATUS, -1};

static int cot_params[] = { ISUP_PARM_CONTINUITY_IND, -1};

static int cpg_params[] = { ISUP_PARM_EVENT_INFO, -1};

static int cicgroup_params[] = { ISUP_PARM_CIRCUIT_GROUP_SUPERVISION_IND, ISUP_PARM_RANGE_AND_STATUS, -1};

static int empty_params[] = { -1};

static struct message_data {
	int messagetype;
	int mand_fixed_params;
	int mand_var_params;
	int opt_params;
	int *param_list;
} messages[] = {
	{ISUP_IAM, 4, 1, 1, iam_params},
	{ISUP_ACM, 1, 0, 1, acm_params},
	{ISUP_ANM, 0, 0, 1, anm_params},
	{ISUP_CON, 1, 0, 1, con_params},
	{ISUP_REL, 0, 1, 1, rel_params},
	{ISUP_RLC, 0, 0, 1, empty_params},
	{ISUP_GRS, 0, 1, 0, greset_params},
	{ISUP_GRA, 0, 1, 0, greset_params},
	{ISUP_CGB, 1, 1, 0, cicgroup_params},
	{ISUP_CGU, 1, 1, 0, cicgroup_params},
	{ISUP_CGBA, 1, 1, 0, cicgroup_params},
	{ISUP_CGUA, 1, 1, 0, cicgroup_params},
	{ISUP_COT, 1, 0, 0, cot_params},
	{ISUP_CCR, 0, 0, 0, empty_params},
	{ISUP_BLO, 0, 0, 0, empty_params},
	{ISUP_UBL, 0, 0, 0, empty_params},
	{ISUP_BLA, 0, 0, 0, empty_params},
	{ISUP_UBA, 0, 0, 0, empty_params},
	{ISUP_RSC, 0, 0, 0, empty_params},
	{ISUP_CPG, 1, 0, 1, cpg_params},
	{ISUP_UCIC, 0, 0, 0, empty_params},
};

static int isup_send_message(struct ss7 *ss7, struct isup_call *c, int messagetype, int parms[]);

static char * message2str(unsigned char message)
{
	switch (message) {
		case ISUP_IAM:
			return "IAM";
		case ISUP_ACM:
			return "ACM";
		case ISUP_ANM:
			return "ANM";
		case ISUP_REL:
			return "REL";
		case ISUP_RLC:
			return "RLC";
		case ISUP_GRS:
			return "GRS";
		case ISUP_GRA:
			return "GRA";
		case ISUP_COT:
			return "COT";
		case ISUP_CCR:
			return "CCR";
		case ISUP_BLO:
			return "BLO";
		case ISUP_UBL:
			return "UBL";
		case ISUP_BLA:
			return "BLA";
		case ISUP_UBA:
			return "UBA";
		case ISUP_CGB:
			return "CGB";
		case ISUP_CGU:
			return "CGU";
		case ISUP_CGBA:
			return "CGBA";
		case ISUP_CGUA:
			return "CGUA";
		case ISUP_RSC:
			return "RSC";
		case ISUP_CPG:
			return "CPG";
		case ISUP_UCIC:
			return "UCIC";
		default:
			return "Unknown";
	}
}

static char char2digit(char localchar)
{
	switch (localchar) {
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		case '#':
			return 0xf;
		default:
			return 0;
	}
}

static char digit2char(unsigned char digit)
{
	switch (digit & 0xf) {
		case 0:
			return '0';
		case 1:
			return '1';
		case 2:
			return '2';
		case 3:
			return '3';
		case 4:
			return '4';
		case 5:
			return '5';
		case 6:
			return '6';
		case 7:
			return '7';
		case 8:
			return '8';
		case 9:
			return '9';
		case 15:
			return '#';
		default:
			return 0;
	}
}

static void isup_dump_buffer(struct ss7 *ss7, unsigned char *data, int len)
{
	int i;

	ss7_message(ss7, "[");
	for (i = 0; i < len; i++) {
		ss7_message(ss7, "0x%x ", data[i]);
	}
	ss7_message(ss7, "]\n");
}

static void isup_get_number(char *dest, unsigned char *src, int srclen, int oddeven)
{
	int i;
	for (i = 0; i < ((srclen * 2) - oddeven); i++)
		dest[i] = digit2char(src[i/2] >> ((i % 2) ? 4 : 0));

	dest[i] = '\0'; 
}

static void isup_put_number(unsigned char *dest, char *src, int *len, int *oddeven)
{
	int i = 0;
	int numlen = strlen(src);

	if (numlen % 2) {
		*oddeven = 1;
		*len = numlen/2 + 1;
	} else {
		*oddeven = 0;
		*len = numlen/2;
	}

	while (i < numlen) {
		if (!(i % 2))
			dest[i/2] |= char2digit(src[i]) & 0xf;
		else
			dest[i/2] |= (char2digit(src[i]) << 4) & 0xf0;
		i++;
	}
}

static FUNC_SEND(nature_of_connection_ind_transmit)
{
	parm[0] = 0x00;

	if (c->cot_check_required)
		parm[0] |= 0x04;
	
	return 1; /* Length plus size of type header */
}

static FUNC_RECV(nature_of_connection_ind_receive)
{
	unsigned char cci = (parm[0] >> 2) & 0x3;

	if (cci == 0x1)
		c->cot_check_required = 1;
	else
		c->cot_check_required = 0;

	return 1;
}

static FUNC_DUMP(nature_of_connection_ind_dump)
{
	unsigned char con = parm[0];
	char *continuity;
	
	ss7_message(ss7, "\tSatellites in connection: %d\n", con&0x03);
	con>>=2; 
	switch (con & 0x03) {
		case 0:
			continuity = "Check not required";
			break;
		case 1:
			continuity = "Check required on this circuit";
			break;
		case 2:
			continuity = "Check required on previous circuit";
			break;
		case 3:
			continuity = "spare";
			break;
	}
	ss7_message(ss7, "\tContinuity Check: %s\n", continuity);
	con>>=2;
	con &= 0x01;

	ss7_message(ss7, "\tOutgoing half echo control device %s\n", con ? "included" : "not included");

	return 2;
}


static FUNC_SEND(forward_call_ind_transmit)
{
	parm[0] = 0x60;
	parm[1] = 0x01;
	return 2;
}

static FUNC_RECV(forward_call_ind_receive)
{
	return 2;
}

static FUNC_DUMP(forward_call_ind_dump)
{
	return 2;
}

static FUNC_RECV(calling_party_cat_receive)
{
	return 1;
}
	
static FUNC_SEND(calling_party_cat_transmit)
{
	parm[0] = 0x0a; /* Default to Ordinary calling subscriber */
	return 1;
}

static FUNC_DUMP(calling_party_cat_dump)
{
	return 1;
}

static FUNC_RECV(user_service_info_receive)
{
	/* NoOp it for now */
	return len;
}

static FUNC_SEND(user_service_info_transmit)
{
	/* Default to Coding standard CCITT / 3.1 khz Audio */
	parm[0] = 0x90;
	/* Default to Circuit mode / 64kbps */
	parm[1] = 0x90;
	/* User Layer 1 set to ulaw */
	parm[2] = 0xa2;

	return 3;
}

static FUNC_SEND(transmission_medium_reqs_transmit)
{
	if (ss7->switchtype != SS7_ITU)
		return 0;
	/* Speech */
	parm[0] = 0;
	return 1;
}

static FUNC_RECV(transmission_medium_reqs_receive)
{
	c->transcap = parm[0] & 0x7f;
	return 1;
}

static FUNC_DUMP(transmission_medium_reqs_dump)
{
	return 1;
}

/* For variable length parameters we pass in the length of the parameter */
static FUNC_RECV(called_party_num_receive)
{
	int odd = 0;

	if (parm[0] & 0x80)
		odd = 1;
	
	isup_get_number(c->called_party_num, &parm[2], len - 2, odd);

	c->called_nai = parm[0] & 0x7f; /* Nature of Address Indicator */

	return len;
}

static FUNC_SEND(called_party_num_transmit)
{
	int numlen, oddeven;

	isup_put_number(&parm[2], c->called_party_num, &numlen, &oddeven);

	parm[0] = c->called_nai & 0x7f; /* Nature of Address Indicator */
	
	if (oddeven)
		parm[0] |= 0x80; /* Odd number of digits */

	parm[1] = 0x1 << 4; /* Assume E.164 ISDN numbering plan, called number complete  */

	return numlen + 2;
}

static FUNC_RECV(backward_call_ind_receive)
{
	return 2;
}

static FUNC_SEND(backward_call_ind_transmit)
{
	parm[0] = 0x40;
	parm[1] = 0x14;
	return 2;
}

static FUNC_DUMP(backward_call_ind_dump)
{
	return 2;
}

static FUNC_RECV(opt_backward_call_ind_receive)
{
	return 1;
}

static FUNC_DUMP(opt_backward_call_ind_dump)
{
	return 1;
}

static FUNC_RECV(cause_receive)
{
	c->causeloc = parm[0] & 0xf;
	c->causecode = (parm[0] & 0x60) >> 5;
	c->cause = (parm[1] & 0x7f);

	return 2;
}

static FUNC_SEND(cause_transmit)
{
	parm[0] = 0x80 | (c->causecode << 5) | c->causeloc;
	parm[1] = 0x80 | c->cause;
	return 2;
}

static FUNC_DUMP(cause_dump)
{
	return 2;
}


static FUNC_DUMP(range_and_status_dump)
{
	ss7_message(ss7, "\tPARM: Range and Status\n");
	ss7_message(ss7, "\t\tRange: %d\n", parm[0] & 0xff);
	return len;
}

static FUNC_RECV(range_and_status_receive)
{
	int i;
	int numcics;

	c->range = parm[0];
	numcics = c->range + 1;

	for (i = 0; i < numcics; i++) {
		if (parm[1 + (i/8)] & (1 << (i%8)))
			c->status[i] = 1;
		else
			c->status[i] = 0;
	}

	return len;
}

static FUNC_SEND(range_and_status_transmit)
{
	int i, statuslen = 0;
	int numcics = c->range + 1;

	parm[0] = c->range & 0xff;

	if (messagetype == ISUP_GRS)
		return 1;
	
	statuslen = (numcics / 8) + !!(numcics % 8);

	if (messagetype == ISUP_GRA) {
		for (i = 0; i < statuslen; i++) {
			parm[1 + i] = 0;
		}
	} else {
		for (i = 0; i < numcics; i++) {
			if (c->status[i])
				parm[1 + (i/8)] |= (1 << (i % 8));
		}
	}

	return statuslen + 1;
}

static FUNC_DUMP(calling_party_num_dump)
{
	int oddeven = (parm[0] >> 7) & 0x1;
	char numbuf[64] = "";

	ss7_message(ss7, "PARM: Calling Party Number\n");
	ss7_message(ss7, "Odd/even: %x\n", (parm[0] >> 7) & 0x1);
	ss7_message(ss7, "Nature of address: %x\n", parm[0] & 0x7f);
	ss7_message(ss7, "NI: %x\n", (parm[1] >> 7) & 0x1);
	ss7_message(ss7, "Numbering plan: %x\n", (parm[1] >> 4) & 0x7);
	ss7_message(ss7, "Presentation: %x\n", (parm[1] >> 2) & 0x3);
	ss7_message(ss7, "Screening: %x\n", parm[1] & 0x3);

	isup_get_number(numbuf, &parm[2], len - 2, oddeven);

	ss7_message(ss7, "Address signals: %s\n", numbuf);

	return len;
}

static FUNC_RECV(calling_party_num_receive)
{
	int oddeven = (parm[0] >> 7) & 0x1;

	isup_get_number(c->calling_party_num, &parm[2], len - 2, oddeven);

	c->calling_nai = parm[0] & 0x7f;                /* Nature of Address Indicator */
	c->presentation_ind = (parm[1] >> 2) & 0x3;
	c->screening_ind = parm[1] & 0x3;

	return len;
	
}

static FUNC_SEND(calling_party_num_transmit)
{
	int oddeven, datalen;
    	
	if (!c->calling_party_num[0])
		return 0;

	isup_put_number(&parm[2], c->calling_party_num, &datalen, &oddeven);

	parm[0] = (oddeven << 7) | c->calling_nai;      /* Nature of Address Indicator */
	parm[1] = (1 << 4) |                            /* Assume E.164 ISDN numbering plan, calling number complete */
	    ((c->presentation_ind & 0x3) << 2) |
	    (c->screening_ind & 0x3);

	return datalen + 2;
}

static FUNC_DUMP(originating_line_information_dump)
{
	char *name;
	
	switch (parm[0]) {
	case 0:
		name = " Plain Old Telephone Service POTS";
		break;
	case 1:
		name = " Multiparty line";
		break;
	case 2:
		name = " ANI Failure";
		break;
	case 3:
	case 4:
	case 5:
		name = " Unassigned";
		break;
	case 6:
		name = " Station Level Rating";
		break;
	case 7:
		name = " Special Operator Handling Required";
		break;
	case 8:
	case 9:
		name = "Unassigned";
		break;
	case 10:
		name = "Not assignable";
		break;
	case 11:
		name = "Unassigned";
		break;
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
		name = "Not assignable";
		break;
	case 20:
		name = "Automatic Identified Outward Dialing";
		break;
	case 21:
	case 22:
		name = "Unassigned";
		break;
	case 23:
		name = "Coin or Non-Coin";
		break;
	case 24:
	case 25:
		name = "Toll Free Service translated to POTS";
		break;
	case 26:
		name = "Unassigned";
		break;
	case 27:
		name = "Pay Station using Coin Control Signalling";
		break;
	case 28:
		name = "Unassigned";
		break;
	case 29:
		name = "Prison/Inmate Service";
		break;
	case 30:
	case 31:
	case 32:
		name = "Intercept";
		break;
	case 33:
		name = "Unassigned";
		break;
	case 34:
		name = "Telco Operator Handled Call";
		break;
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
		name = "Unassigned";
		break;
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
		name = "Unrestricted Use - locally determined by carrier";
		break;
	case 50:
	case 51:
		name = "Unassigned";
		break;
	case 52:
		name = "OUTWATS";
		break;
	case 53:
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 59:
		name = "Unassigned";
		break;
	case 60:
		name = "TRS Unrestricted Line";
		break;
	case 61:
		name = "Cellular Wireless PCS Type 1";
		break;
	case 62:
		name = "Cellular Wireless PCS Type 2";
		break;
	case 63:
		name = "Cellular Wireless PCS Roaming";
		break;
	case 64:
	case 65:
		name = "Unassigned";
		break;
	case 66:
		name = "TRS Hotel Motel";
		break;
	case 67:
		name = "TRS Restricted Line";
		break;
	case 68:
	case 69:
		name = "Unassigned";
		break;
	case 70:
		name = "Pay Station No network Coin Control Signalling";
		break;
	case 71:
	case 72:
	case 73:
	case 74:
	case 75:
	case 76:
	case 77:
	case 78:
	case 79:
		name = "Unassigned";
		break;
	case 80:
	case 81:
	case 82:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
		name = "Reserved";
		break;
	case 90:
	case 91:
	case 92:
		name = "Unassigned";
		break;
	case 93:
		name = "Private Virtual Network Type of service call";
		break;
	case 94:
	case 95:
	case 96:
	case 97:
	case 98:
	case 99:
		name = "Unassigned";
		break;

	default:
		name = "Unknown to Asterisk ";
		
	}
	ss7_message(ss7, "PARM: Originating Line Information\n");
	ss7_message(ss7, "\t\tLine info code: %s\n", name);
	ss7_message(ss7, "\t\tValue: %d\n", parm[0]);


	return 1;
	
}

static FUNC_RECV(originating_line_information_receive) 
{
	return 1;
}

static FUNC_SEND(originating_line_information_transmit) 
{
	parm[0] = 0x00;  /* This value is setting OLI equal to POTS line. */
	return 1;	 /*  Will want to have this be set in SIP.conf or thru dialplan and passed here */
}


static FUNC_DUMP(carrier_identification_dump)
{
	return len;
}

static FUNC_RECV(carrier_identification_receive)
{
	return len;
}

static FUNC_SEND(carrier_identification_transmit)
{
	parm[0] = 0x22;  /* 4 digit CIC */
	parm[1] = 0x00;  /* would send default 0000 */
	parm[2] = 0x00;
	
	return 3;
}

static FUNC_DUMP(hop_counter_dump)
{
	return 1;
}

static FUNC_RECV(hop_counter_receive)
{
	return 1;
}

static FUNC_SEND(hop_counter_transmit)
{
	parm[0] = 0x01; /* would send hop counter with value of 1 */
	return 1;
}

static FUNC_RECV(charge_number_receive)
{
	return len;
}

static FUNC_DUMP(charge_number_dump)
{
	return len;
}

static FUNC_SEND(charge_number_transmit)
{
       int oddeven, datalen;

	if (!c->calling_party_num[0] || strlen(c->calling_party_num) != 10)  /* check to make sure we have 10 digit callerid to put in charge number */
		return 0;

	isup_put_number(&parm[2], c->calling_party_num, &datalen, &oddeven);  /* use the value from callerid in sip.conf to fill charge number */
										     
	parm[0] = (oddeven << 7) | 0x1;        /* Nature of Address Indicator = odd/even and ANI of the Calling party, subscriber number */
	parm[1] = (1 << 4) | 0x0;           /* Assume E.164 ISDN numbering plan, calling number complete and make sure reserved bits are zero */
	    
	return datalen + 2;

}

static FUNC_SEND(continuity_ind_transmit)
{
	if (c->cot_check_passed)
		parm[0] = 0x01;
	else
		parm[0] = 0x00;

	return 1;
}

static FUNC_RECV(continuity_ind_receive)
{
	if (0x1 & parm[0])
		c->cot_check_passed = 1;
	else
		c->cot_check_passed = 0;
	return 1;
}

static FUNC_DUMP(continuity_ind_dump)
{
	ss7_message(ss7, "PARM: Continuity Indicator\n");
	ss7_message(ss7, "Continuity Check: %s\n", (0x01 & parm[0]) ? "successful" : "failed");

	return 1;
}

static FUNC_DUMP(circuit_group_supervision_dump)
{
	char *name;

	switch (parm[0] & 0x3) {
	case 0:
		name = "Maintenance oriented";
		break;
	case 1:
		name = "Hardware Failure oriented";
		break;
	case 2:
		name = "Reserved for national use";
		break;
	case 3:
		name = "Spare";
		break;
	default:
		name = "Huh?!";
	}
	ss7_message(ss7, "PARM: Circuit Group Supervision Indicator\n");
	ss7_message(ss7, "Type indicator: %s\n", name);

	return 1;
}

static FUNC_RECV(circuit_group_supervision_receive)
{
	c->cicgroupsupervisiontype = 0x3 & parm[0];
	return 1;
}

static FUNC_SEND(circuit_group_supervision_transmit)
{
	parm[0] = c->cicgroupsupervisiontype & 0x3;
	return 1;
}

static FUNC_DUMP(event_info_dump)
{
	char *name;

	switch (parm[0]) {
		case 0:
			name = "spare";
			break;
		case 1:
			name = "ALERTING";
			break;
		case 2:
			name = "PROGRESS";
			break;
		case 3:
			name = "In-band information or an appropriate pattern is now available";
			break;
		case 4:
			name = "Call forward on busy";
			break;
		case 5:
			name = "Call forward on no reply";
			break;
		case 6:
			name = "Call forward unconditional";
			break;
		default:
			name = "Spare";
			break;
	}
	ss7_message(ss7, "PARM: Event Information:\n");
	ss7_message(ss7, "%s\n", name);
	return 1;
}

static FUNC_RECV(event_info_receive)
{
	c->event_info = parm[0];
	return 1;
}

static FUNC_SEND(event_info_transmit)
{
	parm[0] = c->event_info;
	return 1;
}

static struct parm_func parms[] = {
	{ISUP_PARM_NATURE_OF_CONNECTION_IND, "Nature of Connection Indicator", nature_of_connection_ind_dump, nature_of_connection_ind_receive, nature_of_connection_ind_transmit },
	{ISUP_PARM_FORWARD_CALL_IND, "Forward Call Indicator", forward_call_ind_dump, forward_call_ind_receive, forward_call_ind_transmit },
	{ISUP_PARM_CALLING_PARTY_CAT, "Calling Party Category", calling_party_cat_dump, calling_party_cat_receive, calling_party_cat_transmit},
	{ISUP_PARM_TRANSMISSION_MEDIUM_REQS, "Transmission Medium Requirements", transmission_medium_reqs_dump, transmission_medium_reqs_receive, transmission_medium_reqs_transmit},
	{ISUP_PARM_USER_SERVICE_INFO, "User Service Information", NULL, user_service_info_receive, user_service_info_transmit},
	{ISUP_PARM_CALLED_PARTY_NUM, "Called Party Number", NULL, called_party_num_receive, called_party_num_transmit},
	{ISUP_PARM_CAUSE, "Cause Indicator", cause_dump, cause_receive, cause_transmit},
	{ISUP_PARM_CONTINUITY_IND, "Continuity Indicator", continuity_ind_dump, continuity_ind_receive, continuity_ind_transmit},
	{ISUP_PARM_ACCESS_TRANS, "Access Transport"},
	{ISUP_PARM_BUSINESS_GRP, "Business Group"},
	{ISUP_PARM_CALL_REF, "Call Reference"},
	{ISUP_PARM_CALLING_PARTY_NUM, "Calling Party Number", calling_party_num_dump, calling_party_num_receive, calling_party_num_transmit},
	{ISUP_PARM_CARRIER_ID, "Carrier Identification", carrier_identification_dump, carrier_identification_receive, carrier_identification_transmit},
	{ISUP_PARM_SELECTION_INFO, "Selection Information"},
	{ISUP_PARM_CHARGE_NUMBER, "Charge Number", charge_number_dump, charge_number_receive, charge_number_transmit},
	{ISUP_PARM_CIRCUIT_ASSIGNMENT_MAP, "Circuit Assignment Map"},
	{ISUP_PARM_CONNECTION_REQ, "Connection Request"},
	{ISUP_PARM_CUG_INTERLOCK_CODE, "Interlock Code"},
	{ISUP_PARM_EGRESS_SERV, "Egress Service"},
	{ISUP_PARM_GENERIC_ADDR, "Generic Address"},
	{ISUP_PARM_GENERIC_DIGITS, "Generic Digits"},
	{ISUP_PARM_GENERIC_NAME, "Generic Name"},
	{ISUP_PARM_GENERIC_NOTIFICATION_IND, "Generic Notification Indication"},
	{ISUP_PARM_PROPAGATION_DELAY, "Propagation Delay"},
	{ISUP_PARM_HOP_COUNTER, "Hop Counter", hop_counter_dump, hop_counter_receive, hop_counter_transmit},
	{ISUP_PARM_BACKWARD_CALL_IND, "Backward Call Indicator", backward_call_ind_dump, backward_call_ind_receive, backward_call_ind_transmit},
	{ISUP_PARM_OPT_BACKWARD_CALL_IND, "Optional Backward Call Indicator", opt_backward_call_ind_dump, opt_backward_call_ind_receive, NULL},
	{ISUP_PARM_CIRCUIT_GROUP_SUPERVISION_IND, "Circuit Group Supervision Indicator", circuit_group_supervision_dump, circuit_group_supervision_receive, circuit_group_supervision_transmit},
	{ISUP_PARM_RANGE_AND_STATUS, "Range and status", range_and_status_dump, range_and_status_receive, range_and_status_transmit},
	{ISUP_PARM_EVENT_INFO, "Event Information", event_info_dump, event_info_receive, event_info_transmit},
	{ISUP_PARM_OPT_FORWARD_CALL_INDICATOR, "Optional forward call indicator"},
	{ISUP_PARM_LOCATION_NUMBER, "Location Number"},
	{ISUP_PARM_ORIG_LINE_INFO, "Originating line information", originating_line_information_dump, originating_line_information_receive, originating_line_information_transmit},
};

static char * param2str(int parm)
{
	int x;
	int totalparms = sizeof(parms)/sizeof(struct parm_func);
	for (x = 0; x < totalparms; x++)
		if (parms[x].parm == parm)
			return parms[x].name;

	return "Unknown";
}

static struct isup_call * __isup_new_call(struct ss7 *ss7, int nolink)
{
	struct isup_call *c, *cur;
	c = calloc(1, sizeof(struct isup_call));
	if (!c)
		return NULL;

	if (nolink)
		return c;
	else {
		cur = ss7->calls;
		if (cur) {
			while (cur->next)
				cur = cur->next;
			cur->next = c;
		} else
			ss7->calls = c;

		return c;
	}
}

struct isup_call * isup_new_call(struct ss7 *ss7)
{
	return __isup_new_call(ss7, 0);
}

void isup_set_call_dpc(struct isup_call *c, unsigned int dpc)
{
	c->dpc = dpc;
}

void isup_set_called(struct isup_call *c, const char *called, unsigned char called_nai, const struct ss7 *ss7)
{
    if (called && called[0]) {
    	if (ss7->switchtype == SS7_ITU)
    		snprintf(c->called_party_num, sizeof(c->called_party_num), "%s#", called);
    	else
    		snprintf(c->called_party_num, sizeof(c->called_party_num), "%s", called);
	c->called_nai = called_nai;
    }
    
}

void isup_set_calling(struct isup_call *c, const char *calling, unsigned char calling_nai, unsigned char presentation_ind, unsigned char screening_ind)
{
    if (calling && calling[0]) {
    	strncpy(c->calling_party_num, calling, sizeof(c->calling_party_num));
	c->calling_nai = calling_nai;
	c->presentation_ind = presentation_ind;
	c->screening_ind = screening_ind;
    }
}

void isup_init_call(struct ss7 *ss7, struct isup_call *c, int cic, unsigned int dpc)
{
	c->cic = cic;
	c->dpc = dpc;
}

static struct isup_call * isup_find_call(struct ss7 *ss7, unsigned int opc, int cic)
{
	struct isup_call *cur, *winner = NULL;

	cur = ss7->calls;
	while (cur) {
		if ((cur->cic == cic) && (cur->dpc == opc)) {
			winner = cur;
			break;
		}
		cur = cur->next;
	}

	if (!winner) {
		winner = __isup_new_call(ss7, 0);
		winner->cic = cic;
		winner->dpc = opc;
	}

	return winner;
}

static void isup_free_call(struct ss7 *ss7, struct isup_call *c)
{
	struct isup_call *cur, *prev = NULL, *winner = NULL;

	cur = ss7->calls;
	while (cur) {
		if (cur == c) {
			winner = cur;
			break;
		}
		prev = cur;
		cur = cur->next;
	}

	if (winner) {
		if (!prev)
			ss7->calls = winner->next;
		else
			prev->next = winner->next;
	}

	free(c);

	return;
}

static int do_parm(struct ss7 *ss7, struct isup_call *c, int message, int parm, unsigned char *parmbuf, int maxlen, int parmtype, int tx)
{
	struct isup_parm_opt *optparm = NULL;
	int x;
	int res = 0;
	int totalparms = sizeof(parms)/sizeof(struct parm_func);

	for (x = 0; x < totalparms; x++) {
		if (parms[x].parm == parm) {
			if ((tx && parms[x].transmit) || (!tx && parms[x].receive)) {
				switch (parmtype) {
					case PARM_TYPE_FIXED:
						if (tx)
							return parms[x].transmit(ss7, c, message, parmbuf, maxlen);
						else
							return parms[x].receive(ss7, c, message, parmbuf, maxlen);
					case PARM_TYPE_VARIABLE:
						if (tx) {
							res = parms[x].transmit(ss7, c, message, parmbuf + 1, maxlen);
							if (res > 0) {
								parmbuf[0] = res;
								return res + 1;
							}
							return res;
						} else
							return 1 + parms[x].receive(ss7, c, message, parmbuf + 1, parmbuf[0]);
					case PARM_TYPE_OPTIONAL:
						optparm = (struct isup_parm_opt *)parmbuf;
						if (tx) {
							optparm->type = parms[x].parm;
							res = parms[x].transmit(ss7, c, message, optparm->data, maxlen);
							if (res > 0) {
								optparm->len = res;
							} else
								return res;
						} else
							res = parms[x].receive(ss7, c, message, optparm->data, optparm->len);
						return res + 2;
				}
			}
		}
	}
	return -1;
}

static int dump_parm(struct ss7 *ss7, int message, int parm, unsigned char *parmbuf, int maxlen, int parmtype)
{
	struct isup_parm_opt *optparm = NULL;
	int x;
	int len = 0;
	int totalparms = sizeof(parms)/sizeof(struct parm_func);
	char *parmname = "Unknown Parm";

	for (x = 0; x < totalparms; x++) {
		if (parms[x].parm == parm) {
			if (parms[x].name)
				parmname = parms[x].name;

			ss7_message(ss7, "\t\t%s\n", parms[x].name ? parms[x].name : "Unknown");

			if (parms[x].dump) {
				switch (parmtype) {
					case PARM_TYPE_FIXED:
						len = parms[x].dump(ss7, message, parmbuf, maxlen);
						break;
					case PARM_TYPE_VARIABLE:
						parms[x].dump(ss7, message, parmbuf + 1, parmbuf[0]);
						len = 1 + parmbuf[0];
						break;
					case PARM_TYPE_OPTIONAL:
						optparm = (struct isup_parm_opt *)parmbuf;
						parms[x].dump(ss7, message, optparm->data, optparm->len);
						len = 2 + optparm->len;
						break;
				}

			} else {
				switch (parmtype) {
					case PARM_TYPE_VARIABLE:
						len = parmbuf[0] + 1;
						break;
					case PARM_TYPE_OPTIONAL:
						optparm = (struct isup_parm_opt *)parmbuf;
						len = optparm->len + 2;
						break;
				}
			}

			ss7_dump_buf(ss7, 2, parmbuf, len);
			return len;
		}
	}

	/* This is if we don't find it....  */
	ss7_message(ss7, "\t\tParm: Unknown");
	optparm = (struct isup_parm_opt *)parmbuf;
	ss7_dump_buf(ss7, 2, optparm->data, optparm->len);
	return optparm->len + 2;
}

static int isup_send_message(struct ss7 *ss7, struct isup_call *c, int messagetype, int parms[])
{
	struct ss7_msg *msg;
	struct isup_h *mh = NULL;
	unsigned char *rlptr;
	int ourmessage = -1;
	int rlsize;
	unsigned char *varoffsets = NULL, *opt_ptr;
	int fixedparams = 0, varparams = 0, optparams = 0;
	int len = sizeof(struct ss7_msg);
	struct routing_label rl;
	int res = 0;
	int offset = 0;
	int x = 0;
	int i = 0;

	/* Do init stuff */
	msg = ss7_msg_new();

	if (!msg) {
		ss7_error(ss7, "Allocation failed!\n");
		return -1;
	}

	rlptr = ss7_msg_userpart(msg);
	rl.opc = ss7->pc;

	if (ss7->switchtype == SS7_ANSI) {
		rl.sls = sls_next(ss7);
	} else
		rl.sls = c->cic & 0xf;

	/* use CIC's DPC instead of linkset's DPC */

	rl.dpc = c->dpc;
	rl.type = ss7->switchtype;
	rlsize = set_routinglabel(rlptr, &rl);
	mh = (struct isup_h *)(rlptr + rlsize); /* Note to self, do NOT put a typecasted pointer next to an addition operation */

	/* Set the CIC - ITU style */
	if (ss7->switchtype == SS7_ITU) {
		mh->cic[0] = c->cic & 0xff;
		mh->cic[1] = (c->cic >> 8) & 0x0f;
	} else {
		mh->cic[0] = c->cic & 0xff;
		mh->cic[1] = (c->cic >> 8) & 0x03f;
	}

	mh->type = messagetype;
	/* Find the metadata for our message */
	for (x = 0; x < sizeof(messages)/sizeof(struct message_data); x++)
		if (messages[x].messagetype == messagetype)
			ourmessage = x;
			
	if (ourmessage < 0) {
		ss7_error(ss7, "Unable to find message %d in message list!\n", mh->type);
		return -1;
	}

	fixedparams = messages[ourmessage].mand_fixed_params;
	varparams = messages[ourmessage].mand_var_params;
	optparams = messages[ourmessage].opt_params;

	/* Again, the ANSI exception */
	if (ss7->switchtype == SS7_ANSI) {
		if (messages[ourmessage].messagetype == ISUP_IAM) {
			fixedparams = 3;
			varparams = 2;
		} else if (messages[ourmessage].messagetype == ISUP_RLC) {
			optparams = 0;
		}
	}

	/* Add fixed params */
	for (x = 0; x < fixedparams; x++) {
		res = do_parm(ss7, c, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_FIXED, 1);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to add mandatory fixed parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}

	varoffsets = &mh->data[offset];
	/* Make sure we grab our opional parameters */
	if (optparams) {
		opt_ptr = &mh->data[offset + varparams];
		offset += varparams + 1; /* add one for the optionals */
		len -= varparams + 1;
	} else {
		offset += varparams;
		len -= varparams;
	}

	/* Whew, some complicated math for all of these offsets and different sections */
	for (; (x - fixedparams) < varparams; x++) {
		varoffsets[i] = &mh->data[offset] - &varoffsets[i];
		i++;
		res = do_parm(ss7, c, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_VARIABLE, 1);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to add mandatory variable parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}
       	/* Optional parameters */
	if (optparams) {
		int addedparms = 0;
		int offsetbegins = offset;
		while (parms[x] > -1) {
			res = do_parm(ss7, c, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_OPTIONAL, 1);
			x++;
	
			if (res < 0) {
				ss7_error(ss7, "!! Unable to add optional parameter '%s'\n", param2str(parms[x]));
				return -1;
			}

			if (res > 0)
				addedparms++;
	
			len -= res;
			offset += res;
		}

		if (addedparms) {
			*opt_ptr = &mh->data[offsetbegins] - opt_ptr;
			/* Add end of optional parameters */
			mh->data[offset++] = 0;
		} else
			*opt_ptr = 0;

	}

	ss7_msg_userpart_len(msg, offset + rlsize + CIC_SIZE + 1);   /* Message type length is 1 */

	return mtp3_transmit(ss7, SIG_ISUP, rl.sls, msg);
}

int isup_dump(struct ss7 *ss7, struct mtp2 *link, unsigned char *buf, int len)
{
	struct isup_h *mh;
	unsigned short cic;
	int ourmessage = -1;
	int *parms = NULL;
	int offset = 0;
	int fixedparams = 0, varparams = 0, optparams = 0;
	int res, x;
	unsigned char *opt_ptr = NULL;

	mh = (struct isup_h*) buf;

	if (ss7->switchtype == SS7_ITU) {
		cic = mh->cic[0] | ((mh->cic[1] << 8) & 0x0f);
	} else {
		cic = mh->cic[0] | ((mh->cic[1] << 8) & 0x3f);
	}
	/* Find us in the message list */
	for (x = 0; x < sizeof(messages)/sizeof(struct message_data); x++)
		if (messages[x].messagetype == mh->type)
			ourmessage = x;

	if (ourmessage < 0) {
		ss7_error(ss7, "!! Unable to handle message of type 0x%x\n", mh->type);
		return -1;
	}

	ss7_message(ss7, "\t\tCIC: %d\n", cic);
	ss7_dump_buf(ss7, 2, buf, 2);
	ss7_message(ss7, "\t\tMessage Type: %s\n", message2str(mh->type), mh->type & 0xff);
	ss7_dump_buf(ss7, 2, &buf[2], 1);

	fixedparams = messages[ourmessage].mand_fixed_params;
	varparams = messages[ourmessage].mand_var_params;
	parms = messages[ourmessage].param_list;
	optparams = messages[ourmessage].opt_params;

	if (ss7->switchtype == SS7_ANSI) {
		/* Check for the ANSI IAM exception */
		if (messages[ourmessage].messagetype == ISUP_IAM) {
			/* Stupid ANSI SS7, they just had to be different, didn't they? */
			fixedparams = 3;
			varparams = 2;
			parms = ansi_iam_params;
		} else if (messages[ourmessage].messagetype == ISUP_RLC) {
			optparams = 0;
		}
	}

	if (fixedparams)
		ss7_message(ss7, "\t\tMandatory Fixed Length Parms:\n");

	/* Parse fixed parms */
	for (x = 0; x < fixedparams; x++) {
		res = dump_parm(ss7, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_FIXED);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to parse mandatory fixed parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}

	if (varparams) {
		offset += varparams; /* add one for the optionals */
		len -= varparams;
	}
	if (optparams) {
		opt_ptr = &mh->data[offset++];
		len++;
	}

	if (varparams)
		ss7_message(ss7, "\t\tMandatory Variable Length Parms:\n");
	for (; (x - fixedparams) < varparams; x++) {
		res = dump_parm(ss7, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_VARIABLE);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to parse mandatory variable parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}

	/* Optional paramter parsing code */
	if (optparams && *opt_ptr) {
		if (len > 0)
			ss7_message(ss7, "\t\tOptional Parms\n");
		while ((len > 0) && (mh->data[offset] != 0)) {
			struct isup_parm_opt *optparm = (struct isup_parm_opt *)(mh->data + offset);

			res = dump_parm(ss7, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_OPTIONAL);

			if (res < 0) {
				ss7_message(ss7, "Unhandled optional parameter 0x%x '%s'\n", optparm->type, param2str(optparm->type));
				isup_dump_buffer(ss7, optparm->data, optparm->len);
				res = optparm->len + 2;
			}

			len -= res;
			offset += res;
		}
	}

	return 0;
}

int isup_receive(struct ss7 *ss7, struct mtp2 *link, unsigned int opc, unsigned char *buf, int len)
{
	unsigned short cic;
	struct isup_h *mh;
	struct isup_call *c;
	int i;
	int *parms = NULL;
	int offset = 0;
	int ourmessage = -1;
	int fixedparams = 0, varparams = 0, optparams = 0;
	int res, x;
	unsigned char *opt_ptr = NULL;
	ss7_event *e;

	mh = (struct isup_h*) buf;
	if (ss7->switchtype == SS7_ITU) {
		cic = mh->cic[0] | ((mh->cic[1] << 8) & 0x0f);
	} else {
		cic = mh->cic[0] | ((mh->cic[1] << 8) & 0x3f);
	}

	/* Find us in the message list */
	for (x = 0; x < sizeof(messages)/sizeof(struct message_data); x++)
		if (messages[x].messagetype == mh->type)
			ourmessage = x;


	if (ourmessage < 0) {
		ss7_error(ss7, "!! Unable to handle message of type 0x%x\n", mh->type);
		return -1;
	}

	fixedparams = messages[ourmessage].mand_fixed_params;
	varparams = messages[ourmessage].mand_var_params;
	parms = messages[ourmessage].param_list;
	optparams = messages[ourmessage].opt_params;

	if (ss7->switchtype == SS7_ANSI) {
		/* Check for the ANSI IAM exception */
		if (messages[ourmessage].messagetype == ISUP_IAM) {
			/* Stupid ANSI SS7, they just had to be different, didn't they? */
			fixedparams = 3;
			varparams = 2;
			parms = ansi_iam_params;
				
		} else if (messages[ourmessage].messagetype == ISUP_RLC) {
			optparams = 0;
		}
	}

	/* Make sure we don't hijack a call associated isup_call for non call
	 * associated messages */
	switch (mh->type) {
		case ISUP_BLO:
		case ISUP_BLA:
		case ISUP_UBL:
		case ISUP_UBA:
		case ISUP_CGB:
		case ISUP_CGBA:
		case ISUP_CGUA:
		case ISUP_CGU:
		case ISUP_UCIC:
			c = __isup_new_call(ss7, 1);
			c->dpc = opc;
			c->cic = cic;
			break;
		default:
			c = isup_find_call(ss7, opc, cic);
	}

	if (!c) {
		ss7_error(ss7, "Huh? No call!!!???\n");
		return -1;
	}

	/* Parse fixed parms */
	for (x = 0; x < fixedparams; x++) {
		res = do_parm(ss7, c, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_FIXED, 0);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to parse mandatory fixed parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}

	if (varparams) {
		offset += varparams; /* add one for the optionals */
		res -= varparams;
	}
	if (optparams) {
		/* ANSI doesn't have optional parameters on RLC */
		opt_ptr = &mh->data[offset++];
	}

	for (; (x - fixedparams) < varparams; x++) {
		res = do_parm(ss7, c, mh->type, parms[x], (void *)(mh->data + offset), len, PARM_TYPE_VARIABLE, 0);

		if (res < 0) {
			ss7_error(ss7, "!! Unable to parse mandatory variable parameter '%s'\n", param2str(parms[x]));
			return -1;
		}

		len -= res;
		offset += res;
	}

	/* Optional paramter parsing code */
	if (optparams && *opt_ptr) {
		while ((len > 0) && (mh->data[offset] != 0)) {
			struct isup_parm_opt *optparm = (struct isup_parm_opt *)(mh->data + offset);

			res = do_parm(ss7, c, mh->type, optparm->type, mh->data + offset, len, PARM_TYPE_OPTIONAL, 0);

			if (res < 0) {
				ss7_message(ss7, "Unhandled optional parameter 0x%x '%s'\n", optparm->type, param2str(optparm->type));
				isup_dump_buffer(ss7, optparm->data, optparm->len);
				res = optparm->len + 2;
			}

			len -= res;
			offset += res;
		}
	}

	e = ss7_next_empty_event(ss7);
	if (!e) {
		ss7_error(ss7, "Event queue full, unable to get next empty event\n");
		return -1;
	}

	switch (mh->type) {
		case ISUP_IAM:
			e->e = ISUP_EVENT_IAM;
			e->iam.cic = c->cic;
			e->iam.transcap = c->transcap;
			e->iam.cot_check_required = c->cot_check_required;
			strncpy(e->iam.called_party_num, c->called_party_num, sizeof(e->iam.called_party_num));
			e->iam.called_nai = c->called_nai;
			strncpy(e->iam.calling_party_num, c->calling_party_num, sizeof(e->iam.calling_party_num));
			e->iam.calling_nai = c->calling_nai;
			e->iam.presentation_ind = c->presentation_ind;
			e->iam.screening_ind = c->screening_ind;
			e->iam.call = c;
			return 0;
		case ISUP_GRS:
			e->e = ISUP_EVENT_GRS;
			e->grs.startcic = cic;
			e->grs.endcic = cic + c->range;
			isup_free_call(ss7, c); /* Won't need this again */
			return 0;
		case ISUP_GRA:
			e->e = ISUP_EVENT_GRA;
			e->gra.startcic = cic;
			e->gra.endcic = cic + c->range;
			for (i = 0; i < (c->range + 1); i++)
				e->gra.status[i] = c->status[i];

			isup_free_call(ss7, c); /* Won't need this again */
			return 0;
		case ISUP_RSC:
			e->e = ISUP_EVENT_RSC;
			e->rsc.cic = cic;
			e->rsc.call = c;
			return 0;
		case ISUP_REL:
			e->e = ISUP_EVENT_REL;
			e->rel.cic = c->cic;
			e->rel.call = c;
			e->rel.cause = c->cause;
			return 0;
		case ISUP_ACM:
			e->e = ISUP_EVENT_ACM;
			e->acm.cic = c->cic;
			e->acm.call = c;
			return 0;
		case ISUP_CON:
			e->e = ISUP_EVENT_CON;
			e->con.cic = c->cic;
			e->con.call = c;
			return 0;
		case ISUP_ANM:
			e->e = ISUP_EVENT_ANM;
			e->anm.cic = c->cic;
			e->anm.call = c;
			return 0;
		case ISUP_RLC:
			e->e = ISUP_EVENT_RLC;
			e->rlc.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		case ISUP_COT:
			e->e = ISUP_EVENT_COT;
			e->cot.cic = c->cic;
			e->cot.passed = c->cot_check_passed;
			e->cot.call = c;
			return 0;
		case ISUP_CCR:
			e->e = ISUP_EVENT_CCR;
			e->ccr.cic = c->cic;
			return 0;
		case ISUP_BLO:
			e->e = ISUP_EVENT_BLO;
			e->blo.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		case ISUP_UBL:
			e->e = ISUP_EVENT_UBL;
			e->ubl.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		case ISUP_BLA:
			e->e = ISUP_EVENT_BLA;
			e->bla.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		case ISUP_UBA:
			e->e = ISUP_EVENT_UBA;
			e->uba.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		case ISUP_CGB:
			e->e = ISUP_EVENT_CGB;
			e->cgb.startcic = cic;
			e->cgb.endcic = cic + c->range;
			e->cgu.type = c->cicgroupsupervisiontype;

			for (i = 0; i < (c->range + 1); i++)
				e->cgb.status[i] = c->status[i];

			isup_free_call(ss7, c);
			return 0;
		case ISUP_CGU:
			e->e = ISUP_EVENT_CGU;
			e->cgu.startcic = cic;
			e->cgu.endcic = cic + c->range;
			e->cgu.type = c->cicgroupsupervisiontype;

			for (i = 0; i < (c->range + 1); i++)
				e->cgu.status[i] = c->status[i];

			isup_free_call(ss7, c);
			return 0;
		case ISUP_CPG:
			e->e = ISUP_EVENT_CPG;
			e->cpg.cic = c->cic;
			e->cpg.event = c->event_info;
			return 0;
		case ISUP_UCIC:
			e->e = ISUP_EVENT_UCIC;
			e->ucic.cic = c->cic;
			isup_free_call(ss7, c);
			return 0;
		default:
			ss7_error(ss7, "!! Unable to handle message type %s\n", message2str(mh->type));
			return -1;
	}
}

static int isup_send_cicgroupmessage(struct ss7 *ss7, int messagetype, int begincic, int endcic, unsigned int dpc, unsigned char status[], int type)
{
	struct isup_call call;
	int i;

	for (i = 0; (i + begincic) <= endcic; i++)
		call.status[i] = status[i];

	call.cic = begincic;
	call.range = endcic - begincic;
	call.cicgroupsupervisiontype = type;
	call.dpc = dpc;

	if (call.range > 31)
		return -1;

	return isup_send_message(ss7, &call, messagetype, cicgroup_params);
}

int isup_grs(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc)
{
	struct isup_call call;

	if (!ss7)
		return -1;

	call.cic = begincic;
	call.range = endcic - begincic;
	call.dpc = dpc;

	if (call.range > 31)
		return -1;

	return isup_send_message(ss7, &call, ISUP_GRS, greset_params);
}

int isup_gra(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc)
{
	struct isup_call call;

	if (!ss7)
		return -1;
	call.cic = begincic;
	call.range = endcic - begincic;
	call.dpc = dpc;
	
	if (call.range > 31)
		return -1;

	return isup_send_message(ss7, &call, ISUP_GRA, greset_params);
}

int isup_cgb(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc, unsigned char state[], int type)
{
	if (!ss7)
		return -1;

 	return isup_send_cicgroupmessage(ss7, ISUP_CGB, begincic, endcic, dpc, state, type);
}

int isup_cgu(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc, unsigned char state[], int type)
{
	if (!ss7)
		return -1;

 	return isup_send_cicgroupmessage(ss7, ISUP_CGU, begincic, endcic, dpc, state, type);
}

int isup_cgba(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc, unsigned char state[], int type)
{
	if (!ss7)
		return -1;
 	return isup_send_cicgroupmessage(ss7, ISUP_CGBA, begincic, endcic, dpc, state, type);
}

int isup_cgua(struct ss7 *ss7, int begincic, int endcic, unsigned int dpc, unsigned char state[], int type)
{
	if (!ss7)
		return -1;

	return isup_send_cicgroupmessage(ss7, ISUP_CGUA, begincic, endcic, dpc, state, type);
}

int isup_iam(struct ss7 *ss7, struct isup_call *c)
{
	if (!ss7 || !c)
		return -1;

	if (ss7->switchtype == SS7_ITU)
		return isup_send_message(ss7, c, ISUP_IAM, iam_params);
	else
		return isup_send_message(ss7, c, ISUP_IAM, ansi_iam_params);
}

int isup_acm(struct ss7 *ss7, struct isup_call *c)
{
	if (!ss7 || !c)
		return -1;

	return isup_send_message(ss7, c, ISUP_ACM, acm_params);
}

int isup_anm(struct ss7 *ss7, struct isup_call *c)
{
	if (!ss7 || !c)
		return -1;

	return isup_send_message(ss7, c, ISUP_ANM, anm_params);
}

int isup_con(struct ss7 *ss7, struct isup_call *c)
{
	if (!ss7 || !c)
		return -1;

	return isup_send_message(ss7, c, ISUP_CON, con_params);
}

int isup_rel(struct ss7 *ss7, struct isup_call *c, int cause)
{
	if (!ss7 || !c)
		return -1;

	if (cause < 0)
		cause = 16;

	c->cause = cause;
	c->causecode = CODE_CCITT;
	c->causeloc = LOC_PRIV_NET_LOCAL_USER;
	return isup_send_message(ss7, c, ISUP_REL, rel_params);
}

int isup_rlc(struct ss7 *ss7, struct isup_call *c)
{
	int res;

	if (!ss7 || !c)
		return -1;

	res = isup_send_message(ss7, c, ISUP_RLC, empty_params);
	isup_free_call(ss7, c);
	return res;
}

static int isup_send_message_ciconly(struct ss7 *ss7, int messagetype, int cic, unsigned int dpc)
{
	int res;
	struct isup_call c;
	c.cic = cic;
	c.dpc = dpc;
	res = isup_send_message(ss7, &c, messagetype, empty_params);
	return res;
}

int isup_cpg(struct ss7 *ss7, struct isup_call *c, int event)
{
	if (!ss7 || !c)
		return -1;

	c->event_info = event;
	return isup_send_message(ss7, c, ISUP_CPG, cpg_params);
}

int isup_rsc(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_RSC, cic, dpc);
}

int isup_blo(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_BLO, cic, dpc);
}

int isup_ubl(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_UBL, cic, dpc);
}

int isup_bla(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_BLA, cic, dpc);
}

int isup_ucic(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_UCIC, cic, dpc);
}


int isup_uba(struct ss7 *ss7, int cic, unsigned int dpc)
{
	if (!ss7)
		return -1;

	return isup_send_message_ciconly(ss7, ISUP_UBA, cic, dpc);
}
/* Janelle is the bomb (Again) */
