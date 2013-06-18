/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CACHECIRCULAR_H
#define CACHECIRCULAR_H

#include "CacheStrategy.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/log.h"

namespace XFILE {

class CacheStats
{
public:
  CacheStats( int reportPeriod, void* pObj )
    : m_object ( pObj )
    , m_reportPeriod( reportPeriod )
  {
    Reset();
  }
  
  unsigned int GetElapsedTime(unsigned int start, unsigned int end)
  {
    if ( end>=start ) {
      return end-start;
    }
    return end+(unsigned int)0xFFFF-start;
  }
  
  void	Update( const char* eventDescription, size_t currentCacheSize )
  {
    if ( m_reportPeriod == 0 ) 
      return;
    
    unsigned int currentTime = XbmcThreads::SystemClockMillis();
    unsigned int elapsedTime = GetElapsedTime(m_lastEventTime,currentTime);
    
    m_lastEventTime = currentTime;
    if ( m_eventsInPeriod == 0 ) {
      m_longestIntervalInPeriod = elapsedTime;
      m_minSizeInPeriod = currentCacheSize;
      m_maxSizeInPeriod = currentCacheSize;
    } else {
      if ( elapsedTime > m_longestIntervalInPeriod ) m_longestIntervalInPeriod = elapsedTime;
      if ( currentCacheSize < m_minSizeInPeriod ) m_minSizeInPeriod = currentCacheSize;
      if ( currentCacheSize > m_maxSizeInPeriod ) m_maxSizeInPeriod = currentCacheSize;
    }
    m_totalSizeInPeriod += currentCacheSize;
    m_eventsInPeriod++;
    
    if ( currentTime - m_lastReportTime > m_reportPeriod && m_reportPeriod > 0 ) {
      Report(eventDescription, currentCacheSize);
      Reset();
    }
  }
  
  void  Report( const char* eventDescription, size_t currentCacheSize )
  {
    unsigned currentTime = XbmcThreads::SystemClockMillis();
    unsigned int elapsedTime = GetElapsedTime(m_lastReportTime,currentTime);
    
    CLog::Log(LOGERROR,"%s, obj=%p: elapsed=%d events=%d maxTime=%d avgTime=%d curSize=%"PRId64" minSize=%"PRId64" maxSize=%"PRId64" avgSize=%"PRId64 , 
	      eventDescription,
	      m_object,
	      elapsedTime,
	      m_eventsInPeriod,
	      m_longestIntervalInPeriod,
	      elapsedTime/m_eventsInPeriod,
	      currentCacheSize,
	      m_minSizeInPeriod,
	      m_maxSizeInPeriod,
	      m_totalSizeInPeriod/m_eventsInPeriod);
    m_lastReportTime = currentTime;
  }
  
  void  Reset()
  {
    m_eventsInPeriod = 0;
    m_lastEventTime = m_lastReportTime =  XbmcThreads::SystemClockMillis();
    m_longestIntervalInPeriod = 0;
    m_minSizeInPeriod = 0;
    m_maxSizeInPeriod = 0;
    m_totalSizeInPeriod = 0;
  }
  
private:
  void*	    m_object;
  unsigned int m_reportPeriod;
  unsigned int m_eventsInPeriod;
  unsigned int m_lastReportTime;
  unsigned int m_lastEventTime;
  unsigned int m_longestIntervalInPeriod;
  size_t    m_minSizeInPeriod;
  size_t    m_maxSizeInPeriod;
  size_t    m_totalSizeInPeriod;
};
  
class CCircularCache : public CCacheStrategy
{
public:
    CCircularCache(size_t front, size_t back, int cacheReportPeriod=0);
    virtual ~CCircularCache();

    virtual int Open() ;
    virtual void Close();

    virtual int WriteToCache(const char *buf, size_t len) ;
    virtual int ReadFromCache(char *buf, size_t len) ;
    virtual int64_t WaitForData(unsigned int minimum, unsigned int iMillis) ;

    virtual int64_t Seek(int64_t pos) ;
    virtual void Reset(int64_t pos) ;

protected:
    uint64_t          m_beg;       /**< index in file (not buffer) of beginning of valid data */
    uint64_t          m_end;       /**< index in file (not buffer) of end of valid data */
    uint64_t          m_cur;       /**< current reading index in file */
    uint8_t          *m_buf;       /**< buffer holding data */
    size_t            m_size;      /**< size of data buffer used (m_buf) */
    size_t            m_size_back; /**< guaranteed size of back buffer (actual size can be smaller, or larger if front buffer doesn't need it) */
    CCriticalSection  m_sync;
    CEvent            m_written;
#ifdef _WIN32
    HANDLE            m_handle;
#endif
    int               m_cacheReportPeriod;
    CacheStats	      m_readStats;
    CacheStats	      m_writeStats;
};

} // namespace XFILE
#endif
