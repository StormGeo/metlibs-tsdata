/*
 * FimexStream.cc
 *
 *  Created on: Aug 2, 2013
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


#include "FimexStream.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <puTools/miString.h>

#include <fimex/CDMReaderUtils.h>
#include <fimex/CDM.h>
#include <fimex/Utils.h>

#include <boost/shared_array.hpp>
#include "fimex/Data.h"

#include <boost/interprocess/anonymous_shared_memory.hpp>
//#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <unistd.h>
#include <wait.h>

#include <numeric>
#include <functional>


using namespace std;
using namespace miutil;

namespace pets
{
FimexPoslist FimexStream::commonposlist;
int          FimexStream::commonposlistVersion=0;
int          FimexStream::progress=100;
std::string  FimexStream::progressMessage="";
std::set<std::string> FimexStream::parameterFilter;
std::vector<pets::FimexParameter> FimexStream::fimexpar;
std::vector<std::string> FimexStream::allParameters;


int  FimexStream::getProgress()
{
  return progress;
}

std::string FimexStream::getProgressMessage()
{
  return progressMessage;
}

std::set<std::string> FimexStream::getParameterFilter()
{
  return parameterFilter;
}

bool FimexStream::isFiltered(std::string petsname)
{
  return bool(parameterFilter.count(petsname));
}



void FimexStream::setParameterFilter(std::set<std::string> pfilter)
{
  parameterFilter=pfilter;
}


void FimexStream::setFimexParameters(std::vector<pets::FimexParameter> par)
{
  fimexpar=par;
  vector<string> apar;
  for(int i=0;i<fimexpar.size();i++)
    apar.push_back(fimexpar[i].parid.alias);
  allParameters=apar;
}

void FimexStream::addToAllParameters(std::vector<std::string> newpar)
{
  allParameters.insert(allParameters.end(),newpar.begin(),newpar.end());
}

std::vector<std::string> FimexStream::getAllParameters()
{
  return allParameters;
}



std::string FimexStream::getParameterFilterAsString()
{
  ostringstream ost;
  set<string>::iterator itr = parameterFilter.begin();
  string delimiter = "";
  for (; itr != parameterFilter.end(); itr++) {
    ost << delimiter << *itr;
    delimiter = ":";
  }
  return ost.str();

}

void FimexStream::setParameterFilterFromString(std::string blist)
{
  set<string> tokens;
  boost::split(tokens, blist, boost::algorithm::is_any_of(":"));
  parameterFilter = tokens;
}




void FimexStream::setCommonPoslist(const FimexPoslist& cposlist)
{
  commonposlist = cposlist;
  commonposlistVersion++;
}

void FimexStream::setCommonPoslistFromStringlist(std::vector<std::string> newposlist)
{
  FimexPoslist newFimexposlist;

  for(unsigned int i=0;i<newposlist.size();i++)
    newFimexposlist.addEntry(newposlist[i]);

  setCommonPoslist(newFimexposlist);

}



void FimexStream::setPositions()
{
  poslist = commonposlist;
  poslistVersion = commonposlistVersion;
}



FimexStream::FimexStream(const std::string& fname,
    const std::string& modname,
    const std::string& ftype,
    const std::string cfile)
:  filename(fname) , modelname(modname), progtime(0), is_open(false), increment(0),
   configfile(cfile)
{

  vector<string> typetokens;
  boost::split(typetokens, ftype, boost::algorithm::is_any_of(":"));


  filetype      =  ( typetokens.size() > 0 ? boost::trim_copy(typetokens[0]) : ftype   );
  parametertype =  ( typetokens.size() > 1 ? boost::trim_copy(typetokens[1]) : filetype);

  timeLineIsRead=false;
  poslist = commonposlist;
  poslistVersion = commonposlistVersion;

}


FimexStream::~FimexStream()
{

}

void FimexStream::clean()
{

}


bool FimexStream::createPoslistInterpolator()
{
  try {
    if(!reader)
      openStream();

    interpol.reset( new MetNoFimex::CDMInterpolator (reader));
    interpol->changeProjection(MIFI_INTERPOL_BILINEAR,poslist.getLon(), poslist.getLat() );

  } catch (exception& e) {
    cerr << e.what() << endl;
    return false;
  }

  return true;
}





void FimexStream::createTimeLine()
{
  basetimeline.clear();


  try {
    if(!interpol)
      createPoslistInterpolator();


    if(!is_open)
      return;

    // get unlimited axis as time axis. NB! this is true for all known models
    // at this time, but could be changed later!!!
    // this makes only sense if all parameters share the same time axis, this is
    // also true now (2013) ...

    std::string timeAxis="time";

    const MetNoFimex::CDMDimension* tmpDim = interpol->getCDM().getUnlimitedDim();
    if(tmpDim)
      timeAxis=tmpDim->getName();



    MetNoFimex::DataPtr timeData = interpol->getScaledDataInUnit(timeAxis,"seconds since 1970-01-01 00:00:00 +00:00");
    boost::shared_array<unsigned long long> uTimes = timeData->asUInt64();

    for(size_t u = 0; u < timeData->size(); ++u) {
      basetimeline.push_back( miutil::miTime(uTimes[u]) );
    }

  } catch (exception& e) {
    cerr << "Exception catched in createTimeline: " << e.what() << endl;
  }

  timeLineIsRead=true;
}



void FimexStream::openStream()
{
  try {
    reader = MetNoFimex::CDMFileReaderFactory::create(filetype,filename,configfile);
    cerr << "Stream " << filename << " opened " << endl;
    is_open=true;
  } catch (exception& e) {
    throw FimexstreamException("Could not open fimexstream");
  }
}


boost::posix_time::ptime FimexStream::getReferencetime()
{

  if(!reader)
    openStream();

  return MetNoFimex::getUniqueForecastReferenceTime(reader);

}

void FimexStream::filterParameters(vector<ParId>& inpar)
{
  vector<ParId> tmp = inpar;
  inpar.clear();
  for(int i=0;i<tmp.size();i++) {
    if(!parameterFilter.count(tmp[i].alias)) {
      inpar.push_back(tmp[i]);
    }
  }
}

bool FimexStream::hasParameter(std::string parametername)
{
  try {
    if(!interpol)
      createPoslistInterpolator();

    return interpol->getCDM().hasVariable(parametername);

  } catch( exception& e) {
    cerr << "Exception in hasParameter: " << e.what() << endl;
  }
  return false;
}



bool FimexStream::hasCompleteDataset(std::string placename,float lat, float lon, vector<ParId> inpar)
{
  // nothing requested  - this is complete

  if(inpar.empty())
    return false;

  filterParameters(inpar);
  // position not found
  if( poslist.getPos(placename,lat,lon) < 0 )
    return false;

  if(cache.empty() )
    return false;

  // check if there are parameters that were not interpolated earlier
  vector<ParId> extrapar;
  cache[0].getExtrapars(inpar,extrapar);
  bool setIsComplete = true;

  for(unsigned int i=0;i<extrapar.size();i++) {
    for( unsigned int j=0;j<fimexpar.size();j++) {
      if ( fimexpar[j].parid == extrapar[i] && fimexpar[j].streamtype == parametertype) {
        // we expect this parameter to be read from the fimex file
        // but does the file have the fimexparameter at all ?
        if(hasParameter(fimexpar[j].parametername)) {
          setIsComplete=false;
          break;
        }
      }
    }
  }
  return setIsComplete;
}

bool FimexStream::readData(std::string placename,float lat, float lon, vector<ParId>& inpar,vector<ParId>& outpar)
{

  filterParameters(inpar);

  try {

    clean();

    if(!is_open)
      return false;

    if(!interpol) {
      createPoslistInterpolator();
    }

    if(!timeLineIsRead) {
      createTimeLine();
    }

    if(placename.empty()) {
      return false;
    }

    activePosition = poslist.getPos(placename,lat,lon);

    // position not found
    if(activePosition < 0 ) {

      // aha the common poslist has changed - try to reload the cache
      if(poslistVersion != commonposlistVersion) {
        poslist = commonposlist;
        poslistVersion = commonposlistVersion;
        createPoslistInterpolator();
        cache.clear();
        activePosition = poslist.getPos(placename,lat,lon);
      }
    }


    if(cache.empty() ) {
      addToCache(0,poslist.getNumPos(),inpar,true);
    } else {
      // check if there are parameters that were not interpolated earlier
      vector<ParId> extrapar;
      cache[0].getExtrapars(inpar,extrapar);


      // try to interpolate the missing parameters
      addToCache(0,poslist.getNumPos(),extrapar,false);

    }



    if(activePosition < 0 ) {
      cerr << placename << " not found " << endl;
      return false;

    }

    cache[activePosition].getOutpars(inpar,outpar);

  } catch ( exception& e) {
    cerr << "exception caught  in Fimex read : " << e.what();
    return false;
  }

  return true;
}



bool FimexStream::addToCache(int posstart, int poslen,vector<ParId>& inpar, bool createPoslist)
{

  bool foundSomeData=false;
  FimexPetsCache tmp;
  if(createPoslist)
    for(unsigned int i=0;i<poslen;i++)
      cache.push_back(tmp);
  // check the parameterlist - what to get and what not....

  int maxprogress=0;
  for(unsigned int i=0;i<inpar.size();i++) {
    for( unsigned int j=0;j<fimexpar.size();j++) {
      if ( fimexpar[j].parid == inpar[i] && fimexpar[j].streamtype == parametertype){
        maxprogress++;
      }
    }
  }

  vector<FimexParameter> activeParameters;
  int localProgress=0;
  for(unsigned int i=0;i<inpar.size();i++) {
    for( unsigned int j=0;j<fimexpar.size();j++) {
      if ( fimexpar[j].parid == inpar[i] && fimexpar[j].streamtype == parametertype) {

        localProgress++;
        progress = (localProgress * 95) / maxprogress;


        ostringstream ost;
        ost << modelname << ": " << fimexpar[j].parametername;
        progressMessage = ost.str();

        try {
          if(readFromFimexSlice(fimexpar[j]))
            foundSomeData=true;
        } catch ( exception& e) {
          cerr <<"Exception from ReadFromFimexSlice:  " <<  e.what() << endl;
        }

        break;


      }
    }
  }
  boost::posix_time::ptime last   = boost::posix_time::microsec_clock::universal_time();
  progress=100;
  return foundSomeData;
}






static MetNoFimex::DataPtr getParallelScaledDataSliceInUnit(size_t maxProcs, boost::shared_ptr<MetNoFimex::CDMReader> reader, const string& parName, const string& parUnit, const vector<MetNoFimex::SliceBuilder>& slices)
{
  vector<size_t> sliceLengths(slices.size(), 1);
  for (int i = 0; i < slices.size(); i++) {
    vector<size_t> ssize = slices.at(i).getDimensionSizes();
    sliceLengths.at(i) = accumulate(ssize.begin(), ssize.end(), 1, std::multiplies<int>());
  }
  size_t total = accumulate(sliceLengths.begin(), sliceLengths.end(), 0);

  //create a anonymous mapped shm-obj in this process
  boost::interprocess::mapped_region region(boost::interprocess::anonymous_shared_memory(total*sizeof(float)));
  // fork the sub-processes
  pid_t pid;
  vector<pid_t> children;
  for (size_t i = 0; i < maxProcs; i++) {

    // starting child process ---------------------
    pid = fork();

    if(pid < 0) {
      cerr << "Error forking - no process id " << endl;
      exit(1);
    } else if (pid == 0) {
      // child code, should end with exit!
      //            boost::interprocess::mapped_region region(shm_obj, boost::interprocess::read_write);
      assert(region.get_size() == (total*sizeof(float)));
      float* regionFloat = reinterpret_cast<float*>(region.get_address());
      size_t startPos = 0;
      for (size_t j = 0; j < slices.size(); j++) {
        if ((j % maxProcs) == i) {
          MetNoFimex::DataPtr data;
          try {
            data = reader->getScaledDataSliceInUnit(parName, parUnit, slices.at(j));
          } catch (runtime_error& ex) {
            cerr << "error fetching data on '" << parName << "', '" << parUnit << "' slice " << j << ": " << ex.what() << endl;
            data = MetNoFimex::createData(MetNoFimex::CDM_FLOAT, 0);
          }
          boost::shared_array<float> array;
          if (data->size() == 0) {
            array = boost::shared_array<float>(new float[sliceLengths.at(j)]);
            for (size_t k = 0; k < sliceLengths.at(j); k++) array[k] = MIFI_UNDEFINED_F;
          } else {
            assert(data->size() == sliceLengths.at(j));
            array = data->asFloat();
          }
          std::copy(array.get(), array.get()+sliceLengths.at(j), regionFloat + startPos);
        }
        startPos += sliceLengths.at(j);
      }
      // ending child process without cleanup ( _exit() ) to avoid qt-trouble
      _exit(0);

    } else  {
      // parent, handled below, should fork more
      children.push_back(pid);
    }
  }
  // parent code
  // wait for all children
  for (int i = 0; i < maxProcs; ++i) {
    int status;
    while (-1 == waitpid(children.at(i), &status, 0));
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      std::cerr << "Process " << i << " (pid " << children.at(i) << ") failed" << std::endl;
      throw runtime_error("child-process did not finish correctly when fetching data");
      exit(1);
    }
  }
  // read the shared_memory
  boost::shared_array<float> allFloats(new float[total]);
  assert(region.get_size() == (total*sizeof(float)));
  float* regionFloat = reinterpret_cast<float*>(region.get_address());
  std::copy(regionFloat, regionFloat+total, allFloats.get());

  return MetNoFimex::createData(total, allFloats);
}


bool FimexStream::readFromFimexSlice(FimexParameter par)
{
  if(!is_open)
    return false;


  cerr << "Interpolating Parameter: " << par.parametername << " for model " << modelname;
  boost::posix_time::ptime start  = boost::posix_time::microsec_clock::universal_time();
  MetNoFimex::SliceBuilder slice(interpol->getCDM(),par.parametername);

  unsigned int numPos=poslist.getNumPos();

  slice.setStartAndSize("x", 0, poslist.getNumPos());

  for(unsigned int i=0;i<cache.size();i++)
    cache[i].clear_tmp();

  unsigned int timeId;

  std::string timeAxis="time";

  const MetNoFimex::CDMDimension* tmpDim = interpol->getCDM().getUnlimitedDim();
  size_t timeSize = 0;
  if(tmpDim) {
    timeAxis=tmpDim->getName();
    timeSize = tmpDim->getLength();
  } else {
    const MetNoFimex::CDMDimension& timeDim = interpol->getCDM().getDimension(timeAxis);
    timeSize = timeDim.getLength();
  }

  for(unsigned int i=0; i<par.dimensions.size();i++) {

    if(interpol->getCDM().hasDimension( par.dimensions[i].name )) {



      if (par.dimensions[i].size < 0 ) {
        const MetNoFimex::CDMDimension& tmpDim = interpol->getCDM().getDimension( par.dimensions[i].name );
        par.dimensions[i].size = tmpDim.getLength() - par.dimensions[i].start;

      }

      cerr << "par.dimensions[i].size for " << par.dimensions[i].name << " = "  << par.dimensions[i].size << endl;
      slice.setStartAndSize(par.dimensions[i].name ,par.dimensions[i].start ,par.dimensions[i].size );
    } else {
      throw FimexstreamException(" request unknown dimension " + par.dimensions[i].name );
    }

  }


  if(basetimeline.empty())
    createTimeLine();

  // dividing in small time-slices
  vector<MetNoFimex::SliceBuilder> slices;
  for (size_t i=0; i< timeSize; i++) {
    slice.setStartAndSize(timeAxis ,i,1);
    slices.push_back(slice);
  }

  // get time-slices parallel
  size_t numProcs = 0;
#ifdef _SC_NPROCESSORS_ONLN
  numProcs = sysconf( _SC_NPROCESSORS_ONLN );
  // even better, but needs c++11: std::thread::hardware_concurrency();
#endif
  if (numProcs < 1) numProcs = 2; // default


  MetNoFimex::DataPtr sliceddata  = getParallelScaledDataSliceInUnit(numProcs, interpol, par.parametername, par.unit, slices);

  int pardim=0;
  int timdim=basetimeline.size();
  if(sliceddata.get()) {


    boost::shared_array<float> valuesInSlice = sliceddata->asFloat();

    // all parameters in all dimensions;
    unsigned int numAll     = sliceddata->size() / numPos;
    unsigned int numTimes   = basetimeline.size();
    unsigned int numPardims = numAll / numTimes; // equals 1 except for ensembles

    for ( unsigned int pardim = 0; pardim < numPardims; pardim++) {
      for( unsigned int tim=0; tim < numTimes; tim++) {

        for(unsigned int pos=0;pos<numPos;pos++) {

          if(!pardim)
            cache[pos].tmp_times.push_back(basetimeline.at(tim));

          unsigned int index = pardim*numPos +  tim*numPardims*numPos + pos;

          if(!MetNoFimex::mifi_isnan(valuesInSlice[ index ]) ) {

            cache[pos].tmp_values.push_back(valuesInSlice[ index ]);

          }else {
            //cache[pos].tmp_values.push_back(100);
          }

        }
      }
    }

    ParId pid = par.parid;
    pid.model = modelname;
    pid.run   = 0;
    pid.level = 0;

    for(unsigned int i=0;i<cache.size();i++)
      cache[i].process(pid);



    boost::posix_time::ptime last   = boost::posix_time::microsec_clock::universal_time();
    cerr << " ... done. Used : "  << (last-start).total_milliseconds() << " ms   " << endl;

    return true;
  }
  cerr << " ... empty  "<< endl;
  return false;


}


bool FimexStream::getOnePar(int i, WeatherParameter& wp)
{
  if(i>=0 && i<(int)cache[activePosition].parameters.size()) {
    string wpname =  cache[activePosition].parameters[i].Id().alias;
    if(isFiltered(wpname))
      return false;

    wp=cache[activePosition].parameters[i];
    return true;
  }
  return false;
}

bool FimexStream::getTimeLine(int index,vector<miTime>& tline, vector<int>& pline)
{
  pline=progline;

  return cache[activePosition].timeLines.Timeline(index,tline);
}

int FimexStream::numParameters()
{
  return cache[activePosition].parameters.size();
}



} /* namespace pets */
