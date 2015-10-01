#ifndef COMPILER_LAYER_H
#define COMPILER_LAYER_H

#include "JITMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"

namespace rjit {

class JITCompileLayer {
  public:
    typedef llvm::orc::ObjectLinkingLayer<> ObjLayer;
    typedef llvm::orc::IRCompileLayer<ObjLayer> CompileLayer;
    typedef CompileLayer::ModuleSetHandleT ModuleHandle;

    static ModuleHandle getHandle(llvm::Module* m);
    static void* getFunctionPointer(ModuleHandle handle, std::string name);
    static uint64_t findSymbolAddress(std::string name) {
        return compileLayer->findSymbol(name, false).getAddress();
    }

  private:
    static void recordStackmaps(ModuleHandle handle, llvm::Module* m,
                                JITMemoryManager* mm);

    static ObjLayer objectLayer;
    static std::unique_ptr<CompileLayer> compileLayer;
};
}

#endif