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



// instruction information extracting macros
#define FUNCT(INST)     (INST & 0x0000001F)
#define SHAMT(INST)     (INST & 0x000007C0)>>5
#define RD(INST)        (INST & 0x0000F800)>>10
#define RT(INST)        (INST & 0x001F0000)>>15
#define RS(INST)        (INST & 0x03E00000)>>20
#define OPCODE(INST)    (INST & 0xFC000000)>>25
#define IMM(INST)       (INST & 0x0000FFFF)
#define J250(INST)      (INST & 0x03FFFFFF)




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
    bool MEM_Branch;    // MEM
    
    bool RegDst;        // EX
    int ALUControl;     // EX
    bool ALUSrc;        // EX
    
    bool jump;
    
    
    // reg
    uint32_t REG1;      // rs
    uint32_t REG2;      // rt
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
    uint32_t RegDstNum;         // 5 bit Register destination (if write)
    
} EX/MEM;

typedef struct MEM/WB_Struct {
    // PC
    uint32_t NPC; 
    
    // Control signals
    bool MemRead;
    bool MemToReg;
    bool RegWrite;
    
    // Reg
    uint32_t Mem_OUT;
    uint32_t ALU_OUT;
    
    uint32_t RegDstNum;         // 5 bit Register destination (if write)
    
} MEM/WB;




typedef struct CPU_State_Struct {
    uint32_t PC;                /* program counter */
    uint32_t REGS[MIPS_REGS];	/* register file */
    uint32_t PIPE[PIPE_STAGE];	/* pipeline stage */
    // Pipelines
    IF/ID IF_ID_pipeline;
    ID/EX ID_EX_pipeline;
    EX/MEM EX_MEM_pipeline;
    MEM/WB MEM_WB_pipeline;
    
} CPU_State;


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
extern bool globaljump;
extern bool globaljal;
extern bool globalJumpAndReturn;
extern uint32_t PC_buffer;
extern int stallcount;
extern int stallcount2;


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
