/*
 * Copyright (C) 2005 Tommi Maekitalo
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * As a special exception, you may use this file as part of a free
 * software library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU Library
 * General Public License.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <tntdb/connect.h>
#include <tntdb/connection.h>
#include <tntdb/error.h>
#include <tntdb/librarymanager.h>
#include <tntdb/iface/iconnectionmanager.h>
#include <tntdb/connectionpool.h>
#include <cxxtools/log.h>
#include <map>
#include "config.h"

#ifdef WITH_STATIC_POSTGRESQL
# include <tntdb/postgresql/impl/connectionmanager.h>
#endif

#ifdef WITH_STATIC_MYSQL
# include <tntdb/mysql/impl/connectionmanager.h>
#endif

#ifdef WITH_STATIC_SQLITE
# include <tntdb/sqlite/impl/connectionmanager.h>
#endif

#ifdef WITH_STATIC_ORACLE
# include <tntdb/oracle/connectionmanager.h>
#endif

log_define("tntdb.connect")

namespace tntdb
{
  typedef std::map<std::string, LibraryManager> librariesType;
  static librariesType libraries;
  static cxxtools::Mutex mutex;

  Connection connect(const std::string& url)
  {
    log_debug("connect(\"" << url << "\")");

    cxxtools::MutexLock lock(mutex);

    std::string::size_type n = url.find(':');
    if (n == std::string::npos)
#ifdef WITH_STATIC_POSTGRESQL
      return connectionManager_postgresql.connect(url);
#elif WITH_STATIC_MYSQL
      return connectionManager_mysql.connect(url);
#elif WITH_STATIC_SQLITE
      return connectionManager_sqlite.connect(url);
#elif WITH_STATIC_ORACLE
      return connectionManager_oracle.connect(url);
#else
      throw Error("invalid dburl \"" + url + '"');
#endif

    std::string driverName =  url.substr(0, n);
#ifdef WITH_STATIC_POSTGRESQL
    if (driverName == "postgresql")
      return connectionManager_postgresql.connect(url.substr(n + 1));
#elif WITH_STATIC_MYSQL
    if (driverName == "mysql")
      return connectionManager_mysql.connect(url.substr(n + 1));
#elif WITH_STATIC_SQLITE
    if (driverName == "sqlite")
      return connectionManager_sqlite.connect(url.substr(n + 1));
#elif WITH_STATIC_ORACLE
    if (driverName == "oracle")
      return connectionManager_oracle.connect(url.substr(n + 1));
#endif

    std::string libraryUrl = url.substr(n + 1);
    log_debug("driver \"" << driverName << "\" url=\"" << libraryUrl << '"');

    // lookup library-manager
    LibraryManager libraryManager;
    librariesType::const_iterator it = libraries.find(driverName);
    if (it == libraries.end())
    {
      libraryManager = LibraryManager(driverName);
      libraries[driverName] = libraryManager;
    }
    else
      libraryManager = it->second;

    return libraryManager.connect(libraryUrl);
  }

  static ConnectionPools connectionPools;

  Connection connectCached(const std::string& url)
  {
    log_debug("connectCached(\"" << url << "\")");
    return connectionPools.connect(url);
  }

  void dropCached(unsigned keep)
  {
    connectionPools.drop(keep);
  }

  void dropCached(const std::string& url, unsigned keep)
  {
    connectionPools.drop(url, keep);
  }

  void setMaxPoolSize(unsigned max)
  {
    connectionPools.setMaximumSize(max);
  }

  unsigned getMaxPoolSize()
  {
    return connectionPools.getMaximumSize();
  }
}
