#include <napi.h>
#include <Windows.h>
#include <queue>
#include <ctime>

namespace focus {
    struct FocusInfo
    {  
        std::time_t date;
        std::string directory;
    };

    Napi::Value iniEventtHook(const Napi::CallbackInfo& info);
    Napi::Object Init(Napi::Env env, Napi::Object exports);

    static void CALLBACK WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    static DWORD WINAPI HooksMessageLoop( LPVOID lpParam);
    static void CallJs(napi_env napiEnv, napi_value napi_js_cb, void* context, void* data);
    static void ExecuteWork(napi_env env, void* data);
    static void WorkComplete(napi_env env, napi_status status, void* data);
}