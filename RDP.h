#ifndef RDP_H
#define RDP_H

typedef struct
{
	u32 w2, w3;
	u32 cmd_ptr;
	u32 cmd_cur;
	u32 cmd_data[0x1000];
} RDPInfo;

extern RDPInfo RDP;

void RDP_Init();
void RDP_Half_1(u32 _c);
void RDP_ProcessRDPList();
void RDP_RepeatLastLoadBlock();

#endif

