# Pipeline-Simulator
CS311 Project 3: Simulating Pipelined Execution


# Implementation 
Inside run.c file.
I implemented process_instruction() which contains a lot of process including these :
>void process_IF();

>void process_ID(bool forwardingEnabled, bool branchPredictionEnabled);
 
>void generate_control_signals(uint32_t instr, bool forwardingEnabled, bool branchPredictionEnabled);

>void process_EX(bool forwardingEnabled);

>void process_MEM(bool forwardingEnabled, bool branchPredictionEnabled);

>void process_WB();

you will notice that process_WB() comes first because register write comes first than register read.

Also, branch prediction and forwarding is implemented inside these functions.
I didn't use global variable for forwardingEnabled and branchPredictionEnabled.
I passed it through the arguments. (In order to see where they are used).

# Output
Many of the output looks the same with the sample solution given by the TA  
except the PC value of the leftmost (IF stage).
In my implementation, I used the PC value when we are starting the cycle (at the very beginning).  
During the cycle, this value may be modified. Therefore it might be different from the reference solution.  
However, as you can see from other pipeline stage PC values, registers and memory, I properly implemented the simulation.  
Also, another difference is that after reaching the end of instruction, I reset the PC value. 

# Pipeline Regster States

``` C++
typedef struct IF_ID_Struct {
    // PC
    uint32_t NPC;
    uint32_t CURRENTPC;
    
    // reg
    uint32_t instr;
    
    
} IF_ID;

typedef struct ID_EX_Struct {
    // PC
    uint32_t NPC;
    uint32_t CURRENTPC;
    
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
    uint32_t REG1;      // Reg[rs]
    uint32_t REG2;      // Reg[rt]
    uint32_t IMM; 		// immediate value
    
    uint32_t RS;		// rs, inst[21:25]
    uint32_t RT;		// rt, inst[16:20]
    uint32_t RD;		// rd, inst[11:15]
    uint32_t SHAMT;     // shamt, inst[6:10]
    
    uint32_t instr_debug;
    
} ID_EX;

typedef struct EX_MEM_Struct {
    // PC
    uint32_t NPC;       // branch target address
    uint32_t CURRENTPC;
    
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
    uint32_t instr_debug;
} EX_MEM;

typedef struct MEM_WB_Struct {
    // PC
    uint32_t NPC;
    uint32_t CURRENTPC;
    
    // Control signals
    bool MemRead;
    bool MemToReg;
    bool RegWrite;
    
    // Reg
    uint32_t Mem_OUT;
    uint32_t ALU_OUT;
    
    uint32_t RegDstNum;         // 5 bit Register destination (if write)
    uint32_t instr_debug;
} MEM_WB;
```

Another thing to notice is that I have changed pdump function.  
I have not used "uint32_t PIPE[PIPE_STAGE]" variable. I used CURRENTPC inside the pipeline registers instead.

