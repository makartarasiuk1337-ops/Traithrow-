#pragma once
#include <map>
#include <string>
#include <vector>
#include <client_class.h>
#include <dt_recv.h>

class NetVarManager {
public:
    static NetVarManager& Get() {
        static NetVarManager instance;
        return instance;
    }

    void Init(ClientClass* pFirstClass);
    uint32_t GetOffset(const std::string& tableName, const std::string& propName);

private:
    void DumpTable(const std::string& tableName, RecvTable* pTable, uint32_t offset);
    std::map<std::string, std::map<std::string, uint32_t>> m_offsets;
};

#define GET_NETVAR(type, name, table, prop) \
    type& name() { \
        static uint32_t offset = NetVarManager::Get().GetOffset(table, prop); \
        return *reinterpret_cast<type*>(reinterpret_cast<uintptr_t>(this) + offset); \
    }
