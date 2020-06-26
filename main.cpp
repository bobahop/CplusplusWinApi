#ifndef UNICODE
#define UNICODE
#endif

#include <combaseapi.h>
#include <comdef.h>
#include <shobjidl.h>
#include <stdio.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

struct BobWindow
{
    int inWindow;
};

enum class BtnId : int
{
    Btn1 = 1,
    Btn2 = 2,
};

int msg_box(HWND hWnd, LPCWSTR msg, LPCWSTR title, UINT btns)
{
    return MessageBoxW(hWnd, msg, title, btns | MB_ICONEXCLAMATION);
}

void show_error_msg(HWND hwnd, HRESULT hr, LPCWSTR title){
    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();
    msg_box(hwnd, (LPCTSTR) errMsg, title, MB_OK);
}

void cleanup_file_open(IFileOpenDialog *pFileOpen){
        pFileOpen->Release();
        CoUninitialize();
}

void show_file_open(HWND hWnd)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
                                          COINIT_DISABLE_OLE1DDE);

    if (FAILED(hr)) {
        show_error_msg(hWnd, hr, L"CoInitialize Failed");
        return;
    }
    
    IFileOpenDialog *pFileOpen;

    // Create the FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                            IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

    if (FAILED(hr)){
        show_error_msg(hWnd, hr, L"CoCreateInstance Failed");
        CoUninitialize();
        return;
    }

    // Show the Open dialog box.
    hr = pFileOpen->Show(NULL);

    if (FAILED(hr)){
        //user could have cancelled
        return cleanup_file_open(pFileOpen);
    }

    // Get the file name from the dialog box.
    IShellItem *pItem;
    hr = pFileOpen->GetResult(&pItem);
    if (FAILED(hr)){
        show_error_msg(hWnd, hr, L"FileOpen GetResult Failed");
        return cleanup_file_open(pFileOpen);
    }    

    PWSTR pszFilePath;
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

    // Display the file name to the user.
    if (SUCCEEDED(hr))
    {
        msg_box(hWnd, pszFilePath, L"File Path", MB_OK);
        CoTaskMemFree(pszFilePath);
    }
    pItem->Release();

    return cleanup_file_open(pFileOpen);
}

void btn1_click(HWND hWnd)
{
    msg_box(hWnd, L"You touched me!", L"Clicked button 1!", MB_OK);
}

void btn2_click(HWND hWnd)
{
    show_file_open(hWnd);
}

void create_button(LPCWSTR btn_name, BtnId id, int x, int y, int w, int h, HWND parent)
{
    CreateWindowExW(
        0,
        L"BUTTON",
        btn_name,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        x,
        y,
        w,
        h,
        parent,
        (HMENU)id,
        GetModuleHandleW(nullptr),
        nullptr);
}

bool is_mouse_in(HWND hWnd)
{
    auto bw = (BobWindow *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    switch (bw->inWindow)
    {
    case 0:
        return false;
    case 1:
        return true;
    default:
        return false;
    }
}

RECT get_rect()
{
    return RECT{
        0,   //left
        0,   //top
        125, //right
        25,  //bottom
    };
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
    {
        if (msg_box(hwnd, L"Really quit?", L"Are you serious?", MB_OKCANCEL) == IDOK)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_COMMAND:
    {
        auto id = (BtnId)LOWORD(wParam);
        switch (id)
        {
        case BtnId::Btn1:
        {
            btn1_click(hwnd);
            break;
        }
        case BtnId::Btn2:
        {
            btn2_click(hwnd);
            break;
        }
        }
        return 0;
    }
    case WM_CREATE:
    {
        CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
        BobWindow *pData = reinterpret_cast<BobWindow *>(pCreate->lpCreateParams);
        int inWindow = pData->inWindow;
        LONG_PTR result = SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
        create_button((LPCWSTR)L"button", BtnId::Btn1, 50, 100, 100, 25, hwnd);
        create_button((LPCWSTR)L"button2", BtnId::Btn2, 250, 100, 100, 25, hwnd);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MOUSELEAVE:
    {
        LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        BobWindow *pData = reinterpret_cast<BobWindow *>(ptr);
        pData->inWindow = 0;
        InvalidateRect(hwnd, &get_rect(), TRUE);
        UpdateWindow(hwnd);

        return 0;
    }
    case WM_MOUSEMOVE:
    {
        LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        BobWindow *pData = reinterpret_cast<BobWindow *>(ptr);
        int inWindow = pData->inWindow;
        if (inWindow == 1)
        {
            return 0;
        }
        pData->inWindow = 1;
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.hwndTrack = hwnd;
        tme.dwFlags = TME_LEAVE;
        tme.dwHoverTime = 1;
        TrackMouseEvent(&tme);
        SetCursor(LoadCursorW(NULL, IDC_ARROW));
        InvalidateRect(hwnd, &get_rect(), TRUE);
        UpdateWindow(hwnd);
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        char mouse_status[15];
        sprintf_s(mouse_status, "mouse is in: %d", is_mouse_in(hwnd));
        TextOutA(
            hdc,
            5,
            5,
            mouse_status,
            sizeof(mouse_status));
        EndPaint(hwnd, &ps);
    }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND create_window(HINSTANCE hInstance, LPCWSTR class_name, LPCWSTR title, BobWindow *dataptr){
// Register the window class.

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;

    RegisterClass(&wc);

    // Create the window.

    return CreateWindowEx(
        0,                           // Optional window styles.
        class_name,                  // Window class
        title, // Window text
        WS_OVERLAPPEDWINDOW,         // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,      // Parent window
        NULL,      // Menu
        hInstance, // Instance handle
        dataptr    // Additional application data
    );

}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    BobWindow mydata = {0};
    BobWindow *dataptr = &mydata;

    auto hwnd = create_window(hInstance, L"Sample Window Class",L"Learn to Program Windows", dataptr);

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

