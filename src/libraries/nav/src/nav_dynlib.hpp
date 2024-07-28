#ifndef _NAV_DYNLIB_HPP_
#define _NAV_DYNLIB_HPP_

#include <string>

namespace nav
{

class DynLib
{
public:
	DynLib();
	DynLib(const std::string &name);
	DynLib(DynLib&& other);
	DynLib(DynLib&) = delete;
	~DynLib();
	DynLib& operator=(DynLib&& other);

	template<typename T> bool get(const std::string &name, T* dest)
	{
		bool err = false;
		void *result = _get(name, err);
		*dest = (T) result;
		return err;
	}

private:
	void *_get(const std::string &name, bool &err);
	void close();

	void *mod;
};

}

#endif /* _NAV_DYNLIB_HPP_ */
