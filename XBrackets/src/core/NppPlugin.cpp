#include "NppPlugin.h"


CNppPlugin::CNppPlugin() : m_hDllModule(NULL)
{
}

CNppPlugin::~CNppPlugin()
{
}

void CNppPlugin::OnDllProcessAttach(HINSTANCE hDLLInstance)
{
    TCHAR szPath[2*MAX_PATH + 1];

    m_hDllModule = (HMODULE) hDLLInstance;
    size_t pos = ::GetModuleFileName(m_hDllModule, szPath, 2*MAX_PATH);
    tstr dllFilePath(szPath, pos);

    pos = dllFilePath.find_last_of(_T("\\/"));
    if ( pos == tstr::npos )
        return; // should not happen

    m_sDllDir.assign(dllFilePath, 0, pos);
    m_sDllFileName.assign(dllFilePath, pos + 1);
    m_sIniFileName.assign(dllFilePath, pos + 1);

    pos = m_sIniFileName.rfind(_T('.'));
    if ( pos == tstr::npos )
        return; // should not happen

    m_sIniFileName.erase(pos, m_sIniFileName.length() - pos);
    m_sIniFileName.append(_T(".ini"));
}

void CNppPlugin::OnDllProcessDetach()
{
}

void CNppPlugin::nppSetInfo(const NppData& nppd)
{
    m_nppMsgr.setNppData(nppd);
    OnNppSetInfo(nppd);
}

