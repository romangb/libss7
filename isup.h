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
#ifndef _SS7_ISUP_H
#define _SS7_ISUP_H

/* ISUP messages */
#define ISUP_IAM	0x01
#define ISUP_SAM	0x02
#define ISUP_INR	0x03
#define ISUP_INF	0x04
#define ISUP_COT	0x05
#define ISUP_ACM	0x06
#define ISUP_CON	0x07
#define ISUP_FOT	0x08
#define ISUP_ANM	0x09
#define ISUP_REL	0x0c
#define ISUP_SUS	0x0d
#define ISUP_RES	0x0e
#define ISUP_RLC	0x10
#define ISUP_CCR	0x11
#define ISUP_RSC	0x12
#define ISUP_BLO	0x13
#define ISUP_UBL	0x14
#define ISUP_BLA	0x15
#define ISUP_UBA	0x16
#define ISUP_GRS	0x17
#define ISUP_CGB	0x18
#define ISUP_CGU	0x19
#define ISUP_CGBA	0x1a
#define ISUP_CGUA	0x1b
#define ISUP_CMR	0x1c
#define ISUP_CMC	0x1d
#define ISUP_CMRJ	0x1e
#define ISUP_FAR	0x1f
#define ISUP_FAA	0x20
#define ISUP_FRJ	0x21
#define ISUP_FAD	0x22
#define ISUP_FAI	0x23
#define ISUP_LPA	0x24
#define ISUP_CSVR	0x25
#define ISUP_CSVS	0x26
#define ISUP_DRS	0x27
#define ISUP_PAM	0x28
#define ISUP_GRA	0x29
#define ISUP_CQM	0x2a
#define ISUP_CQR	0x2b
#define ISUP_CPG	0x2c
#define ISUP_USR	0x2d
#define ISUP_UCIC	0x2e
#define ISUP_CFN	0x2f
#define ISUP_OLM	0x30
#define ISUP_CRG	0x31
#define ISUP_FAC	0x33
#define ISUP_CRA	0xe9
#define ISUP_CRM	0xea
#define ISUP_CVR	0xeb
#define ISUP_CVT	0xec
#define ISUP_EXM	0xed

/* ISUP Parameters */
#define ISUP_PARM_NATURE_OF_CONNECTION_IND 0x06
#define ISUP_PARM_FORWARD_CALL_IND 0x07
#define ISUP_PARM_CALLING_PARTY_CAT 0x09
#define ISUP_PARM_USER_SERVICE_INFO 0x1d
#define ISUP_PARM_TRANSMISSION_MEDIUM_REQS 0x02
#define ISUP_PARM_CALLED_PARTY_NUM 0x04
#define ISUP_PARM_ACCESS_TRANS 0x03
#define ISUP_PARM_BUSINESS_GRP 0xc6
#define ISUP_PARM_CALL_REF 0x01
#define ISUP_PARM_CALLING_PARTY_NUM 0x0a
#define ISUP_PARM_CARRIER_ID 0xc5
#define ISUP_PARM_SELECTION_INFO 0xee
#define ISUP_PARM_CHARGE_NUMBER 0xeb
#define ISUP_PARM_CIRCUIT_ASSIGNMENT_MAP 0x25
#define ISUP_PARM_OPT_BACKWARD_CALL_IND 0x29
#define ISUP_PARM_CONNECTION_REQ 0x0d
#define ISUP_PARM_CONTINUITY_IND 0x10
#define ISUP_PARM_CUG_INTERLOCK_CODE 0x1c
#define ISUP_PARM_EGRESS_SERV 0xc3
#define ISUP_PARM_GENERIC_ADDR 0xc0
#define ISUP_PARM_GENERIC_DIGITS 0xc1
#define ISUP_PARM_GENERIC_NAME 0xc7
#define ISUP_PARM_GENERIC_NOTIFICATION_IND 0x2c
#define ISUP_PARM_BACKWARD_CALL_IND 0x11
#define ISUP_PARM_CAUSE 0x12
#define ISUP_PARM_CIRCUIT_GROUP_SUPERVISION_IND 0x15
#define ISUP_PARM_RANGE_AND_STATUS 0x16
#define ISUP_PARM_PROPAGATION_DELAY 0x31
#define ISUP_PARM_EVENT_INFO 0x24
#define ISUP_PARM_HOP_COUNTER 0x3d
#define ISUP_PARM_OPT_FORWARD_CALL_INDICATOR 0x08
#define ISUP_PARM_LOCATION_NUMBER 0x3f
#define ISUP_PARM_ORIG_LINE_INFO 0xea
#define ISUP_PARM_REDIRECTION_INFO 0x13
#define ISUP_PARM_ORIGINAL_CALLED_NUM 0x28
#define ISUP_PARM_JIP 0xc4
#define ISUP_PARM_ECHO_CONTROL_INFO 0x37
#define ISUP_PARM_PARAMETER_COMPAT_INFO 0x39
#define ISUP_PARM_CIRCUIT_STATE_IND 0x26



/* ISUP Parameter Pseudo-type */
struct isup_parm_opt {
	unsigned char type;
	unsigned char len;
	unsigned char data[0];
};

struct isup_h {
	unsigned char cic[2];
	unsigned char type;
	unsigned char data[0]; /* This is the contents of the message */
};

#define CIC_SIZE 2
#define ISUP_MAX_NUM 64

struct mtp2;

struct isup_call {
	char called_party_num[ISUP_MAX_NUM];
	unsigned char called_nai;
	char calling_party_num[ISUP_MAX_NUM];
	unsigned char calling_nai;
	unsigned char presentation_ind;
	unsigned char screening_ind;
	char charge_number[ISUP_MAX_NUM];
	unsigned char charge_nai;
	unsigned char charge_num_plan;
	int oli_ani2;
	int range;
	unsigned char status[255];
	int transcap;
	int l1prot;
	int cause;
	int causecode;
	int causeloc;
	int cot_check_passed;
	int cot_check_required;
	int cicgroupsupervisiontype;
	unsigned char event_info;
	unsigned short cic;
	unsigned short slc;
	struct isup_call *next;
	/* set DPC according to CIC's DPC, not linkset */
	unsigned int dpc;
};

int isup_receive(struct ss7 *ss7, struct mtp2 *sl, unsigned int opc, unsigned char *sif, int len);

int isup_dump(struct ss7 *ss7, struct mtp2 *sl, unsigned char *sif, int len);
#endif /* _SS7_ISUP_H */
