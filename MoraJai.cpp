#define WIN32_LEAN_AND_MEAN 1
#include "Windows.h"

#include <iostream>
#include <string>

#include "Grid.h"
#include "Solver.h"

COLORREF RGB_COLORS[] = {
    RGB(0x33, 0x66, 0x99), // Invalid == whatever color this is
    RGB(0xFF, 0x00, 0x00), // Red
    RGB(0xFF, 0x9F, 0x00), // Orange (slightly more green due to colorblindness)
    RGB(0xFF, 0xFF, 0x00), // Yellow
    RGB(0x00, 0xFF, 0x00), // Green
    RGB(0x7F, 0x00, 0xFF), // Purple
    RGB(0xFF, 0x00, 0xFF), // Pink
    RGB(0xFF, 0xFF, 0xFF), // White
    RGB(0x7F, 0x7F, 0x7F), // Gray
    RGB(0x00, 0x00, 0x00), // Black
};
static_assert(sizeof(RGB_COLORS) / sizeof(RGB_COLORS[0]) == NUM_COLORS, "MoraJai.cpp is missing RGB definitions for one or more colors");


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
    case WM_TIMER:
        PostQuitMessage(0);
        return 0;
    case WM_DRAWITEM: // Each button gets a callback here because it's BS_OWNERDRAW
        if (HIWORD(wParam) == 0 && wParam >= WM_USER) {
            int x = ((int)wParam - WM_USER) / 3;
            int y = ((int)wParam - WM_USER - x * 3);
            Grid* grid = (Grid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            Color color = grid->Get(x, y);

            LPDRAWITEMSTRUCT item = (LPDRAWITEMSTRUCT)lParam;
            SetDCBrushColor(item->hDC, RGB_COLORS[color]);
            SelectObject(item->hDC, GetStockObject(DC_BRUSH));
            Rectangle(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && wParam >= WM_USER) {
            int x = ((int)wParam - WM_USER) / 3;
            int y = ((int)wParam - WM_USER - x * 3);
            Grid* grid = (Grid*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            grid->Click(x, y);
            std::cout << "(" << x << ", " << y << ") ";
            RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE);
            if (grid->Victory()) SetTimer(hwnd, NULL, 1000, NULL);
            return 0;
        }
        break;
    case WM_ERASEBKGND: // Fixes some weird cases where dragging/alt-tabbing would bleed background images.
    {
        RECT rc;
        ::GetClientRect(hwnd, &rc);
        HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect((HDC)wParam, &rc, brush);
        DeleteObject(brush);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
        // Get rid of the gross gray background. https://stackoverflow.com/a/4495814
        SetTextColor((HDC)wParam, RGB(0, 0, 0));
        SetBkColor((HDC)wParam, RGB(255, 255, 255));
        SetBkMode((HDC)wParam, OPAQUE);
        static HBRUSH s_solidBrush = CreateSolidBrush(RGB(255, 255, 255));
        return (LRESULT)s_solidBrush;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

static void SolveInteractively(Grid& grid) {
    constexpr LPCWSTR WINDOW_CLASS = L"MoraJai";
    WNDCLASS wndClass = {
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,
        0,
        nullptr,
        NULL,
        NULL,
        NULL,
        WINDOW_CLASS,
        WINDOW_CLASS,
    };
    RegisterClass(&wndClass);

    constexpr int width = 190;
    constexpr int height = 190;
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    HWND mainWindow = CreateWindow(
        WINDOW_CLASS, L"Mora Jai",
        WS_VISIBLE,
        /*x*/ (rect.left + rect.right) / 2 - width,
        /*y*/ (rect.top + rect.bottom) / 2 - height,
        /*width*/ width,
        /*height*/ height,
        nullptr, nullptr, nullptr, nullptr);
    SetWindowLongPtr(mainWindow, GWLP_USERDATA, (LONG_PTR)&grid); // Save the grid object on the global window's HWND

    SetWindowLong(mainWindow, GWL_STYLE, 0); // Remove the title bar
    ShowWindow(mainWindow, SW_SHOW); // Other required win32 stuff to make the window visible
    UpdateWindow(mainWindow);

    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            INT_PTR message = WM_USER + (x * 3 + y); // Each button is given its own message.

            HWND button = CreateWindow(L"BUTTON", L"",
                WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
                x * 60 + 10, y * 60 + 10, 50, 50,
                mainWindow, (HMENU)message, nullptr, NULL);
            SetWindowLongPtr(button, GWLP_USERDATA, (LONG_PTR)(x * 3 + y)); // Save the button index on the button's HWND
        }
    }

    // Pump messages until the user closes the window.
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int main() {
    Grid closedExhibit({
        Orange, Black, Orange,
        Orange, Red,   Orange,
        Orange, Black, Orange,
    }, Red);
    Grid tunnel({
        Black,  Orange, Pink,
        Orange, Orange, Orange,
        Pink,   Orange, Orange,
    }, Orange);
    Grid masterBedroom({
        White,  Gray,   White,
        White,  Gray,   Gray,
        Gray,   Gray,   White,
    }, White);
    Grid tradingPost({
        Pink,   Gray,   Gray,
        Gray,   Yellow, Yellow,
        Gray,   Yellow, Yellow,
    }, Yellow);
    Grid theTomb({
        Gray,   Purple, Gray,
        Gray,   Pink,   Gray,
        Purple, Purple, Purple,
    }, Purple);
    Grid sanctumRoom1({
        Green, Black,   Green,
        Black, Black,   Black,
        Green, Yellow,  Green,
    }, Black);
    
    Grid grid = masterBedroom;

    auto bestSolution = Solver(grid).Solve();
    std::cout << "Best solution (" << bestSolution.size() << " moves):" << std::endl;
    for (const auto& [x, y] : bestSolution) {
        std::cout << "(" << x << ", " << y << ") ";
    }
    std::cout << std::endl;

    SolveInteractively(grid);
}
