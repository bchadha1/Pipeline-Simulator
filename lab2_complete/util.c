/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   util.c                                                    */
/*                                                             */
/***************************************************************/

#include "util.h"

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/

/* memory will be dynamically allocated at initialization */
mem_region_t MEM_REGIONS[] = {
    { MEM_TEXT_START, MEM_TEXT_SIZE, NULL },
    { MEM_DATA_START, MEM_DATA_SIZE, NULL },
};

#define MEM_NREGIONS (sizeof(MEM_REGIONS)/sizeof(mem_region_t))

/***************************************************************/
/* CPU State info.                                             */
/***************************************************************/
CPU_State CURRENT_STATE;
int RUN_BIT;		/* run bit */
int INSTRUCTION_COUNT;
int CYCLE_COUNT;

/***************************************************************/
/* CPU State info.                                             */
/***************************************************************/
uint32_t *INST_INFO;
int NUM_INST;
int TEXT_SIZE;

/***************************************************************/
/*                                                             */
/* Procedure: str_split                                        */
/*                                                             */
/* Purpose: To parse main function argument                    */
/*                                                             */
/***************************************************************/
char** str_split(char *a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    
    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    
    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);
    
    /* Add space for terminating null string so caller
     *        knows where the list of returned strings ends. */
    count++;
    
    result = malloc(sizeof(char*) * count);
    
    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);
        
        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    
    return result;
}

/***************************************************************/
/*                                                             */
/* Procedure: fromBinary                                       */
/*                                                             */
/* Purpose: From binary to integer                             */
/*                                                             */
/***************************************************************/
int fromBinary(char *s)
{
    return (int) strtol(s, NULL, 2);
}

/***************************************************************/
/*                                                             */
/* Procedure: mem_read_32                                      */
/*                                                             */
/* Purpose: Read a 32-bit word from memory                     */
/*                                                             */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
    int i;
    int valid_flag = 0;
    
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
            address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;
            
            valid_flag = 1;
            
            return
            (MEM_REGIONS[i].mem[offset+3] << 24) |
            (MEM_REGIONS[i].mem[offset+2] << 16) |
            (MEM_REGIONS[i].mem[offset+1] <<  8) |
            (MEM_REGIONS[i].mem[offset+0] <<  0);
        }
    }
    
    if (!valid_flag){
        printf("Memory Read Error: Exceed memory boundary 0x%x\n", address);
        exit(1);
    }
    
    
    return 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: mem_write_32                                     */
/*                                                             */
/* Purpose: Write a 32-bit word to memory                      */
/*                                                             */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
    int i;
    int valid_flag = 0;
    
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
            address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;
            
            valid_flag = 1;
            
            MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
            MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
            MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
            MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
            return;
        }
    }
    if(!valid_flag){
        printf("Memory Write Error: Exceed memory boundary 0x%x\n", address);
        exit(1);
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle(bool forwardingEnabled, bool branchPredictionEnabled) {
    process_instruction(forwardingEnabled, branchPredictionEnabled);
    CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate MIPS for n instructions                */
/*                                                             */
/***************************************************************/
void run(int num_inst, bool forwardingEnabled, bool branchPredictionEnabled) {
    int i;
    
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }
    
    printf("Simulating for %d cycles...\n\n", CYCLE_COUNT);
    for (INSTRUCTION_COUNT = 0; i < num_inst;) {
        if (RUN_BIT == FALSE) {
            printf("Simulator halted\n\n");
            break;
        }
        if (reachedEnd) {
            cycle(forwardingEnabled, branchPredictionEnabled);
            cycle(forwardingEnabled, branchPredictionEnabled);
            cycle(forwardingEnabled, branchPredictionEnabled);
            cycle(forwardingEnabled, branchPredictionEnabled);
            break;
        }
        cycle(forwardingEnabled, branchPredictionEnabled);
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate MIPS until HALTed                      */
/*                                                             */
/***************************************************************/
void go(bool forwardingEnabled, bool branchPredictionEnabled) {
    if (RUN_BIT == FALSE) {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }
    
    printf("Simulating...\n\n");
    while (RUN_BIT)
        cycle(forwardingEnabled, branchPredictionEnabled);
    printf("Simulator halted\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(int start, int stop) {
    int address;
    
    printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = start; address <= stop; address += 4)
        printf("0x%08x: 0x%08x\n", address, mem_read_32(address));
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump() {
    int k;
    
    printf("Current register values :\n");
    printf("-------------------------------------\n");
    printf("PC: 0x%08x\n", CURRENT_STATE.PC+4);       // adjusted
    printf("Registers:\n");
    for (k = 0; k < MIPS_REGS; k++)
        printf("R%d: 0x%08x\n", k, CURRENT_STATE.REGS[k]);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : pdump                                           */
/*                                                             */
/* Purpose   : Dump current pipeline PC state                  */
/*                                                             */
/***************************************************************/
void pdump() {
    int k;
    
    printf("Current pipeline PC state :\n");
    printf("-------------------------------------\n");
    printf("CYCLE %d:", CYCLE_COUNT );
    
    if ((!CURRENT_STATE.PC) || ((CURRENT_STATE.PC - MEM_TEXT_START) >> 2) >= TEXT_SIZE) {
        printf("          ");
    } else {
        printf("0x%08x", CURRENT_STATE.PC);
    }
    printf("|");
    if(CURRENT_STATE.IF_ID_pipeline.CURRENTPC) printf("0x%08x", CURRENT_STATE.IF_ID_pipeline.CURRENTPC);
    else printf("          ");
    printf("|");
    if(CURRENT_STATE.ID_EX_pipeline.CURRENTPC) printf("0x%08x", CURRENT_STATE.ID_EX_pipeline.CURRENTPC);
    else printf("          ");
    printf("|");
    if(CURRENT_STATE.EX_MEM_pipeline.CURRENTPC) printf("0x%08x", CURRENT_STATE.EX_MEM_pipeline.CURRENTPC);
    else printf("          ");
    printf("|");
    if(CURRENT_STATE.MEM_WB_pipeline.CURRENTPC) printf("0x%08x", CURRENT_STATE.MEM_WB_pipeline.CURRENTPC);
    else printf("          ");
    
    
    /*
     for(k = 0; k < 5; k++)
     {
    	if(CURRENT_STATE.PIPE[k])
	    printf("0x%08x", CURRENT_STATE.PIPE[k]);
     else
	    printf("          ");
     
     if( k != PIPE_STAGE - 1 )
	    printf("|");
     }
     */
    
    printf("\n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Allocate and zero memory                        */
/*                                                             */
/***************************************************************/
void init_memory() {
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        MEM_REGIONS[i].mem = malloc(MEM_REGIONS[i].size);
        memset(MEM_REGIONS[i].mem, 0, MEM_REGIONS[i].size);
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_inst_info                                  */
/*                                                             */
/* Purpose   : Initialize instruction info                     */
/*                                                             */
/***************************************************************/
void init_inst_info()
{
    int i;
    
    for(i = 0; i < NUM_INST; i++)
    {
        INST_INFO[i] =0;
    }
}
