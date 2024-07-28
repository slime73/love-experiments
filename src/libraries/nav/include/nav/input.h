#ifndef _NAV_INPUT_H_
#define _NAV_INPUT_H_

#include "types.h"

/**
 * @brief NAV "input stream".
 * 
 * For user-defined "input stream", populate the struct accordingly.
 * This struct can be allocated on stack, and is assumed so.
 */
typedef struct nav_input
{
	/**
	 * @brief Function-specific userdata.
	 */
	void *userdata;

	/**
	 * @brief Function to close **and** deallocate `userdata`.
	 * @param userdata Double pointer to function-specific userdata.
	 */
	void (*close)(void **userdata);

	/**
	 * @brief Function to read from "input stream"
	 * @param userdata Function-specific userdata.
	 * @param dest Output buffer.
	 * @param size Amount of bytes to read.
	 * @return Amount of bytes read.
	 */
	size_t (*read)(void *userdata, void *dest, size_t size);

	/**
	 * @brief Function to seek "input stream". Stream must support seeking!
	 * @param pos New position of the stream, based on the beginning of the file.
	 * @param userdata Function-specific userdata.
	 * @return Non-zero on success, zero on failure.
	 */
	nav_bool (*seek)(void *userdata, uint64_t pos);

	/**
	 * @brief Function to get current "input stream" position. Stream must support this!
	 * @param userdata Function-specific userdata.
	 * @return Stream position relative to the beginning.
	 */
	uint64_t (*tell)(void *userdata);

	/**
	 * @brief Function to get "input stream" size. Stream must support this!
	 * @param userdata Function-specific userdata.
	 * @return Stream size in bytes.
	 */
	uint64_t (*size)(void *userdata);

#ifdef __cplusplus
	inline void closef()
	{
		close(&userdata);
	}

	inline size_t readf(void *dest, size_t size)
	{
		return read(userdata, dest, size);
	}

	inline bool seekf(uint64_t pos)
	{
		return seek(userdata, pos) != 0;
	}

	inline uint64_t tellf()
	{
		return tell(userdata);
	}

	inline uint64_t sizef()
	{
		return size(userdata);
	}
#endif /* __cplusplus */
} nav_input;

#endif /* _NAV_INPUT_H_ */
