#pragma once

#include <faabric/util/bytes.h>
#include <faabric/util/locks.h>

#include <threads/ThreadState.h>
#include <wasm/WasmExecutionContext.h>
#include <wasm/WasmModule.h>
#include <wavm/LoadedDynamicModule.h>

#include <WAVM/Runtime/Intrinsics.h>
#include <WAVM/Runtime/Linker.h>
#include <WAVM/Runtime/Runtime.h>

namespace wasm {

WAVM_DECLARE_INTRINSIC_MODULE(env)

WAVM_DECLARE_INTRINSIC_MODULE(wasi)

std::vector<uint8_t> wavmCodegen(std::vector<uint8_t>& wasmBytes,
                                 const std::string& fileName);

template<class T>
T unalignedWavmRead(WAVM::Runtime::Memory* memory, WAVM::Uptr offset)
{
    const uint8_t* bytes =
      WAVM::Runtime::memoryArrayPtr<uint8_t>(memory, offset, sizeof(T));
    return faabric::util::unalignedRead<T>(bytes);
}

template<class T>
void unalignedWavmWrite(const T& value,
                        WAVM::Runtime::Memory* memory,
                        WAVM::Uptr offset)
{
    uint8_t* bytes =
      WAVM::Runtime::memoryArrayPtr<uint8_t>(memory, offset, sizeof(T));
    faabric::util::unalignedWrite<T>(value, bytes);
}

class WAVMWasmModule final
  : public WasmModule
  , WAVM::Runtime::Resolver
{
  public:
    WAVMWasmModule();

    WAVMWasmModule(int threadPoolSizeIn);

    WAVMWasmModule(const WAVMWasmModule& other);

    WAVMWasmModule& operator=(const WAVMWasmModule& other);

    ~WAVMWasmModule();

    static void clearCaches();

    // ----- Module lifecycle -----
    void doBindToFunction(faabric::Message& msg, bool cache) override;

    void bindToFunctionNoZygote(faabric::Message& msg);

    void reset(faabric::Message& msg, const std::string& snapshotKey) override;

    // ----- Memory management -----
    uint32_t mmapFile(uint32_t fd, size_t length) override;

    uint8_t* wasmPointerToNative(uint32_t wasmPtr) override;

    size_t getMemorySizeBytes() override;

    size_t getMaxMemoryPages() override;

    uint8_t* getMemoryBase() override;

    // ----- Environment variables -----
    void writeWasmEnvToMemory(uint32_t envPointers,
                              uint32_t envBuffer) override;

    // ----- Debug -----
    void printDebugInfo() override;

    // ----- Internals -----
    WAVM::Runtime::GCPointer<WAVM::Runtime::Memory> defaultMemory;

    WAVM::Runtime::GCPointer<WAVM::Runtime::Table> defaultTable;

    WAVM::Runtime::GCPointer<WAVM::Runtime::Context> executionContext;

    WAVM::Runtime::GCPointer<WAVM::Runtime::Compartment> compartment;

    // ----- Execution -----
    void executeWasmFunction(
      WAVM::Runtime::Function* func,
      WAVM::IR::FunctionType funcType,
      const std::vector<WAVM::IR::UntaggedValue>& arguments,
      WAVM::IR::UntaggedValue& result);

    void executeWasmFunction(
      WAVM::Runtime::Function* func,
      const std::vector<WAVM::IR::UntaggedValue>& arguments,
      WAVM::IR::UntaggedValue& result);

    void executeWasmFunction(
      WAVM::Runtime::Context* ctx,
      WAVM::Runtime::Function* func,
      const std::vector<WAVM::IR::UntaggedValue>& arguments,
      WAVM::IR::UntaggedValue& result);

    void writeArgvToMemory(uint32_t wasmArgvPointers,
                           uint32_t wasmArgvBuffer) override;

    // ----- Resolution/ linking -----
    static WAVM::Runtime::Function* getFunction(WAVM::Runtime::Instance* module,
                                                const std::string& funcName,
                                                bool strict);

    WAVM::Runtime::Function* getFunctionFromPtr(int funcPtr) const;

    bool resolve(const std::string& moduleName,
                 const std::string& name,
                 WAVM::IR::ExternType type,
                 WAVM::Runtime::Object*& resolved) override;

    int32_t getGlobalI32(const std::string& globalName,
                         WAVM::Runtime::Context* context);

    // ----- Threading -----
    static WAVM::Runtime::Context* createThreadContext(
      uint32_t stackTop,
      WAVM::Runtime::ContextRuntimeData* contextRuntimeData);

    // ----- Disassembly -----
    std::map<std::string, std::string> buildDisassemblyMap();

    // ----- Dynamic linking -----
    int dynamicLoadModule(const std::string& path,
                          WAVM::Runtime::Context* context);

    uint32_t getDynamicModuleFunction(int handle, const std::string& funcName);

    int getDynamicModuleCount();

    uint32_t addFunctionToTable(WAVM::Runtime::Object* exportedFunc) const;

    int getNextMemoryBase();

    int getNextStackPointer();

    int getNextTableBase();

    int getFunctionOffsetFromGOT(const std::string& funcName);

    int getDataOffsetFromGOT(const std::string& name);

    int32_t executeFunction(faabric::Message& msg) override;

    int32_t executeOMPThread(int threadPoolIdx,
                             uint32_t stackTop,
                             faabric::Message& msg) override;

    int32_t executePthread(int threadPoolIdx,
                           uint32_t stackTop,
                           faabric::Message& msg) override;

  private:
    std::shared_mutex resetMx;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> envModule;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> wasiModule;
    WAVM::Runtime::GCPointer<WAVM::Runtime::Instance> moduleInstance;

    // Map of dynamically loaded modules
    std::unordered_map<std::string, int> dynamicPathToHandleMap;
    std::unordered_map<int, LoadedDynamicModule> dynamicModuleMap;
    int lastLoadedDynamicModuleHandle = 0;
    LoadedDynamicModule& getLastLoadedDynamicModule();

    // Dynamic linking tables and memories
    std::unordered_map<std::string, WAVM::Uptr> globalOffsetTableMap;
    std::unordered_map<std::string, std::pair<int, bool>> globalOffsetMemoryMap;
    std::unordered_map<std::string, int> missingGlobalOffsetEntries;

    // Memory management
    bool doGrowMemory(uint32_t pageChange) override;

    // OpenMP
    std::vector<WAVM::Runtime::Context*> openMPContexts;

    static WAVM::Runtime::Instance* getEnvModule();

    static WAVM::Runtime::Instance* getWasiModule();

    void doBindToFunctionInternal(faabric::Message& msg,
                                  bool executeZygote,
                                  bool useCache);

    void writeStringArrayToMemory(const std::vector<std::string>& strings,
                                  uint32_t strPoitners,
                                  uint32_t strBuffer) const;

    void clone(const WAVMWasmModule& other, const std::string& snapshotKey);

    void addModuleToGOT(WAVM::IR::Module& mod, bool isMainModule);

    void executeZygoteFunction();

    void executeWasmConstructorsFunction(WAVM::Runtime::Instance* module);

    void doWAVMGarbageCollection();

    WAVM::Runtime::Instance* createModuleInstance(
      const std::string& name,
      const std::string& sharedModulePath);

    static WAVM::Runtime::Function* getMainFunction(
      WAVM::Runtime::Instance* module);

    static WAVM::Runtime::Function* getDefaultZygoteFunction(
      WAVM::Runtime::Instance* module);

    static WAVM::Runtime::Function* getWasmConstructorsFunction(
      WAVM::Runtime::Instance* module);
};

class WAVMModuleCache
{
  public:
    std::pair<wasm::WAVMWasmModule&, faabric::util::SharedLock> getCachedModule(
      faabric::Message& msg);

    std::string registerResetSnapshot(wasm::WasmModule& module,
                                      faabric::Message& msg);

    void clear();

    size_t getTotalCachedModuleCount();

  private:
    std::shared_mutex mx;
    std::unordered_map<std::string, wasm::WAVMWasmModule> cachedModuleMap;

    int getCachedModuleCount(const std::string& key);
};

WAVMModuleCache& getWAVMModuleCache();

WAVMWasmModule* getExecutingWAVMModule();
}
