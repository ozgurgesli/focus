#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <psapi.h>

#include "mqtt.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")

#define BUF_SIZE 2048

char mqtt_host[128];
int  mqtt_port;
char mqtt_topic[128];
int  interval_ms;

SOCKET sock;
struct mqtt_client client;
uint8_t sendbuf[BUF_SIZE];
uint8_t recvbuf[BUF_SIZE];

void read_config() {
    GetPrivateProfileStringA("mqtt", "host", "", mqtt_host, 128, ".\\focus.ini");
    mqtt_port = GetPrivateProfileIntA("mqtt", "port", 1883, ".\\focus.ini");
    GetPrivateProfileStringA("mqtt", "topic", "pc/activity/focus", mqtt_topic, 128, ".\\focus.ini");
    interval_ms = GetPrivateProfileIntA("general", "interval_ms", 1000, ".\\focus.ini");
}

void mqtt_connect() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mqtt_port);
    addr.sin_addr.s_addr = inet_addr(mqtt_host);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    mqtt_init(&client, sock, sendbuf, BUF_SIZE, recvbuf, BUF_SIZE, NULL);

    mqtt_connect(&client, "focus-client", NULL, NULL, 0, NULL, NULL, 0, 400);
}

void mqtt_publish_state(const char *msg) {
    mqtt_publish(&client, mqtt_topic, msg, strlen(msg), MQTT_PUBLISH_QOS_0);
}

int main() {
    read_config();
    mqtt_connect();

    char last_app[MAX_PATH] = "";

    while (1) {
        mqtt_sync(&client);

        HWND hwnd = GetForegroundWindow();
        if (!hwnd) {
            Sleep(interval_ms);
            continue;
        }

        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!h) {
            Sleep(interval_ms);
            continue;
        }

        char path[MAX_PATH] = "";
        GetModuleFileNameExA(h, NULL, path, MAX_PATH);
        CloseHandle(h);

        char *exe = strrchr(path, '\\');
        exe = exe ? exe + 1 : path;

        if (strcmp(exe, last_app) != 0) {
            strcpy(last_app, exe);
            mqtt_publish_state(exe);
        }

        Sleep(interval_ms);
    }
}
