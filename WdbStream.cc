/*
 * WdbStream.cpp
 *
 *  Created on: Feb 26, 2010
 *      Author: juergens
 */

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

#include "WdbStream.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <puTools/miString.h>

using namespace std;
using namespace miutil;

namespace pets
{


void  WdbStream::DataFromWdb::setData(miTime tim, float dat)
{
  double dd=double(dat);
  if(transform ) transform->calc(dd);
  rawdata[tim] = float(dd);
}

void WdbStream::DataFromWdb::sortData()
{
  data.clear();
  times.clear();
  map<miTime,float>::iterator itr=rawdata.begin();
  for(;itr!=rawdata.end();itr++) {
    data.push_back(itr->second);
    times.push_back(itr->first);
  }
}

void WdbStream::DataFromWdb::adaptTimelines( WdbStream::DataFromWdb& rhs    )
{
  vector<miTime> s= adaptSingle( rhs.times);
  sortData();
  rhs.adaptSingle(times);
  rhs.sortData();
}

vector<miTime> WdbStream::DataFromWdb::adaptSingle(vector<miTime> ntim)
{
  vector<miTime> res;
  map<miTime,float> buf;
  for(int i=0;i<ntim.size();i++) {
    if(rawdata.count(ntim[i])) {
      buf[ntim[i]] = rawdata[ntim[i]];
      res.push_back(ntim[i]);
    }
  }
  rawdata=buf;
  return res;
}



WdbStream::WdbStream(std::string host, map<string, string> parlist, vector<string> vectorFunctionList,
    std::string u) :
    wdb( QUERY::CONNECT(host,u) ) , user(u), DataStream("WdbStream")
{
  geoGrid.setGeographic();


  map<string,string>::iterator itr=parlist.begin();

  for(;itr!=parlist.end();itr++){

    string key   = itr->first;
    string token = itr->second;

    if(token.empty()) continue;

    if(transformIndex.count(key)) {
      cerr << "TransformIndex for " << key << " Already exists - ignored" << endl;
      continue;
    }

    vector<string> tokens;
    boost::split(tokens,token, boost::algorithm::is_any_of(":") );

    transformIdx trans;
    transformIndex[key]         = trans;
    transformIndex[key].wdbName = tokens[0];
    if(tokens.size() > 1)
      transformIndex[key].transform = new pets::math::DynamicFunction(tokens[1]);
  }

  setRotateToGeo(vectorFunctionList);

  setDataProviders();

}


WdbStream::~WdbStream()
{

}

bool WdbStream::setDataProviders()
{
  dataProviders.clear();
  pqxx::work query(wdb,"getDataProvider");
  query.exec(QUERY::BEGIN(user));
  string providername;

  pqxx::result res = query.exec(QUERY::BROWSE("NULL::wci.browsedataprovider" ));

  for (pqxx::result::size_type i = 0; i != res.size(); ++i) {
    res.at(i).at(0).to(   providername );
    dataProviders.insert( providername );
  }
  return !dataProviders.empty();
}

bool WdbStream::setCurrentProvider(string currentProviderName)
{
  if(!dataProviders.count(currentProviderName))
    return false;

  currentProvider = currentProviderName;
  setReferenceTimes();
}

WdbStream::BoundaryBox WdbStream::getGeometry()
{
  pqxx::work query(wdb,"getGrid");
  query.exec(QUERY::BEGIN(user));
  string gridname;

  pqxx::result gridres = query.exec(QUERY::GRIDNAME(currentProvider));

  for (pqxx::result::size_type i = 0; i != gridres.size(); ++i) {
    gridres.at(i).at(0).to(   gridname );
  }

  if(gridname == currentGridName) return boundaries;
  currentGridName = gridname;


  query.exec(QUERY::BEGIN(user));
  string geoname;

  pqxx::result geores = query.exec(QUERY::GEOMETRY(currentGridName));

  for (pqxx::result::size_type i = 0; i != geores.size(); ++i) {
    geores.at(i).at(0).to(   geoname );
  }


  if( !geoname.empty()) {
    boost::replace_all(geoname,"POLYGON((","");
    boost::replace_all(geoname,"))","");
    vector<string> pos;
    boost::split(pos,geoname,boost::algorithm::is_any_of(","));
    boundaries.setMinMax();
    for(int i=0;i<pos.size();i++) {
      vector<string> loc;
      boost::split(loc,pos[i],boost::algorithm::is_any_of(" "));
      if(loc.size() < 2) continue;
      int x = atoi(loc[0].c_str());
      int y = atoi(loc[1].c_str());

      if(x < boundaries.minLon)
        boundaries.minLon = x;
      else if ( x+1 >  boundaries.maxLon)
        boundaries.maxLon = x+1;

      if(y < boundaries.minLat)
        boundaries.minLat = y;
      else if ( y+1 >  boundaries.maxLat)
        boundaries.maxLat = y+1;
    }
  }

  string newProjString;
  pqxx::result projres = query.exec(QUERY::PROJECTION(currentGridName));


  for (pqxx::result::size_type i = 0; i != projres.size(); ++i)
      projres.at(i).at(0).to( newProjString );


  cout << "Using proj definition:  [ " << newProjString << " ] " <<  endl;

  currentGrid.set_proj_definition(newProjString,1.0,1.0);

  return boundaries;

}


void WdbStream::rotate2geo(float x,float y, std::vector<float>& xdata ,std::vector<float>& ydata)
{
  if(xdata.size() != ydata.size()) return;

  for(int i=0;i<xdata.size();i++) {

    geoGrid.convertVectors(currentGrid,1, &x, &y, &xdata[i], &ydata[i]);
  }
}



bool WdbStream::setReferenceTimes()
{
  if(!dataProviders.count(currentProvider))
    return false;

  referenceTimes.clear();
  pqxx::work query(wdb,"getReferencetime");
  query.exec(QUERY::BEGIN(user));
  string reftime;

  pqxx::result res = query.exec(QUERY::REFERENCETIMES(currentProvider));

  for (pqxx::result::size_type i = 0; i != res.size(); ++i) {
    res.at(i).at(0).to( reftime );
    if(!reftime.empty())
      referenceTimes.insert(miTime(reftime));
   }

  if(referenceTimes.empty()) return false;


  query.commit();
  if(!setCurrentReferenceTime(currentReferenceTime))
    setCurrentReferenceTime(*referenceTimes.begin());

  return true;
}

bool WdbStream::setCurrentReferenceTime(miutil::miTime newReferenceTime)
{
  if(currentReferenceTime == newReferenceTime)
    return true;

  if(!referenceTimes.count(newReferenceTime))
    return false;

  currentReferenceTime = newReferenceTime;
//  setLevels();
  setParameters();
}

bool WdbStream::setLevels()
 {
   // TODO: setLevels is not good enough implemented in  WDB
   // WDB gives only min and max level, but we need any level at a time and model
   // to readjust the gui... A bug has been notified to the wdb team to solve this problem
   // until then the level is skipped.

   if(!dataProviders.count(currentProvider))       return false;
   if(!referenceTimes.count(currentReferenceTime)) return false;
     levels.clear();
     pqxx::work query(wdb,"getLevel");
     query.exec(QUERY::BEGIN(user));
     int lvl;

     pqxx::result res = query.exec(QUERY::LEVELS(currentProvider,currentReferenceTime));

     for (pqxx::result::size_type i = 0; i != res.size(); ++i) {
       res.at(i).at(0).to( lvl );
       levels.insert(lvl);
      }

     if(referenceTimes.empty()) return false;

     if(!setCurrentReferenceTime(currentReferenceTime))
       setCurrentReferenceTime(*referenceTimes.begin());

     return true;

 }

bool  WdbStream::setParameters()
{
  pqxx::work query(wdb,"getDataParameterIndex");
  query.exec(QUERY::BEGIN(user));
  string parametername;
  parametersFound.clear();

  pqxx::result res = query.exec(QUERY::PARAMETERS(currentProvider,currentReferenceTime));

  for (pqxx::result::size_type i = 0; i != res.size(); ++i) {
       res.at(i).at(0).to(   parametername );
       parametersFound.insert( parametername );
  }

  return !parametersFound.empty();

}

void WdbStream::setRotateToGeo(vector<std::string> str)
{
  rot.clear();

  for(int i=0; i< str.size();i++) {
    vector<string> xy;
    boost::split(xy,str[i], boost::algorithm::is_any_of(",") );
    if(xy.size() < 2 ) {
      cerr << "parse error : invalid pair of x,y in vector definition in setup file " << str[i] << endl;
      continue;
    }

    if(!transformIndex.count(xy[0])) {
      cerr << "parse error : unknown Parameter x= " << xy[0] << " in vector definition in setup file " << endl;
      continue;
    }
    if(!transformIndex.count(xy[1])) {
      cerr << "parse error : unknown Parameter y= " << xy[1] << " in vector definition in setup file " << endl;
      continue;
    }

    rotateParameters rotpar;
    rotpar.x=transformIndex[xy[0]].wdbName;
    rotpar.y=transformIndex[xy[1]].wdbName;

    rot.push_back(rotpar);
  }
}




bool WdbStream::openStream(ErrorFlag* error)
{
  try {
    setDataProviders();
  } catch (exception& e) {
    cerr << "Exception while opening WdbStream: " << e.what() <<endl;
    *error = DF_FILE_OPEN_ERROR;
    return false;
  }

  *error=OK;
  return true;
}


bool WdbStream::readWdbData(float lat, float lon,miString model, const miTime& run,vector<ParId>& inpar,vector<ParId>& outpar,
    unsigned long& readtime)
{
  readtime=0;
  if(!setCurrentReferenceTime(run)) return false;

  pets::math::DynamicFunction * transform=NULL;
  DataFromWdb dwdb;
  wdbNames.clear();
  map<string, DataFromWdb> datafromWdb;
  // check the parameterlist - what to get and what not....
  for(int i=0;i<inpar.size();i++) {

    string  petsName = inpar[i].alias;

    if(!transformIndex.count(petsName)) {
      // we dont know what this is in WDB - skipping
      ParId outId=inpar[i];

      outId.run   = run.hour();
      outId.level = 0; // < temporary
      outId.model = model;
      outpar.push_back(outId);
      continue;
    }


    string wdbName =  transformIndex[petsName].wdbName;

    if(!parametersFound.count(wdbName)) {
      // we know the parameter, but we cannot find it in wdb - skipping
      outpar.push_back(inpar[i]);
      continue;
    }

    datafromWdb[wdbName]=dwdb;
    datafromWdb[wdbName].transform=transformIndex[petsName].transform;
    datafromWdb[wdbName].petsName = petsName;

    wdbNames.push_back(wdbName);
  }

  // get the data .....

  pqxx::work query(wdb,"getTimeseries");
  query.exec(QUERY::BEGIN(user));


  string querystring = QUERY::TIMESERIES(currentProvider,currentReferenceTime,wdbNames,lat,lon,"NULL");

  try {




    boost::posix_time::ptime before  = boost::posix_time::microsec_clock::universal_time();
    pqxx::result   res = query.exec(querystring);

    boost::posix_time::ptime after  = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration duration = after-before;
    readtime = duration.total_milliseconds();



  // fetch the wdb result ....
  for (pqxx::result::size_type ii = 0; ii != res.size(); ++ii) {
    double value;
    string validstr;
    string parname;
    res.at(ii).at(0).to( value );
    res.at(ii).at(6).to( validstr );
    res.at(ii).at(8).to( parname );

    miTime valid(validstr.c_str());

    datafromWdb[parname].setData(valid,value);
  }
  } catch (exception& e) {
    cerr <<  endl << "No Data " << e.what() << endl;
    return false;
  }

  map<string, DataFromWdb>::iterator itr= datafromWdb.begin();
  for(;itr!=datafromWdb.end();itr++)
    itr->second.sortData();

  // rotate vectors from the current grid to geographic
  if(rot.size()) {

    float x_geoproj = lon * DEG_TO_RAD;
    float y_geoproj = lat * DEG_TO_RAD;

    for ( int k=0; k< rot.size();k++)
      if(datafromWdb.count(rot[k].x) && datafromWdb.count(rot[k].y )) {
        datafromWdb[rot[k].x].adaptTimelines(datafromWdb[rot[k].y]);
        rotate2geo(x_geoproj,y_geoproj, datafromWdb[rot[k].x].data ,datafromWdb[rot[k].y].data);
      }
  }


  // from here its the pets world again ....

  itr=datafromWdb.begin();


  for(;itr!=datafromWdb.end();itr++) {
    WeatherParameter wp;

    ParId pid = ID_UNDEF;
    pid.model = model;
    pid.run   = run.hour();

    //timeLine= dataList[didx].times;
    TimeLineIsRead = true;


    wp.setDims(itr->second.times.size(),1);
    int ipar= parameters.size();
    int tlindex;
    parameters.push_back(wp); // add it to the vector
    for (int j=0; j< itr->second.times.size(); j++) {
      parameters[ipar].setData(j,0,itr->second.data[j]);
    }

    if ((tlindex=timeLines.Exist(itr->second.times))==-1){
      tlindex=numTimeLines;
      timeLines.insert(itr->second.times,tlindex);
      numTimeLines++;
    }

    pid.alias = itr->second.petsName;
    pid.level = 0; // < temporary



    parameters[ipar].setTimeLineIndex(tlindex);
    parameters[ipar].setId(pid);
    parameters[ipar].calcAllProperties();


    transform=NULL;
  }

  npar= parameters.size();
  DataIsRead = true;
  return true;
}



bool WdbStream::getOnePar(int i, WeatherParameter& wp)
{
  if(i>=0 && i<parameters.size()) {
    wp=parameters[i];
    return true;
  }
  return false;
}

bool WdbStream::getTimeLine(const int& index, vector<miTime>& tline, vector<int>& pline)
{
  if (TimeLineIsRead && timeLines.Timeline(index,tline)) {
    if (index<progLines.size())
      pline = progLines[index];
     return true;
  }
  return false;
}

void WdbStream::clean()
{
  parameters.clear();
  npar= 0;
  timeLine.clear();
  timeLines.clear();
  numTimeLines=0;
  progLine.clear();
  progLines.clear();
  DataIsRead = false;
  TimeLineIsRead = false;
  IsCleaned = true;
}










//
//
// ------------   unimplemented---------------------------
//
//





static const string unimplemented(string func)
{
  cerr << "unimplemented function " << func << " called in WdbStream " << endl;
}






int WdbStream::findStation(const miString& s)
{
  unimplemented("findStation");
  return 0;
}

int WdbStream::findDataPar(const ParId& p)
{
  unimplemented("findDataPar");
  return 0;
}

void WdbStream::cleanParData()
{
  DataStream::cleanParData();
  unimplemented("cleanParData");
}

bool WdbStream::openStreamForWrite(ErrorFlag* e)
{
  unimplemented("openStreamForWrite");
  return false;
}

bool WdbStream::readData(const int posIndex, const ParId&,
    const miTime& p, const miTime& t, ErrorFlag* e)
{
  unimplemented("readData");
  return true;
}


bool WdbStream::getTimeLine(const int& index, vector<miTime>& tline,
      vector<int>& pline, ErrorFlag* e)
{
  unimplemented("getTimeLine");
  return true;
}

bool WdbStream::putTimeLine(const int& index, vector<miTime>& tline, vector<int>& pline, ErrorFlag* e)
{
  unimplemented("putTimeLine 1");
  return false;
}

bool WdbStream::putTimeLine(TimeLine& tl, vector<int>& pline, ErrorFlag* e)
{
  unimplemented("putTimeLine 2");
  return false;
}



bool WdbStream::getOnePar(int i, WeatherParameter& w, ErrorFlag* e)
{
  unimplemented("getOnePar");
  return true;
}

bool WdbStream::putOnePar(WeatherParameter& w, ErrorFlag* e)
{
  unimplemented("putOnePar");
  return false;
}

bool WdbStream::getStations(vector<miPosition>& s)
{
  unimplemented("getStations");
  return false;
}


bool WdbStream::getStationSeq(int i, miPosition& p)
{
  unimplemented("getStationSeq");
  return true;
}


bool WdbStream::getModelSeq(int i, Model& m, Run& r, int& ii)
{
  unimplemented("getModelSeq 1");
  return true;
}


bool WdbStream::getModelSeq(int i, Model& m, Run& r, int& ii, vector<miString>& n)
{
  unimplemented("getModelSeq 2");
  return true;
}

int  WdbStream::putStation(const miPosition& s, ErrorFlag* e)
{
  unimplemented("putStation");
  return 0;
}


bool WdbStream::writeData(const int posIndex, const int modIndex, ErrorFlag* e, bool complete_write, bool write_submodel)
{
  unimplemented("writeData");
  return false;
}

bool WdbStream::close()
{
  unimplemented("close");
  return true;
}

void WdbStream::getTextLines(const ParId p, vector<miString>& tl)
{
  unimplemented("getTextLines");
  tl = textLines;
}




}
