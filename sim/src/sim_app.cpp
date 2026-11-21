// 交互式仿真窗口：自己开一个 Win32 窗口，把 LVGL 离屏帧缓冲直接贴上去。
//
// 不用 LVGL 自带的 lv_windows_create_display：那条路在 LVGL 9.2 上要求后端
// 自己分配的 draw buffer，宿主程序拿不到时序，容易出现 "No draw buffer" 白屏。
// 这里复用测试用的离屏 display，渲染完就用 StretchDIBits 贴到窗口，行为确定。

#include <windows.h>

#include <cstdio>
#include <cstring>

#include "sim_harness.h"

namespace {

sim::Fixture* g_fixture = nullptr;
HWND g_window = nullptr;
bool g_running = true;

// 前一次按键状态，用来做“按下沿”判定，避免一次按键触发很多次。
bool g_keyDown[256] = {false};

bool pressedEdge(int vk) {
    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool edge = down && !g_keyDown[vk & 0xFF];
    g_keyDown[vk & 0xFF] = down;
    return edge;
}

void publish(hb::EventType type, int32_t value = 0, hb::AlertKind alert = hb::AlertKind::None) {
    hb::Event event{};
    event.type = type;
    event.value = value;
    event.alert = alert;
    g_fixture->bus().publish(event);
}

void updateTitle() {
    if (!g_window) return;
    const int pct = g_fixture->backlight().lastPct;
    wchar_t title[160];
    swprintf(title, 160, L"角色 E · 界面仿真（800×480） 背光 %d%%  当前页 %d",
             pct, static_cast<int>(g_fixture->nav().current()));
    SetWindowTextW(g_window, title);
}

void blit(HDC dc) {
    const uint8_t* pixels = g_fixture->pixels();
    if (!pixels) return;

    BITMAPINFO info;
    std::memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = sim::kWidth;
    info.bmiHeader.biHeight = -sim::kHeight;  // 负数 = 从上往下写
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, 0, 0, sim::kWidth, sim::kHeight, 0, 0, sim::kWidth, sim::kHeight,
                  pixels, &info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            blit(dc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;  // 背景由 blit 全量覆盖，避免闪烁
        case WM_CLOSE:
            g_running = false;
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace

int main() {
    lv_init();

    sim::Fixture fixture;
    if (!fixture.ok()) {
        std::printf("仿真初始化失败：UiBridge::begin 返回 false\n");
        return 1;
    }
    g_fixture = &fixture;

    WNDCLASSEXW wc;
    std::memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // IDC_ARROW
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"HBUISimWindow";
    if (!RegisterClassExW(&wc)) {
        std::printf("RegisterClassExW 失败\n");
        return 1;
    }

    RECT rect{0, 0, sim::kWidth, sim::kHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    g_window = CreateWindowExW(0, L"HBUISimWindow", L"角色 E · 界面仿真（800×480）",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                               rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
                               GetModuleHandleW(nullptr), nullptr);
    if (!g_window) {
        std::printf("CreateWindowExW 失败\n");
        return 1;
    }
    ShowWindow(g_window, SW_SHOW);
    UpdateWindow(g_window);

    std::printf("窗口已打开。键位：1-6 切页 | L 长按 | V 超长按 | A 久坐提醒 | F 跌倒 | S 步数 | Esc 退出\n");
    updateTitle();

    int stepCount = 0;
    while (g_running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running) break;

        for (int vk = '1'; vk <= '6'; ++vk) {
            if (pressedEdge(vk)) publish(hb::EventType::KeyShort, vk - '0');
        }
        if (pressedEdge('L')) publish(hb::EventType::KeyLong);
        if (pressedEdge('V')) publish(hb::EventType::KeyVeryLong);
        if (pressedEdge('A')) publish(hb::EventType::Alert, 45, hb::AlertKind::Sedentary);
        if (pressedEdge('F')) publish(hb::EventType::Alert, 0, hb::AlertKind::Fall);
        if (pressedEdge('S')) {
            // 步数 +100：直接改模型再 tick 一次，方便看表盘数字跳变
            hb::ui::Model model = fixture.ui().snapshot();
            model.steps = static_cast<uint16_t>(model.steps + 100);
            fixture.ui().setModel(model);
        }
        if (pressedEdge(VK_ESCAPE)) {
            g_running = false;
            DestroyWindow(g_window);
            break;
        }

        fixture.step();
        lv_timer_handler();
        InvalidateRect(g_window, nullptr, FALSE);

        if ((++stepCount % 60) == 0) updateTitle();
        Sleep(16);
    }

    return 0;
}
