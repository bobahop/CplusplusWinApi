#ifndef UNICODE
#define UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <cmath>
#include <combaseapi.h>
#include <comdef.h>
#include <new>
#include <shobjidl.h>
#include <stdio.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

struct BobWindow
{
    int inWindow;
};

static HWND txt1;
static HWND txt2;
static HWND txtEpsilon;

enum class Ctrl : int
{
    Id1 = 1,
    Id2 = 2,
    Id3 = 3,
    Id4 = 4,
    Id5 = 5,
    Id6 = 6
};

int msg_box(HWND hWnd, LPCWSTR msg, LPCWSTR title, UINT btns = MB_OK)
{
    return MessageBoxW(hWnd, msg, title, btns | MB_ICONEXCLAMATION);
}

void show_error_msg(HWND hwnd, HRESULT hr, LPCWSTR title){
    _com_error err(hr);
    LPCTSTR errMsg {err.ErrorMessage()};
    msg_box(hwnd, (LPCTSTR) errMsg, title);
}

void cleanup_file_open(IFileOpenDialog *pFileOpen){
        pFileOpen->Release();
        CoUninitialize();
}

double get_txt_dbl(HWND hwnd){
    int len {GetWindowTextLengthW(hwnd) + 1};
    wchar_t *  buf = new (std::nothrow) wchar_t [len];
    GetWindowTextW(hwnd, buf, len);
    auto txtdouble {_wtof(buf)};
    delete[] buf;
    return txtdouble;
}

void show_file_open(HWND hWnd)
{
    HRESULT hr {CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
                                          COINIT_DISABLE_OLE1DDE)};

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
        msg_box(hWnd, pszFilePath, L"File Path");
        CoTaskMemFree(pszFilePath);
    }
    pItem->Release();

    return cleanup_file_open(pFileOpen);
}

void converged(HWND hwnd){
    auto double1 {get_txt_dbl(txt1)};
    auto double2 {get_txt_dbl(txt2)};
    double test_val = std::max(std::abs(double1),std::abs(double2));
    auto epsilon {get_txt_dbl(txtEpsilon) * test_val};
    auto is_converged = std::abs(double1 - double2) <= epsilon;
    wchar_t msg[20];
    swprintf_s(msg, L"Is converged: %d", is_converged);
    msg_box(hwnd, msg, L"Close enough?");
}

void btn1_click(HWND hWnd)
{
    msg_box(hWnd, L"You touched me!", L"Clicked button 1!");
}

void btn2_click(HWND hWnd)
{
    show_file_open(hWnd);
}

void btn5_click(HWND hWnd){
    converged(hWnd);
}

void create_button(LPCWSTR btn_text, Ctrl id, int x, int y, int w, int h, HWND parent)
{
    CreateWindowExW(
        0,
        L"BUTTON",
        btn_text,
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

HWND create_txtbox(LPCWSTR txt_name, Ctrl id, int x, int y, int w, int h, HWND parent){
    return CreateWindowExW(
        0,
        L"EDIT",
        txt_name,
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | WS_BORDER,
        x,
        y,
        w,
        h,
        parent,
        (HMENU)id,
        GetModuleHandleW(nullptr),
        nullptr);
}

void draw_label(LPCWSTR label_text, int x, int y, HDC hdc){
        TextOutW(
            hdc,
            x,
            y,
            label_text,
            wcslen(label_text));
}

bool is_mouse_in(HWND hWnd)
{
    auto bw {reinterpret_cast<BobWindow *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA))};
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

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        auto id {(Ctrl)LOWORD(wParam)};
        switch (id)
        {
        case Ctrl::Id1:
        {
            btn1_click(hwnd);
            break;
        }
        case Ctrl::Id2:
        {
            btn2_click(hwnd);
            break;
        }
        case Ctrl::Id5:
        {
            btn5_click(hwnd);
        }
        }
        //ignore 3 and 4, which are textboxes
        return 0;
    }
    case WM_CREATE:
    {
        CREATESTRUCT *pCreate {reinterpret_cast<CREATESTRUCT *>(lParam)};
        BobWindow *pData = static_cast<BobWindow *>(pCreate->lpCreateParams);
        LONG_PTR result {SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pData)};
        txt1 = create_txtbox((LPCWSTR)L"1.01", Ctrl::Id3, 115, 100, 100, 25, hwnd);
        txt2 = create_txtbox((LPCWSTR)L"1.02", Ctrl::Id4, 315, 100, 100, 25, hwnd);
        txtEpsilon = create_txtbox((LPCWSTR)L".001", Ctrl::Id6, 505, 100, 100, 25, hwnd);
        create_button((LPCWSTR)L"button1", Ctrl::Id1, 50, 200, 100, 25, hwnd);
        create_button((LPCWSTR)L"button2", Ctrl::Id2, 200, 200, 100, 25, hwnd);
        create_button((LPCWSTR)L"converge", Ctrl::Id5, 350, 200, 100, 25, hwnd);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MOUSELEAVE:
    {
        LONG_PTR ptr {GetWindowLongPtr(hwnd, GWLP_USERDATA)};
        BobWindow *pData = reinterpret_cast<BobWindow *>(ptr);
        pData->inWindow = 0;
        InvalidateRect(hwnd, &get_rect(), TRUE);
        UpdateWindow(hwnd);

        return 0;
    }
    case WM_MOUSEMOVE:
    {
        LONG_PTR ptr {GetWindowLongPtr(hwnd, GWLP_USERDATA)};
        BobWindow *pData = reinterpret_cast<BobWindow *>(ptr);
        if (pData->inWindow == 1)
        {
            return 0;
        }
        pData->inWindow = 1;
        TRACKMOUSEEVENT tme {sizeof(tme), TME_LEAVE, hwnd, 1}; //cbSize, dwFlags, hwndTrack, dwHoverTime
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
        wchar_t mouse_status[15];
        swprintf_s(mouse_status, L"mouse is in: %d", is_mouse_in(hwnd));
        draw_label(mouse_status, 5, 5, hdc);
        draw_label(L"double 1:", 50, 100, hdc);
        draw_label(L"double 2:", 250, 100, hdc);
        draw_label(L"epsilon:", 450, 100, hdc);
        EndPaint(hwnd, &ps);
    }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND create_window(HINSTANCE hInstance, LPCWSTR class_name, LPCWSTR title, BobWindow *dataptr){
// Register the window class.

    WNDCLASS wc {};

    wc.lpfnWndProc = window_proc;
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
    BobWindow mydata {0};
    BobWindow *dataptr = &mydata;

    auto hwnd = create_window(hInstance, L"Sample Window Class",L"Learn to Program Windows", dataptr);

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

