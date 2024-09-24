// ExeSum.cpp --- Check/Clear/Set the check-sum of an EXE/DLL file
#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>
#include <tchar.h>

void version(void)
{
    puts("ExeSum by katahiromz Version 0.2");
    puts("License: MIT");
}

void usage(void)
{
    puts(
        "exesum.exe --- Check/Clear/Set the check-sum of an EXE/DLL file\n"
        "\n"
        "Usage:\n"
        "    exesum --check your_file.exe\n"
        "    exesum --clear your_file.exe\n"
        "    exesum --set your_file.exe\n"
        "    exesum --set --force your_file.exe VALUE\n"
        "    exesum /?\n"
        "    exesum --help\n"
        "    exesum --version\n");
}

bool do_check_checksum(const _TCHAR *filename, DWORD *pHeaderSum, DWORD *pCheckSum)
{
    DWORD headerSum = 0, checkSum = 0;
    DWORD result = MapFileAndCheckSum(filename, &headerSum, &checkSum);

    if (result != CHECKSUM_SUCCESS)
    {
        _ftprintf(stderr, _T("Error: %d\n"), result);
        return false;
    }

    if (pHeaderSum)
        *pHeaderSum = headerSum;
    if (pCheckSum)
        *pCheckSum = checkSum;

    _tprintf(_T("Header checksum: 0x%X\n"), headerSum);
    _tprintf(_T("Calculated checksum: 0x%X\n"), checkSum);
    return true;
}

bool do_write_checksum(const _TCHAR *filename, DWORD headerSum)
{
    HANDLE hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Error opening file: %d\n"), GetLastError());
        return false;
    }

    IMAGE_DOS_HEADER dosHeader;
    DWORD bytesRead;
    if (!ReadFile(hFile, &dosHeader, sizeof(dosHeader), &bytesRead, NULL) || bytesRead != sizeof(dosHeader))
    {
        _ftprintf(stderr, _T("Error reading DOS header: %d\n"), GetLastError());
        CloseHandle(hFile);
        return false;
    }

    if (SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        _ftprintf(stderr, _T("Error seeking to NT headers: %d\n"), GetLastError());
        CloseHandle(hFile);
        return false;
    }

    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadFile(hFile, &ntHeaders, sizeof(ntHeaders), &bytesRead, NULL) || bytesRead != sizeof(ntHeaders))
    {
        _ftprintf(stderr, _T("Error reading NT headers: %d\n"), GetLastError());
        CloseHandle(hFile);
        return false;
    }

    DWORD checksumOffset = dosHeader.e_lfanew +
                           FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) +
                           FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum);

    if (SetFilePointer(hFile, checksumOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        _ftprintf(stderr, _T("Error seeking to checksum offset: %d\n"), GetLastError());
        CloseHandle(hFile);
        return false;
    }

    DWORD bytesWritten;
    if (!WriteFile(hFile, &headerSum, sizeof(headerSum), &bytesWritten, NULL) || bytesWritten != sizeof(headerSum))
    {
        _ftprintf(stderr, _T("Error writing checksum: %d\n"), GetLastError());
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);
    return true;
}

bool do_set_checksum(const _TCHAR *filename, DWORD *pCheckSum)
{
    DWORD headerSum, checkSum;
    if (!do_check_checksum(filename, &headerSum, &checkSum))
        return false;

    DWORD newCheckSum = pCheckSum ? *pCheckSum : checkSum;
    if (!do_write_checksum(filename, newCheckSum))
        return false;

    _tprintf(_T("Checksum set successfully.\n"));
    return true;
}

bool do_clear_checksum(const _TCHAR *filename)
{
    if (!do_write_checksum(filename, 0))
        return false;

    _tprintf(_T("Checksum cleared successfully.\n"));
    return true;
}

int _tmain(int argc, _TCHAR **argv)
{
    if (argc <= 1)
    {
        usage();
        return 0;
    }

    for (int iarg = 1; iarg < argc; ++iarg)
    {
        if (_tcscmp(argv[1], _T("/?")) == 0 || _tcscmp(argv[1], _T("--help")) == 0)
        {
            usage();
            return 0;
        }
        if (_tcscmp(argv[iarg], _T("--version")) == 0)
        {
            version();
            return 0;
        }
        if (argc == 5 && _tcscmp(argv[iarg], _T("--set")) == 0 && _tcscmp(argv[iarg + 1], _T("--force")) == 0)
        {
            DWORD value = _tcstoul(argv[iarg + 3], NULL, 0);
            return do_set_checksum(argv[iarg + 2], &value) ? 0 : 1;
        }
        if (argc != 3)
        {
            usage();
            return 1;
        }
        if (_tcscmp(argv[iarg], _T("--check")) == 0)
        {
            return do_check_checksum(argv[iarg + 1], NULL, NULL) ? 0 : 1;
        }
        if (_tcscmp(argv[iarg], _T("--clear")) == 0)
        {
            return do_clear_checksum(argv[iarg + 1]) ? 0 : 1;
        }
        if (_tcscmp(argv[iarg], _T("--set")) == 0)
        {
            return do_set_checksum(argv[iarg + 1], NULL) ? 0 : 1;
        }
    }

    usage();
    return 1;
}

#ifdef _UNICODE
int main(void)
{
    INT argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    INT ret = wmain(argc, argv);
    LocalFree(argv);
    return ret;
}
#endif
