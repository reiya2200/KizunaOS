#include "main.h"

#define KERNEL_NAME L"KizunaOS.ELF"

EFI::EFI_GUID EFI_FILE_INFO_GUID = {0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

void puts(EFI *efi, unsigned short *s)
{
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, s);
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\n");
}

void puth(EFI *efi, unsigned long long val, unsigned char num_digits)
{
    int i;
    unsigned short unicode_val;
    unsigned short str[100];

    for (i = num_digits - 1; i >= 0; i--)
    {
        unicode_val = (unsigned short)(val & 0x0f);
        if (unicode_val < 0xa)
            str[i] = L'0' + unicode_val;
        else
            str[i] = L'A' + (unicode_val - 0xa);
        val >>= 4;
    }
    str[num_digits] = L'\0';

    puts(efi, str);
}

int strcmp(const char *s1, const char *s2)
{
    const unsigned char *ss1, *ss2;
    for (ss1 = (const unsigned char *)s1, ss2 = (const unsigned char *)s2;
         *ss1 == *ss2 && *ss1 != '\0';
         ss1++, ss2++)
        ;
    return *ss1 - *ss2;
}

void memset(void *dst, int value, int size)
{
    const unsigned char uc = value;
    unsigned char *p = reinterpret_cast<unsigned char *>(dst);
    while (size-- > 0)
        *p++ = uc;
}

extern "C"
{
    void memcpy(void *dst, const void *src, int size)
    {
        char *p1 = reinterpret_cast<char *>(dst);
        const char *p2 = reinterpret_cast<const char *>(src);

        while (size-- > 0)
            *p1++ = *p2++;
    }
}

unsigned long long file_sizeof(EFI *efi, EFI::EFI_FILE_PROTOCOL *file)
{
    EFI::EFI_FILE_INFO *file_info;
    unsigned long long file_size = 180;
    unsigned long long file_buf[180];
    unsigned long long stat = file->GetInfo(file, &EFI_FILE_INFO_GUID, &file_size, file_buf);
    file_info = (EFI::EFI_FILE_INFO *)file_buf;
    return file_info->FileSize;
}

void load_kernel(EFI::EFI_HANDLE ImageHandle, EFI *efi, FrameBuffer *fb)
{

    EFI::EFI_STATUS status = 0;
    EFI::EFI_FILE_PROTOCOL *root, *kernel_file;

    /* open kernel file */
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Open root directory...");
    status = efi->getSimpleFileSystemProtocol()->OpenVolume(efi->getSimpleFileSystemProtocol(), &root);
    if (status == EFI::EFI_SUCCESS)
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
    }
    else
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\nCannot open root directory.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"An unexpected error has occurred in opening root directory.\r\n");
        return;
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Load kernel file...");
    status = root->Open(root, &kernel_file, (EFI::CHAR16 *)L"KizunaOS.ELF", EFI_FILE_MODE_READ, 0);
    if (status == EFI::EFI_SUCCESS)
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"The kernel file name is KizunaOS.ELF\r\n");
    }
    else
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\nCannot open kernel file.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"An unexpected error has occurred in opening kernel file.\r\n");
        return;
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Kernel file successfully opened!\r\n\n");

    /* read kernel data */
    unsigned long long kernel_size = 0;
    kernel_size = file_sizeof(efi, kernel_file);
    EFI::EFI_PHYSICAL_ADDRESS kernel_addr = 0x00100000lu;
    EFI::EFI_PHYSICAL_ADDRESS kernel_tmp_addr = 0x01100000lu;
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Allocate memory to 0x100000...");
    status = efi->getSystemTable()->BootServices->AllocatePages(EFI::AllocateAddress, EFI::EfiLoaderData, (kernel_size + 0xfff) / 0x1000, &kernel_addr);
    if (status == EFI::EFI_SUCCESS)
    {
        status = efi->getSystemTable()->BootServices->AllocatePages(EFI::AllocateAddress, EFI::EfiLoaderData, (kernel_size + 0xfff) / 0x1000, &kernel_tmp_addr);
        if (status == EFI::EFI_SUCCESS)
        {
            efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
        }
        else
        {
            efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\nCannot allocate memory to 0x100000.\r\n");
            efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"An unexpected error has occurred in allocating memory.\r\n");
            return;
        }
    }


    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Read kernel data to memory...");
    status = kernel_file->Read(kernel_file, &kernel_size, reinterpret_cast<EFI::VOID *>(kernel_tmp_addr));
    if (status == EFI::EFI_SUCCESS)
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
    }
    else
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\nCannot read kernel data to memory.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"An unexpected error has occurred in reading kernel data\r\n");
        return;
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Kernel data successfully stored to memory!\r\n");

    /* check elf-header */
    Elf64_Ehdr *elf_header = reinterpret_cast<Elf64_Ehdr *>(kernel_tmp_addr);
    unsigned long long elf_head_size = sizeof(elf_header);
    /* header's magic number == ELF ? */
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Check ELF magic number in ELF header...");
    if ((EFI::CHAR8 *)elf_header->e_ident[0] == (EFI::CHAR8 *)'\x7f' && (EFI::CHAR8 *)elf_header->e_ident[1] == (EFI::CHAR8 *)'E' && (EFI::CHAR8 *)elf_header->e_ident[2] == (EFI::CHAR8 *)'L' && (EFI::CHAR8 *)elf_header->e_ident[3] == (EFI::CHAR8 *)'F')
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"This file is correct ELF file\r\n");
    }
    else
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"\r\nHeader's magic number is not ELF.\r\n");
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"This file is not a correct ELF file.\r\n");
        return;
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"ELF header successfully checked!\r\n");

    /* Relocate sections */
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Relocate program data...");
    Elf64_Phdr *elf_program_headers = reinterpret_cast<Elf64_Phdr *>(kernel_tmp_addr + elf_header->e_phoff);
    Elf64_Shdr *elf_section_headers = reinterpret_cast<Elf64_Shdr *>(kernel_tmp_addr + elf_header->e_shoff);

    for (unsigned int i = 0; i < elf_header->e_phnum; ++i)
    {
        Elf64_Phdr program_header = elf_program_headers[i];
        if (program_header.p_type != PT_LOAD)
            continue;
        memcpy(reinterpret_cast<void *>(kernel_addr + program_header.p_vaddr), reinterpret_cast<void *>(kernel_tmp_addr + program_header.p_offset), program_header.p_filesz);
        memset(reinterpret_cast<void *>(kernel_addr + program_header.p_vaddr + program_header.p_filesz), 0, program_header.p_memsz - program_header.p_filesz);
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"ELF sections successfully relocated!\r\n");

    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Search symbol table section...");
    Elf64_Shdr symtab_sections;
    for (unsigned int k = 0; k < elf_header->e_shnum; ++k)
    {
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L".");
        Elf64_Shdr search_section = elf_section_headers[k];
        if (search_section.sh_type != SHT_SYMTAB)
            continue;
        symtab_sections = search_section;
        break;
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"Relocate dynamic symbols...");
    for (unsigned int i = 0; i < elf_header->e_shnum; ++i)
    {
        Elf64_Shdr section_header = elf_section_headers[i];
        if (section_header.sh_type != SHT_RELA)
            continue;
        efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L".");
        for (unsigned int j = 0; j < section_header.sh_size / section_header.sh_entsize; ++j)
        {
            Elf64_Rela *rela = (Elf64_Rela *)(kernel_tmp_addr + section_header.sh_offset + section_header.sh_entsize * j);
            Elf64_Sym *target_sym = nullptr;
            for (unsigned int k = 0; k < symtab_sections.sh_size / symtab_sections.sh_entsize; ++k)
            {
                Elf64_Sym *search_sym = (Elf64_Sym *)(kernel_tmp_addr + symtab_sections.sh_offset + symtab_sections.sh_entsize * k);
                if (search_sym->st_value != static_cast<unsigned long long>(rela->r_addend))
                    continue;
                target_sym = search_sym;
                break;
            }
            unsigned long long *memto = (unsigned long long *)(kernel_addr + rela->r_offset);
            *memto = kernel_addr + target_sym->st_value;
        }
    }
    efi->getSystemTable()->ConOut->OutputString(efi->getSystemTable()->ConOut, (EFI::CHAR16 *)L"done.\r\n");

    /* Make boot arguments */
    unsigned long long entry_point = elf_header->e_entry + kernel_addr;
    BootStruct boot_info;
    boot_info.frame_buffer = *fb;
    kernel_file->Close(kernel_file);
    root->Close(root);
    efi->getSystemTable()->BootServices->FreePages(kernel_tmp_addr, (kernel_size + 0xfff)/0x1000);
    efi->getSystemTable()->ConOut->ClearScreen(efi->getSystemTable()->ConOut);

    /* Ready For ExitBootServices() */
    EFI::EFI_MEMORY_DESCRIPTOR *MemoryMap = nullptr;
    EFI::UINTN MemoryMapSize = 0;
    EFI::UINTN MapKey, DescriptorSize;
    EFI::UINT32 DescriptorVersion;
    do
    {
        status = efi->getSystemTable()->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        while (status == EFI::EFI_BUFFER_TOO_SMALL)
        {
            if (MemoryMap)
            {
                efi->getSystemTable()->BootServices->FreePool(MemoryMap);
            }
            status = efi->getSystemTable()->BootServices->AllocatePool(EFI::EfiLoaderData, MemoryMapSize, reinterpret_cast<EFI::VOID **>(&MemoryMap));
            status = efi->getSystemTable()->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        }
        boot_info.frame_buffer.frame_buffer_base = reinterpret_cast<unsigned long long *>(efi->getGraphicsOutputProtocol()->Mode->FrameBufferBase);
        puts(&boot_info.frame_buffer, "Exited!");
        status = efi->getSystemTable()->BootServices->ExitBootServices(ImageHandle, MapKey);
    } while (status != EFI::EFI_SUCCESS);

    unsigned long long stack_pointer = 0x7f0000lu;
    jump_entry(&boot_info, entry_point, stack_pointer);
}