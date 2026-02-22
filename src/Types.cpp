#include "Types.hpp"

CgiState::CgiState()
		: pid(-1),
			stdinFd(-1),
			stdoutFd(-1),
			connectionFd(-1),
			inputData(),
			inputWritten(0),
			outputData(),
			stdinClosed(false),
			stdoutClosed(false),
			active(false),
			pollCycles(0)
{
}
