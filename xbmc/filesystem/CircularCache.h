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

namespace XFILE {

class CacheStats
{
public:
  CacheStats( int reportPeriod, void* pObj )
    : m_reportPeriod( reportPeriod )
    , m_object ( pObj )
  {
    Reset();
  }
  
  void	Update( const char* eventDescription, int64_t currentCacheSize )
  {
    if ( m_reportPeriod == 0 ) 
      return;
    
    unsigned currentTime = XbmcThreads::SystemClockMillis();
    unsigned elapsedTime = currentTime - m_lastEventTime;
    
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
      Report(eventDescription);
      Reset();
    }
  }
  
  void  Report( const char* eventDescription )
  {
    unsigned currentTime = XbmcThreads::SystemClockMillis();
    
    CLog::Log(LOGERROR,"%s, obj=%p: maxTime=%"PRId64" avgTime=%"PRId64" minSize=%"PRId64" maxSize=%"PRId64" avgSize=%"PRId64 , 
	      eventDescription,
	      m_object,
	      m_longestIntervalInPeriod,
	      (currentTime-m_lastReportTime)/m_eventsInPeriod,
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
  int 	    m_reportPeriod;
  int       m_eventsInPeriod;
  unsigned  m_lastReportTime;
  unsigned  m_lastEventTime;
  int64_t   m_longestIntervalInPeriod;
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
