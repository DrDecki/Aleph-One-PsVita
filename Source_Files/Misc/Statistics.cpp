/*
	Copyright (C) 2011 Gregory Smith
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Collects and uploads game stats
*/

#include "Statistics.h"
#include "HTTP.h"
#include "lua_script.h"

#include "sdl_widgets.h"
#include "alephversion.h"
#include "preferences.h"

#include <functional>
#include <sstream>

class ScopedMutex
{
public:
	ScopedMutex(SDL_mutex* mutex) : mutex_(mutex) {
		SDL_LockMutex(mutex_);
	}

	~ScopedMutex() { 
		SDL_UnlockMutex(mutex_);
	}
private:
	SDL_mutex* mutex_;
};

StatsManager::StatsManager() : thread_(0), run_(true), busy_(false)
{
	entry_mutex_ = SDL_CreateMutex();

#ifndef __vita__
	// do uploads in a separate thread
	thread_ = SDL_CreateThread(Run, "StatsManager_uploadThread", this);
#endif
}

void StatsManager::Process()
{
#ifdef __vita__
	// Vita: skip stats collection/upload entirely - avoids a background
	// thread doing unrelated heap activity concurrently with the main
	// thread (implicated in intermittent heap corruption crashes)
	return;
#endif
	Entry entry;
	if (CollectLuaStats(entry.options, entry.parameters))
	{
		ScopedMutex mutex(entry_mutex_);
		busy_ = true;
		entries_.push(entry);
	}
}

void StatsManager::CheckForDone(dialog* d)
{
	if (!busy_)
	{
		run_ = false;
		SDL_WaitThread(thread_, NULL);
		thread_ = 0;
		d->quit(0);
	}
}

void StatsManager::Finish()
{
	if (busy_)
	{
		dialog d;
		vertical_placer* placer = new vertical_placer;
		placer->dual_add(new w_static_text("Uploading stats"), d);
		placer->add(new w_spacer, true);
		w_button *button = new w_button("CANCEL", dialog_cancel, &d);
		placer->dual_add(button, d);
		d.set_widget_placer(placer);
		d.activate_widget(button);
		
		d.set_processing_function(std::bind(&StatsManager::CheckForDone, this, std::placeholders::_1));
		d.run();
	}
	else
	{
		run_ = false;
		SDL_WaitThread(thread_, NULL);
		thread_ = 0;
	}
}

static uint32 checksum_string(const std::string& s)
{
	uint32 checksum = 0;
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
	{
		checksum += reinterpret_cast<const unsigned char&>(*it);
	}

	return checksum;
}

int StatsManager::Run(void *pv)
{
	StatsManager* sm = reinterpret_cast<StatsManager*>(pv);
	HTTPClient client;

	while (sm->run_)
	{
		std::unique_ptr<Entry> entry;
		{
			ScopedMutex mutex(sm->entry_mutex_);
			if (sm->entries_.empty())
			{
				sm->busy_ = false;
			}
			else
			{
				entry = std::make_unique<Entry>(sm->entries_.front());
				sm->entries_.pop();
			}
		}

		if (entry.get())
		{
			entry->parameters["platform"] = A1_DISPLAY_PLATFORM;
			if (dynamic_world->player_count > 1)
				entry->parameters["session id"] = NetSessionIdentifier();
			entry->parameters["username"] = network_preferences->metaserver_login;
			entry->parameters["password"] = network_preferences->metaserver_password;
			
			// generate checksum
			uint32 checksum = 0;
			for (std::map<std::string, std::string>::const_iterator it = entry->parameters.begin(); it != entry->parameters.end(); ++it)
			{
				checksum += checksum_string(it->first);
				checksum += checksum_string(it->second);
			}
		
			std::ostringstream oss;
			oss << checksum;
			entry->parameters["checksum"] = oss.str();

			client.Post(A1_STATSERVER_ADD_URL, entry->parameters);
		}
		else
		{
			sleep_for_machine_ticks(MACHINE_TICKS_PER_SECOND / 5);
		}
		
	}

	return 0;
}

#ifdef __vita__
// ---------------------------------------------------------------
// TEMPORARY diagnostic heap guard - wraps global operator new/delete
// to detect out-of-bounds writes around C++ heap allocations.
// Logs to ux0:/heap_guard_hit.txt and forces an immediate crash at
// the moment corruption is detected, instead of letting it surface
// later at an unrelated malloc/free call. Remove once the root
// cause of the intermittent heap corruption crash is found.
// ---------------------------------------------------------------
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>

namespace {
	const size_t HEAP_GUARD_PAD = 32;
	const unsigned char HEAP_GUARD_FRONT_BYTE = 0xA5;
	const unsigned char HEAP_GUARD_BACK_BYTE = 0x5A;

	void heap_guard_abort(const char* reason, void* user_ptr, size_t size, void* caller)
	{
		FILE* f = fopen("ux0:/heap_guard_hit.txt", "a");
		if (f)
		{
			fprintf(f, "HEAP CORRUPTION DETECTED: %s\n", reason);
			fprintf(f, "  user_ptr=%p size=%lu\n", user_ptr, (unsigned long)size);
			fprintf(f, "  caller (return address of delete call)=%p\n", caller);
			fclose(f);
		}
		volatile int* crash_ptr = 0;
		*crash_ptr = 0;
	}

	void* guarded_alloc(size_t size)
	{
		size_t total = sizeof(size_t) + HEAP_GUARD_PAD * 2 + size;
		unsigned char* base = static_cast<unsigned char*>(malloc(total));
		if (!base) throw std::bad_alloc();
		*reinterpret_cast<size_t*>(base) = size;
		memset(base + sizeof(size_t), HEAP_GUARD_FRONT_BYTE, HEAP_GUARD_PAD);
		unsigned char* user_ptr = base + sizeof(size_t) + HEAP_GUARD_PAD;
		memset(user_ptr + size, HEAP_GUARD_BACK_BYTE, HEAP_GUARD_PAD);
		return user_ptr;
	}

	void guarded_free(void* ptr, void* caller)
	{
		if (!ptr) return;
		unsigned char* user_ptr = static_cast<unsigned char*>(ptr);
		unsigned char* base = user_ptr - HEAP_GUARD_PAD - sizeof(size_t);
		size_t size = *reinterpret_cast<size_t*>(base);
		unsigned char* front = base + sizeof(size_t);
		for (size_t i = 0; i < HEAP_GUARD_PAD; ++i)
		{
			if (front[i] != HEAP_GUARD_FRONT_BYTE)
			{
				heap_guard_abort("front guard overwritten (buffer underflow)", user_ptr, size, caller);
			}
		}
		unsigned char* back = user_ptr + size;
		for (size_t i = 0; i < HEAP_GUARD_PAD; ++i)
		{
			if (back[i] != HEAP_GUARD_BACK_BYTE)
			{
				heap_guard_abort("back guard overwritten (buffer overflow)", user_ptr, size, caller);
			}
		}
		free(base);
	}
}

void* operator new(std::size_t size)
{
	return guarded_alloc(size);
}

void* operator new[](std::size_t size)
{
	return guarded_alloc(size);
}

void operator delete(void* ptr) noexcept
{
	guarded_free(ptr, __builtin_return_address(0));
}

void operator delete[](void* ptr) noexcept
{
	guarded_free(ptr, __builtin_return_address(0));
}

void operator delete(void* ptr, std::size_t) noexcept
{
	guarded_free(ptr, __builtin_return_address(0));
}

void operator delete[](void* ptr, std::size_t) noexcept
{
	guarded_free(ptr, __builtin_return_address(0));
}
#endif
