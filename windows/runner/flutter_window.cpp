#include "flutter_window.h"

#include <optional>

// Include Windows Shell headers for thumbnail toolbar
#include <ShObjIdl.h>   // For ITaskbarList3
#include <Shellapi.h>   // For THUMBBUTTON, THBN_CLICKED, etc.

#include "flutter/generated_plugin_registrant.h"

// Unique IDs for your thumbnail buttons
static const int kThumbBtnIdPrevious = 100;
static const int kThumbBtnIdPlayPause = 101;
static const int kThumbBtnIdNext = 102;

// Forward-declare a helper function to set up the thumbnail toolbar
static void SetupThumbnailToolbar(HWND hwnd);

FlutterWindow::FlutterWindow(const flutter::DartProject& project)
    : project_(project) {}

FlutterWindow::~FlutterWindow() {}

bool FlutterWindow::OnCreate() {
  if (!Win32Window::OnCreate()) {
    return false;
  }

  RECT frame = GetClientArea();

  // The size here must match the window dimensions to avoid unnecessary surface
  // creation / destruction in the startup path.
  flutter_controller_ = std::make_unique<flutter::FlutterViewController>(
      frame.right - frame.left, frame.bottom - frame.top, project_);
  // Ensure that basic setup of the controller was successful.
  if (!flutter_controller_->engine() || !flutter_controller_->view()) {
    return false;
  }
  RegisterPlugins(flutter_controller_->engine());
  SetChildContent(flutter_controller_->view()->GetNativeWindow());

  // --- Setup the Windows Taskbar Thumbnail Toolbar ---
  SetupThumbnailToolbar(GetHandle());

  return true;
}

void FlutterWindow::OnDestroy() {
  if (flutter_controller_) {
    flutter_controller_ = nullptr;
  }
  Win32Window::OnDestroy();
}

LRESULT
FlutterWindow::MessageHandler(HWND hwnd, UINT const message,
                              WPARAM const wparam,
                              LPARAM const lparam) noexcept {
  // Give Flutter, including plugins, an opportunity to handle window messages.
  if (flutter_controller_) {
    std::optional<LRESULT> result =
        flutter_controller_->HandleTopLevelWindowProc(hwnd, message, wparam, lparam);
    if (result) {
      return *result;
    }
  }

  switch (message) {
    // Handle thumbnail button clicks
    case WM_COMMAND: {
      if (HIWORD(wparam) == THBN_CLICKED) {
        int button_id = LOWORD(wparam);
        switch (button_id) {
          case kThumbBtnIdPrevious:
            // TODO: Integrate "previous track" logic (or send to Dart via MethodChannel)
            break;
          case kThumbBtnIdPlayPause:
            // TODO: Integrate "play/pause" logic
            break;
          case kThumbBtnIdNext:
            // TODO: Integrate "next track" logic
            break;
        }
      }
      break;
    }

    case WM_FONTCHANGE:
      flutter_controller_->engine()->ReloadSystemFonts();
      break;
  }

  return Win32Window::MessageHandler(hwnd, message, wparam, lparam);
}

// --------------------------------------------------------------------------
// Helper function to set up the thumbnail toolbar
// --------------------------------------------------------------------------
static void SetupThumbnailToolbar(HWND hwnd) {
  // 1) Create the ITaskbarList3 instance
  ITaskbarList3* taskbar_list = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&taskbar_list));
  if (FAILED(hr) || !taskbar_list) {
    return;  // Could not create TaskbarList
  }
  taskbar_list->HrInit();

  // 2) Define the thumbnail buttons
  THUMBBUTTON buttons[3];
  ZeroMemory(buttons, sizeof(buttons));

  // --- PREVIOUS ---
  buttons[0].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
  buttons[0].iId = kThumbBtnIdPrevious;
  buttons[0].hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PREV));
  wcscpy_s(buttons[0].szTip, L"Previous");
  buttons[0].dwFlags = THBF_ENABLED;

  // --- PLAY/PAUSE ---
  buttons[1].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
  buttons[1].iId = kThumbBtnIdPlayPause;
  buttons[1].hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_PLAY));
  wcscpy_s(buttons[1].szTip, L"Play/Pause");
  buttons[1].dwFlags = THBF_ENABLED;

  // --- NEXT ---
  buttons[2].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
  buttons[2].iId = kThumbBtnIdNext;
  buttons[2].hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON_NEXT));
  wcscpy_s(buttons[2].szTip, L"Next");
  buttons[2].dwFlags = THBF_ENABLED;

  // 3) Add them to the taskbar thumbnail
  taskbar_list->ThumbBarAddButtons(hwnd, ARRAYSIZE(buttons), buttons);

  // Release the COM interface
  taskbar_list->Release();
}
