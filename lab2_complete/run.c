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
uint32_t* get_inst_info(uint32_t pc)
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
void process_instruction(bool forwardingEnabled){
    
    process_IF();
    process_ID();
    process_EX(forwardingEnabled);
    process_MEM();
    process_WB();
    
    
    // buffer -> Current CPU state
    CURRENT_STATE.IF_ID_pipeline = IF_ID_pipeline_buffer;
    CURRENT_STATE.ID_EX_pipeline = ID_EX_pipeline_buffer;
    CURRENT_STATE.EX_MEM_pipeline = EX_MEM_pipeline_buffer;
    CURRENT_STATE.MEM_WB_pipeline = MEM_WB_pipeline_buffer;
    
}




/*
 Assumes PC is updated
 
 */

void process_IF() {
    // PC
    IF_ID_pipeline_buffer.NPC = CURRENT_STATE.PC + 4;
    
    // Instruction Fetch
    uint32_t *inst = get_inst_info(CURRENT_STATE.PC);;
    IF_ID_pipeline_buffer.instr = inst; // both are pointers
}



/**************************************************************
 input :    IF/ID.REG1
            IF/ID.REG2
            MEM/WB.WriteReg
            WB/.WriteData
 
 output:
 **************************************************************/
void process_ID(){
    IF/ID prevIF_ID_pipeline = CURRENT_STATE.IF_ID_pipeline;
    uint32_t inst = prevIF_ID_pipeline.instr;
    
    // TODO : control signals
    generate_control_signals(inst);
    
    
    
    // Read register rs and rt
    ID_EX_pipeline_buffer.REG1 = CURRENT_STATE.REGS[RS(inst)];
    ID_EX_pipeline_buffer.Reg2 = CURRENT_STATE.REGS[RT(inst)];

    // sign-extend Immediate value
    ID_EX_pipeline_buffer.IMM = SIGN_EX(IMM(inst));
    
    // transfer possible register write destinations (11-15 and 16-20)
    ID_EX_pipeline_buffer.inst11_15 = RD(inst); // 11-15
    ID_EX_pipeline_buffer.inst16_20 = RT(inst); // 16-20
    
}


void generate_control_signals(uint32_t instr){
    bool jump = false;
    
    if (OPCODE(instr) == 0) {                               // R-type
        ID_EX_pipeline_buffer.RegDst = 1;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
        switch (FUNCT(instr)) {
            case 0x21:                          // addu
                ID_EX_pipeline_buffer.ALUControl = 0;
                break;
            case 0x24:                          // and
                ID_EX_pipeline_buffer.ALUControl = 2;
                break;
            case 0x08:                          // jr
                jump = true;
                // dont care
                break;
            case 0x27:                          // nor
                ID_EX_pipeline_buffer.ALUControl = 7;
                break;
            case 0x25:                          // or
                ID_EX_pipeline_buffer.ALUControl = 3;
                break;
            case 0x2b:                          // sltu
                ID_EX_pipeline_buffer.ALUControl = 8;
                break;
            case 0x00:                          // sll
                ID_EX_pipeline_buffer.ALUControl = 4;
                break;
            case 0x02:                          // srl
                ID_EX_pipeline_buffer.ALUControl = 5;
                break;
            case 0x23:                          // subu
                ID_EX_pipeline_buffer.ALUControl = 1;
                break;
                
            default:
                printf("Unrecognized input in 'generate_control_signals' with input : %d", instr);
                break;
        }                                                   // J-type
    } else if (OPCODE(instr) == 2){             // j
        jump = true;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
    } else if (OPCODE(instr) == 3){             // jal
        jump = true;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
                                                            // I-type
    } else if (OPCODE(instr) == 0x23){          // lw
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 1;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 1;
        
    } else if (OPCODE(instr) == 0x2B){          // sw
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 1;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        
    } else if (OPCODE(instr) == 0x4){           // beq
        D_EX_pipeline_buffer.ALUControl = 1;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        
    } else if (OPCODE(instr) == 0x5){           // bne
        D_EX_pipeline_buffer.ALUControl = 11;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        
    } else if (OPCODE(instr) == 0x9){           // addiu
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
    } else if (OPCODE(instr) == 0xC){           // andi
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 2;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
    } else if (OPCODE(instr) == 0xF){           // lui
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 10;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
    } else if (OPCODE(instr) == 0xD){           // ori
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 3;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
    } else if (OPCODE(instr) == 0xB){           // sltiu
        ID_EX_pipeline_buffer.RegDst = 0;
        ID_EX_pipeline_buffer.ALUControl = 8;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        
    } else {
        printf("Unrecognized OPCODE : %d", OPCODE(instr));
    }
    
    
    if (jump) {
        // *TO DO*
        // DO FLUSH
        
        // set jump buffer
        ID_EX_pipeline_buffer.jump = true;
    } else {
        ID_EX_pipeline_buffer.jump = false;
    }
    
}









void process_EX(bool forwardingEnabled){
    
    
    ID/EX prevID_EX_pipeline = CURRENT_STATE.ID_EX_pipeline;
    
    // shift the pipelined control signals
    EX_MEM_pipeline_buffer.WB_MemToReg = prevID_EX_pipeline.WB_MemToReg;
    EX_MEM_pipeline_buffer.WB_RegToWrite = prevID_EX_pipeline.WB_RegToWrite;
    
    EX_MEM_pipeline_buffer.MemWrite = prevID_EX_pipeline.MEM_MemWrite;
    EX_MEM_pipeline_buffer.MemRead = prevID_EX_pipeline.MEM_MemRead;
    EX_MEM_pipeline_buffer.Branch = prevID_EX_pipeline.MEM_Branch;
    
    // PC (for J and JR instruction)
    EX_MEM_pipeline_buffer.NPC = prevID_EX_pipeline.NPC + (prevID_EX_pipeline.IMM << 2);
    
    if (prevID_EX_pipeline.jump) {
        <#statements#>
    }
    
    int forwardA = 00;
    int forwardB = 00;
    
    // Forwarding unit.
    // EX hazard
    if (forwardingEnabled) {
        
    
    if (CURRENT_STATE.EX_MEM_pipeline.RegWrite) {
        if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum != 0) {
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG1) {
                forwardA = 10;
            }
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG2) {
                forwardB = 10;
            }
        }
    }
    
    // MEM hazard
    if (CURRENT_STATE.MEM_WB_pipeline.RegWrite) {
        if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum != 0) {
            if !(CURRENT_STATE.EX_MEM_pipeline.RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG1)) {
                
                if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG1) {
                    forwardA = 01;
                }
            }
            
            if !(CURRENT_STATE.EX_MEM_pipeline.RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG2)) {
                
                if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.REG2) {
                    forwardB = 01;
                }
            }
        }
    }
    
    } // end of "forwardingEnabled"
    
    
        
    // For this section, refer to figure 4.57 !!! Other figures are wrong!
    // select ALU input
    uint32_t ALUinput1;
    uint32_t ALUinput2;
    
    // 3 to 1 MUX
    if (forwardA == 00) {
        ALUinput1 = prevID_EX_pipeline.data1;
    } else if (forwardA == 10) {
        ALUinput1 = CURRENT_STATE.EX_MEM_pipeline.ALU_OUT;
    } else if (forwardA == 01) {
        // MUX for which data to write.
        uint32_t writeData;
        if (CURRENT_STATE.MEM_WB_pipeline.MemToReg) {
            writeData = CURRENT_STATE.MEM_WB_pipeline.Mem_OUT;
        } else {
            writeData = CURRENT_STATE.MEM_WB_pipeline.ALU_OUT;
        }
        ALUinput1 = writeData;
    } else {
        printf("unrecognized fowardA signal : %d", fowardA);
    }
    
    // 3 to 1 MUX
    if (forwardB == 00) {
        ALUinput2 = prevID_EX_pipeline.data2;
    } else if (forwardB == 10) {
        ALUinput2 = CURRENT_STATE.EX_MEM_pipeline.ALU_OUT;
    } else if (forwardB == 01){
        // MUX for which data to write.
        uint32_t writeData;
        if (CURRENT_STATE.MEM_WB_pipeline.MemToReg) {
            writeData = CURRENT_STATE.MEM_WB_pipeline.Mem_OUT;
        } else {
            writeData = CURRENT_STATE.MEM_WB_pipeline.ALU_OUT;
        }
        ALUinput2 = writeData;
    } else {
        printf("unrecognized fowardB signal : %d", fowardB);
    }
    
    // Another MUX inside
    // if ALUSrc == 1, the second ALU operand is the sign-extended, lower 16 bits of the instruction.
    // if ALUSrc == 0, the second ALU operand comes from the second register file output. (Read data 2).
    if (prevID_EX_pipeline.ALUSrc) {
        ALUinput2 = prevID_EX_pipeline.IMM;
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

void process_MEM(bool forwardingEnabled){
    EX/MEM prevEX_MEM_pipeline = CURRENT_STATE.EX_MEM_pipeline;
    
    // shift the pipelined control signals
    MEM_WB_pipeline_buffer.MemToReg = prevEX_MEM_pipeline.WB_MemToReg;
    MEM_WB_pipeline_buffer.RegWrite = prevEX_MEM_pipeline.WB_RegWrite;
    MEM_WB_pipeline_buffer.MemRead = prevEX_MEM_pipeline.MemRead;
    
    // branching
    if (prevEX_MEM_pipeline.zero & prevEX_MEM_pipeline.Branch) {
        CURRENT_STATE.PC = prevEX_MEM_pipeline.NPC;
    } else {
        CURRENT_STATE.PC += 4;
    }
    
    
    
    
    // forwarding
    bool forwardingDone = false;
    
    if (forwardingEnabled) {
        if (CURRENT_STATE.MEM_WB_pipeline.MemRead & CURRENT_STATE.EX_MEM_pipeline.MemWrite) {
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.MEM_WB_pipeline.RegDstNum) {
                mem_write_32(prevID_EX_pipeline.ALU_OUT, CURRENT_STATE.MEM_WB_pipeline.Mem_OUT);
                forwardingDone = true;
            }
        }
    }
    
    // Read data from memory
    if (prevID_EX_pipeline.MemRead) {
        MEM_WB_pipeline_buffer.Mem_OUT = mem_read_32(prevID_EX_pipeline.ALU_OUT);
    } else {
        MEM_WB_pipeline_buffer.ALU_OUT = prevID_EX_pipeline.ALU_OUT;
    }
    
    // Write data to memory
    if (prevID_EX_pipeline.MemWrite & (!forwardingDone)) {
        mem_write_32(prevID_EX_pipeline.ALU_OUT, prevID_EX_pipeline.data2);
    }
    
    // shift Register destination number (on write)
    MEM_WB_pipeline_buffer.RegDstNum = prevID_EX_pipeline.RegDstNum;
    
}

void process_WB(){
    MEM/WB prevMEM_WB_pipeline = CURRENT_STATE.MEM_WB_pipeline;
    
    // MUX for which data to write.
    uint32_t writeData;
    if (prevMEM_WB_pipeline.MemToReg) {
        writeData = prevMEM_WB_pipeline.Mem_OUT;
    } else {
        writeData = prevMEM_WB_pipeline.ALU_OUT;
    }
    
    // Write data if (RegWrite == 1)
    if (prevMEM_WB_pipeline.RegWrite) {
        CURRENT_STATE.REGS[prevMEM_WB_pipeline.RegDstNum] = writeData;
    }
    
}

// control line ranges from 0 to 10 (4digits)
uint32_t ALU(int control_line, uint32_t data1, uint32_t data2)) {
    if (control_line == 0) {        // 0000 : add
        return data1 + data2;
    } else if (control_line == 1) { // 0001 : sub
        return data1 - data2;
    } else if (control_line == 2) { // 0010 : AND
        return data1 & data2;
    } else if (control_line == 3) { // 0011 : OR
        return data1 | data2;
    } else if (control_line == 4) { // 0100 : SLL (shift left logical)
        return data1 << data2;
    } else if (control_line == 5) { // 0101 : SRL (shift right logical)
        return data1 >> data2;
    } else if (control_line == 7) { // 0111 : NOR
        return ~(data1 | data2);
    } else if (control_line == 7) { // 1000 : SLT (set on less than)
        return data1 < data2;
    } else if (control_line == 12) { // 1010 : LUI
        return (data2<<16);
    } else if (control_line == ) {  // 1011 : not equal
        return !(data1-data2);
    }
    else {
        printf("Error in ALU. ALU control line value is : %d", control_line);
    }
}



// ALU Control is not used!
/*
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
*/






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
