#include <sigma0/sigma0.h>
#include <sigma0/elf.h>

Elf32_Shdr *elf_get_shdr( Elf32_Ehdr *, unsigned );
Elf32_Shdr *elf_get_shdr_byname( Elf32_Ehdr *, char * );

Elf32_Phdr *elf_get_phdr( Elf32_Ehdr *elf, unsigned index ){
	Elf32_Phdr *ret = NULL;

	if ( index < elf->e_phnum ){
		uintptr_t temp = (uintptr_t)elf + elf->e_phoff;
		temp += elf->e_phentsize * index;

		ret = (Elf32_Phdr *)temp;
	}

	return ret;
}

Elf32_Sym  *elf_get_sym( Elf32_Ehdr *, int, char * );
char       *elf_get_sym_name( Elf32_Ehdr *, Elf32_Sym *, char * );
Elf32_Sym  *elf_get_sym_byname( Elf32_Ehdr *, char *, char * );
