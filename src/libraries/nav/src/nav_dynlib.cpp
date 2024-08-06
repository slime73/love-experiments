#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "nav_dynlib.hpp"

namespace nav
{

DynLib::DynLib()
: mod(nullptr)
{}

DynLib::DynLib(const std::string &name)
{
#ifdef _WIN32
	mod = LoadLibraryA(name.c_str());
#else
	mod = dlopen(name.c_str(), RTLD_LAZY);
#endif

	if (mod == nullptr)
		throw std::runtime_error("cannot load " + name);
}

DynLib::DynLib(DynLib&& other)
: mod(other.mod)
{
	other.mod = nullptr;
}

DynLib &DynLib::operator=(DynLib&& other)
{
	close();
	mod = other.mod;
	other.mod = nullptr;
	return *this;
}

DynLib::~DynLib()
{
	close();
}

void *DynLib::_get(const std::string &name, bool &success)
{
	void *result = nullptr;

#ifdef _WIN32
	SetLastError(0);
	result = GetProcAddress((HMODULE) mod, name.c_str());
	success = GetLastError() == 0;
#else
	dlerror();
	result = dlsym(mod, name.c_str());
	success = dlerror() == nullptr;
#endif

	return result;
}

void DynLib::close()
{
	if (mod != nullptr)
	{
#ifdef _WIN32
	FreeLibrary((HMODULE) mod);
#else
	dlclose(mod);
#endif
	}
}

}
