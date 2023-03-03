/**************************************************************************\
*//*! \file nanotube-ebpf.cc
** \author Neil Turton <neilt@amd.com>
**  \brief EBPF to LLVM converter.
**   \date 2018-09-17
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

/* This code takes an EBPF program and converts it to LLVM
 * intermediate representation (LLVM-IR).  It is designed to be
 * flexible and stand-alone.
 *
 * Input formats:
 *   - ELF-EBPF
 *
 * Output format:
 *   - LLVM-IR
 *
 * Output options:
 *   - Text or bitcode
 *   - EBPF ABI or Tap ABI.
 *
 * The output format is expected to be fixed, but it may be useful to
 * support different input formats.
 *
 * Writing the output in text format rather than bitcode will make
 * debugging easier.  The EBPF ABI matches what is expected to be in a
 * EBPF ELF file such that running the output through LLC is expected
 * to reproduce the original ELF file, at least as far as
 * functionality goes.  The purpose of the Tap ABI is to provide an
 * interface which is closer to what the hardware supports than the
 * EBPF ABI.  It should be possible to write programs against the Tap
 * ABI and avoid using EBPF.
 */

#include <cstdint>
#include <cstring>
#include <iostream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>

#include <llvm/BinaryFormat/ELF.h>

extern "C" {
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <unistd.h>
}

#define stringify2(x) #x
#define stringify(x) stringify2(x)

using namespace llvm;
using namespace llvm::ELF;

void gen_llvm()
{
  LLVMContext the_context;
  IRBuilder<> builder(the_context);
  std::unique_ptr<Module> the_module;
//  std::map<std::string, Value *> NamedValues;

  the_module = llvm::make_unique<Module>("simple_test", the_context);

  std::vector<Type *> func_arg_types;
  FunctionType *func_type =
    FunctionType::get(Type::getInt32Ty(the_context), func_arg_types, false);
  Function *func =
    Function::Create(func_type, Function::ExternalLinkage, "get_value", the_module.get());

  BasicBlock *bb = BasicBlock::Create(the_context, "entry", func);
  builder.SetInsertPoint(bb);
  
  Value *v = ConstantInt::get(the_context, APInt(32, 42));
  builder.CreateRet(v);

  verifyFunction(*func);

  raw_os_ostream raw_stream(std::cout);
  the_module->print(raw_stream, NULL);
}

void load_maps(Elf_Scn *scn, const GElf_Shdr *shdr, const std::string &sec_name)
{
  std::cout << "Loading maps from section " << sec_name << std::endl;
}

void load_data(Elf_Scn *scn, const GElf_Shdr *shdr, const std::string &sec_name)
{
  std::cout << "Loading data from section " << sec_name << std::endl;
}

void load_code(Elf_Scn *scn, const GElf_Shdr *shdr, const std::string &sec_name)
{
  std::cout << "Loading code from section " << sec_name << std::endl;
}

void load_symtab(Elf_Scn *scn, const GElf_Shdr *shdr, const std::string &sec_name)
{
  std::cout << "Loading symtab from section " << sec_name << std::endl;
}

void load_relocs(Elf_Scn *scn, const GElf_Shdr *shdr, const std::string &sec_name)
{
  std::cout << "Loading relocs from section " << sec_name << std::endl;
}

/**
 * Parse, check, and print ELF header information
 * Params: e (the ELF file), argv (the arguemnt vector), shdr_num (OUT - number of headers)
 * Return: 0 on success, 1 on failure
 */
int parse_check_elf(Elf *e, char **argv, size_t *shdr_num, size_t *shdr_strndx) {
  Elf_Kind ek = elf_kind(e);
  if (ek != ELF_K_ELF) {
    std::cerr << argv[0] << ": Unknown elf kind " << int(ek)
              << std::endl;
    return 1;
  }

  GElf_Ehdr ehdr;
  if (gelf_getehdr(e, &ehdr) == NULL) {
    std::string msg(elf_errmsg(-1));
    std::cerr << argv[0] << ": Failed to fetch ELF header: "
              << msg << std::endl;
    return 1;
  }

  uint32_t elf_class = gelf_getclass(e);
  if (elf_class != ELFCLASS32 && elf_class != ELFCLASS64) {
    std::cerr << argv[0] << ": Unknown ELF class: "
              << elf_class << std::endl;
    return 1;
  }
  uint32_t elf_class_bits = (elf_class == ELFCLASS32 ? 32 : 64);

  uint32_t elf_data = ehdr.e_ident[EI_DATA];
  std::string elf_data_name("unknown");
  switch(elf_data) {
  case ELFDATANONE: elf_data_name = "none"; break;
  case ELFDATA2LSB: elf_data_name = "LE";   break;
  case ELFDATA2MSB: elf_data_name = "BE";   break;
  }

  if (elf_getshdrnum(e, shdr_num) != 0) {
    std::string msg(elf_errmsg(-1));
    std::cerr << argv[0] << ": elf_getshdrnum failed: "
              << msg << std::endl;
    return 1;
  }

  if (elf_getshdrstrndx(e, shdr_strndx) != 0) {
    std::string msg(elf_errmsg(-1));
    std::cerr << argv[0] << ": elf_getshdrstrndx failed: "
              << msg << std::endl;
    return 1;
  }

  std::string e_type_name("unknown");
  switch(ehdr.e_type) {
  case ET_NONE: e_type_name = "none"; break;
  case ET_REL:  e_type_name = "rel";  break;
  case ET_EXEC: e_type_name = "exec"; break;
  case ET_DYN:  e_type_name = "dyn";  break;
  case ET_CORE: e_type_name = "core"; break;
  }

  std::string e_mach_name("unknown");
  switch(ehdr.e_machine) {
  case EM_NONE:       e_mach_name = "none";       break;
  case EM_SPARC:      e_mach_name = "SPARC";      break;
  case EM_386:        e_mach_name = "386";        break;
  case EM_MIPS:       e_mach_name = "MIPS";       break;
  case EM_PPC:        e_mach_name = "PPC";        break;
  case EM_PPC64:      e_mach_name = "PPC64";      break;
  case EM_ARM:        e_mach_name = "ARM";        break;
  case EM_X86_64:     e_mach_name = "x86_64";     break;
  //case EM_MICROBLAZE: e_mach_name = "microblaze"; break;
  case EM_BPF:        e_mach_name = "BPF";        break;
  }

  std::cout << "; elf_class   = " << elf_class
            << " (" << elf_class_bits << "-bit)" << std::endl;
  std::cout << "; elf_data    = " << elf_data
            << " (" << elf_data_name << ")" << std::endl;
  std::cout << "; elf_type    = " << uint32_t(ehdr.e_type)
            << " (" << e_type_name << ")" << std::endl;
  std::cout << "; elf_machine = " << uint32_t(ehdr.e_machine)
            << " (" << e_mach_name << ")" << std::endl;
  std::cout << "; shdr_num    = " << *shdr_num << std::endl;
  std::cout << "; shdr_strndx = " << *shdr_strndx << std::endl;
  std::cout << std::endl;

  if (elf_class != ELFCLASS64 || ehdr.e_type != ET_REL ||
      (elf_data != ELFDATA2LSB && elf_data != ELFDATA2MSB) ||
      (ehdr.e_machine != EM_NONE && ehdr.e_machine != EM_BPF)) {
    std::cout << "; Unknown ELF file type." << std::endl;
    return 1;
  }

  return 0;
}

int main(int argc, char **argv)
{
  size_t shdr_num;
  size_t shdr_strndx;

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>"
              << std::endl;
    return 1;
  }

  static_assert(EV_CURRENT==1, "Unexpected value of EV_CURRENT="
                stringify(EV_CURRENT));
  if (elf_version(EV_CURRENT) == EV_NONE) {
    std::cerr << argv[0] << ": ELF library initialization failed."
              << std::endl;
    return 1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    std::string msg(strerror(errno));
    std::cerr << argv[0] << ": Failed to open '"
              << argv[1] << "': " << msg << std::endl;
    return 1;
  }

  Elf *e = elf_begin(fd, ELF_C_READ, NULL);
  if (e == NULL) {
    std::string msg(elf_errmsg(-1));
    std::cerr << argv[0] << ": elf_begin() failed: "
              << msg << std::endl;
    return 1;
  }

  if (parse_check_elf(e, argv, &shdr_num, &shdr_strndx))
    return 1;

  Elf_Scn *scn = NULL;
  while((scn = elf_nextscn(e, scn)) != NULL) {
    GElf_Shdr shdr;

    if (gelf_getshdr(scn, &shdr) != &shdr) {
      std::string msg(elf_errmsg(-1));
      std::cerr << argv[0] << ": Failed to fetch ELF section header: "
                << msg << std::endl;
      return 1;
    }

    char *sec_name = elf_strptr(e, shdr_strndx, shdr.sh_name);
    if (sec_name == NULL) {
      std::string msg(elf_errmsg(-1));
      std::cerr << argv[0] << ": Failed to fetch ELF section name: "
                << msg << std::endl;
      return 1;
    }

    std::string sec_type_name("unknown");
    switch(shdr.sh_type) {
    case SHT_NULL:     sec_type_name = "NULL";     break;
    case SHT_PROGBITS: sec_type_name = "PROGBITS"; break;
    case SHT_SYMTAB:   sec_type_name = "SYMTAB";   break;
    case SHT_STRTAB:   sec_type_name = "STRTAB";   break;
    case SHT_HASH:     sec_type_name = "HASH";     break;
    case SHT_DYNAMIC:  sec_type_name = "DYNAMIC";  break;
    case SHT_RELA:     sec_type_name = "RELA";     break;
    case SHT_NOBITS:   sec_type_name = "NOBITS";   break;
    case SHT_REL:      sec_type_name = "REL";      break;
    }

    std::string sec_flags("");
    if (shdr.sh_flags & SHF_WRITE)
      sec_flags += "write ";
    if (shdr.sh_flags & SHF_ALLOC)
      sec_flags += "alloc ";
    if (shdr.sh_flags & SHF_EXECINSTR)
      sec_flags += "execinstr ";
    if (shdr.sh_flags & SHF_MERGE)
      sec_flags += "merge ";
    if (shdr.sh_flags & SHF_STRINGS)
      sec_flags += "strings ";
    if (shdr.sh_flags & SHF_INFO_LINK)
      sec_flags += "info_link ";
    if (shdr.sh_flags & ~(SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR |
                          SHF_MERGE | SHF_STRINGS | SHF_INFO_LINK))
      sec_flags += "other ";

    if (sec_flags.size() != 0)
      sec_flags.resize(sec_flags.size()-1);

//#if 0
    std::cout << "; Section '" << sec_name << "'" << std::endl;
    std::cout << ";   Index:   " << elf_ndxscn(scn) << std::endl;
    std::cout << ";   Type:    " << shdr.sh_type
              << " (" << sec_type_name << ")" << std::endl;
    std::cout << ";   Flags:   0x"
              << std::hex << shdr.sh_flags << std::dec
              << " (" << sec_flags << ")" << std::endl;
    std::cout << ";   Address: 0x"
              << std::hex << shdr.sh_addr << std::dec
              << std::endl;
    std::cout << ";   Offset:  0x"
              << std::hex << shdr.sh_offset << std::dec
              << std::endl;
    std::cout << ";   Size:    0x"
              << std::hex << shdr.sh_size << std::dec
              << std::endl;
    std::cout << ";   Link:    " << shdr.sh_link << std::endl;
    std::cout << ";   Info:    " << shdr.sh_info << std::endl;
//#endif

    bool handled_sec = false;
    switch(shdr.sh_type) {
    case SHT_PROGBITS:
      switch (shdr.sh_flags & (SHF_WRITE|SHF_ALLOC|SHF_EXECINSTR)) {
      case SHF_ALLOC|SHF_WRITE:
        if (!strcmp(sec_name, "maps")) {
          load_maps(scn, &shdr, sec_name);
          handled_sec = true;
        } else if (!strcmp(sec_name, "license")) {
          load_data(scn, &shdr, sec_name);
          handled_sec = true;
        }
        break;

      case SHF_ALLOC|SHF_EXECINSTR:
        load_code(scn, &shdr, sec_name);
        handled_sec = true;
        break;

      default:
        if (!strcmp(sec_name, ".debug_str") ||
            !strcmp(sec_name, ".debug_loc") ||
            !strcmp(sec_name, ".debug_abbrev") ||
            !strcmp(sec_name, ".debug_info") ||
            !strcmp(sec_name, ".debug_ranges") ||
            !strcmp(sec_name, ".debug_macinfo") ||
            !strcmp(sec_name, ".debug_pubnames") ||
            !strcmp(sec_name, ".debug_pubtypes") ||
            !strcmp(sec_name, ".eh_frame") ||
            !strcmp(sec_name, ".debug_line"))
          handled_sec = true;
      };
      break;

    case SHT_SYMTAB:
      handled_sec = true;
      load_symtab(scn, &shdr, sec_name);
      break;

    case SHT_STRTAB:
      handled_sec = true;
      break;

    case SHT_REL:
      handled_sec = true;
      load_relocs(scn, &shdr, sec_name);
      break;
    }

    if (!handled_sec) {
      std::cout << "; Warning: Unhandled section '"
                << sec_name << "'." << std::endl;
    }

//#if 0
    Elf_Data *data;
    while((data = elf_getdata(scn, data)) != NULL) {
      std::cout << ";   Data at 0x"
                << std::hex << data->d_off
                << " length 0x"
                << std::hex << data->d_size
                << " align 0x"
                << std::hex << data->d_align
                << " type "
                << std::dec << data->d_type
                << std::endl;
    }
    std::cout << std::endl;
//#endif
  }

  gen_llvm();

  elf_end(e);
  close(fd);

  return 0;
}
