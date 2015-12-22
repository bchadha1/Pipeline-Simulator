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
    process_MEM(forwardingEnabled);
    process_WB();
    
    
    // buffer -> Current CPU state
    
    if (!stallcount2) {
        CURRENT_STATE.IF_ID_pipeline = IF_ID_pipeline_buffer;
    } else {
        stallcount2--;
    }
    CURRENT_STATE.ID_EX_pipeline = ID_EX_pipeline_buffer;
    CURRENT_STATE.EX_MEM_pipeline = EX_MEM_pipeline_buffer;
    CURRENT_STATE.MEM_WB_pipeline = MEM_WB_pipeline_buffer;
    if (!stallcount) {
        CURRENT_STATE.PC = PC_buffer;
    }
    
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
    // check stall
    if (stallcount) {
        CURRENT_STATE.IF_ID_pipeline.instr = 0; // no-op
        stallcount--;
    }
    
    IF/ID prevIF_ID_pipeline = CURRENT_STATE.IF_ID_pipeline;
    uint32_t inst = prevIF_ID_pipeline.instr;
    
    // control signals
    generate_control_signals(inst);
    
    
    
    // Read register rs and rt
    ID_EX_pipeline_buffer.REG1 = CURRENT_STATE.REGS[RS(inst)];
    ID_EX_pipeline_buffer.REG2 = CURRENT_STATE.REGS[RT(inst)];

    // sign-extend Immediate value
    ID_EX_pipeline_buffer.IMM = SIGN_EX(IMM(inst));
    
    // transfer possible register write destinations (11-15 and 16-20)
    ID_EX_pipeline_buffer.inst11_15 = RD(inst); // 11-15
    ID_EX_pipeline_buffer.inst16_20 = RT(inst); // 16-20
    
    if (globaljal) {
        ID_EX_pipeline_buffer.REG1 = 0;
        ID_EX_pipeline_buffer.REG2 = 0;
        ID_EX_pipeline_buffer.IMM = CURRENT_STATE.PC+4;
        ID_EX_pipeline_buffer.inst11_15 = 31;
    }
    
}


void generate_control_signals(uint32_t instr){
    bool jump = false;
    bool jal = false;
    bool jumpandreturn = false;
    bool ALUinstruction = true;
    
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
                jumpandreturn = true;
                stallcount = 1;
                ALUinstruction = false;
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
        stallcount = 1;
        ALUinstruction = false;
        
    } else if (OPCODE(instr) == 3){             // jal
        jump = true;
        ID_EX_pipeline_buffer.RegDst = 1;
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        jal = true;
        stallcount = 1;
        ALUinstruction = false;
        
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
        ALUinstruction = false;
        
    } else if (OPCODE(instr) == 0x2B){          // sw
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 1;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        ALUinstruction = false;
        
    } else if (OPCODE(instr) == 0x4){           // beq
        D_EX_pipeline_buffer.ALUControl = 1;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        stallcount = 3;
        
    } else if (OPCODE(instr) == 0x5){           // bne
        D_EX_pipeline_buffer.ALUControl = 11;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        stallcount = 3;
        
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
        
        
        // set jump varibale
        globaljump = true;
    } else {
        globaljump = false;
    }
    
    if (jal) {
        globaljal = true;
    } else {
        globaljal = false;
    }
    
    if (jumpandreturn) {
        globalJumpAndReturn = true;
    } else {
        globalJumpAndReturn = false;
    }
    
    
    if (ALUinstruction) {
        if (CURRENT_STATE.ID_EX_pipeline.MEM_MemRead) {
            if ((CURRENT_STATE.ID_EX_pipeline.inst16_20 == RS(instr)) |(CURRENT_STATE.ID_EX_pipeline.inst16_20 == RT(instr))  ) {
                if (forwardingEnabled) {
                    stallcount2 = 1;
                } else {
                    stallcount2 = 2;
                }
            }
        }
    }
}









void process_EX(bool forwardingEnabled){
    if (stallcount2) { // no-op
        CURRENT_STATE.ID_EX_pipeline.NPC = 0;
        CURRENT_STATE.ID_EX_pipeline.WB_MemToReg = 0;
        CURRENT_STATE.ID_EX_pipeline.WB_RegWrite = 0;
        CURRENT_STATE.ID_EX_pipeline.MEM_MemWrite = 0;
        CURRENT_STATE.ID_EX_pipeline.MEM_MemRead = 0;
        CURRENT_STATE.ID_EX_pipeline.MEM_Branch = 0;
        CURRENT_STATE.ID_EX_pipeline.RegDst = 0;
        CURRENT_STATE.ID_EX_pipeline.ALUControl = 0;
        CURRENT_STATE.ID_EX_pipeline.ALUSrc = 0;
        CURRENT_STATE.ID_EX_pipeline.jump = 0;
        CURRENT_STATE.ID_EX_pipeline.REG1 = 0;
        CURRENT_STATE.ID_EX_pipeline.REG2 = 0;
        CURRENT_STATE.ID_EX_pipeline.IMM = 0;
        CURRENT_STATE.ID_EX_pipeline.inst16_20 = 0;
        CURRENT_STATE.ID_EX_pipeline.inst11_15 = 0;
    }
    
    ID/EX prevID_EX_pipeline = CURRENT_STATE.ID_EX_pipeline;
    
    // shift the pipelined control signals
    EX_MEM_pipeline_buffer.WB_MemToReg = prevID_EX_pipeline.WB_MemToReg;
    EX_MEM_pipeline_buffer.WB_RegToWrite = prevID_EX_pipeline.WB_RegToWrite;
    
    EX_MEM_pipeline_buffer.MemWrite = prevID_EX_pipeline.MEM_MemWrite;
    EX_MEM_pipeline_buffer.MemRead = prevID_EX_pipeline.MEM_MemRead;
    EX_MEM_pipeline_buffer.Branch = prevID_EX_pipeline.MEM_Branch;
    
    // PC (for J and JR instruction)
    EX_MEM_pipeline_buffer.NPC = prevID_EX_pipeline.NPC + (prevID_EX_pipeline.IMM << 2);
    
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
    MEM_WB_pipeline_buffer.NPC = prevEX_MEM_pipeline.NPC;
    
    // branching
    uint32_t inst = prevIF_ID_pipeline.instr;
    
    if (jump) {
        PC_buffer = ((CURRENT_STATE.PC+4)&0xF0000000) + (J250(inst)<<2);
        if (globalJumpAndReturn) {
            PC_buffer = CURRENT_STATE.REGS[31];
        }
        
    } else {
        if (prevEX_MEM_pipeline.zero & prevEX_MEM_pipeline.Branch) {
            PC_buffer = prevEX_MEM_pipeline.NPC;
        } else {
            PC_buffer = CURRENT_STATE + 4;
        }
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



