#include "worker.h"
#include "resolver.h"

#include "Programs/CLI.h"

using namespace IR;
using namespace Runtime;

namespace worker {
    WasmModule::WasmModule() {

    }

    int WasmModule::execute(message::FunctionCall &call) {
        std::cout << "Received call:  " << call.user() << " - " << call.function() << "\n";

        std::string filePath = infra::getFunctionFile(call);
        std::cout << "Executing function at:  " << filePath << "\n";

        Module module;
        if (!loadModule(filePath.c_str(), module)) { return EXIT_FAILURE; }

        // Link the module with the intrinsic modules.
        Compartment *compartment = Runtime::createCompartment();
        Context *context = Runtime::createContext(compartment);
        RootResolver rootResolver(compartment);

        // Emscripten set-up
        Emscripten::Instance *emscriptenInstance = Emscripten::instantiate(compartment, module);

        if (emscriptenInstance) {
            rootResolver.moduleNameToInstanceMap.set("env", emscriptenInstance->env);
            rootResolver.moduleNameToInstanceMap.set("asm2wasm", emscriptenInstance->asm2wasm);
            rootResolver.moduleNameToInstanceMap.set("global", emscriptenInstance->global);
        }

        // Linking
        LinkResult linkResult = linkModule(module, rootResolver);
        if(!linkResult.success)
        {
            std::cerr << "Failed to link module:" << std::endl;
            for(auto& missingImport : linkResult.missingImports)
            {
                std::cerr << "Missing import: module=\"" << missingImport.moduleName
                          << "\" export=\"" << missingImport.exportName
                          << "\" type=\"" << asString(missingImport.type) << "\"" << std::endl;
            }
            return EXIT_FAILURE;
        }

        // Instantiate the module.
        ModuleInstance *moduleInstance = instantiateModule(
                compartment,
                module,
                std::move(linkResult.resolvedImports),
                filePath.c_str());
        if(!moduleInstance) { return EXIT_FAILURE; }

        // Call the module start function, if it has one.
        FunctionInstance *startFunction = getStartFunction(moduleInstance);
        if (startFunction) {
            invokeFunctionChecked(context, startFunction, {});
        }

        // Call the Emscripten global initalizers.
        Emscripten::initializeGlobals(context, module, moduleInstance);

        // Extract the module's main function
        // Note that emscripten can add an underscore before main
        FunctionInstance *functionInstance = asFunctionNullable(getInstanceExport(moduleInstance, "main"));
        if(!functionInstance) { functionInstance = asFunctionNullable(getInstanceExport(moduleInstance,"_main")); }

        // Make the call
        std::vector<Value> invokeArgs;
        IR::ValueTuple functionResults = invokeFunctionChecked(context, functionInstance, invokeArgs);

        // Clear up
        Runtime::collectGarbage();

        return 0;
    }
}