/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   util.h                                                    */
/*                                                             */
/***************************************************************/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FALSE 0
#define TRUE  1

/* Basic Information */
#define MEM_TEXT_START	0x00400000
#define MEM_TEXT_SIZE	0x00100000
#define MEM_DATA_START	0x10000000
#define MEM_DATA_SIZE	0x00100000
#define MIPS_REGS	32
#define BYTES_PER_WORD	4
#define PIPE_STAGE	5



typedef struct IF/ID_Struct {
    // PC
    uint32_t NPC;
    
    // reg
    uint32_t instr;
    
    
} IF/ID;

typedef struct ID/EX_Struct {
    // PC
    uint32_t NPC;
    
    // Control Signals
    bool WB_MemToReg;   // WB
    bool WB_RegWrite;   // WB
    
    bool MEM_MemWrite;  // MEM
    bool MEM_MemRead;   // MEM
    bool MEM_Branch;
    
    bool RegDst;        // EX
    bool ALUOp0;        // EX
    bool ALUOp1;        // EX
    bool ALUSrc;        // EX
    
    
    // reg
    uint32_t REG1;
    uint32_t REG2;
    uint32_t IMM; // immediate value
    
    uint32_t inst16_20;
    uint32_t inst11_15;
    
} ID/EX;

typedef struct EX/MEM_Struct {
    // PC
    uint32_t NPC;       // branch target address
    
    // Control signals
    bool WB_MemToReg; // WB
    bool WB_RegWrite; // WB
    
    bool MemWrite;
    bool MemRead;
    bool Branch; // branch -> eventually PCSrc
    
    
    
    // reg
    bool zero;
    uint32_t ALU_OUT;
    uint32_t data2;             // untouched data2 from register.
    uint32_t RegDstNum;       // 5 bit Register destination (if write)
    
} EX/MEM;

typedef struct MEM/WB_Struct {
    // Control signals
    bool MemToReg;
    bool RegWrite;
    
    // Reg
    uint32_t Mem_OUT;
    uint32_t ALU_OUT;
    
    
} MEM/WB;




typedef struct CPU_State_Struct {
    uint32_t PC;		/* program counter */
    uint32_t REGS[MIPS_REGS];	/* register file */
    uint32_t PIPE[PIPE_STAGE];	/* pipeline stage */
    // Pipelines
    IF/ID IF_ID_pipeline;
    ID/EX ID_EX_pipeline;
    EX/MEM EX_MEM_pipeline;
    MEM/WB MEM_WB_pipeline;
    
} CPU_State;

typedef struct inst_s {
    short opcode;
    
    /*R-type*/
    short func_code;

    union {
        /* R-type or I-type: */
        struct {
	    unsigned char rs;
	    unsigned char rt;

	    union {
	        short imm;

	        struct {
		    unsigned char rd;
		    unsigned char shamt;
		} r;
	    } r_i;
	} r_i;
        /* J-type: */
        uint32_t target;
    } r_t;

    uint32_t value;
    
    //int32 encoding;
    //imm_expr *expr;
    //char *source_line;
} instruction;

typedef struct {
    uint32_t start, size;
    uint8_t *mem;
} mem_region_t;

/* For PC * Registers */
extern CPU_State CURRENT_STATE;

/* Pipelines */
extern IF/ID IF_ID_pipeline_buffer;
extern ID/EX ID_EX_pipeline_buffer;
extern EX/MEM EX_MEM_pipeline_buffer;
extern MEM/WB MEM_WB_pipeline_buffer;

/* For Instructions */
extern instruction *INST_INFO;
extern int NUM_INST;

/* For Memory Regions */
extern mem_region_t MEM_REGIONS[2];

/* For Execution */
extern int RUN_BIT;	/* run bit */
extern int INSTRUCTION_COUNT;

/* Functions */
char**		str_split(char *a_str, const char a_delim);
int		fromBinary(char *s);
uint32_t	mem_read_32(uint32_t address);
void		mem_write_32(uint32_t address, uint32_t value);
void		cycle();
void		run(int num_cycles);
void		go();
void		mdump(int start, int stop);
void		rdump();
void		init_memory();
void		init_inst_info();

/* YOU IMPLEMENT THIS FUNCTION */
void	process_instruction();

#endif
