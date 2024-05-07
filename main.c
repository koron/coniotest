// Dump INPUT_RECORD from CONIN$
//
// USAGE:
//   > cl main.c
//   > .\main.exe
//
// Use the task manager to stop this program.

#include <stdio.h>
#include <windows.h>

HANDLE GetConsoleInputHandle() {
    SECURITY_ATTRIBUTES sa;
    static HANDLE	s_hInputConsole = INVALID_HANDLE_VALUE;

    if (s_hInputConsole != INVALID_HANDLE_VALUE)
        return s_hInputConsole;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    s_hInputConsole = CreateFile(TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            &sa, OPEN_EXISTING, 0, NULL);
    return s_hInputConsole;
}

BOOL DataAvailable(HANDLE h) {
    DWORD dwRet = WaitForSingleObjectEx(h, INFINITE, TRUE);
    if (dwRet == WAIT_OBJECT_0)
        return TRUE;
    if (dwRet == WAIT_FAILED)
        return FALSE;
    return FALSE;
}

int main(int argc, char **argv) {
    int ret = 0;
    int savedInCP = 0;

    HANDLE hIn = GetConsoleInputHandle();
    if (hIn == INVALID_HANDLE_VALUE) {
        ret = 1;
        printf("GetConsoleInputHandle failed\n");
        goto MAINEND;
    }
    printf("hIn=%08p\n", hIn);

    DWORD dwAttrs = 0;
    if (!GetConsoleMode((hIn), &dwAttrs)) {
        ret = 1;
        printf("GetConsoleMode failed\n");
        goto MAINEND;
    }
    printf("dwAttrs(#0)=%08x\n", dwAttrs);
    dwAttrs &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
    printf("dwAttrs(#1)=%08x\n", dwAttrs);
    dwAttrs |= ENABLE_WINDOW_INPUT;
    printf("dwAttrs(#2)=%08x\n", dwAttrs);
    dwAttrs |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    printf("dwAttrs(#3)=%08x\n", dwAttrs);
    if (!SetConsoleMode(hIn, dwAttrs)) {
        ret = 1;
        printf("SetConsoleMode failed\n");
        goto MAINEND;
    }

    savedInCP = GetConsoleCP();
    if (!SetConsoleCP(CP_UTF8)) {
        savedInCP = 0;
        ret = 1;
        printf("SetConsoleCP(CP_UTF8) failed\n");
        goto MAINEND;
    }

    INPUT_RECORD inputRecordArray[16];
    DWORD inputRecordArraySize = sizeof(inputRecordArray) / sizeof(inputRecordArray[0]);
    DWORD dwInput;

    while (DataAvailable(hIn)) {
        ReadConsoleInput(hIn, inputRecordArray, inputRecordArraySize, &dwInput);
        if (dwInput == 0) {
            continue;
        }
        printf("dwInput=%d\n", dwInput);
        for (DWORD i = 0; i < dwInput; i++) {
            INPUT_RECORD r = inputRecordArray[i];
            if (r.EventType != KEY_EVENT) {
                printf("  #%d: type=0x%04X\n", i, r.EventType);
                continue;
            }
            KEY_EVENT_RECORD kr = r.Event.KeyEvent;
            printf("  #%d: type=%04X (key) down=%-3s repeat=%-2d vk=%04X vs=%04X char=%04X control=%08X\n",
                    i,
                    r.EventType,
                    kr.bKeyDown ? "ON" : "OFF",
                    kr.wRepeatCount,
                    kr.wVirtualKeyCode,
                    kr.wVirtualScanCode,
                    kr.uChar.UnicodeChar,
                    kr.dwControlKeyState);
        }
    }

MAINEND:
    if (savedInCP != 0) {
        SetConsoleCP(savedInCP);
    }
    if (hIn != INVALID_HANDLE_VALUE) {
        CloseHandle(hIn);
    }
    return ret;
}
