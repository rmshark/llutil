/*
 * Copyright (C) 2015 Rishabh Mukherjee
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the 
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see http://www.gnu.org/licenses/.
 */


#ifndef _LLUTIL_LOG_H_
#define _LLUTIL_LOG_H_

#define LOG(level,...) Logger::GetInstance().Log(LOG_LEVEL::level,__VA_ARGS__)

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <thread>
#include <utility>

#include "concurrent/mpsc_atomic.h"

static const int LOG_MESSAGE_SIZE = 512;

enum class LOG_LEVEL : int32_t
{
  TRACE = 0,
  DEBUG = 1,
  INFO  = 2,
  ERROR = 3,
  FATAL = 4,
};

class LogMessage
{
  private:
  timespec _timestamp;
  char _message[LOG_MESSAGE_SIZE];
  LOG_LEVEL _log_level;

  public:
  template<class ...Args>
  LogMessage(LOG_LEVEL log_level, Args&&... args)
  {
    _log_level = log_level;
    snprintf(_message, LOG_MESSAGE_SIZE, std::forward<Args>(args)...);
    clock_gettime(CLOCK_REALTIME,&_timestamp);
  };
  LogMessage(LOG_LEVEL log_level)
  {
    clock_gettime(CLOCK_REALTIME,&_timestamp);
  }
  LogMessage()
  {
    clock_gettime(CLOCK_REALTIME,&_timestamp);
  }
  ~LogMessage()
  {
    memset(_message,'\0',LOG_MESSAGE_SIZE);
    _timestamp.tv_sec=0;
    _timestamp.tv_nsec=0;
  };
};

class Logger
{
  private:
  std::string _log_dir;
  std::string _log_prefix;
  std::string _log_file;

  LOG_LEVEL   _log_level;

  MPSCAtomic<LogMessage>* _queue;

  void background_thread()
  {
    return;
  };
  
  std::thread _background_thread;
  bool _background_thread_is_running;
  
  Logger(void):_log_dir("./"),_log_prefix("log"),_log_level(LOG_LEVEL::INFO)
  {
    _queue = new MPSCAtomic<LogMessage>(1000);
    _background_thread_is_running = true;
  };

  Logger& operator=(const Logger&) =  delete;
  Logger(const Logger& ) = delete;

  static std::unique_ptr<Logger> _instance;
  static std::once_flag _once_flag;

  public:
  static Logger& GetInstance(void)
  {
    std::call_once(_once_flag, []{_instance.reset(new Logger);});
    return *_instance.get();
  };
  
  void SetLogFileDir(std::string& log_dir){_log_dir = log_dir;};
  void SetLogFilePrefix(std::string& log_prefix){_log_prefix = log_prefix;};
  void SetLogLevel(LOG_LEVEL log_level){_log_level = log_level;};

  template<class ...Args>
  void Log(LOG_LEVEL log_level, Args&& ... args)
  {
    _queue->enqueue(log_level, std::forward<Args>(args)...);
  };

  ~Logger()
  {
    delete _queue;
  };
};

std::unique_ptr<Logger> Logger::_instance;
std::once_flag Logger::_once_flag;

#endif//_LLUTIL_LOG_H_
