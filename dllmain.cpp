#include "pch.h"
#include "AkelEdit.h"
#include "AkelDLL.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}


extern "C"
{ // Без этого функции AkelPad'ом не распознаются...

    const size_t NEWLINE_COUNT = 1;

    // Конкатенация + Форматирование текста
    void Copy(wchar_t*& buf, wchar_t* src, size_t cnt, int tabs)
    {
        if (!cnt)
            return;

        size_t   length = wcslen(buf);
        size_t   new_length = length+cnt+tabs+NEWLINE_COUNT+1;
        wchar_t* new_buffer = new wchar_t[new_length] { 0 };

        wcscpy_s(new_buffer, new_length, buf);
        wcsncat_s(new_buffer, new_length, src, cnt);
        if (tabs >= 0)
        {
            new_buffer[length + cnt] = '\n';
            for (int i = 0; i < tabs; i++)
                new_buffer[length+cnt+i+1] = '\t';
        }
        new_buffer[new_length-1] = '\0';

        delete[] buf;
        buf = new_buffer;
    }

    wchar_t* ExpandJson(wchar_t* base)
    {
        int      flag   = 0;
        size_t   tabs   = 0;
        wchar_t* prev = base;
        wchar_t* ptr  = base;
        wchar_t* buf  = new wchar_t[16] { 0 };
        while (*ptr) {
            if (*ptr == '\"' && (ptr-base > 0 && *(ptr-1) != '\\'))
                flag = !flag;
            else
            {
                if (!flag)
                {
                    if (*ptr == '{' || *ptr == '[')
                    {
                        Copy(buf, prev, ptr-prev+1, ++tabs);
                        prev = ptr+1;
                    }
                    else if (*ptr == '}' || *ptr == ']')
                    {
                        Copy(buf, prev, ptr-prev, --tabs);
                        prev = ptr;
                    }
                    else if (*ptr == ',')
                    {
                        Copy(buf, prev, ptr-prev+1, tabs);
                        prev = ptr+1;
                    }
                    else if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
                    {
                        Copy(buf, prev, ptr-prev, -1);
                        prev = ptr+1;
                    }
                }
            }
            ptr++;
        }
        Copy(buf, prev, ptr-prev, -1);
        return buf;
    }

    void Expand(PLUGINDATA* pData)
    {
        GETTEXTRANGE tr;
        tr.pText = nullptr;
        tr.cpMax = -1;
        tr.cpMin = 0;

        if (SendMessage(pData->hMainWnd, AKD_GETTEXTRANGEW, NULL, reinterpret_cast<LPARAM>(&tr)))
        {
            wchar_t* buf = ExpandJson(reinterpret_cast<wchar_t*>(tr.pText));
            SendMessage(pData->hMainWnd, AKD_FREETEXT, NULL, reinterpret_cast<LPARAM>(tr.pText));

            AESETTEXTW st;
            st.pText     = buf;
            st.dwTextLen = static_cast<UINT_PTR>(-1);
            st.nNewLine  = AELB_ASINPUT;
            SendMessage(pData->hWndEdit, AEM_EXSETTEXTW, NULL, reinterpret_cast<LPARAM>(&st));
            delete[] buf;
        }
    }

    void __declspec(dllexport) DllAkelPadID(PLUGINVERSION* pv)
    {
        pv->dwAkelDllVersion  = AKELDLL;
        pv->dwExeMinVersion3x = MAKE_IDENTIFIER(-1, -1, -1, -1);
        pv->dwExeMinVersion4x = MAKE_IDENTIFIER(4, 9, 7, 0);
        pv->pPluginName       = "JsonExpander";
    }

    void __declspec(dllexport) Run(PLUGINDATA* pData)
    {
        // Достигается при установлении галочки автозапуска
        pData->dwSupport |= PDS_SUPPORTALL;    // Автозапуск разрешен
        if (pData->dwSupport & PDS_GETSUPPORT)
            return;
        // Достигается при вызове функции
        Expand(pData);
    }

}