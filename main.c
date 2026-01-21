#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

char mqtt_host[128];
int  mqtt_port;
char mqtt_user[64];
char mqtt_pass[64];
char mqtt_topic[128];
int  interval_ms;

void read_config() {
    GetPrivateProfileStringA("mqtt", "host", "", mqtt_host, 128, ".\\focus.ini");
    mqtt_port = GetPrivateProfileIntA("mqtt", "port", 1883, ".\\focus.ini");
    GetPrivateProfileStringA("mqtt", "username", "", mqtt_user, 64, ".\\focus.ini");
    GetPrivateProfileStringA("mqtt", "password", "", mqtt_pass, 64, ".\\focus.ini");
    GetPrivateProfileStringA("mqtt", "topic", "pc/activity/focus", mqtt_topic, 128, ".\\focus.ini");
    interval_ms = GetPrivateProfileIntA("general", "interval_ms", 1000, ".\\focus.ini");
}

int main() {
    read_config();

    char last_app[MAX_PATH] = "";

    while (1) {
        HWND hwnd = GetForegroundWindow();
        if (!hwnd) {
            Sleep(interval_ms);
            continue;
        }

        DWORD pid = 0;
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

            char cmd[512];
            sprintf(cmd,
                "mosquitto_pub -h %s -p %d -t \"%s\" -m \"%s\"",
                mqtt_host, mqtt_port, mqtt_topic, exe
            );

            system(cmd);
        }

        Sleep(interval_ms);
    }
}
