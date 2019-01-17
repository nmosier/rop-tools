/*
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCDisassembler/MCDisassembler.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstPrinter.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCSubtargetInfo.h>
*/

#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ropasm.h"
#include "vec.h"

//LLVMDisasmContextRef dcr;

// LLVMDisasmDispose()
#define ROPASM_TRIPLENAME "x86_64-unknown-linux-gnu"

LLVMDisasmContextRef ropasm_init() {
  LLVMDisasmContextRef dcr;
  
  LLVMInitializeAllAsmPrinters();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllDisassemblers();

  if ((dcr = LLVMCreateDisasm(ROPASM_TRIPLENAME, NULL, 0, NULL, NULL)) == NULL) {
    return NULL;
  }
  
  if (LLVMSetDisasmOptions(dcr, LLVMDisassembler_Option_AsmPrinterVariant) != 1) {
    /* cannot set options  */
    LLVMDisasmDispose(dcr);
    return NULL;
  }

  return dcr;

}

void ropasm_end(LLVMDisasmContextRef dcr) {
  if (dcr) {
    LLVMDisasmDispose(dcr);
  }
}


void instr_init(instr_t *instr) {
  memset(instr, 0, sizeof(*instr));
}

/*allocates new instruction; initialzies with gien params; attempts to disasm
  ( if dcr is non-null) */
int instr_create(uint8_t *mc, size_t mclen, Elf64_Off mcoff,
		   LLVMDisasmContextRef dcr, instr_t *instr) {

  assert (mclen <= INSTR_MC_MAXLEN);
  memcpy(instr->mc, mc, mclen);
  instr->mclen = mclen;
  instr->mcoff = mcoff;

  if (dcr) {
    return instr_disasm(instr, dcr);
  }
  return INSTR_OK;
}

int instr_disasm(instr_t *instr, LLVMDisasmContextRef dcr) {
  if ((LLVMDisasmInstruction(dcr, instr->mc, instr->mclen, instr->mcoff,
			     instr->disasm, INSTR_DISASM_MAXLEN)) == 0) {
    return INSTR_BAD;
  }

  return INSTR_OK;
}

int instr_eq(const instr_t *lhs, const instr_t *rhs) {
  size_t lsize, rsize;

  if ((lsize = lhs->mclen) != (rsize = rhs->mclen)) {
    return 0;
  }
  if (lhs->mcoff != rhs->mcoff) {
    return 0;
  }
  if (memcmp(lhs->mc, rhs->mc, lsize)) {
    return 0;
  }
  return 1; // equal
}

int instr_print(const instr_t *instr, FILE *f, int mode) {
  if (instr) {
    switch (mode) {
    case INSTR_PRINT_HEX:
      for (size_t i = 0; i < instr->mclen; ++i) {
	fprintf(f, "%1.1hx ", instr->mc[i]);
      }
    break;
    case INSTR_PRINT_DISASM:
      fprintf(f, "%s", instr->disasm);
      break;
    default:
      return -1;
    }
  }
  
  return 0;
}

/* simple wrapper functions */
void instrs_init(instrs_t *instrs) {
  VECTOR_INIT(instrs);
}
int instrs_insert(instr_t *instr, instrs_t *instrs) {
  return VECTOR_INSERT(instr, instrs);
}
void instrs_delete(instrs_t *instrs) {
  VECTOR_DELETE(instrs, NULL);
}
