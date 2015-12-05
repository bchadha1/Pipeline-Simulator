/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) 
{ 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
    CPU_State new_CPU_STATE;
    
    process_IF();
    process_ID();
    process_EX();
    process_MEM();
    process_WB();
    
    CURRENT_STATE = TEMP_STATE;
}




/*
 Assumes PC is updated
 
 */

void process_IF() {
    IF_ID_pipeline_buffer.NPC = CURRENT_STATE.PC;
    instruction *inst;
    inst = get_inst_info(CURRENT_STATE.PC);
    
}



/**************************************************************
 input :    IF/ID.REG1
            IF/ID.REG2
            MEM/WB.WriteReg
            WB/.WriteData
 
 output:
 **************************************************************/
void process_ID(){
    
}

void process_EX(){
    ID/EX prevID_EX_pipeline = CURRENT_STATE.ID_EX_pipeline;
    
    // shift the pipelined control signals
    EX_MEM_pipeline_buffer.WB_MemToReg = prevID_EX_pipeline.WB_MemToReg;
    EX_MEM_pipeline_buffer.WB_RegToWrite = prevID_EX_pipeline.WB_RegToWrite;
    
    EX_MEM_pipeline_buffer.MemWrite = prevID_EX_pipeline.MEM_MemWrite;
    EX_MEM_pipeline_buffer.MemRead = prevID_EX_pipeline.MEM_MemRead;
    EX_MEM_pipeline_buffer.Branch = prevID_EX_pipeline.MEM_Branch;
    
    // PC (for J and JR instruction)
    EX_MEM_pipeline_buffer.NPC = prevID_EX_pipeline.NPC + (prevID_EX_pipeline.IMM << 2);
    
    // select ALU input
    uint32_t ALUinput1 = prevID_EX_pipeline.data1;
    uint32_t ALUinput2;
    
    // if ALUSrc == 1, the second ALU operand is the sign-extended, lower 16 bits of the instruction.
    // if ALUSrc == 0, the second ALU operand comes from the second register file output. (Read data 2).
    if (prevID_EX_pipeline.ALUSrc) {
        ALUinput2 = prevID_EX_pipeline.IMM;
    } else {         ALUinput2 = prevID_EX_pipeline.data2;
    }
    
    // transfer data2
    EX_MEM_pipeline_buffer.data2 = prevID_EX_pipeline.data2;
    
    // calculate ALU control
    int funct_field = (prevID_EX_pipeline.IMM & 63); // extract funct field from instruction[0~15]
    int control_line = ALU_control(funct_field, prevID_EX_pipeline.ALUOp0, prevID_EX_pipeline.ALUOp1);
    
    // process ALU
    uint32_t ALUresult = ALU(control_line, ALUinput1, ALUinput2);
    EX_MEM_pipeline_buffer.zero = !ALUresult;
    EX_MEM_pipeline_buffer.ALU_OUT = ALUresult;
    
    // Mux for Write Register
    // if RegDst == 1, the register destination number for the Write register comes from the rd field.(bits 15:11)
    // if RegDst == 0, the register destination number for the Write register comes from the rt field.(bits 20:16)
    if (prevID_EX_pipeline.RegDst) {
        EX_MEM_pipeline_buffer.RegDstNum = prevID_EX_pipeline.inst11_15;
    } else {
        EX_MEM_pipeline_buffer.RegDstNum = prevID_EX_pipeline.inst16_20;
    }
    
}

void process_MEM(){
    
}

void process_WB(){
    
}


uint32_t ALU(int control_line, uint32_t data1, uint32_t data2)) {
    if (control_line == 0) {        // 0000 : AND
        return data1 & data2;
    } else if (control_line == 1) { // 0001 : OR
        return data1 | data2;
    } else if (control_line == 2) { // 0010 : add
        return data1 + data2;
    } else if (control_line == 6) { // 0110 : subtract
        return data1 - data2;
    } else if (control_line == 7) { // 0111 : set on less than
        return data1 < data2;
    } else if (control_line == 12) { // 1100 : NOR
        return
    } else {
        printf("Error in ALU. ALU control line value is : %d", control_line);
    }
}

// outputs ALU control line (textbook page 247)
int ALU_control(int funct_field, bool ALUOp0, bool ALUOp1) {
    if (ALUOp0 == 0 && ALUOp1 == 0) {
        return 2;                                               // 0010
    } else if (ALUOp0 == 1) {
        return 6;                                               // 0110
    } else { // ALUOp1 = 1
        funct_field = funct_field & 15; // mask last 4 digits
        if (funct_field == 0) {
            return 2;                                           // 0010
        } else if (funct_field == 2) {
            return 6;                                           // 0110
        } else if (funct_field == 4) {
            return 0;                                           // 0000
        } else if (funct_field == 5) {
            return 1;                                           // 0001
        } else if (funct_field == 10) {
            return 7;                                           // 0111
        } else {
            printf("Error in ALU_control. func_field value is %d", funct_field);
        }
    }
}






#ifdef ORIGINAL
void process_instruction(){
    instruction *inst;
    int i;		// for loop

    /* pipeline */
    for ( i = PIPE_STAGE - 1; i > 0; i--)
	CURRENT_STATE.PIPE[i] = CURRENT_STATE.PIPE[i-1];
    CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;

    inst = get_inst_info(CURRENT_STATE.PC);
    CURRENT_STATE.PC += BYTES_PER_WORD;

    switch (OPCODE(inst))
    {
	case 0x9:		//(0x001001)ADDIU
	    CURRENT_STATE.REGS[RT (inst)] = CURRENT_STATE.REGS[RS (inst)] + (short) IMM (inst);
	    break;
	case 0xc:		//(0x001100)ANDI
	    CURRENT_STATE.REGS[RT (inst)] = CURRENT_STATE.REGS[RS (inst)] & (0xffff & IMM (inst));
	    break;
	case 0xf:		//(0x001111)LUI	
	    CURRENT_STATE.REGS[RT (inst)] = (IMM (inst) << 16) & 0xffff0000;
	    break;
	case 0xd:		//(0x001101)ORI
	    CURRENT_STATE.REGS[RT (inst)] = CURRENT_STATE.REGS[RS (inst)] | (0xffff & IMM (inst));
	    break;
	case 0xb:		//(0x001011)SLTIU 
	    {
		int x = (short) IMM (inst);

		if ((uint32_t) CURRENT_STATE.REGS[RS (inst)] < (uint32_t) x)
		    CURRENT_STATE.REGS[RT (inst)] = 1;
		else
		    CURRENT_STATE.REGS[RT (inst)] = 0;
		break;
	    }
	case 0x23:		//(0x100011)LW	
	    LOAD_INST (&CURRENT_STATE.REGS[RT (inst)], mem_read_32((CURRENT_STATE.REGS[BASE (inst)] + IOFFSET (inst))), 0xffffffff);
	    break;
	case 0x2b:		//(0x101011)SW
	    mem_write_32(CURRENT_STATE.REGS[BASE (inst)] + IOFFSET (inst), CURRENT_STATE.REGS[RT (inst)]);
	    break;
	case 0x4:		//(0x000100)BEQ
	    BRANCH_INST (CURRENT_STATE.REGS[RS (inst)] == CURRENT_STATE.REGS[RT (inst)], CURRENT_STATE.PC + IDISP (inst), 0);
	    break;
	case 0x5:		//(0x000101)BNE
	    BRANCH_INST (CURRENT_STATE.REGS[RS (inst)] != CURRENT_STATE.REGS[RT (inst)], CURRENT_STATE.PC + IDISP (inst), 0);
	    break;

	case 0x0:		//(0x000000)ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU  if JR
	    {
		switch(FUNC (inst)){
		    case 0x21:	//ADDU
			CURRENT_STATE.REGS[RD (inst)] = CURRENT_STATE.REGS[RS (inst)] + CURRENT_STATE.REGS[RT (inst)];
			break;
		    case 0x24:	//AND
			CURRENT_STATE.REGS[RD (inst)] = CURRENT_STATE.REGS[RS (inst)] & CURRENT_STATE.REGS[RT (inst)];
			break;
		    case 0x27:	//NOR
			CURRENT_STATE.REGS[RD (inst)] = ~ (CURRENT_STATE.REGS[RS (inst)] | CURRENT_STATE.REGS[RT (inst)]);
			break;
		    case 0x25:	//OR
			CURRENT_STATE.REGS[RD (inst)] = CURRENT_STATE.REGS[RS (inst)] | CURRENT_STATE.REGS[RT (inst)];
			break;
		    case 0x2B:	//SLTU
			if ( CURRENT_STATE.REGS[RS (inst)] <  CURRENT_STATE.REGS[RT (inst)])
			    CURRENT_STATE.REGS[RD (inst)] = 1;
			else
			    CURRENT_STATE.REGS[RD (inst)] = 0;
			break;
		    case 0x0:	//SLL
			{
			    int shamt = SHAMT (inst);

			    if (shamt >= 0 && shamt < 32)
				CURRENT_STATE.REGS[RD (inst)] = CURRENT_STATE.REGS[RT (inst)] << shamt;
			    else
				CURRENT_STATE.REGS[RD (inst)] = CURRENT_STATE.REGS[RT (inst)];
			    break;
			}
		    case 0x2:	//SRL
			{
			    int shamt = SHAMT (inst);
			    uint32_t val = CURRENT_STATE.REGS[RT (inst)];

			    if (shamt >= 0 && shamt < 32)
				CURRENT_STATE.REGS[RD (inst)] = val >> shamt;
			    else
				CURRENT_STATE.REGS[RD (inst)] = val;
			    break;
			}
		    case 0x23:	//SUBU
			CURRENT_STATE.REGS[RD(inst)] = CURRENT_STATE.REGS[RS(inst)]-CURRENT_STATE.REGS[RT(inst)];
			break;

		    case 0x8:	//JR
			{
			    uint32_t tmp = CURRENT_STATE.REGS[RS (inst)];
			    JUMP_INST (tmp);

			    break;
			}
		    default:
			printf("Unknown function code type: %d\n", FUNC(inst));
			break;
		}
	    }
	    break;

	case 0x2:		//(0x000010)J
	    JUMP_INST (((CURRENT_STATE.PC & 0xf0000000) | TARGET (inst) << 2));
	    break;
	case 0x3:		//(0x000011)JAL
	    CURRENT_STATE.REGS[31] = CURRENT_STATE.PC;
	    JUMP_INST (((CURRENT_STATE.PC & 0xf0000000) | (TARGET (inst) << 2)));
	    break;

	default:
	    printf("Unknown instruction type: %d\n", OPCODE(inst));
	    break;
    }

    if (CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= (MEM_REGIONS[0].start + (NUM_INST * 4)))
	RUN_BIT = FALSE;
}
#endif
