#include "netvars.h"
#include <iostream>
#include <client_class.h>
#include <dt_recv.h>

void NetVarManager::Init(ClientClass* pFirstClass) {
    for (ClientClass* pCurr = pFirstClass; pCurr; pCurr = pCurr->m_pNext) {
        if (pCurr->m_pRecvTable) {
            DumpTable(pCurr->m_pRecvTable->GetName(), pCurr->m_pRecvTable, 0);
        }
    }
}

void NetVarManager::DumpTable(const std::string& tableName, RecvTable* pTable, uint32_t offset) {
    for (int i = 0; i < pTable->GetNumProps(); ++i) {
        RecvProp* pProp = pTable->GetProp(i);

        if (!pProp || isdigit(pProp->GetName()[0]))
            continue;

        if (strcmp(pProp->GetName(), "baseclass") == 0)
            continue;

        if (pProp->GetDataTable()) {
            DumpTable(tableName, pProp->GetDataTable(), offset + pProp->GetOffset());
        } else {
            m_offsets[tableName][pProp->GetName()] = offset + pProp->GetOffset();
        }
    }
}

uint32_t NetVarManager::GetOffset(const std::string& tableName, const std::string& propName) {
    auto it = m_offsets.find(tableName);
    if (it != m_offsets.end()) {
        auto propIt = it->second.find(propName);
        if (propIt != it->second.end()) {
            return propIt->second;
        }
    }
    return 0;
}
