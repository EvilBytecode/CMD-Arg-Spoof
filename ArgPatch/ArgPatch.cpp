#include <windows.h>
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")

typedef struct _PEB_FREE {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
} PEB_FREE, *PPEB_FREE;

typedef struct _UNICODE_STRING_CUSTOM {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING_CUSTOM, *PUNICODE_STRING_CUSTOM;

typedef struct _RTL_USER_PROCESS_PARAMETERS_CUSTOM {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING_CUSTOM ImagePathName;
    UNICODE_STRING_CUSTOM CommandLine;
} RTL_USER_PROCESS_PARAMETERS_CUSTOM, *PRTL_USER_PROCESS_PARAMETERS_CUSTOM;

// asm patch: mov rax, imm64; ret
// 0x48 = REX.W prefix
// 0xB8 = mov rax, imm64
// 8 bytes = addr to ret
// 0xC3 = ret instruction
BYTE patch[11] = {
    0x48, 0xB8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0xC3
};

static PPEB_FREE GetPEB() {
    PPEB_FREE peb;
#ifdef _WIN64
    peb = (PPEB_FREE)__readgsqword(0x60);
#else
    peb = (PPEB_FREE)__readfsdword(0x30);
#endif
    return peb;
}

static BOOL PatchGetCommandLineW(LPCWSTR newcmdline) {
    HMODULE hkernel = GetModuleHandleW(L"kernelbase.dll");
    if (!hkernel) hkernel = GetModuleHandleA("kernelbase.dll"); // to lazy to add an actual fce to do so
    if (!hkernel) return FALSE;
    FARPROC pfunc = GetProcAddress(hkernel, "GetCommandLineW");
    if (!pfunc) return FALSE;
    SIZE_T newlen = wcslen(newcmdline) * sizeof(WCHAR);
    LPWSTR pnewstring = (LPWSTR)VirtualAlloc(NULL, newlen + 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pnewstring) return FALSE;
    wcscpy_s(pnewstring, wcslen(newcmdline) + 1, newcmdline);
    // insert your fake cmd line address into the patch
    *(UINT64*)(patch + 2) = (UINT64)pnewstring;
    // patch the function->change memory protection->write our code->restore protection
    DWORD oldprotect;
    VirtualProtect(pfunc, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(pfunc, patch, sizeof(patch));
    VirtualProtect(pfunc, sizeof(patch), oldprotect, &oldprotect);
    return TRUE;
}

int main() {
    PPEB_FREE peb = GetPEB();
    PRTL_USER_PROCESS_PARAMETERS_CUSTOM params = (PRTL_USER_PROCESS_PARAMETERS_CUSTOM)peb->ProcessParameters;
    LPCWSTR fakecmd = L"notepad.exe fake_argument.txt";
    params->CommandLine.Length = (USHORT)(wcslen(fakecmd) * sizeof(WCHAR));
    params->CommandLine.MaximumLength = params->CommandLine.Length + sizeof(WCHAR);
    wcscpy_s(params->CommandLine.Buffer, wcslen(fakecmd) + 1, fakecmd);
    PatchGetCommandLineW(fakecmd);
    MessageBoxW(NULL, GetCommandLineW(), L"Fake Command Line", MB_OK);
    return 0;
}
