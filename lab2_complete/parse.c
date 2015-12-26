/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "parse.h"

int text_size;
int data_size;

uint32_t parsing_instr(char *buffer, const int index)
{
    uint32_t word;
    word = fromBinary(buffer);
    mem_write_32(MEM_TEXT_START + index, word);
    return word;
}

void parsing_data(char *buffer, const int index)
{
    uint32_t word;
    word = fromBinary(buffer);
    mem_write_32(MEM_DATA_START + index, word);
}
