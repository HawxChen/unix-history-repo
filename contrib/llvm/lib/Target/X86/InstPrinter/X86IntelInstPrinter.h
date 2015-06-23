//= X86IntelInstPrinter.h - Convert X86 MCInst to assembly syntax -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an X86 MCInst to Intel style .s file syntax.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_INSTPRINTER_X86INTELINSTPRINTER_H
#define LLVM_LIB_TARGET_X86_INSTPRINTER_X86INTELINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

class MCOperand;

class X86IntelInstPrinter final : public MCInstPrinter {
public:
  X86IntelInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                      const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI) {}

  void printRegName(raw_ostream &OS, unsigned RegNo) const override;
  void printInst(const MCInst *MI, raw_ostream &OS, StringRef Annot,
                 const MCSubtargetInfo &STI) override;

  // Autogenerated by tblgen.
  void printInstruction(const MCInst *MI, raw_ostream &O);
  static const char *getRegisterName(unsigned RegNo);

  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printMemReference(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printSSEAVXCC(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printXOPCC(const MCInst *MI, unsigned Op, raw_ostream &O);
  void printPCRelImm(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printMemOffset(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printSrcIdx(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printDstIdx(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  void printRoundingControl(const MCInst *MI, unsigned Op, raw_ostream &OS);
  void printU8Imm(const MCInst *MI, unsigned Op, raw_ostream &O);

  void printanymem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    printMemReference(MI, OpNo, O);
  }

  void printopaquemem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "opaque ptr ";
    printMemReference(MI, OpNo, O);
  }

  void printi8mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "byte ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi16mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "word ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi32mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "dword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi64mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "qword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi128mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "xmmword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi256mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "ymmword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printi512mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "zmmword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf32mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "dword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf64mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "qword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf80mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "xword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf128mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "xmmword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf256mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "ymmword ptr ";
    printMemReference(MI, OpNo, O);
  }
  void printf512mem(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "zmmword ptr ";
    printMemReference(MI, OpNo, O);
  }


  void printSrcIdx8(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "byte ptr ";
    printSrcIdx(MI, OpNo, O);
  }
  void printSrcIdx16(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "word ptr ";
    printSrcIdx(MI, OpNo, O);
  }
  void printSrcIdx32(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "dword ptr ";
    printSrcIdx(MI, OpNo, O);
  }
  void printSrcIdx64(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "qword ptr ";
    printSrcIdx(MI, OpNo, O);
  }
  void printDstIdx8(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "byte ptr ";
    printDstIdx(MI, OpNo, O);
  }
  void printDstIdx16(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "word ptr ";
    printDstIdx(MI, OpNo, O);
  }
  void printDstIdx32(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "dword ptr ";
    printDstIdx(MI, OpNo, O);
  }
  void printDstIdx64(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "qword ptr ";
    printDstIdx(MI, OpNo, O);
  }
  void printMemOffs8(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "byte ptr ";
    printMemOffset(MI, OpNo, O);
  }
  void printMemOffs16(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "word ptr ";
    printMemOffset(MI, OpNo, O);
  }
  void printMemOffs32(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "dword ptr ";
    printMemOffset(MI, OpNo, O);
  }
  void printMemOffs64(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
    O << "qword ptr ";
    printMemOffset(MI, OpNo, O);
  }
};

} // namespace llvm

#endif
