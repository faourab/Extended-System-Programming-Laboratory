#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <elf.h>

// Function prototypes
static long get_file_size(const char *filename);
static int process_program_headers(void *map_start, void (*handler)(Elf32_Phdr *, int), int arg);
static void display_program_header(Elf32_Phdr *phdr, int index);
static void print_program_header_info(Elf32_Phdr *phdr, int index);
static void map_program_header(Elf32_Phdr *phdr, int fd);
static void validate_elf_header(Elf32_Ehdr *ehdr, void *map_start, long file_size, int fd);
static void handle_mapping_error(void *map_start, int fd);
static void print_elf_header_info(Elf32_Ehdr *ehdr);
static void display_segment_flags(uint32_t flags);

// External function declaration
extern int startup(int argc, char **argv, void (*start)());

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    long file_size = get_file_size(filename);
    void *map_start = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    handle_mapping_error(map_start, fd);

    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)map_start;
    validate_elf_header(elf_header, map_start, file_size, fd);

    print_elf_header_info(elf_header);

    process_program_headers(map_start, display_program_header, fd);
    printf("\n\n\n");

    printf("Type  Offset  VirtAddr  PhysAddr  FileSiz  MemSiz  Flg  Align\n");
    process_program_headers(map_start, print_program_header_info, fd);
    printf("\n\n\n");

    process_program_headers(map_start, map_program_header, fd);

    startup(argc - 1, argv + 1, (void *)(elf_header->e_entry));

    munmap(map_start, file_size);
    close(fd);
    return 0;
}

static void handle_mapping_error(void *map_start, int fd) {
    if (map_start == MAP_FAILED) {
        perror("Failed to map file to memory");
        close(fd);
        exit(EXIT_FAILURE);
    }
}

static void validate_elf_header(Elf32_Ehdr *ehdr, void *map_start, long file_size, int fd) {
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Not a valid ELF file\n");
        munmap(map_start, file_size);
        close(fd);
        exit(EXIT_FAILURE);
    }
}

static long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("Error getting file size");
        exit(EXIT_FAILURE);
    }
    return st.st_size;
}

static int process_program_headers(void *map_start, void (*handler)(Elf32_Phdr *, int), int arg) {
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    Elf32_Phdr *phdr_table = (Elf32_Phdr *)(map_start + ehdr->e_phoff);

    for (int i = 0; i < ehdr->e_phnum; i++) {
        handler(&phdr_table[i], arg);
    }
    return 0;
}

static void display_program_header(Elf32_Phdr *phdr, int index) {
    printf("Program header number %d at address %p\n", index, (void *)phdr);
}

static void print_program_header_info(Elf32_Phdr *phdr, int index) {
    const char *type;
    switch (phdr->p_type) {
        case PT_LOAD: type = "LOAD"; break;
        case PT_DYNAMIC: type = "DYNAMIC"; break;
        case PT_INTERP: type = "INTERP"; break;
        case PT_NOTE: type = "NOTE"; break;
        case PT_SHLIB: type = "SHLIB"; break;
        case PT_PHDR: type = "PHDR"; break;
        default: type = "UNKNOWN"; break;
    }

    printf("%-5s 0x%06x 0x%08x 0x%08x 0x%05x 0x%05x %c%c%c 0x%x\n",
           type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
           phdr->p_filesz, phdr->p_memsz,
           (phdr->p_flags & PF_R) ? 'R' : ' ',
           (phdr->p_flags & PF_W) ? 'W' : ' ',
           (phdr->p_flags & PF_X) ? 'E' : ' ',
           phdr->p_align);

    display_segment_flags(phdr->p_flags);
}

static void display_segment_flags(uint32_t flags) {
    const char *flag;
    if ((flags & PF_R) && (flags & PF_W) && (flags & PF_X))
        flag = "READ-WRITE-EXECUTE";
    else if ((flags & PF_R) && (flags & PF_W))
        flag = "READ-WRITE";
    else if ((flags & PF_R) && (flags & PF_X))
        flag = "READ-EXECUTE";
    else if (flags & PF_R)
        flag = "READ";
    else if (flags & PF_W)
        flag = "WRITE";
    else if (flags & PF_X)
        flag = "EXECUTE";
    else
        flag = "NONE";

    printf("The protection flags are: %s\n", flag);
}

static void print_elf_header_info(Elf32_Ehdr *ehdr) {
    printf("ELF Header:\n");
    printf("  Entry point: 0x%x\n", ehdr->e_entry);
    printf("  Program header offset: 0x%x\n", ehdr->e_phoff);
    printf("  Section header offset: 0x%x\n", ehdr->e_shoff);
    printf("  Number of program headers: %d\n", ehdr->e_phnum);
    printf("  Number of section headers: %d\n", ehdr->e_shnum);
}

static void map_program_header(Elf32_Phdr *phdr, int fd) {
    if (phdr->p_type != PT_LOAD) return;

    int prot = 0;
    if (phdr->p_flags & PF_R) prot |= PROT_READ;
    if (phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (phdr->p_flags & PF_X) prot |= PROT_EXEC;

    void *vaddr = (void *)(phdr->p_vaddr & ~0xfff);
    int offset = phdr->p_offset & ~0xfff;
    int padding = phdr->p_vaddr & 0xfff;

    void *map_start = mmap(vaddr, phdr->p_memsz + padding, prot, MAP_FIXED | MAP_PRIVATE, fd, offset);

    if (map_start == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(EXIT_FAILURE);
    }
}
