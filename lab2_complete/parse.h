/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.h                                                   */
/*                                                             */
/***************************************************************/

#ifndef _PARSE_H_
#define _PARSE_H_

#include <stdio.h>

#include "util.h"

extern int text_size;
extern int data_size;

/* functions */
uint32_t	parsing_instr(char *buffer, const int index);
void		parsing_data(char *buffer, const int index);
void		print_parse_result();

#endif
