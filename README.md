# smolog
Small logging library

- Not header only, compiles into only one TU
- Header only pulls in <memory> and <cstdarg>
- Format string based logging
- Compiler checks format string at compile time (for now via ugly macros)


```cpp
#include "smolog.hpp"

int main() {
	// create a logger called main
	smolog::logger main_logger("main");

	// when you log to a logger, it writes the formatted string to one or more sinks
	// you manipulate the sinks via add_sink and remove_sink
	main_logger.add_sink(std::make_shared<smolog::stdout_sink>());
	auto stderr_sink = std::make_shared<smolog::stderr_sink>();
	main_logger.add_sink(stderr_sink);
	
	// now we can write to the logger by calling either a function that specifies the level
	main_logger.debug("Hello Smolog!");

	// let's remove the stderr sink for less clutter
	main_logger.remove_sink(stderr_sink.get());

	// you can also specify the log level via log_at
	main_logger.log_at(smolog::Level::Error, "Oh-oh something went wrong :(");

	// these are *not* member function, even though they look like that
	// unfortunately some macro hacks are required to check the arguments you pass in
	// so code completion will not be your friend for this
	// (you can disable the macros via CMake)

	// arguments to the logging functions are standard format strings, passed on to snprintf
	main_logger.info("My friend %s is %d years old", "Martin", 24);
	
	// via the macros the compiler checks what you do
	// main_logger.critical("Floating away to %d", 4.2f); -> warning!

	// there are a number of sinks already implemented, but it is easy to add more
	// just implement write() and flush() (if needed)!

#ifdef _MSC_VER
	// for example in MSVC we can log to the debug output
	main_logger.add_sink(std::make_shared<smolog::msvc_sink>());
	main_logger.error("Good luck with smolog!")
#endif

	return 0;
}
```