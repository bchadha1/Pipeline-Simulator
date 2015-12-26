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

int IsDebug = 0; // for printing debugging outputs

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
uint32_t get_inst_info(uint32_t pc)
{
    return INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(bool forwardingEnabled, bool branchPredictionEnabled){
    
    // buffer -> Current CPU state
    if(PC_jump){
        CURRENT_STATE.PC = PC_jump;
        PC_jump = 0;
    }
    if(!stall_ID_EX_count && !stall_IF_ID_count) CURRENT_STATE.PC = PC_buffer;
    if(!stall_ID_EX_count) CURRENT_STATE.IF_ID_pipeline = IF_ID_pipeline_buffer;
    
    CURRENT_STATE.ID_EX_pipeline = ID_EX_pipeline_buffer;
    CURRENT_STATE.EX_MEM_pipeline = EX_MEM_pipeline_buffer;
    CURRENT_STATE.MEM_WB_pipeline = MEM_WB_pipeline_buffer;
    
    if(stall_IF_ID_count && !stall_ID_EX_count) flush_IF_ID();
    // when both are enabled. We shouldn't flush IF/ID pipeline.
    if(stall_ID_EX_count) flush_ID_EX();
    
    if(branchFlush){
        //printf("LOOK HERE@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        //printf("branchFlush is activated. Flushing IF/ID, ID/EX, EX/MEM... \n");
        flush_IF_ID();
        flush_ID_EX();
        flush_EX_MEM();
        branchFlush = false;
    }
    
    process_IF();
    process_WB();
    
    
    
    process_ID(forwardingEnabled, branchPredictionEnabled); // Data Hazards are detected here (if forwarding is not enabled)
    process_EX(forwardingEnabled);                          // Data Forwarding is done here (if fowarding is enabled)
    process_MEM(forwardingEnabled, branchPredictionEnabled);
    
    
    if(stall_ID_EX_count) stall_ID_EX_count--;
    if(stall_IF_ID_count) stall_IF_ID_count--;
    if(IsDebug) printf("*cycle: stall_IF_ID %d, stall_ID_EX %d, branchFlush %d\n", stall_IF_ID_count, stall_ID_EX_count, branchFlush);
}


/*
 Assumes PC is updated
 */

void process_IF() {
    // PC
    PC_buffer = CURRENT_STATE.PC + 4;
    IF_ID_pipeline_buffer.CURRENTPC = CURRENT_STATE.PC;
    IF_ID_pipeline_buffer.NPC = PC_buffer;
    
    // Instruction Fetch
    uint32_t inst = get_inst_info(CURRENT_STATE.PC);
    IF_ID_pipeline_buffer.instr = inst; // both are pointers
    if(IsDebug) printf("*debug process_IF: CURRENTPC 0x%08x, instr 0x%08x\n", IF_ID_pipeline_buffer.CURRENTPC, inst);
}



/**************************************************************
 input :    IF/ID.REG1
 IF/ID.REG2
 MEM/WB.WriteReg
 WB/.WriteData
 
 output:
 **************************************************************/
void process_ID(bool forwardingEnabled, bool branchPredictionEnabled){
    
    IF_ID prevIF_ID_pipeline = CURRENT_STATE.IF_ID_pipeline;
    ID_EX_pipeline_buffer.NPC = prevIF_ID_pipeline.NPC;
    ID_EX_pipeline_buffer.CURRENTPC = prevIF_ID_pipeline.CURRENTPC;
    
    uint32_t inst = prevIF_ID_pipeline.instr;
    ID_EX_pipeline_buffer.instr_debug = inst;
    
    
    // control signals
    generate_control_signals(inst, forwardingEnabled, branchPredictionEnabled);
    
    
    
    // Read register rs and rt
    ID_EX_pipeline_buffer.REG1 = CURRENT_STATE.REGS[RS(inst)];
    ID_EX_pipeline_buffer.REG2 = CURRENT_STATE.REGS[RT(inst)];
    
    // sign-extend Immediate value
    if(OPCODE(inst) == 0xC || OPCODE(inst) == 0xD) ID_EX_pipeline_buffer.IMM = ZERO_EX(IMM(inst));
    else ID_EX_pipeline_buffer.IMM = SIGN_EX(IMM(inst));
    
    // transfer possible register write destinations (11-15 and 16-20)
    ID_EX_pipeline_buffer.RS = RS(inst); // inst [21-25]
    ID_EX_pipeline_buffer.RT = RT(inst); // inst [16-20]
    ID_EX_pipeline_buffer.RD = RD(inst); // inst [11-15]
    
    if (globaljal) {
        ID_EX_pipeline_buffer.REG1 = 0;
        ID_EX_pipeline_buffer.REG2 = 0;
        ID_EX_pipeline_buffer.IMM = CURRENT_STATE.PC+4;
        ID_EX_pipeline_buffer.RD = 31;
    }
    if(IsDebug){
        printf("*debug process_ID: CURRENTPC 0x%08x, instr 0x%08x\n", ID_EX_pipeline_buffer.CURRENTPC, inst);
        printf("    RS %d, REG1 %d, RT %d, REG2 %d, RD %d, imm %08x\n", RS(inst), ID_EX_pipeline_buffer.REG1, RT(inst), ID_EX_pipeline_buffer.REG2, RD(inst), ID_EX_pipeline_buffer.IMM);
    }
}


void generate_control_signals(uint32_t instr, bool forwardingEnabled, bool branchPredictionEnabled){
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
                stall_IF_ID_count = 1+1;
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
                printf("Unrecognized input in 'generate_control_signals' with input : %08x", instr);
                break;
        }                                                   // J-type
    } else if (OPCODE(instr) == 2){             // j
        jump = true;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        stall_IF_ID_count = 1+1;
        ALUinstruction = false;
        
    } else if (OPCODE(instr) == 3){             // jal
        jump = true;
        ID_EX_pipeline_buffer.RegDst = 1;
        ID_EX_pipeline_buffer.ALUControl = 0;
        ID_EX_pipeline_buffer.ALUSrc = 1;
        ID_EX_pipeline_buffer.MEM_Branch = 0;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 1;
        ID_EX_pipeline_buffer.WB_MemToReg = 0;
        jal = true;
        stall_IF_ID_count = 1+1;
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
        ID_EX_pipeline_buffer.ALUControl = 1;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        if(!branchPredictionEnabled) stall_IF_ID_count = 3+1;
        
    } else if (OPCODE(instr) == 0x5){           // bne
        ID_EX_pipeline_buffer.ALUControl = 11;
        ID_EX_pipeline_buffer.ALUSrc = 0;
        ID_EX_pipeline_buffer.MEM_Branch = 1;
        ID_EX_pipeline_buffer.MEM_MemRead = 0;
        ID_EX_pipeline_buffer.MEM_MemWrite = 0;
        ID_EX_pipeline_buffer.WB_RegWrite = 0;
        if(!branchPredictionEnabled) stall_IF_ID_count = 3+1;
        
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
        PC_jump = ((CURRENT_STATE.PC+4)&0xF0000000) + (J250(instr)<<2);
        if (globalJumpAndReturn) {
            PC_jump = CURRENT_STATE.REGS[31];
        }
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
    
    if (!forwardingEnabled) {
        // EX hazard
        if (CURRENT_STATE.ID_EX_pipeline.WB_RegWrite) {
            uint32_t tempRegDstNum;
            if (CURRENT_STATE.ID_EX_pipeline.RegDst) {
                tempRegDstNum = CURRENT_STATE.ID_EX_pipeline.RD;
            } else {
                tempRegDstNum = CURRENT_STATE.ID_EX_pipeline.RT;
            }
            
            if (tempRegDstNum != 0) {
                if (tempRegDstNum == RS(instr)) {
                    if(IsDebug) printf("generate_control signal : EX Hazard detected!!!!! \n");
                    stall_ID_EX_count = 1+2;
                }
                if (tempRegDstNum == RT(instr)) {
                    if(IsDebug) printf("generate_control signal : EX Hazard detected!!!!! \n");
                    stall_ID_EX_count = 1+2;
                }
            }
        }

        
        // MEM hazard
        
        if (CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite) {
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum != 0) {
                
                uint32_t tempRegDstNum; // ID/EX's RegDstNum
                if (CURRENT_STATE.ID_EX_pipeline.RegDst) {
                    tempRegDstNum = CURRENT_STATE.ID_EX_pipeline.RD;
                } else {
                    tempRegDstNum = CURRENT_STATE.ID_EX_pipeline.RT;
                }
                
                
                if (!(CURRENT_STATE.ID_EX_pipeline.WB_RegWrite && (tempRegDstNum !=0) && (tempRegDstNum == RS(instr)))) {
                    if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == RS(instr)) {
                        if(IsDebug) printf("generate_control signal : MEM Hazard detected!!!!! \n");
                        stall_ID_EX_count = 1+1;
                    }
                }
                
                if (!(CURRENT_STATE.ID_EX_pipeline.WB_RegWrite && (tempRegDstNum !=0) && (tempRegDstNum == RT(instr)))) {
                    
                    if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == RT(instr)) {
                        if(IsDebug) printf("generate_control signal : MEM Hazard detected!!!!! \n");
                        stall_ID_EX_count = 1+1;
                    }
                }
            }
        }
        
        
        // original
        /*
        if (CURRENT_STATE.MEM_WB_pipeline.RegWrite) {
            if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum != 0) {
                if (!(CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RS))) {
                    
                    if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RS) {
                        if(IsDebug) printf("generate_control signal : MEM Hazard detected!!!!! \n");
                        stall_IF_ID_count = 1+2;
                    }
                }
                
                if (!(CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RT))) {
                    
                    if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RT) {
                        if(IsDebug) printf("generate_control signal : MEM Hazard detected!!!!! \n");
                        stall_IF_ID_count = 1+2;
                    }
                }
            }
        }
         */
    }
    
    
    
    // ALU operation followed by lw instruction
    if (ALUinstruction) {
        if (CURRENT_STATE.ID_EX_pipeline.MEM_MemRead) {
            if ((CURRENT_STATE.ID_EX_pipeline.RT == RS(instr)) |(CURRENT_STATE.ID_EX_pipeline.RT == RT(instr))  ) {
                if (forwardingEnabled) {
                    stall_ID_EX_count = 1+1;
                } else {
                    stall_ID_EX_count = 2+1;
                }
            }
        }
    }
    if(IsDebug) {
        printf("*debug generate_control_signals: jump=%d, jl=%d, jr=%d, alu=%d\n", jump, jal, jumpandreturn, ALUinstruction);
        printf("    RegDst %d, ALUControl %d, ALUSrc %d\n", ID_EX_pipeline_buffer.RegDst, ID_EX_pipeline_buffer.ALUControl, ID_EX_pipeline_buffer.ALUSrc);
        printf("    Branch %d, MemRead %d, MemWrite %d\n", ID_EX_pipeline_buffer.MEM_Branch, ID_EX_pipeline_buffer.MEM_MemRead, ID_EX_pipeline_buffer.MEM_MemWrite);
        printf("    RegWrite %d, MemToReg %d\n", ID_EX_pipeline_buffer.WB_RegWrite, ID_EX_pipeline_buffer.WB_MemToReg);
        printf("    stall_IF_ID_count %d, stall_ID_EX_count %d\n", stall_IF_ID_count, stall_ID_EX_count);
        printf("    PC_jump %08x\n", PC_jump);
    }
}


void process_EX(bool forwardingEnabled){
    
    ID_EX prevID_EX_pipeline = CURRENT_STATE.ID_EX_pipeline;
    EX_MEM_pipeline_buffer.instr_debug = CURRENT_STATE.ID_EX_pipeline.instr_debug;
    
    // shift the pipelined control signals
    EX_MEM_pipeline_buffer.CURRENTPC = prevID_EX_pipeline.CURRENTPC;
    EX_MEM_pipeline_buffer.WB_MemToReg = prevID_EX_pipeline.WB_MemToReg;
    EX_MEM_pipeline_buffer.WB_RegWrite = prevID_EX_pipeline.WB_RegWrite;
    
    EX_MEM_pipeline_buffer.MemWrite = prevID_EX_pipeline.MEM_MemWrite;
    EX_MEM_pipeline_buffer.MemRead = prevID_EX_pipeline.MEM_MemRead;
    EX_MEM_pipeline_buffer.Branch = prevID_EX_pipeline.MEM_Branch;
    
    // PC (for Branch instruction)
    EX_MEM_pipeline_buffer.NPC = prevID_EX_pipeline.NPC + (prevID_EX_pipeline.IMM << 2);
    
    int forwardA = 00;
    int forwardB = 00;
    
    // Forwarding unit.
    if (forwardingEnabled) {
        if(IsDebug) printf("RegDstNum %d, RS %d, RT %d\n", CURRENT_STATE.EX_MEM_pipeline.RegDstNum, CURRENT_STATE.ID_EX_pipeline.RS, CURRENT_STATE.ID_EX_pipeline.RT);
        
        // EX hazard
        if (CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite) {
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum != 0) {
                if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RS) {
                    if(IsDebug) printf("EX Hazard detected!!!!! \n");
                    forwardA = 10;
                }
                if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RT) {
                    if(IsDebug) printf("EX Hazard detected!!!!! \n");
                    forwardB = 10;
                }
            }
        }
        
        // MEM hazard
        if (CURRENT_STATE.MEM_WB_pipeline.RegWrite) {
            if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum != 0) {
                if (!(CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RS))) {
                    
                    if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RS) {
                        if(IsDebug) printf("MEM Hazard detected!!!!! \n");
                        forwardA = 01;
                    }
                }
                
                if (!(CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum !=0) && (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RT))) {
                    
                    if (CURRENT_STATE.MEM_WB_pipeline.RegDstNum == CURRENT_STATE.ID_EX_pipeline.RT) {
                        if(IsDebug) printf("MEM Hazard detected!!!!! \n");
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
        ALUinput1 = prevID_EX_pipeline.REG1;
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
        printf("unrecognized fowardA signal : %d", forwardA);
    }
    
    // 3 to 1 MUX
    if (forwardB == 00) {
        ALUinput2 = prevID_EX_pipeline.REG2;
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
        printf("unrecognized fowardB signal : %d", forwardB);
    }
    
    // transfer data2
    EX_MEM_pipeline_buffer.data2 = ALUinput2;
    
    // Another MUX inside
    // if ALUSrc == 1, the second ALU operand is the sign-extended, lower 16 bits of the instruction.
    // if ALUSrc == 0, the second ALU operand comes from the second register file output. (Read data 2).
    if (prevID_EX_pipeline.ALUSrc) {
        ALUinput2 = prevID_EX_pipeline.IMM;
    }
    
    
    
    // calculate ALU control
    int funct_field = (prevID_EX_pipeline.IMM & 63); // extract funct field from instruction[0~15]
    int control_line = prevID_EX_pipeline.ALUControl;
    
    // process ALU
    uint32_t ALUresult = ALU(control_line, ALUinput1, ALUinput2);
    EX_MEM_pipeline_buffer.zero = !ALUresult;
    EX_MEM_pipeline_buffer.ALU_OUT = ALUresult;
    
    // Mux for Write Register
    // if RegDst == 1, the register destination number for the Write register comes from the rd field.(bits 15:11)
    // if RegDst == 0, the register destination number for the Write register comes from the rt field.(bits 20:16)
    if (prevID_EX_pipeline.RegDst) {
        EX_MEM_pipeline_buffer.RegDstNum = prevID_EX_pipeline.RD;
    } else {
        EX_MEM_pipeline_buffer.RegDstNum = prevID_EX_pipeline.RT;
    }
    if(IsDebug) {
        printf("*debug process_EX: CURRENTPC 0x%08x, instr %08x\n", EX_MEM_pipeline_buffer.CURRENTPC, EX_MEM_pipeline_buffer.instr_debug);
        printf("    forwardA %d, forwardB %d, ALUinput1 %d, ALUinput2 %d, ALU_OUT %d\n", forwardA, forwardB, ALUinput1, ALUinput2, ALUresult);
    }
}

void process_MEM(bool forwardingEnabled, bool branchPredictionEnabled){
    EX_MEM prevEX_MEM_pipeline = CURRENT_STATE.EX_MEM_pipeline;
    MEM_WB_pipeline_buffer.instr_debug = prevEX_MEM_pipeline.instr_debug;
    
    // shift the pipelined control signals
    MEM_WB_pipeline_buffer.MemToReg = prevEX_MEM_pipeline.WB_MemToReg;
    MEM_WB_pipeline_buffer.RegWrite = prevEX_MEM_pipeline.WB_RegWrite;
    MEM_WB_pipeline_buffer.MemRead = prevEX_MEM_pipeline.MemRead;
    MEM_WB_pipeline_buffer.CURRENTPC = prevEX_MEM_pipeline.CURRENTPC;
    
    // branching
    if(prevEX_MEM_pipeline.Branch) { // branch condition
        if(prevEX_MEM_pipeline.zero){
            PC_jump = prevEX_MEM_pipeline.NPC;
            if(branchPredictionEnabled) branchFlush = true;
        }
    }
    
    // forwarding
    bool forwardingDone = false;
    
    if (forwardingEnabled) {
        if (CURRENT_STATE.MEM_WB_pipeline.MemRead & CURRENT_STATE.EX_MEM_pipeline.MemWrite) {
            if (CURRENT_STATE.EX_MEM_pipeline.RegDstNum == CURRENT_STATE.MEM_WB_pipeline.RegDstNum) {
                mem_write_32(prevEX_MEM_pipeline.ALU_OUT, CURRENT_STATE.MEM_WB_pipeline.Mem_OUT);
                forwardingDone = true;
            }
        }
    }
    
    // Read data from memory
    if (prevEX_MEM_pipeline.MemRead) {
        MEM_WB_pipeline_buffer.Mem_OUT = mem_read_32(prevEX_MEM_pipeline.ALU_OUT);
    } else {
        MEM_WB_pipeline_buffer.ALU_OUT = prevEX_MEM_pipeline.ALU_OUT;
    }
    
    // Write data to memory
    if (prevEX_MEM_pipeline.MemWrite & (!forwardingDone)) {
        mem_write_32(prevEX_MEM_pipeline.ALU_OUT, prevEX_MEM_pipeline.data2);
    }
    
    // shift Register destination number (on write)
    MEM_WB_pipeline_buffer.RegDstNum = prevEX_MEM_pipeline.RegDstNum;
    if(IsDebug){
        printf("*debug process_MEM: CURRENTPC 0x%08x, instr %08x\n", MEM_WB_pipeline_buffer.CURRENTPC, MEM_WB_pipeline_buffer.instr_debug);
        printf("    jump %d, Branch %d, zero %d, PC_jump 0x%08x\n", globaljump, prevEX_MEM_pipeline.Branch, prevEX_MEM_pipeline.zero, PC_jump);
    }
}

void process_WB(){
    MEM_WB prevMEM_WB_pipeline = CURRENT_STATE.MEM_WB_pipeline;
    if (prevMEM_WB_pipeline.instr_debug) { // not no-op
        INSTRUCTION_COUNT++;
    }
    
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
    if(IsDebug) printf("*debug process_WB: PC 0x%08x, instr %08x\n", prevMEM_WB_pipeline.CURRENTPC, prevMEM_WB_pipeline.instr_debug);
}

// control line ranges from 0 to 10 (4digits)
uint32_t ALU(int control_line, uint32_t data1, uint32_t data2) {
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
    } else if (control_line == 8) { // 1000 : SLT (set on less than)
        return data1 < data2;
    } else if (control_line == 10) { // 1010 : LUI
        return (data2<<16);
    } else if (control_line == 11) {  // 1011 : not equal
        return !(data1-data2);
    }
    else {
        printf("Error in ALU. ALU control line value is : %d", control_line);
    }
    return 0;
}




// Flush functions
void flush_IF_ID(){
    CURRENT_STATE.IF_ID_pipeline.NPC = 0;
    CURRENT_STATE.IF_ID_pipeline.CURRENTPC = 0;
    CURRENT_STATE.IF_ID_pipeline.instr = 0;
}

void flush_ID_EX(){
    CURRENT_STATE.ID_EX_pipeline.NPC = 0;
    CURRENT_STATE.ID_EX_pipeline.CURRENTPC = 0;
    CURRENT_STATE.ID_EX_pipeline.WB_MemToReg = false;
    CURRENT_STATE.ID_EX_pipeline.WB_RegWrite = false;
    CURRENT_STATE.ID_EX_pipeline.MEM_MemWrite = false;
    CURRENT_STATE.ID_EX_pipeline.MEM_MemRead = false;
    CURRENT_STATE.ID_EX_pipeline.MEM_Branch = false;
    CURRENT_STATE.ID_EX_pipeline.RegDst = false;
    CURRENT_STATE.ID_EX_pipeline.ALUSrc = false;
    CURRENT_STATE.ID_EX_pipeline.jump = false;
    CURRENT_STATE.ID_EX_pipeline.REG1 = 0;
    CURRENT_STATE.ID_EX_pipeline.REG2 = 0;
    CURRENT_STATE.ID_EX_pipeline.IMM = 0;
    CURRENT_STATE.ID_EX_pipeline.RS = 0;
    CURRENT_STATE.ID_EX_pipeline.RT = 0;
    CURRENT_STATE.ID_EX_pipeline.RD = 0;
    CURRENT_STATE.ID_EX_pipeline.instr_debug = 0;
}

void flush_EX_MEM(){
    CURRENT_STATE.EX_MEM_pipeline.NPC = 0;
    CURRENT_STATE.EX_MEM_pipeline.CURRENTPC = 0;
    CURRENT_STATE.EX_MEM_pipeline.WB_MemToReg = false;
    CURRENT_STATE.EX_MEM_pipeline.WB_RegWrite = false;
    CURRENT_STATE.EX_MEM_pipeline.MemWrite = false;
    CURRENT_STATE.EX_MEM_pipeline.MemRead = false;
    CURRENT_STATE.EX_MEM_pipeline.Branch = false;
    CURRENT_STATE.EX_MEM_pipeline.zero = false;
    CURRENT_STATE.EX_MEM_pipeline.ALU_OUT = 0;
    CURRENT_STATE.EX_MEM_pipeline.data2 = 0;
    CURRENT_STATE.EX_MEM_pipeline.RegDstNum = 0;
    CURRENT_STATE.EX_MEM_pipeline.instr_debug = 0;
}
