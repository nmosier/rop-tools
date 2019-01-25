/* cgen.h
 * Nicholas Mosier 2019
 */

#include <stdlib.h>
#include <stdio.h>

int codegen_program(const program *prog, FILE *outfile) {
  const struct blocks *blocks, *block_it, *block_end;

  /* generate code for all code blocks */
  blocks = &prog->blocks;
  for (block_it = blocks->blockv, block_end = block_it + blocks->blockc;
       block_it < block_end; ++block_it) {
    codegen_block(block_it, outfile);
  }

}

int codegen_block(const block *block, FILE *outfile) {
  
}

int instruction_match_definition(const struct instruction *instr,
				 const struct rule *def) {
  return -1; // to be implemented
}
