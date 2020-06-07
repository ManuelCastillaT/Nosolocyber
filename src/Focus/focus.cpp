#include "focus.h"
#include <psapi.h>
#include <assert.h>


typedef struct 
{
  napi_async_work work;
  napi_threadsafe_function tsfn;
} AddonData;

AddonData* addon_data = (AddonData*)malloc(sizeof(*addon_data));

HANDLE _hookMessageLoop;
HANDLE _readMessageLoop;

std::queue<focus::FocusInfo> focusQueue;

void CALLBACK focus::WinEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (dwEvent == EVENT_SYSTEM_FOREGROUND)
     {
        char drct[100];
        
        if (hwnd != NULL)
        {
            DWORD dwProcId;
            GetWindowThreadProcessId(hwnd, &dwProcId);
            
            if (dwProcId != NULL)
            {
                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ , FALSE, dwProcId);

                if (hProc != NULL)
                {
                    if (GetModuleFileNameExA(hProc, NULL, drct, 100) != 0)
                    {
                        CloseHandle(hProc);

                        std::string directory = std::string(drct);

                        FocusInfo focus;
                        focus.directory = std::string(directory);

                        // we use time(NULL) instead of dwmsEventTime because we need the current time instead of what dwmsEventTime provides us
                        focus.date = time(NULL);

                        focusQueue.push(focus);
                    }
                }
            }
        }
    }
}

DWORD WINAPI focus::HooksMessageLoop( LPVOID lpParam)
{
	MSG msg;

    SetWinEventHook( EVENT_SYSTEM_FOREGROUND , EVENT_SYSTEM_FOREGROUND,NULL, focus::WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS );

	while( GetMessage( &msg, NULL, 0, 0 ) > 0 )
	{
		TranslateMessage( &msg );
		DispatchMessage ( &msg );
	}

	return 0;
}

void focus::CallJs(napi_env napiEnv, napi_value napi_js_cb, void* context, void* data) 
{
	FocusInfo focus = *((FocusInfo*)data);
	Napi::Env env = Napi::Env(napiEnv);

    char dt[100];
    struct tm tmNow;

    // Date format to ISO 8601
    _localtime64_s(&tmNow, &focus.date);
    strftime(dt , 100, "%Y-%m-%dT%H:%M:%S", &tmNow);
    std::string date = std::string(dt);

    Napi::Object jsFocusInfo = Napi::Object::New(env);
	jsFocusInfo.Set("Directory", Napi::String::New(env, focus.directory));
	jsFocusInfo.Set("Date", Napi::String::New(env, date));
	
	Napi::Function js_cb = Napi::Value(env, napi_js_cb).As<Napi::Function>();
	js_cb.Call(env.Global(), { jsFocusInfo } );
}

void focus::ExecuteWork(napi_env env, void* data) 
{
	AddonData* addon_data = (AddonData*)data;
    FocusInfo* focusInfo = NULL;

	assert(napi_acquire_threadsafe_function(addon_data->tsfn) == napi_ok);

  	while(true)
	{
        if (!focusQueue.empty())
        {
            focusInfo = &focusQueue.front();	
            FocusInfo out_focus = *focusInfo;
            assert(napi_call_threadsafe_function(addon_data->tsfn, &out_focus, napi_tsfn_blocking) == napi_ok);
            focusQueue.pop();
        }
	}
	
	assert(napi_release_threadsafe_function(addon_data->tsfn, napi_tsfn_release) == napi_ok);
}

void focus::WorkComplete(napi_env env, napi_status status, void* data) 
{
  AddonData* addon_data = (AddonData*)data;
}

Napi::Value focus::iniEventtHook(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsFunction()) {
        Napi::TypeError::New(env, "Function expected as argument[0]").ThrowAsJavaScriptException();
		return env.Undefined();;
    } 
	
	Napi::String work_name = Napi::String::New(env, "async-work");
	
	Napi::Function js_cb = info[0].As<Napi::Function>();
	
	addon_data->work = NULL;

	assert(napi_create_threadsafe_function(env,
                                         js_cb,
                                         NULL,
                                         work_name,
                                         0,
                                         1,
                                         NULL,
                                         NULL,
                                         NULL,
                                         focus::CallJs,
                                         &(addon_data->tsfn)) == napi_ok);
										 
    assert(napi_create_async_work(env,
                                NULL,
                                work_name,
                                focus::ExecuteWork,
                                focus::WorkComplete,
                                addon_data,
                                &(addon_data->work)) == napi_ok);

    // Queue the work item for execution.
    assert(napi_queue_async_work(env, addon_data->work) == napi_ok);										 

	if(_hookMessageLoop)
	{
		return env.Undefined();
	}

	_hookMessageLoop = CreateThread( NULL, 0, focus::HooksMessageLoop, NULL, 0, NULL);

	return env.Undefined();
}

Napi::Object focus::Init(Napi::Env env, Napi::Object exports)
{
    exports.Set("initEventHook", Napi::Function::New(env, focus::iniEventtHook));

    return exports;
}