std::atomic<unsigned> qa_diagnostic_closes{};
decltype(&CloseHandle) qa_real_close{};
BOOL WINAPI qa_logging_close(HANDLE file) {
    if(diagnostic_file_io) ++qa_diagnostic_closes;
    return qa_real_close(file);
}
HANDLE qa_hid_file=reinterpret_cast<HANDLE>(700);
std::array<unsigned char,64> qa_hid_physical{};
void* qa_hid_buffer{};OVERLAPPED* qa_hid_request{};
LPOVERLAPPED_COMPLETION_ROUTINE qa_hid_callback{};
bool qa_hid_pending{},qa_hid_incomplete{},qa_hid_abort{},qa_hid_shared{},qa_hid_batch{};
unsigned qa_hid_callbacks{},qa_hid_sequence{};DWORD qa_hid_callback_error{},qa_hid_callback_hat{};
BOOLEAN __stdcall fake_native_attributes(HANDLE file,HIDD_ATTRIBUTES* attributes) {
    SetLastError(999);if(file!=qa_hid_file) return FALSE;
    attributes->VendorID=0x054c;attributes->ProductID=0x0df2;return TRUE;
}
BOOLEAN __stdcall fake_native_preparsed(HANDLE,PHIDP_PREPARSED_DATA* data) {*data=reinterpret_cast<PHIDP_PREPARSED_DATA>(1);return TRUE;}
NTSTATUS __stdcall fake_native_caps(PHIDP_PREPARSED_DATA,HIDP_CAPS* caps) {caps->UsagePage=1;caps->Usage=5;caps->InputReportByteLength=64;return HIDP_STATUS_SUCCESS;}
BOOLEAN __stdcall fake_native_free(PHIDP_PREPARSED_DATA) {return TRUE;}
BOOL WINAPI fake_native_read(HANDLE file,void* buffer,DWORD,DWORD* bytes,OVERLAPPED* request) {
    qa_hid_buffer=buffer;qa_hid_request=request;
    if(qa_hid_pending) {SetLastError(ERROR_IO_PENDING);return FALSE;}
    std::memcpy(buffer,qa_hid_physical.data(),64);const DWORD size=qa_hid_batch?128:64;
    if(qa_hid_batch) {auto* second=static_cast<unsigned char*>(buffer)+64;std::memcpy(second,qa_hid_physical.data(),64);++second[7];}
    if(bytes) *bytes=size;if(request) request->InternalHigh=size;
    if(qa_hid_shared) {StockHidRead read{file,buffer,8192,dingosdk::PlayStationPad::dualsense};filtered_stock_hid(read,size);}
    SetLastError(777);return TRUE;
}
BOOL WINAPI fake_native_read_ex(HANDLE,void* buffer,DWORD,OVERLAPPED* request,LPOVERLAPPED_COMPLETION_ROUTINE callback) {
    if(qa_hid_abort) {SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    qa_hid_buffer=buffer;qa_hid_request=request;qa_hid_callback=callback;SetLastError(777);return TRUE;
}
BOOL WINAPI fake_native_overlapped(HANDLE,OVERLAPPED*,DWORD* bytes,BOOL) {
    if(qa_hid_incomplete) {SetLastError(ERROR_IO_INCOMPLETE);return FALSE;}
    if(qa_hid_abort) {SetLastError(ERROR_OPERATION_ABORTED);return FALSE;}
    std::memcpy(qa_hid_buffer,qa_hid_physical.data(),64);*bytes=64;SetLastError(777);return TRUE;
}
BOOL WINAPI fake_native_overlapped_ex(HANDLE file,OVERLAPPED* request,DWORD* bytes,DWORD,BOOL) {
    return fake_native_overlapped(file,request,bytes,FALSE);
}
BOOL WINAPI fake_native_completion(HANDLE,DWORD* bytes,ULONG_PTR* key,OVERLAPPED** request,DWORD) {
    *request=qa_hid_request;*key=123;return fake_native_overlapped(qa_hid_file,qa_hid_request,bytes,FALSE);
}
BOOL WINAPI fake_native_completion_ex(HANDLE,OVERLAPPED_ENTRY* entries,ULONG,ULONG* count,DWORD,BOOL) {
    *count=1;entries[0].lpOverlapped=qa_hid_request;entries[0].Internal=qa_hid_abort?0xc0000120:0;
    std::memcpy(qa_hid_buffer,qa_hid_physical.data(),64);
    entries[0].dwNumberOfBytesTransferred=64;SetLastError(777);return TRUE;
}
BOOL WINAPI fake_native_close(HANDLE) {SetLastError(777);return TRUE;}
BOOLEAN __stdcall fake_native_report(HANDLE,void* buffer,ULONG) {std::memcpy(buffer,qa_hid_physical.data(),64);SetLastError(777);return TRUE;}
void CALLBACK fake_native_callback(DWORD error,DWORD,OVERLAPPED*) {
    ++qa_hid_callbacks;qa_hid_callback_error=error;qa_hid_callback_hat=static_cast<unsigned char*>(qa_hid_buffer)[8]&15;
}
void test_native_hid_adapters() {
    native_attributes=fake_native_attributes;native_original_read=fake_native_read;
    native_preparsed=fake_native_preparsed;native_caps=fake_native_caps;native_free_preparsed=fake_native_free;
    native_original_read_ex=fake_native_read_ex;native_original_overlapped=fake_native_overlapped;
    native_original_overlapped_ex=fake_native_overlapped_ex;native_original_completion=fake_native_completion;
    native_original_completion_ex=fake_native_completion_ex;native_original_close=fake_native_close;
    native_original_report=fake_native_report;
    dingosdk::FirstPersonLookControls bindings;for(unsigned i=0;i<4;++i)bindings[i].combo=0x100|(1u<<i);
    DWORD toggle{};
    auto publish=[&] {publish_glance_filter(bindings,qa_active,{1,2,3,4},0x100,toggle,toggle!=0,1200);};
    auto reset=[&] {clear_glance_filter();native_hid_devices.clear();native_hid_requests.clear();
        qa_hid_pending=qa_hid_incomplete=qa_hid_abort=qa_hid_shared=qa_hid_batch=false;
        qa_focus=qa_active=true;qa_menu=qa_spectator=false;toggle=0;qa_hid_callbacks=0;qa_now+=1000;publish();};
    auto physical=[&](unsigned hat,unsigned shoulders=1) {
        qa_hid_physical.fill(0);qa_hid_physical[0]=1;
        for(unsigned i=1;i<5;++i) qa_hid_physical[i]=128;
        qa_hid_physical[7]=static_cast<unsigned char>(++qa_hid_sequence);
        qa_hid_physical[8]=static_cast<unsigned char>(0x20|hat);qa_hid_physical[9]=static_cast<unsigned char>(shoulders);
        qa_now+=10;publish();
    };
    std::array<unsigned char,8192> buffer{};DWORD bytes{};OVERLAPPED request{};
    auto sync=[&](unsigned hat,unsigned shoulders=1) {
        physical(hat,shoulders);check(native_read_file(qa_hid_file,buffer.data(),8192,&bytes,nullptr) && bytes==64 && GetLastError()==777,"Native sync read status/error changed");
        auto expected=qa_hid_physical;expected[8]=buffer[8];check(std::equal(expected.begin(),expected.end(),buffer.begin()),"Native HID filter changed unrelated controls");
        return buffer[8]&15;
    };
    // Edge input is 64 bytes even when the game's buffer capacity is 8192.
    reset();check(sync(0)==8,"Native Edge first glance leaked with oversized game buffer");
    check(sync(0)==8 && !glance_emote_active(),"Native Edge first hold became emote");
    sync(8);check(sync(0)==0 && glance_emote_active(),"Native Edge double tap blocked");
    check(sync(0)==0,"Native Edge second hold blocked");sync(8);check(!glance_emote_active(),"Native Edge emote release stuck");
    reset();qa_hid_shared=true;check(sync(0)==8,"Shared/native HID chain first press failed");
    check(sync(0)==8 && !glance_emote_active(),"Shared/native HID chain duplicated a tap");sync(8);
    check(sync(0)==0 && glance_emote_active(),"Shared/native HID chain lost double tap");
    reset();qa_hid_shared=qa_hid_batch=true;physical(0);
    check(native_read_file(qa_hid_file,buffer.data(),2048,&bytes,nullptr) && bytes==128 &&
        (buffer[8]&15)==8 && (buffer[72]&15)==8 && !glance_emote_active(),"Batched USB reports used Bluetooth layout or duplicated policy");
    physical(0);check(native_read_file(qa_hid_file,buffer.data(),2048,&bytes,nullptr) &&
        (buffer[8]&15)==8 && (buffer[72]&15)==8 && !glance_emote_active(),"Repeated native batch hold became a double tap");
    // The actual user's hotkey: L1 + R1 + Down, in both camera modes.
    for(bool active:{false,true}) {
        reset();qa_active=active;toggle=0x302;publish();
        check(sync(4,3)==8,"Native Edge enter/exit full chord gesture leaked");
        sync(8,3);check(sync(4,3)==8 && !glance_emote_active(),"Native Edge enter/exit repeated tap became emote");
        qa_active=!active;qa_spectator=true;qa_now+=1200;
        check(sync(4,2)==8,"Native Edge hotkey latch lost after L1 release/camera transition");
        sync(8,2);check(sync(4,2)==4,"Native Edge hotkey latch did not release");
    }
    for(unsigned completion=0;completion<4;++completion) {
        reset();qa_hid_pending=true;
        auto submit=[&](unsigned hat) {
            physical(hat);check(!native_read_file(qa_hid_file,buffer.data(),8192,nullptr,&request) && GetLastError()==ERROR_IO_PENDING,"Native async read did not retain pending result");
            if(completion==0) check(native_overlapped(qa_hid_file,&request,&bytes,FALSE),"Native overlapped result failed");
            if(completion==1) check(native_overlapped_ex(qa_hid_file,&request,&bytes,100,FALSE),"Native overlapped-ex result failed");
            if(completion==2) {OVERLAPPED* returned{};ULONG_PTR key{};check(native_completion(nullptr,&bytes,&key,&returned,100) && returned==&request && key==123,"Native completion-port values changed");}
            if(completion==3) {OVERLAPPED_ENTRY entry{};ULONG count{};check(native_completion_ex(nullptr,&entry,1,&count,100,FALSE) && count==1 && entry.lpOverlapped==&request,"Native completion-port-ex values changed");}
            check(GetLastError()==777 && native_hid_requests.empty(),"Native completion error/request lifetime changed");return buffer[8]&15;
        };
        check(submit(0)==8,"Native async first tap leaked");submit(8);
        check(submit(0)==0 && glance_emote_active(),"Native async double tap blocked");
    }
    reset();qa_hid_pending=true;physical(0);native_read_file(qa_hid_file,buffer.data(),8192,nullptr,&request);
    qa_hid_incomplete=true;check(!native_overlapped(qa_hid_file,&request,&bytes,FALSE) && native_hid_requests.size()==1,"Incomplete native read was retired");
    qa_hid_incomplete=false;qa_hid_abort=true;
    check(!native_overlapped(qa_hid_file,&request,&bytes,FALSE) && GetLastError()==ERROR_OPERATION_ABORTED && native_hid_requests.empty(),"Aborted native read changed error/lifetime");
    reset();qa_hid_pending=true;physical(0);native_read_file(qa_hid_file,buffer.data(),8192,nullptr,&request);
    qa_hid_abort=true;OVERLAPPED_ENTRY failed_entry{};ULONG failed_count{};
    check(native_completion_ex(nullptr,&failed_entry,1,&failed_count,100,FALSE) &&
        (buffer[8]&15)==0 && native_hid_requests.empty(),"Failed NTSTATUS was treated as successful on x64");
    reset();qa_hid_abort=true;physical(0);
    check(!native_read_file_ex(qa_hid_file,buffer.data(),8192,&request,fake_native_callback) &&
        GetLastError()==ERROR_ACCESS_DENIED && native_hid_requests.empty(),"Failed callback submission retained an unowed callback");
    // Callback delivery is preserved, including a close/cancellation before APC.
    reset();physical(0);check(native_read_file_ex(qa_hid_file,buffer.data(),8192,&request,fake_native_callback),"Native callback read failed");
    std::memcpy(buffer.data(),qa_hid_physical.data(),64);qa_hid_callback(0,64,&request);
    check(qa_hid_callbacks==1 && qa_hid_callback_hat==8 && native_hid_requests.empty(),"Native callback saw unfiltered report or was lost");
    physical(8);native_read_file_ex(qa_hid_file,buffer.data(),8192,&request,fake_native_callback);
    native_overlapped(qa_hid_file,&request,&bytes,FALSE);qa_hid_callback(0,64,&request);
    check(qa_hid_callbacks==2,"Polling a callback read lost the owed APC callback");
    physical(0);native_read_file_ex(qa_hid_file,buffer.data(),8192,&request,fake_native_callback);
    native_close(qa_hid_file);qa_hid_callback(ERROR_OPERATION_ABORTED,0,&request);
    check(qa_hid_callbacks==3 && qa_hid_callback_error==ERROR_OPERATION_ABORTED && native_hid_requests.empty(),"Closing a callback read lost cancellation callback");
    reset();physical(0);check(native_input_report(qa_hid_file,buffer.data(),64) && (buffer[8]&15)==8 && GetLastError()==777,"Native input-report adapter failed");
    reset();private_controller_handle(qa_hid_file,true);check(sync(0)==0,"Independent binding reader was filtered");private_controller_handle(qa_hid_file,false);
    raw_controller_read=true;check(sync(0)==0,"Raw sampler was filtered");raw_controller_read=false;
    {NativeHidInternal internal;check(sync(0)==0,"Native adapter internal read was filtered");}
    check(sync(0)==8,"Native Edge filter did not resume after sampler bypass");
    qa_focus=false;check(sync(0)==0,"Unfocused native HID was edited");qa_focus=true;
    check(sync(0)==8 && !glance_emote_active(),"Focus restore retained native HID double tap");
    native_close(qa_hid_file);check(!native_hid_devices.contains(qa_hid_file),"Closed native HID handle retained identity");
    reset();native_attributes=&HidD_GetAttributes;native_preparsed=&HidD_GetPreparsedData;
    native_caps=&HidP_GetCaps;native_free_preparsed=&HidD_FreePreparsedData;
    std::vector<void*> pending;check(prepare_native_hid(pending) && pending.size()==8,"Native Windows API adapters were not prepared");
    direct_enable(pending);
    qa_real_close=native_original_close;native_original_close=qa_logging_close;
    // Real ordinary file I/O still forwards through the active API adapters.
    wchar_t directory[MAX_PATH]{};GetTempPathW(MAX_PATH,directory);
    const std::wstring path=std::wstring(directory)+L"jish-native-hid-test-"+std::to_wstring(GetCurrentProcessId())+L".tmp";
    const auto file=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    check(file!=INVALID_HANDLE_VALUE,"Native file forwarding fixture failed");
    constexpr char text[]="ordinary file reads remain unchanged";DWORD wrote{};
    check(WriteFile(file,text,sizeof(text),&wrote,nullptr),"Ordinary file write failed");
    LARGE_INTEGER start{};check(SetFilePointerEx(file,start,nullptr,FILE_BEGIN),"Ordinary file seek failed");
    check(ReadFile(file,buffer.data(),64,&bytes,nullptr) && bytes==sizeof(text) && !std::memcmp(text,buffer.data(),bytes),"Native HID adapters changed ordinary file I/O");
    {std::lock_guard lock(glance_mutex);
        check(CloseHandle(file),"Ordinary file close entered gesture cleanup");}
    // The production logger must close its file without waiting on either input
    // lock. CTest runs this executable as a child with a hard timeout.
    {
        std::unique_lock native_lock(native_hid_mutex);
        std::unique_lock glance_lock(glance_mutex);
        auto startup=std::async(std::launch::async,[]{
            for(unsigned i=0;i<100;++i) log("Concurrent startup diagnostic "+std::to_string(i));
        });
        check(startup.wait_for(std::chrono::seconds(3))==std::future_status::ready,
            "Production logging deadlocked against native input locks");
        startup.get();
    }
    auto startup=std::async(std::launch::async,[]{for(unsigned i=0;i<100;++i) log("Startup overlap");});
    for(unsigned i=0;i<100;++i) {
        NativeDiagnostics diagnostics;
        std::lock_guard native_lock(native_hid_mutex);
        std::lock_guard glance_lock(glance_mutex);
        diagnostics.messages.push_back("HID startup overlap");
    }
    startup.get();
    check(qa_diagnostic_closes.load()>=300,"Real logger file closures did not traverse the native hook");
    native_original_close=qa_real_close;
    const auto log_path=std::filesystem::temp_directory_path()/
        (L"jish-startup-log-"+std::to_wstring(GetCurrentProcessId()))/L"JishsFirstPersonMod.log";
    check(std::filesystem::file_size(log_path)>0,"Production logger did not write a real file");
    check(dingosdk::rollback_owned_hooks(),"Native Windows API adapter rollback failed");
    std::cout<<"Native DualSense Edge HID sync/async/callback/port adapters, oversized buffers, full enter/exit chord and private reader parity passed\n";
}
