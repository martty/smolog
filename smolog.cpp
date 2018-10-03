//
// Copyright(c) 2018 Marcell Kiss
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <ctime>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <iostream>

#include "smolog.hpp"

namespace smolog {
	struct _logger_state {
		std::vector<std::shared_ptr<sink>> sinks;

		// internal buffer to format to
		std::vector<char> buf;
		std::string name;

		_logger_state(std::string name) : name(std::move(name)) {}
	};

	logger::~logger() {}

	constexpr const char * logger::level_as_string(Level level) const {
		switch (level) {
		case Level::Off: return "off";
		case Level::Trace: return "trace";
		case Level::Debug: return "debug";
		case Level::Info: return "info";
		case Level::Warning: return "warn";
		case Level::Error: return "error";
		case Level::Critical: return "critical";
		default: return "??";
		}
	}

	void logger::add_sink(std::shared_ptr<sink> s) {
		_internal->sinks.emplace_back(std::move(s));
	}

	void logger::remove_sink(sink * to_remove) {
		for (auto it = _internal->sinks.begin(); it != _internal->sinks.end(); it++) {
			if (it->get() == to_remove) {
				_internal->sinks.erase(it);
				return;
			}
		}
	}

	void logger::remove_sink(std::shared_ptr<sink>& s) {
		remove_sink(s.get());
	}

	void logger::set_name(const char* name) {
		_internal->name = name;
	}

	const char* logger::get_name() const {
		return _internal->name.c_str();
	}

	// [ date time ] [level] [logger_name] message
	void logger::emit_prompt(Level level) {
		auto& buf = _internal->buf;
		auto& name = _internal->name;
		std::time_t t = std::time(nullptr);
		char time_buf[100];
		std::strftime(time_buf, sizeof time_buf, "[ %D %T ]", std::gmtime(&t));
		auto time_size = strlen(time_buf);
		auto buf_size = buf.size();
		auto lnsize = snprintf(nullptr, 0, " [%s] [%s] ", level_as_string(level), name.c_str());
		buf.resize(buf_size + time_size + lnsize + 1);
		strcpy_s(buf.data() + buf_size, time_size + 1, time_buf);
		snprintf(buf.data() + buf_size + time_size, lnsize + 1, " [%s] [%s] ", level_as_string(level), name.c_str());
		// remove trailing \0
		buf.resize(buf.size() - 1);
	}
	void logger::format(const char * fmt, va_list args1) {
		auto& buf = _internal->buf;
		va_list args2;
		va_copy(args2, args1);

		auto size_req = 1 + std::vsnprintf(NULL, 0, fmt, args1);
		va_end(args1);

		auto prev_size = buf.size();
		buf.resize(buf.size() + size_req + 1);

		std::vsnprintf(buf.data() + prev_size, size_req, fmt, args2);
		va_end(args2);

		// write newline
		buf[buf.size() - 2] = '\n';
		buf[buf.size() - 1] = '\0';
	}
	void logger::_log(Level level, const char * fmt, ...) {
		auto& buf = _internal->buf;
		auto& sinks = _internal->sinks;

		if (current_level > level) return;
		emit_prompt(level);
		va_list args;
		va_start(args, fmt);
		format(fmt, args);
		va_end(args);

		for (auto& sink : sinks) {
			sink->write({ buf.data(), buf.size() - 1 /* like strlen */, level });
		}

		if (level >= flush_level)
			flush();

		buf.clear();
	}
	void logger::flush() {
		auto& sinks = _internal->sinks;
		for (auto& sink : sinks) {
			sink->flush();
		}
	}
	logger::logger(const char * name) : _internal(std::make_unique<_logger_state>(name)){}

	void stdout_sink::write(const message & msg) {
		fputs(msg.str, stdout);
	}
	void stdout_sink::flush() {
		::fflush(stdout);
	}
	void stderr_sink::write(const message & msg) {
		fputs(msg.str, stdout);
	}
	void stderr_sink::flush() {
		::fflush(stdout);
	}
};

// Some sinks adapted from spdlog https://github.com/gabime/spdlog/
// Copyright(c) 2016 spdlog
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifdef _WIN32 
#include <Windows.h>
#include <Wincon.h>
namespace smolog {
	static const WORD BOLD = FOREGROUND_INTENSITY;
	static const WORD RED = FOREGROUND_RED;
	static const WORD CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE;
	static const WORD WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	static const WORD YELLOW = FOREGROUND_RED | FOREGROUND_GREEN;

	struct _wincolor_state {
		HANDLE out_handle;
		std::vector<WORD> colors;

		_wincolor_state(HANDLE out_handle) : out_handle(out_handle) {}
	};

	wincolor_sink::~wincolor_sink() {}

	wincolor_sink::wincolor_sink(HANDLE std_handle) : _internal(std::make_unique<_wincolor_state>(std_handle)){
		auto& colors = _internal->colors;
		colors.resize(smolog::Level::Critical + 1);
		colors[smolog::Level::Trace] = CYAN;
		colors[smolog::Level::Debug] = CYAN;
		colors[smolog::Level::Info] = WHITE | BOLD;
		colors[smolog::Level::Warning] = YELLOW | BOLD;
		colors[smolog::Level::Error] = RED | BOLD; // red bold
		colors[smolog::Level::Critical] = BACKGROUND_RED | WHITE | BOLD; // white bold on red background
		colors[smolog::Level::Off] = 0;
	}

	void wincolor_sink::set_color(smolog::Level level, WORD color) {
		_internal->colors[level] = color;
	}

	void wincolor_sink::write(const message & msg) {
		auto color = _internal->colors[msg.level];
		auto orig_attribs = set_console_attribs(color);
		::WriteConsoleA(_internal->out_handle, msg.str, static_cast<DWORD>(msg.size), nullptr, nullptr);
		::SetConsoleTextAttribute(_internal->out_handle, orig_attribs); //reset to orig colors
	}

	// set color and return the orig console attributes (for resetting later)
	WORD wincolor_sink::set_console_attribs(WORD attribs){
		CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
		::GetConsoleScreenBufferInfo(_internal->out_handle, &orig_buffer_info);
		WORD back_color = orig_buffer_info.wAttributes;
		// retrieve the current background color
		back_color &= static_cast<WORD>(~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
		// keep the background color unchanged
		::SetConsoleTextAttribute(_internal->out_handle, attribs | back_color);
		return orig_buffer_info.wAttributes; //return orig attribs
	}

	wincolor_stdout_sink::wincolor_stdout_sink() : wincolor_sink(GetStdHandle(STD_OUTPUT_HANDLE)) {}

	wincolor_stderr_sink::wincolor_stderr_sink() : wincolor_sink(GetStdHandle(STD_ERROR_HANDLE)) {}
#endif
#ifdef _MSC_VER
	void msvc_sink::write(const message & msg) {
		OutputDebugStringA(msg.str);
	}
};
#endif

namespace smolog {
	struct _dist_state {
		std::vector<std::shared_ptr<sink>> sinks;
	};

	void dist_sink::write(const message & msg) {
		for (auto& sink : _internal->sinks) {
			sink->write(msg);
		}
	}

	dist_sink::dist_sink() : _internal(std::make_unique<_dist_state>()){}

	void dist_sink::add_sink(std::shared_ptr<sink> s) {
		_internal->sinks.emplace_back(std::move(s));
	}

	void dist_sink::remove_sink(sink * to_remove) {
		for (auto it = _internal->sinks.begin(); it != _internal->sinks.end(); it++) {
			if (it->get() == to_remove) {
				_internal->sinks.erase(it);
				return;
			}
		}
	}

	struct _file_state {
		FILE * fd;
	};

	file_sink::file_sink(const char * filename, bool truncate) : _internal(std::make_unique<_file_state>()){
		const char * open_flags = truncate ? "w" : "a";
		int err = fopen_s(&_internal->fd, filename, open_flags);
	}

	void file_sink::write(const message & msg) {
		fputs(msg.str, _internal->fd);
	}

	void file_sink::flush() {
		fflush(_internal->fd);
	}

	file_sink::~file_sink() {
		fclose(_internal->fd);
	}
	
	ostream_sink::ostream_sink(std::ostream& os) : ostream(os) {}

    void ostream_sink::write(const message & msg){
        ostream.write(msg.str, msg.size);
    }

    void ostream_sink::flush() {
        ostream.flush();
    }

	mt_sink::mt_sink(std::shared_ptr<sink> l) : wrapped(std::move(l)) {}

	void mt_sink::write(const message & msg) {
		static std::mutex mut;
		std::lock_guard<std::mutex> _(mut);
		wrapped->write(msg);
	}
};