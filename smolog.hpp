//
// Copyright(c) 2018 Marcell Kiss
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <cstdarg>
#include <memory>
#include <iosfwd>

namespace smolog {
	enum Level : unsigned char {
		Off, Trace, Debug, Info, Warning, Error, Critical
	};

	// sinks receive this struct to write
	struct message {
		const char * str;
		size_t size;
		Level level;
	};

	// base class for sinks
	struct sink {
		virtual void write(const message& msg) = 0;
		virtual void flush() {};
		virtual ~sink() {}
	};

	struct logger {
		// all messages below this level will be discarded
		Level current_level = Level::Debug;
		// all messages received at this or higher level will be flushed
		Level flush_level = Level::Debug;

		void add_sink(std::shared_ptr<sink>);
		void remove_sink(sink*);
		void remove_sink(std::shared_ptr<sink>&);
	
		void _log(Level level, const char* fmt, ...);
		void flush();

		logger(const char* name);
		~logger();
	private:
		// [ date time ] [level] [logger_name] message
		void emit_prompt(Level level);
		void format(const char* fmt, va_list args1);
		constexpr const char* level_as_string(Level level) const;

		std::unique_ptr<struct _logger_state> _internal;
	};
};

// While very ugly, these macros enable cross-compiler checking of the format strings
// These can be removed as MSVC implements an attribute to check user-defined functions
// We rely on the compiler frontend not figuring out that sqrtf(4.0f) is always "true"

#ifndef SMOLOG_DISABLE_MACROS
#define trace(...) _log(smolog::Level::Trace, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define debug(...) _log(smolog::Level::Debug, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define info(...) _log(smolog::Level::Info, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define warn(...) _log(smolog::Level::Warning, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define error(...) _log(smolog::Level::Error, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define critical(...) _log(smolog::Level::Critical, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#define log_at(level, ...) _log(level, __VA_ARGS__), sqrtf(4.0f) || printf(__VA_ARGS__);
#endif

/****************
*
*    SINKS
*
****************/

namespace smolog {
	struct stdout_sink : sink {
		virtual void write(const message& msg) override;
		virtual void flush() override;
	};
	
	struct stderr_sink : sink {
		virtual void write(const message& msg) override;
		virtual void flush() override;
	};

#ifdef _WIN32
#ifndef HANDLE
	using HANDLE = void*;
#endif
#ifndef WORD
	using WORD = unsigned short;
#endif

	// colored log for Windows consoles
	struct wincolor_sink : public sink {

		wincolor_sink(HANDLE std_handle);
		~wincolor_sink();

		// change the color for the given level
		void set_color(smolog::Level level, WORD color);

		void write(const message& msg) override;

	private:
		WORD set_console_attribs(WORD attribs);

		std::unique_ptr<struct _wincolor_state> _internal;
	};

	struct wincolor_stdout_sink : public wincolor_sink {
		wincolor_stdout_sink();
	};

	struct wincolor_stderr_sink : public wincolor_sink {
		wincolor_stderr_sink();
	};
#endif
#ifdef _MSC_VER
	// write to Debug Output in MSVC
	struct msvc_sink : sink {
		virtual void write(const message& msg) override;
	};
#endif
	// distribute written content to multiple sinks
	struct dist_sink : public sink {
		dist_sink();

		void add_sink(std::shared_ptr<sink>);
		void remove_sink(sink*);

		virtual void write(const message& msg) override;
	private:
		std::unique_ptr<struct _dist_state> _internal;
	};

	// append to a file, optionally truncating it first
	struct file_sink : public sink {
		file_sink(const char * filename, bool truncate = false);

		virtual void write(const message& msg) override;

		virtual void flush() override;

		~file_sink();
	private:
		std::unique_ptr<struct _file_state> _internal;
	};
	
	// write to the given std::ostream
	struct ostream_sink : public sink {
		ostream_sink(std::ostream&);
		virtual void write(const message& msg) override;
		virtual void flush() override;
	private:
		std::ostream& ostream;
	};

	// protect the given sink by locking it for writing / flushing
	struct mt_sink : public sink {
		mt_sink(std::shared_ptr<sink>);

		virtual void write(const message& msg) override;
	private:
		std::shared_ptr<sink> wrapped;
	};
};