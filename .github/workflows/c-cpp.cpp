#include <windows.h>
#include <fstream>
#include <ctime>
#include <string.h>
#define WINDOW_WIDTH  360
#define WINDOW_HEIGHT 240
#define ID_USERNAME   101
#define ID_PASSWORD   102
#define ID_LOGINBTN   103
#define ID_CLEARBTN   104
#define ID_EXITBTN    105
#define ID_STATUS     106
#define MAX_INPUT_LEN 32
#define MAX_ATTEMPTS  3

DWORD simpleSHA256(const WCHAR* str, size_t len) {
    DWORD hash = 0x6a09e667;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (hash << 5) + (hash >> 2) + str[i];
        hash = (hash << 7) | (hash >> 25);
    }
    return hash;
}

struct Account {
    const WCHAR* username;
    const DWORD passwordHash;
};

Account accounts[] = {
    { L"Tran Anh Dung", simpleSHA256(L"123456", 6) },
    { L"Hoang Vu Nghi", simpleSHA256(L"abcdef", 6) },
    { L"Dung dz", simpleSHA256(L"guestpass", 9) },
};

const int NUM_ACCOUNTS = sizeof(accounts) / sizeof(Account);  

BOOL isPasswordStrong(const WCHAR* password, size_t len) {
    return len >= 6;
}

void logFailedAttempt(const WCHAR* username) {
    std::wofstream logFile("login_attempts.log", std::ios::app);
    if (logFile.is_open()) {
        time_t now = time(nullptr);
        wchar_t timeStr[64];
        wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", localtime(&now));
        logFile << L"Failed login for user '" << username << L"' at " << timeStr << L"\n";
        logFile.close();
    }
}

BOOL checkLogin(const WCHAR* username, const WCHAR* password, size_t userLen, size_t passLen) {
    DWORD passHash = simpleSHA256(password, passLen);
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        if (wcsncmp(username, accounts[i].username, wcslen(accounts[i].username)) == 0 &&
            passHash == accounts[i].passwordHash) {
            return TRUE;
        }
    }
    return FALSE;
}

void createControls(HWND hwnd, HINSTANCE hInstance, HWND& hUsername, HWND& hPassword, HWND& hStatus) {
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    HWND hLabelUser = CreateWindowW(L"STATIC", L"Tên đăng nhập:", WS_VISIBLE | WS_CHILD,
        30, 30, 120, 20, hwnd, NULL, hInstance, NULL);
    SendMessageW(hLabelUser, WM_SETFONT, (WPARAM)hFont, TRUE);
    hUsername = CreateWindowW(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        160, 30, 160, 24, hwnd, (HMENU)ID_USERNAME, hInstance, NULL);
    SendMessageW(hUsername, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLabelPass = CreateWindowW(L"STATIC", L"Mật khẩu:", WS_VISIBLE | WS_CHILD,
        30, 70, 120, 20, hwnd, NULL, hInstance, NULL);
    SendMessageW(hLabelPass, WM_SETFONT, (WPARAM)hFont, TRUE);
    hPassword = CreateWindowW(L"EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
        160, 70, 160, 24, hwnd, (HMENU)ID_PASSWORD, hInstance, NULL);
    SendMessageW(hPassword, WM_SETFONT, (WPARAM)hFont, TRUE);

    hStatus = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD,
        30, 110, 290, 20, hwnd, (HMENU)ID_STATUS, hInstance, NULL);
    SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hLoginBtn = CreateWindowW(L"BUTTON", L"Đăng nhập", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        30, 150, 90, 30, hwnd, (HMENU)ID_LOGINBTN, hInstance, NULL);
    SendMessageW(hLoginBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    HWND hClearBtn = CreateWindowW(L"BUTTON", L"Xóa", WS_VISIBLE | WS_CHILD,
        130, 150, 90, 30, hwnd, (HMENU)ID_CLEARBTN, hInstance, NULL);
    SendMessageW(hClearBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    HWND hExitBtn = CreateWindowW(L"BUTTON", L"Thoát", WS_VISIBLE | WS_CHILD,
        230, 150, 90, 30, hwnd, (HMENU)ID_EXITBTN, hInstance, NULL);
    SendMessageW(hExitBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void centerWindow(HWND hwnd) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int xPos = (GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2;
    int yPos = (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2;
    SetWindowPos(hwnd, NULL, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hUsername, hPassword, hStatus;
    static int attempts = MAX_ATTEMPTS;

    switch (uMsg) {
    case WM_CREATE:
        createControls(hwnd, ((LPCREATESTRUCT)lParam)->hInstance, hUsername, hPassword, hStatus);
        centerWindow(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_LOGINBTN: {
            WCHAR username[MAX_INPUT_LEN] = { 0 };
            WCHAR password[MAX_INPUT_LEN] = { 0 };
            GetWindowTextW(hUsername, username, MAX_INPUT_LEN);
            GetWindowTextW(hPassword, password, MAX_INPUT_LEN);

            size_t userLen = wcslen(username);
            size_t passLen = wcslen(password);

            if (userLen == 0 || passLen == 0) {
                SetWindowTextW(hStatus, L"Vui lòng nhập đầy đủ thông tin!");
                return 0;
            }
            if (!isPasswordStrong(password, passLen)) {
                SetWindowTextW(hStatus, L"Mật khẩu phải có ít nhất 6 ký tự!");
                return 0;
            }

            if (checkLogin(username, password, userLen, passLen)) {
                MessageBoxW(hwnd, L"Chào mừng bạn!", L"Thành công", MB_OK | MB_ICONINFORMATION);
                SetWindowTextW(hStatus, L"Đăng nhập thành công!");
                attempts = MAX_ATTEMPTS;
            } else {
                attempts--;
                logFailedAttempt(username);
                WCHAR msg[64];
                if (attempts > 0)
                    wsprintfW(msg, L"Đăng nhập thất bại. Còn %d lần thử.", attempts);
                else
                    wsprintfW(msg, L"Hết lượt thử. Tài khoản bị khóa!");
                SetWindowTextW(hStatus, msg);

                if (attempts == 0) {
                    MessageBoxW(hwnd, L"Tài khoản đã bị khóa sau 3 lần sai!", L"Lỗi", MB_OK | MB_ICONERROR);
                    DestroyWindow(hwnd);
                }
            }

            SecureZeroMemory(password, sizeof(password));
            SetWindowTextW(hPassword, L"");
            return 0;
        }

        case ID_CLEARBTN:
            SetWindowTextW(hUsername, L"");
            SetWindowTextW(hPassword, L"");
            SetWindowTextW(hStatus, L"");
            attempts = MAX_ATTEMPTS;
            return 0;

        case ID_EXITBTN:
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            PostMessageW(hwnd, WM_COMMAND, ID_LOGINBTN, 0);
            return 0;
        }
        break;

    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LoginWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"LoginWindowClass", L"Hệ thống Đăng nhập",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Không thể tạo cửa sổ!", L"Lỗi", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
