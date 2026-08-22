#include "ps14/integrity.h"
#include "ps14/logger.h"
#include <string.h>

// Verify PE file signature (Windows)
bool ps14_code_signature_verify_pe(const char* filepath) {
    #ifdef PS14_PLATFORM_WINDOWS
    HANDLE hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        PS14_LOG_ERROR("Failed to open file: %s", filepath);
        return false;
    }
    
    DWORD bytes_read;
    IMAGE_DOS_HEADER dos_header;
    if (!ReadFile(hFile, &dos_header, sizeof(dos_header), &bytes_read, NULL) || 
        bytes_read != sizeof(dos_header) || 
        dos_header.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(hFile);
        PS14_LOG_ERROR("Not a valid PE file: %s", filepath);
        return false;
    }
    
    // Seek to PE header
    SetFilePointer(hFile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    
    IMAGE_NT_HEADERS nt_headers;
    if (!ReadFile(hFile, &nt_headers, sizeof(nt_headers), &bytes_read, NULL) || 
        bytes_read != sizeof(nt_headers) || 
        nt_headers.Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle(hFile);
        PS14_LOG_ERROR("Not a valid NT headers: %s", filepath);
        return false;
    }
    
    // Check for certificate directory
    DWORD cert_dir_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
    DWORD cert_dir_offset = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress;
    
    if (cert_dir_size == 0 || cert_dir_offset == 0) {
        CloseHandle(hFile);
        PS14_LOG_WARNING("No certificate directory in PE file: %s", filepath);
        return false;
    }
    
    // File has a signature
    CloseHandle(hFile);
    PS14_LOG_DEBUG("PE file has signature: %s", filepath);
    return true;
    
    #else
    PS14_LOG_WARNING("PE signature verification not implemented for this platform");
    return false;
    #endif
}

// Verify using Windows CryptoAPI
bool ps14_code_signature_verify_crypto(const char* filepath) {
    #ifdef PS14_PLATFORM_WINDOWS
    HCERTSTORE hStore = NULL;
    HCCRYPTMSG hMsg = NULL;
    bool result = false;
    
    // Open the file
    HANDLE hFile = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Get file size
    DWORD file_size = GetFileSize(hFile, NULL);
    if (file_size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return false;
    }
    
    // Read the file
    BYTE* file_data = (BYTE*)malloc(file_size);
    if (!file_data) {
        CloseHandle(hFile);
        return false;
    }
    
    DWORD bytes_read;
    ReadFile(hFile, file_data, file_size, &bytes_read, NULL);
    CloseHandle(hFile);
    
    if (bytes_read != file_size) {
        free(file_data);
        return false;
    }
    
    // Try to decode the message
    if (CryptQueryObject(CERT_QUERY_OBJECT_FILE, file_data, file_size, CERT_QUERY_FORMAT_FLAG_ALL, 
                         CERT_QUERY_CONTENT_FLAG_ALL, 0, NULL, NULL, NULL, &hStore, &hMsg, NULL)) {
        result = true;
    }
    
    free(file_data);
    
    if (hMsg) CryptMsgClose(hMsg);
    if (hStore) CertCloseStore(hStore, 0);
    
    return result;
    
    #else
    return false;
    #endif
}
