#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <elf.h>
#include "decode.h"

/* Given the in-memory ELF header pointer as `ehdr` and a section
   header pointer as `shdr`, returns a pointer to the memory that
   contains the in-memory content of the section */
#define AT_SEC(ehdr, shdr) ((void *)(ehdr) + (shdr)->sh_offset)

static int get_secrecy_level(const char *s); /* foward declaration */
static Elf64_Shdr *section_name(Elf64_Ehdr *ehdr, char *name);
static Elf64_Shdr *rela_dyn_shdr;
static Elf64_Rela *relas_dyn;
static Elf64_Shdr *rela_plt_shdr;
static Elf64_Rela *relas_plt;
static char *run_time_addr_move(Elf64_Ehdr *ehdr, Elf64_Addr addr);
static char *run_time_addr_jump(Elf64_Ehdr *ehdr, Elf64_Addr addr);
static void handle_funcs(Elf64_Ehdr *ehdr);
static Elf64_Shdr *shdrs;
static Elf64_Shdr *dynsym_shdr;
static Elf64_Sym *syms;
static char *strs;
static int dynsym_count;

/*************************************************************/

void redact(Elf64_Ehdr *ehdr) {
    /* Your work starts here --- but add helper functions that you call
       from here, instead of trying to put all the work in one
       function. */

    /*
        Prepare for the work.
        These variables are useful across the multiple functions
        implemented in this class.
     */
    shdrs = (void *) ehdr + ehdr->e_shoff;
    dynsym_shdr = section_name(ehdr, ".dynsym");
    syms = AT_SEC(ehdr, dynsym_shdr);
    strs = AT_SEC(ehdr, section_name(ehdr, ".dynstr"));
    dynsym_count = dynsym_shdr->sh_size / sizeof(Elf64_Sym);
    rela_dyn_shdr = section_name(ehdr, ".rela.dyn");
    relas_dyn = AT_SEC(ehdr, rela_dyn_shdr);
    rela_plt_shdr = section_name(ehdr, ".rela.plt");
    relas_plt = AT_SEC(ehdr, rela_plt_shdr);
    handle_funcs(ehdr);
    //printf("%s\n", "End of redaction");
}

/*
    Helper method to find section name
 */
static Elf64_Shdr *section_name(Elf64_Ehdr *ehdr, char *name) {
    //strs = AT_SEC(ehdr, shdrs + ehdr->e_shstrndx)
    char *strs = AT_SEC(ehdr, shdrs + ehdr->e_shstrndx);

    int i;
    for (i = 0; i < ehdr->e_shnum; i++) {
        char *found_name = strs + shdrs[i].sh_name;

        if (strcmp(found_name, name) == 0) {
            return &shdrs[i];
        }
    }
    return NULL;
}

/*
    Helper method to handle the function according to the desired behavior
    on shared-library.
    This method handles jump and move instruction accordigly to the map.
*/
static void handle_funcs(Elf64_Ehdr *ehdr){
    int m;
    for (m = 0; m < dynsym_count; m++) {
        int isFunction = (ELF64_ST_TYPE(syms[m].st_info) == STT_FUNC); //check against symbol

        if (isFunction) {
            char *func_name_dynstr = strs + syms[m].st_name;
            // printf("%s\n", func_name_dynstr);

            int check_func = strcmp(func_name_dynstr, "__cxa_finalize") && strcmp(func_name_dynstr, "_init") && strcmp(func_name_dynstr, "_fini");

            // Omit the unnecessary functions.
            if (check_func) {
                printf("Current fuction: %s\n", func_name_dynstr);

                //Snippest from the map slide.
                int j = syms[m].st_shndx;
                instruction_t ins;
                code_t *code_ptr = AT_SEC(ehdr, shdrs + j) + (syms[m].st_value - shdrs[j].sh_addr);
                Elf64_Addr third_par_in_decode = syms[m].st_value;

                int func_len_bytes = syms[m].st_size; // size of a function in bytes
                printf("Length of the function: %d\n", func_len_bytes);

                int inst_count = 0; // Keep track of the number of instructions.
                // Iterate each instructions.
                while (inst_count < func_len_bytes) {
                    char *glob_var = NULL;
                    decode(&ins, code_ptr, third_par_in_decode);

                    /*
                       MOV_ADDR_TO_REG_OP
                          - The instruction moves a constant address into a
                            register, possibly to access a global
                            variable.

                       JMP_TO_ADDR_OP
                          — The instruction jumps to a constant address,
                            possibly as a tail call to an global function.

                       MAYBE_JMP_TO_ADDR_OP
                          — The instruction conditionally jumps to a constant
                            address, most likely due to an if in the original
                            program.
                     */
                    if (ins.op == MOV_ADDR_TO_REG_OP) {
                        Elf64_Addr move_to_addr = ins.mov_addr_to_reg.addr;
                        glob_var = run_time_addr_move(ehdr, move_to_addr);
                        printf("Move detected to this address: %d.  %s found\n", (int)move_to_addr, glob_var);
                    } else if (ins.op == JMP_TO_ADDR_OP) {
                        Elf64_Addr jump_to_addr = ins.jmp_to_addr.addr;
                        glob_var = run_time_addr_jump(ehdr, jump_to_addr);
                        printf("Jump detected to this address: %d.  %s found\n", (int)jump_to_addr, glob_var);
                    }
                    // crash when appropriate.
                    if (glob_var != NULL && get_secrecy_level(func_name_dynstr) < get_secrecy_level(glob_var)) {
                        printf("Replacing with crash: %s %s\n", func_name_dynstr, glob_var);
                        replace_with_crash(code_ptr, &ins);
                    }

                    /*
                        ins->length field is set to the length of the machine-code instruction in bytes,
                        so code_ptr + ins->length is a pointer to the next instruction.
                     */
                    code_ptr += ins.length;
                    third_par_in_decode += ins.length;
                    inst_count += ins.length;
                }
                printf("\n");
            }
        }
    }
}

/*
    Helper method to find the name of the function at the jumped address.
    Return the name of that function.
 */
static char *run_time_addr_jump(Elf64_Ehdr *ehdr, Elf64_Addr addr) {
    // Iterate through the symbol table
    int i;
    for (i = 0; i < dynsym_count; i++) {
        //do the same thing done at handle_funcs method at the new address.
        instruction_t ins;
        Elf64_Addr third_par_in_decode = addr;
        code_t *code_ptr = (unsigned char *) ehdr + addr;
        decode(&ins, code_ptr, third_par_in_decode);

        int rela_plt_count = rela_plt_shdr->sh_size / sizeof(Elf64_Rela); //We need to focus on rela.plt section

        int k;
        for (k = 0; k < rela_plt_count; k++) {
            int index = ELF64_R_SYM(relas_plt[k].r_info);
            char *name = strs + syms[index].st_name;

            //function name at the same address as the jumped address.
            if (ins.jmp_to_addr.addr == relas_plt[k].r_offset) {
                return name;
            }
        }
    }
    return NULL;
}

/*
    Helper method to find the name of the function at the moved address.
    Occures mostly when if statement is called.
    Return the name of that function.
 */
static char *run_time_addr_move(Elf64_Ehdr *ehdr, Elf64_Addr addr) {
    int rela_dyn_count = rela_dyn_shdr->sh_size / sizeof(Elf64_Rela);

    int i;
    for (i = 0; i < rela_dyn_count; i++) {
        int m = ELF64_R_SYM(relas_dyn[i].r_info);
        char *name = strs + syms[m].st_name;

        //function name at the same address as the moved address.
        if (addr == relas_dyn[i].r_offset) {
            return name;
        }
    }
    return NULL;
}


/*************************************************************/

static int get_secrecy_level(const char *s) {
    int level = 0;
    int len = strlen(s);
    while (len && (s[len - 1] >= '0') && (s[len - 1] <= '9')) {
        level = (level * 10) + (s[len - 1] - '0');
        --len;
    }
    return level;
}

static void fail(char *reason, int err_code) {
    fprintf(stderr, "%s (%d)\n", reason, err_code);
    exit(1);
}

static void check_for_shared_object(Elf64_Ehdr *ehdr) {
    if ((ehdr->e_ident[EI_MAG0] != ELFMAG0)
        || (ehdr->e_ident[EI_MAG1] != ELFMAG1)
        || (ehdr->e_ident[EI_MAG2] != ELFMAG2)
        || (ehdr->e_ident[EI_MAG3] != ELFMAG3))
        fail("not an ELF file", 0);

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        fail("not a 64-bit ELF file", 0);

    if (ehdr->e_type != ET_DYN)
        fail("not a shared-object file", 0);
}

int main(int argc, char **argv) {
    int fd_in, fd_out;
    size_t len;
    void *p_in, *p_out;
    Elf64_Ehdr *ehdr;

    if (argc != 3)
        fail("expected two file names on the command line", 0);

    /* Open the shared-library input file */
    fd_in = open(argv[1], O_RDONLY);
    if (fd_in == -1)
        fail("could not open input file", errno);

    /* Find out how big the input file is: */
    len = lseek(fd_in, 0, SEEK_END);

    /* Map the input file into memory: */
    p_in = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd_in, 0);
    if (p_in == (void *) -1)
        fail("mmap input failed", errno);

    /* Since the ELF file starts with an ELF header, the in-memory image
       can be cast to a `Elf64_Ehdr *` to inspect it: */
    ehdr = (Elf64_Ehdr *) p_in;

    /* Check that we have the right kind of file: */
    check_for_shared_object(ehdr);

    /* Open the shared-library output file and set its size: */
    fd_out = open(argv[2], O_RDWR | O_CREAT, 0777);
    if (fd_out == -1)
        fail("could not open output file", errno);
    if (ftruncate(fd_out, len))
        fail("could not set output file file", errno);

    /* Map the output file into memory: */
    p_out = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_out, 0);
    if (p_out == (void *) -1)
        fail("mmap output failed", errno);

    /* Copy input file to output file: */
    memcpy(p_out, p_in, len);

    /* Close input */
    if (munmap(p_in, len))
        fail("munmap input failed", errno);
    if (close(fd_in))
        fail("close input failed", errno);

    ehdr = (Elf64_Ehdr *) p_out;

    redact(ehdr);

    if (munmap(p_out, len))
        fail("munmap input failed", errno);
    if (close(fd_out))
        fail("close input failed", errno);

    return 0;
}
