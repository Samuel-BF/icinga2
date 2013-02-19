/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

boost::mutex StreamLogger::m_Mutex;

/**
 * Constructor for the StreamLogger class.
 */
StreamLogger::StreamLogger(void)
	: ILogger(), m_Stream(NULL), m_OwnsStream(false), m_Tty(false)
{ }

/**
 * Constructor for the StreamLogger class.
 *
 * @param stream The stream.
 */
StreamLogger::StreamLogger(ostream *stream)
	: ILogger(), m_Stream(stream), m_OwnsStream(false), m_Tty(IsTty(*stream))
{ }

/**
 * Destructor for the StreamLogger class.
 */
StreamLogger::~StreamLogger(void)
{
	if (m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::OpenFile(const String& filename)
{
	ofstream *stream = new ofstream();

	try {
		stream->open(filename.CStr(), fstream::out | fstream::trunc);

		if (!stream->good())
			BOOST_THROW_EXCEPTION(runtime_error("Could not open logfile '" + filename + "'"));
	} catch (...) {
		delete stream;
		throw;
	}

	m_Stream = stream;
	m_OwnsStream = true;
	m_Tty = false;
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param stream The output stream.
 * @param tty Whether the output stream is a TTY.
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(ostream& stream, bool tty, const LogEntry& entry)
{
	char timestamp[100];

	time_t ts = entry.Timestamp;
	tm tmnow = *localtime(&ts);

	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S %z", &tmnow);

	if (tty) {
		switch (entry.Severity) {
			case LogWarning:
				stream << "\x1b[1;33m"; // yellow;
				break;
			case LogCritical:
				stream << "\x1b[1;31m"; // red
				break;
			default:
				break;
		}
	}

	boost::mutex::scoped_lock lock(m_Mutex);
	stream << "[" << timestamp << "] "
		 << Logger::SeverityToString(entry.Severity) << "/" << entry.Facility << ": "
		 << entry.Message;

	if (tty)
		stream << "\x1b[0m"; // clear colors

	stream << std::endl;
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(const LogEntry& entry)
{
	ProcessLogEntry(*m_Stream, m_Tty, entry);
}

bool StreamLogger::IsTty(ostream& stream)
{
#ifndef _WIN32
	/* Eww... */
	if (stream == std::cout)
		return isatty(fileno(stdout));

	if (stream == std::cerr)
		return isatty(fileno(stderr));
#endif /*_ WIN32 */

	return false;
}
