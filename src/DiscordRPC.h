#pragma once
#include <Windows.h>
#include <string>
#include <thread>
#include <vector>

// Simple header-only Discord RPC implementation using Named Pipes
// Based on Discord IPC protocol
namespace DiscordRPC {
    inline HANDLE hPipe = INVALID_HANDLE_VALUE;
    inline bool g_Running = false;

    struct Activity {
        std::string details;
        std::string state;
        std::string largeImage = "logo"; // Needs to be configured in Discord Dev Portal
        std::string largeText = "CloudDLC";
    };

    inline void SendPayload(const std::string& json) {
        if (hPipe == INVALID_HANDLE_VALUE) return;

        struct PacketHeader {
            uint32_t opcode;
            uint32_t length;
        } header;

        header.opcode = 1; // FRAME
        header.length = (uint32_t)json.length();

        DWORD written;
        if (WriteFile(hPipe, &header, sizeof(header), &written, nullptr) &&
            WriteFile(hPipe, json.c_str(), (DWORD)json.length(), &written, nullptr)) {
            // std::cout << "[D] RPC Payload sent (" << written << " bytes)\n";
        } else {
            std::cout << "[-] Failed to write to Discord pipe\n";
        }
    }

    inline void UpdateActivity(const Activity& act) {
        static uint64_t startTime = GetTickCount64() / 1000;
        std::string payload = "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" + std::to_string(GetCurrentProcessId()) + ",\"activity\":{";
        payload += "\"details\":\"" + act.details + "\",";
        payload += "\"state\":\"" + act.state + "\",";
        payload += "\"timestamps\":{\"start\":" + std::to_string(startTime) + "},";
        payload += "\"assets\":{\"large_image\":\"" + act.largeImage + "\",\"large_text\":\"" + act.largeText + "\"}";
        payload += "}},\"nonce\":\"" + std::to_string(GetTickCount64()) + "\"}";

        SendPayload(payload);
    }

    inline void Initialize(const std::string& applicationId) {
        for (int i = 0; i < 10; ++i) {
            std::string pipeName = "\\\\.\\pipe\\discord-ipc-" + std::to_string(i);
            hPipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hPipe != INVALID_HANDLE_VALUE) {
                std::cout << "[+] Connected to Discord Pipe " << i << "\n";
                break;
            }
        }

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cout << "[-] Could not find Discord Pipe\n";
            return;
        }

        // Handshake
        struct PacketHeader {
            uint32_t opcode;
            uint32_t length;
        } header;

        std::string handshake = "{\"v\":1,\"client_id\":\"" + applicationId + "\"}";
        header.opcode = 0; // HANDSHAKE
        header.length = (uint32_t)handshake.length();

        DWORD written;
        WriteFile(hPipe, &header, sizeof(header), &written, nullptr);
        WriteFile(hPipe, handshake.c_str(), (DWORD)handshake.length(), &written, nullptr);
        std::cout << "[+] Discord Handshake sent\n";

        // IMPORTANT: Read handshake response. Some Discord versions require this to activate RPC.
        char respHeader[8];
        DWORD read;
        if (ReadFile(hPipe, respHeader, 8, &read, nullptr)) {
            uint32_t len = *(uint32_t*)(respHeader + 4);
            std::vector<char> respBody(len);
            ReadFile(hPipe, respBody.data(), len, &read, nullptr);
            std::cout << "[+] Discord Handshake response received\n";
        }
    }

    inline void Shutdown() {
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }
    }
}
