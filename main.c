// https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
// https://refspecs.linuxfoundation.org/elf/elf.pdf

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define ELF_HEADER_SIZE 0x40
#define PROGRAM_HEADER_SIZE 0x38
#define SECTION_HEADER_SIZE 0x40

#define SHT_PROGBITS 0x1
#define SHT_SYMTAB 0x2
#define SHT_STRTAB 0x3

#define SHF_WRITE 0x1
#define SHF_STRINGS 0x20

void write_elf64_header(FILE* file, uint64_t e_shoff, uint16_t e_shnum, uint16_t e_shstrndx) {
    char header[64] = {};

    // The magic number
    char magic_number[] = { 0x7F, 'E', 'L', 'F' };
    memcpy(header, magic_number, sizeof(magic_number));

    // To indicate that this is 64 bit
    header[0x04] = 2;
    // To indicate that this is little endian (that's the format of the integers)
    header[0x05] = 1;
    // The version of ELF - we are using 1
    header[0x06] = 1;
    // This is to indicate a System V ABI - though not sure if this really matters
    // for what we are doing
    header[0x07] = 0;
    // ABI version - Linux apparently ignores this
    header[0x08] = 0;

    // Reserved padding bytes
    memset(header + 0x09, 0, 7);

    // This indicates that the file is a relocatable file (or object file)
    uint16_t e_type = 0x01;
    memcpy(header + 0x10, &e_type, sizeof(uint16_t));

    // This indicates that we are on the AMD x86_64 ISA
    uint16_t e_machine = 0x3E;
    memcpy(header + 0x12, &e_machine, sizeof(uint16_t));

    // This is the version of ELF - again, we are using 1
    uint32_t e_version = 1;
    memcpy(header + 0x14, &e_version, sizeof(uint32_t));

    // The memory address entry point of the ELF - since we have none, this is 0
    uint64_t e_entry = 0;
    memcpy(header + 0x18, &e_entry, sizeof(uint64_t));

    // Start of the program header table (we don't have one)
    uint64_t e_phoff = 0;
    memcpy(header + 0x20, &e_phoff, sizeof(uint64_t));

    // Start of the section header table
    memcpy(header + 0x28, &e_shoff, sizeof(uint64_t));

    // Flags - this depends on the architecture
    uint32_t e_flags = 0;
    memcpy(header + 0x30, &e_flags, sizeof(uint32_t));

    // The size of this header - for us, it is 64
    uint16_t e_ehsize = ELF_HEADER_SIZE;
    memcpy(header + 0x34, &e_ehsize, sizeof(uint16_t));

    // The size of the program header (0 because we don't have one)
    uint16_t e_phentsize = 0;
    memcpy(header + 0x36, &e_phentsize, sizeof(uint16_t));

    // The number of entries in the program header (we don't have any)
    uint16_t e_phnum = 0;
    memcpy(header + 0x38, &e_phnum, sizeof(uint16_t));

    // The size of a section header table entry (0x40 for 64-bit)
    uint16_t e_shentsize = SECTION_HEADER_SIZE;
    memcpy(header + 0x3A, &e_shentsize, sizeof(uint16_t));

    // The number of entries in the section header table
    memcpy(header + 0x3C, &e_shnum, sizeof(uint16_t));

    // Index of the section header table entry that contains the names 
    // of all the sections
    memcpy(header + 0x3E, &e_shstrndx, sizeof(e_shstrndx));

    fwrite(header, 1, 64, file);
}

void write_section_header(FILE* file, uint32_t sh_name, uint32_t sh_type, uint64_t sh_flags, uint64_t sh_offset, uint64_t sh_size) {
    char header[0x40] = {};

    *(uint32_t*)header = sh_name;
    *(uint32_t*)(header + 0x04) = sh_type;
    *(uint64_t*)(header + 0x08) = sh_flags;

    // The address - not applicable for us at the moment.
    *(uint64_t*)(header + 0x10) = 0;

    *(uint64_t*)(header + 0x18) = sh_offset;
    *(uint64_t*)(header + 0x20) = sh_size;

    // Section index of an associated section
    *(uint32_t*)(header + 0x28) = 0;

    // Extra info about the section.
    *(uint32_t*)(header + 0x2C) = 0;

    // The required alignment of the section - we do not care.
    *(uint64_t*)(header + 0x30) = 0;

    // The size of each entry in bytes for sections that contains fixed-size
    // entires. This doesn't matter I don't think.
    *(uint64_t*)(header + 0x38) = 0;

    fwrite(header, 1, 0x40, file);
}

// https://refspecs.linuxbase.org/elf/gabi4+/ch4.symtab.html
void write_symbol_table_entry(FILE* file, uint32_t st_name, uint8_t st_info, uint8_t st_other, uint16_t st_shndx, uint64_t st_value, uint64_t st_size) {
    char symbol[24] = {};
    *(uint32_t*)symbol = st_name; // This is the index into a string section - it points to the first character of the string if I remember correctly
    *(uint8_t*)(symbol + 4) = st_info;
    *(uint8_t*)(symbol + 5) = st_other;
    *(uint16_t*)(symbol + 6) = st_shndx; // The index of the section that the symbol is in
    *(uint64_t*)(symbol + 8) = st_value; // The address of the actual symbol, relative to the start of the respective section I think
    *(uint64_t*)(symbol + 16) = st_size;

    fwrite(symbol, 1, 24, file);
}

int main(void) {
    FILE* file = fopen("test.o", "w");
    if (!file) {
        printf("Cannot open file: %s\n", strerror(errno));
        return -1;
    }

    char section_names_section[] = ".shstrtab\0.text";
    char generic_section[] = "The Quick Brown Fox jumped over the Lazy Dog";

    uint64_t data_size = sizeof(section_names_section) + sizeof(generic_section);
    uint64_t shoff = ELF_HEADER_SIZE + data_size;

    write_elf64_header(file, shoff, 3, 1);

    fwrite(section_names_section, 1, sizeof(section_names_section), file);
    fwrite(generic_section, 1, sizeof(generic_section), file);

    // The specs require that the first section header has to be null
    char null_header[0x40] = {};
    fwrite(null_header, 1, 0x40, file);
    write_section_header(file, 0, SHT_STRTAB, SHF_STRINGS, ELF_HEADER_SIZE, sizeof(section_names_section));
    write_section_header(file, 10, SHT_PROGBITS, 0, ELF_HEADER_SIZE + sizeof(section_names_section), sizeof(generic_section));

    fclose(file);
    return 0;
}
