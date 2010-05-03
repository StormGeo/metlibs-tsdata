/*
 * WdbStream.h
 *
 *  Created on: Feb 26, 2010
 *      Author: juergens
 */

#ifndef WDBSTREAM_H_
#define WDBSTREAM_H_

/*
 $Id$

 Copyright (C) 2006 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This file is part of generated by met.no

 This is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Tseries; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string>
#include <set>
#include <vector>
#include <map>
#include <puTools/miTime.h>

#include <pqxx/pqxx>

#include "WdbQueries.h"
#include "ptDataStream.h"
#include "DynamicFunction.h"
#include <diField/diProjection.h>

// the pets (tseries) wdb-connection

namespace pets{



class WdbStream : public DataStream {

public:
    struct BoundaryBox {
      int minLat;
      int maxLat;
      int minLon;
      int maxLon;
      BoundaryBox() : minLat(-90) , maxLat(90), minLon(-180), maxLon(180) {}
      void setMinMax() { minLat = 1000; maxLat = -1000; minLon = 1000; maxLon=-1000; }
    };


  private:

private:
  struct transformIdx {
    std::string wdbName;
    pets::math::DynamicFunction * transform;
    transformIdx() : transform(NULL) {}
    ~transformIdx() { if ( transform ) delete transform; }
  };

  struct rotateParameters { std::string x; std::string y; };

  struct DataFromWdb {
    std::string petsName;
    std::vector<float>          data;
    std::vector<miutil::miTime> times;
    std::map<miutil::miTime,float> rawdata;

    pets::math::DynamicFunction * transform;
    DataFromWdb() : transform(NULL) {}
    ~DataFromWdb() {transform=NULL; }

    void adaptTimelines( DataFromWdb& rhs    );
    std::vector<miutil::miTime> adaptSingle(std::vector<miutil::miTime> ntim);
    void setData(miutil::miTime tim, float dat);
    void sortData();

  };

private:
  Projection               currentGrid;
  Projection               geoGrid;

  vector<rotateParameters> rot;
  WdbStream::BoundaryBox   boundaries;
  std::set<std::string>    dataProviders;
  std::string              currentProvider;
  std::set<miutil::miTime> referenceTimes;
  std::set<std::string>    parametersFound;
  miutil::miTime           currentReferenceTime;
  std::set<int>            levels;
  int                      currentLevel;
  std::string              currentGridName;
  std::map<std::string,transformIdx>  transformIndex;

  pqxx::connection wdb;
  std::string      user;
  std::vector<std::string>   wdbNames;
  void setRotateToGeo(std::vector<std::string>);
  void rotate2geo(float x,float y, std::vector<float>& xdata ,std::vector<float>& ydata);

public:
  WdbStream(std::string host,std::map<std::string,std::string> pars,std::vector<std::string> vfunctions,
      std::string u="proffread");
   ~WdbStream();

    /// Implemented from DataStream interface -------------------------------

    bool openStream(ErrorFlag*);

    bool writeData(const int posIndex, const int modIndex, ErrorFlag*, bool complete_write, bool write_submodel);    // unimplemented
    bool getModelSeq(int, Model&, Run&, int&, vector<miString>&);                              // unimplemented
    bool readData(const int posIndex, const ParId&, const miTime&, const miTime&, ErrorFlag*); // unimplemented
    bool getTimeLine(const int& index, vector<miTime>& tline, vector<int>& pline, ErrorFlag*); // unimplemented
    bool putTimeLine(const int& index, vector<miTime>& tline, vector<int>& pline, ErrorFlag*); // unimplemented
    bool putTimeLine(TimeLine& tl, vector<int>& pline, ErrorFlag*);                            // unimplemented
    int findStation(const miString&);                      // unimplemented
    int findDataPar(const ParId&);                         // unimplemented
    void clean();                                          // unimplemented
    void cleanParData();                                   // unimplemented
    bool openStreamForWrite(ErrorFlag*);                   // unimplemented
    bool getOnePar(int, WeatherParameter&, ErrorFlag*);    // unimplemented
    bool putOnePar(WeatherParameter&, ErrorFlag*);         // unimplemented
    bool getStations(vector<miPosition>&);                 // unimplemented
    bool getStationSeq(int, miPosition&);                  // unimplemented
    bool getModelSeq(int, Model&, Run&, int&);             // unimplemented
    int  putStation(const miPosition& s, ErrorFlag*);      // unimplemented
    void getTextLines(const ParId p, vector<miString>& tl);// unimplemented
    bool close();                                          // unimplemented


  /// WDB functions -------------------------------------------------
  /// dataProviders
  bool                  setDataProviders();
  std::set<std::string> getDataProviders() const  {return dataProviders;}

  bool        setCurrentProvider(std::string currentProviderName);
  std::string getCurrentProvider() const { return currentProvider;}

  /// referenceTimes
  bool                     setReferenceTimes();
  std::set<miutil::miTime> getReferenceTimes() const {return referenceTimes;}

  bool setCurrentReferenceTime(miutil::miTime newReferenceTime);

  // levels -- not implemented yet (missing functionality in wdb 0.9.6)
  bool setLevels();

  // get all parameters for provider/referencetime
  bool setParameters();

  // get a boundary box to control the sliders in tseries
  WdbStream::BoundaryBox getGeometry();

  // get the names of all valid parameters of the last query
  std::vector<std::string> getWdbParameterNames() const { return wdbNames; }

  bool readWdbData(float lat, float lon,miString model, const miTime& run,std::vector<ParId>& inpar,
      std::vector<ParId>& outpar, unsigned long& readtime);
  bool getOnePar(int, WeatherParameter&);
  bool getTimeLine(const int& index, vector<miTime>& tline, vector<int>& pline);
};
}

#endif /* WDBSTREAM_H_ */
