#include "__init__.h"

#if defined(_WIN32)

#include <Windows.h>
#include <assert.h>

static WCHAR* _infra_utf8_to_wide(const char* str)
{
    int multi_byte_sz = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    size_t buf_sz = multi_byte_sz * sizeof(WCHAR);

    WCHAR* buf = (WCHAR*)malloc(buf_sz);
    int ret = MultiByteToWideChar(CP_UTF8, 0, str, -1, (WCHAR*)buf, multi_byte_sz);
    assert(ret == multi_byte_sz);

    return buf;
}

static char* _infra_wide_to_utf8(const WCHAR* str)
{
    int utf8_sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char* utf8 = malloc(utf8_sz + 1);
    if (utf8 == NULL)
    {
        return NULL;
    }

    int ret = WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8, utf8_sz, NULL, NULL);
    assert(ret == utf8_sz);
    utf8[utf8_sz] = '\0';

    return utf8;
}

static int _infra_readdir_raise_error(lua_State* L, DWORD errcode)
{
    wchar_t buf[256];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(wchar_t)), NULL);

    char* msg = _infra_wide_to_utf8(buf);
    lua_pushstring(L, msg);
    free(msg);

    return lua_error(L);
}

static const char* _infra_file_type(DWORD type)
{
    if (type & FILE_ATTRIBUTE_DIRECTORY)
    {
        return "dir";
    }

    return "file";
}

static int _infra_readdir(lua_State* L)
{
    luaL_checkstring(L, 1);
    lua_pushstring(L, "/*");
    lua_concat(L, 2);

    WCHAR* wpath = _infra_utf8_to_wide(lua_tostring(L, 1));

    WIN32_FIND_DATAW FindFileData;
    HANDLE hFile = FindFirstFileW(wpath, &FindFileData);
    free(wpath);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD errcode = GetLastError();
        return _infra_readdir_raise_error(L, errcode);
    }

    lua_newtable(L);

    do 
    {
        if (wcscmp(FindFileData.cFileName, L".") == 0 || wcscmp(FindFileData.cFileName, L"..") == 0)
        {
            continue;
        }

        lua_newtable(L);

        lua_pushstring(L, _infra_file_type(FindFileData.dwFileAttributes));
        lua_setfield(L, -2, "type");

        char* name = _infra_wide_to_utf8(FindFileData.cFileName);
        lua_setfield(L, -2, name);
        free(name);

    } while (FindNextFileW(hFile, &FindFileData));

    FindClose(hFile);
    return 1;
}

#else

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

static const char* _infra_file_type(int type)
{
    switch (type)
    {
    case DT_DIR:
        return "dir";

    case DT_REG:
        return "file";

    default:
        break;
    }

    return "unknown";
}

static int _infra_readdir(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);

    DIR* p_dir = opendir(path);
    if (p_dir == NULL)
    {
        return luaL_error(L, "%s (%d).", strerror(errno), errno);
    }

    lua_newtable(L); // this will be the return value.

    struct dirent* p_dp;
    while ((p_dp = readdir(p_dir)) != NULL)
    {
        if (strcmp(p_dp->d_name, ".") == 0 || strcmp(p_dp->d_name, "..") == 0)
        {
            continue;
        }

        lua_newtable(L);

        lua_pushstring(L, _infra_file_type(p_dp->d_type));
        lua_setfield(L, -2, "type");

        lua_setfield(L, -2, p_dp->d_name);
    }

    closedir(p_dir);

    return 1;
}

#endif

const infra_lua_api_t infra_f_readdir = {
"readdir", _infra_readdir, 0,
"Read directory entry.",

"[SYNOPSIS]\n"
"table readdir(string path)\n"
"\n"
"[DESCRIPTION]\n"
"readdir() returns an array of information that `path` contains. If directory is\n"
"empty, the length is zero. If directory is not exists, error will be raised.\n"
"\n"
"The contant if each element is:\n"
"  + name: The name of that file or directory.\n"
"  + type: The type of that them, can be `dir`, `file` or `unknown`.\n"
};
