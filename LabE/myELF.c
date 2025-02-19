#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

#define MAX_FILES 2

struct fun_desc {
    char *name;
    char (*fun)(char);
};

size_t file_sizes[MAX_FILES] = {0, 0};
int debug_mode = 0;
int current_fd[MAX_FILES] = {-1, -1};           
void *current_map[MAX_FILES] = {NULL, NULL};   
int num_files = 0;

char toggle_debug_mode(char c) {
    debug_mode = !debug_mode;
    printf("Debug mode is now %s\n", debug_mode ? "on" : "off");
    return 0;
}

char examine_elf_file(char c) {
    if (num_files >= MAX_FILES) {
        printf("Cannot open more files. Maximum is %d\n", MAX_FILES);
        return 0;
    }

    char filename[100];
    printf("Enter ELF file name: ");
    scanf("%s", filename);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 0;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        perror("Error getting file size");
        close(fd);
        return 0;
    }

    void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 0;
    }
    file_sizes[num_files] = st.st_size;
    // Check if this is an ELF file
    Elf32_Ehdr *header = (Elf32_Ehdr *)map_start;
    if (header->e_ident[EI_MAG0] != ELFMAG0 ||
        header->e_ident[EI_MAG1] != ELFMAG1 ||
        header->e_ident[EI_MAG2] != ELFMAG2 ||
        header->e_ident[EI_MAG3] != ELFMAG3) {
        printf("Not an ELF file\n");
        munmap(map_start, st.st_size);
        close(fd);
        return 0;
    }

    current_fd[num_files] = fd;
    current_map[num_files] = map_start;

    // Print header information
    printf("\nELF Header information:\n");
    printf("Magic numbers: %c%c%c\n", 
           header->e_ident[EI_MAG1],
           header->e_ident[EI_MAG2],
           header->e_ident[EI_MAG3]);
    printf("Data encoding scheme: %d\n", header->e_ident[EI_DATA]);
    printf("Entry point: 0x%x\n", header->e_entry);
    printf("Section header table offset: %d\n", header->e_shoff);
    printf("Number of section header entries: %d\n", header->e_shnum);
    printf("Size of each section header entry: %d\n", header->e_shentsize);
    printf("Program header table offset: %d\n", header->e_phoff);
    printf("Number of program header entries: %d\n", header->e_phnum);
    printf("Size of each program header entry: %d\n", header->e_phentsize);

    num_files++;
    return 0;
}

char print_section_names(char c) {
    if (num_files == 0) {
        printf("No ELF files loaded\n");
        return 0;
    }

    for (int file_idx = 0; file_idx < num_files; file_idx++) {
        if (current_map[file_idx] == NULL) {
            continue;
        }

        Elf32_Ehdr *header = (Elf32_Ehdr *)current_map[file_idx];
        Elf32_Shdr *section_header = (Elf32_Shdr *)((char *)current_map[file_idx] + header->e_shoff);
        
        // Get section header string table
        Elf32_Shdr *sh_strtab = &section_header[header->e_shstrndx];
        const char *const sh_strtab_p = current_map[file_idx] + sh_strtab->sh_offset;

        if (debug_mode) {
            printf("Section header string table index: %d\n", header->e_shstrndx);
            printf("Number of section headers: %d\n", header->e_shnum);
        }

        printf("\nFile %d Section Headers:\n", file_idx + 1);
        printf("[Nr] Name                 Addr     Off      Size     Type\n\n");

        for (int i = 0; i < header->e_shnum; i++) {
            Elf32_Shdr *section = &section_header[i];
            const char *section_name = sh_strtab_p + section->sh_name;

            printf("[%2d] %-20s %08x %08x %08x %d\n",
                   i,                    
                   section_name,         
                   section->sh_addr,     
                   section->sh_offset,   
                   section->sh_size,     
                   section->sh_type);    

            if (debug_mode) {
                printf("      Name offset: %d\n", section->sh_name);
            }
            printf("\n");
        }
        printf("\n");
    }
    return 0;
}

char print_symbols(char c) {
    if (num_files == 0) {
        printf("No ELF files loaded\n");
        return 0;
    }

    for (int file_idx = 0; file_idx < num_files; file_idx++) {
        if (current_map[file_idx] == NULL) {
            continue;
        }

        printf("\nFile %d Symbol Table:\n", file_idx + 1);

        Elf32_Ehdr *header = (Elf32_Ehdr *)current_map[file_idx];
        Elf32_Shdr *section_header = (Elf32_Shdr *)((char *)current_map[file_idx] + header->e_shoff);

        Elf32_Shdr *symtab = NULL;
        Elf32_Shdr *strtab = NULL;

        // Get section header string table
        Elf32_Shdr *sh_strtab = &section_header[header->e_shstrndx];
        const char *const sh_strtab_p = current_map[file_idx] + sh_strtab->sh_offset;

        for (int i = 0; i < header->e_shnum; i++) {
            const char *section_name = sh_strtab_p + section_header[i].sh_name;
            
            if (strcmp(section_name, ".symtab") == 0) {
                symtab = &section_header[i];
            }
            else if (strcmp(section_name, ".strtab") == 0) {
                strtab = &section_header[i];
            }
        }

        if (!symtab || !strtab) {
            printf("no symbol table found in file %d\n", file_idx + 1);
            continue;
        }

        if (debug_mode) {
            printf("symbol table size: %d\n", symtab->sh_size);
            printf("number of symbols: %d\n", symtab->sh_size / sizeof(Elf32_Sym));
            printf("string table offset: 0x%x\n", strtab->sh_offset);
        }

        Elf32_Sym *symbols = (Elf32_Sym *)((char *)current_map[file_idx] + symtab->sh_offset);
        const char *str_table = (char *)current_map[file_idx] + strtab->sh_offset;
        int num_symbols = symtab->sh_size / sizeof(Elf32_Sym);

        printf("[Nr] Value     Sec_Index Section_Name    Symbol_Name\n\n");
        for (int i = 0; i < num_symbols; i++) {
            Elf32_Sym *sym = &symbols[i];
            const char *sym_name = str_table + sym->st_name;
            
            const char *section_name = "UNDEF";
            if (sym->st_shndx < header->e_shnum && sym->st_shndx != SHN_UNDEF) {
                section_name = sh_strtab_p + section_header[sym->st_shndx].sh_name;
            }

            printf("[%2d] 0x%-8x %-10d %-14s %s\n",
                   i,                    
                   sym->st_value,        
                   sym->st_shndx,        
                   section_name,         
                   sym_name);            

            if (debug_mode) {
                printf("     Name offset: %d\n", sym->st_name);
            }
            printf("\n");
        }
        printf("\n");
    }
    return 0;
}

unsigned char check_symbol(const char *name, Elf32_Sym *symbols, int num_symbols,
                         const char *str_table, Elf32_Shdr *section_headers, 
                         const Elf32_Ehdr *header) {
    Elf32_Shdr *section;
    Elf32_Sym *sym;
    const char *sym_name;

    for (int i = 0; i < num_symbols; ++i) {
        sym = &symbols[i];
        sym_name = str_table + sym->st_name;
        
        if (sym->st_info == STT_SECTION) {
            section = &section_headers[sym->st_shndx];
            sym_name = (char *)(current_map[0] + 
                      section_headers[header->e_shstrndx].sh_offset + 
                      section->sh_name);
        }

        if (strcmp(name, sym_name) == 0 && sym->st_shndx != SHN_UNDEF)
            return 1;
    }
    return 0;
}

char check_files_for_merge(char c) {
    if (num_files < 2) {
        printf("Must have exactly two ELF files loaded\n");
        return 0;
    }

    // Initialize the arrays for both files
    Elf32_Ehdr *headers[2];
    Elf32_Shdr *section_headers[2];
    Elf32_Shdr *symtabs[2] = {NULL, NULL};
    Elf32_Shdr *strtabs[2] = {NULL, NULL};
    Elf32_Sym *symbols[2];
    const char *str_tables[2];
    int num_symbols[2];
    int symtab_count[2] = {0, 0};
    int symtab_indices[2] = {-1, -1};
    int isgood = 1;
    
    for (int i = 0; i < 2; i++) {
        headers[i] = (Elf32_Ehdr *)current_map[i];
        section_headers[i] = (Elf32_Shdr *)(current_map[i] + headers[i]->e_shoff);

        for (int j = 0; j < headers[i]->e_shnum; ++j) {
            if (section_headers[i][j].sh_type == SHT_SYMTAB || 
                section_headers[i][j].sh_type == SHT_DYNSYM) {
                symtab_count[i]++;
                symtab_indices[i] = j;
            }
        }
    }

    // check if there is only 1 symbol
    if (symtab_count[0] != 1 || symtab_count[1] != 1) {
        fprintf(stderr, "invalid number of symbol tables\n");
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        symtabs[i] = &section_headers[i][symtab_indices[i]];
        symbols[i] = (Elf32_Sym *)(current_map[i] + symtabs[i]->sh_offset);
        strtabs[i] = &section_headers[i][symtabs[i]->sh_link];
        str_tables[i] = (char *)(current_map[i] + strtabs[i]->sh_offset);
        num_symbols[i] = symtabs[i]->sh_size / sizeof(Elf32_Sym);
    }

    for (int i = 1; i < num_symbols[0]; ++i) {
        Elf32_Sym *sym = &symbols[0][i];
        const char *sym_name;

        if (sym->st_info == STT_SECTION) {
            Elf32_Shdr *section = &section_headers[0][sym->st_shndx];
            sym_name = (char *)(current_map[0] + 
                      section_headers[0][headers[0]->e_shstrndx].sh_offset + 
                      section->sh_name);
        } else {
            sym_name = str_tables[0] + sym->st_name;
        }

        // Check for undefined symbols
        if (sym->st_shndx == SHN_UNDEF &&
            !check_symbol(sym_name, symbols[1], num_symbols[1], 
                         str_tables[1], section_headers[1], headers[1])) {
            fprintf(stderr, "symbol %s undefined\n", sym_name);
            isgood = 0;
        }

        // Check for multiply defined symbols
        else if (sym->st_shndx != SHN_UNDEF &&
                 check_symbol(sym_name, symbols[1], num_symbols[1], 
                            str_tables[1], section_headers[1], headers[1])) {
            fprintf(stderr, "symbol %s multiply defined\n", sym_name);
            isgood = 0;
        }
    }
    if (isgood)
        printf("files are good for merging, no errors found");
    return 0;
}

char merge_elf_files(char c) {
    if (num_files < 2) {
        printf("must have exactly two ELF files loaded\n");
        return 0;
    }

    FILE* output = fopen("out.ro", "wb");
    if (!output) {
        printf("rror creating output file\n");
        return 0;
    }

    Elf32_Ehdr *header1 = (Elf32_Ehdr *)current_map[0];
    Elf32_Ehdr *header2 = (Elf32_Ehdr *)current_map[1];
    Elf32_Shdr *sections1 = (Elf32_Shdr *)(current_map[0] + header1->e_shoff);
    Elf32_Shdr *sections2 = (Elf32_Shdr *)(current_map[1] + header2->e_shoff);

    fwrite(header1, 1, sizeof(Elf32_Ehdr), output);

    Elf32_Shdr *new_sections = malloc(header1->e_shnum * sizeof(Elf32_Shdr));
    memcpy(new_sections, sections1, header1->e_shnum * sizeof(Elf32_Shdr));

    char *strtab1 = (char *)current_map[0] + sections1[header1->e_shstrndx].sh_offset;
    char *strtab2 = (char *)current_map[1] + sections2[header2->e_shstrndx].sh_offset;

    if (debug_mode) {
        printf("Procesing sections for merge...\n");
    }

    for (int i = 0; i < header1->e_shnum; i++) {
        // Update section offset 
        new_sections[i].sh_offset = ftell(output);
        char *section_name = strtab1 + sections1[i].sh_name;

        if (debug_mode) {
            printf("Procesing section: %s\n", section_name);
        }

        fwrite(current_map[0] + sections1[i].sh_offset, 1, sections1[i].sh_size, output);

        if (strcmp(section_name, ".text") == 0 ||
            strcmp(section_name, ".data") == 0 ||
            strcmp(section_name, ".rodata") == 0) {

            // check for matching section in second file
            for (int j = 0; j < header2->e_shnum; j++) {
                char *section_name2 = strtab2 + sections2[j].sh_name;
                
                if (strcmp(section_name, section_name2) == 0) {
                    if (debug_mode) {
                        printf("merging section %s from second file\n", section_name);
                    }

                    fwrite(current_map[1] + sections2[j].sh_offset, 
                           1, sections2[j].sh_size, output);

                    new_sections[i].sh_size += sections2[j].sh_size;
                    break;
                }
            }
        }
        // set nex section to 4 byte 
        long current_pos = ftell(output);
        long padding = (4 - (current_pos % 4)) % 4;
        if (padding > 0) {
            char pad[4] = {0};
            fwrite(pad, 1, padding, output);
        }
    }

    long section_header_offset = ftell(output);
    fwrite(new_sections, sizeof(Elf32_Shdr), header1->e_shnum, output);
    fseek(output, 32, SEEK_SET); // 
    fwrite(&section_header_offset, sizeof(Elf32_Off), 1, output);

    if (debug_mode) {
        printf("section header offset: %ld\n", section_header_offset);
    }

    free(new_sections);
    fclose(output);

    printf("Succesfully created merged file 'out.ro'\n");
    return 0;
}



char quit(char c) {
    for(int i = 0; i < MAX_FILES; i++) {
        if(current_map[i] != NULL) {
            munmap(current_map[i], file_sizes[i]);
            current_map[i] = NULL;
        }
        if(current_fd[i] != -1) {
            close(current_fd[i]);
            current_fd[i] = -1;
        }
    }
    exit(0);
    return 0;
}

int main() {
    struct fun_desc menu[] = {
        {"Toggle Debug Mode", toggle_debug_mode},
        {"Examine ELF File", examine_elf_file},
        {"Print Section Names", print_section_names},
        {"Print Symbols", print_symbols},
        {"Check Files for Merge", check_files_for_merge},
        {"Merge ELF Files", merge_elf_files},
        {"Quit", quit},
        {NULL, NULL}
    };

    while(1) {
        printf("\nChoose action:\n");
        for(int i = 0; menu[i].name != NULL; i++) {
            printf("%d-%s\n", i, menu[i].name);
        }

        unsigned int option;
        printf("\nOption: ");
        scanf("%u", &option);

        if (option>6 || option<0){
            printf("Not within bounds\n");
            continue;
        }
        
        
        if(menu[option].name == NULL) {
            printf("Not within bounds\n");
            exit(0);
        }
        if(option == 6)
            printf("Quit the program");
        
        menu[option].fun(0);
    }

    return 0;
}