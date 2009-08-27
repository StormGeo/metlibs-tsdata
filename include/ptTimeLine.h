/*
 libtsData - Time Series Data

 $Id$

 Copyright (C) 2006 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// ptTimeLine.h

#ifndef timeline_h
#define timeline_h

#include <puTools/miTime.h>

#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

const int MAXTIMELINES = 30;

typedef vector<int> ProgLine;

// The timeline class is actually several timelines in one,
// the idea being that there should be a timeline for each
// unique dataset (in terms of model, run AND level)
// Each unique timepoint is stored only once, in a TimeLineItem.
// The different timelines are kept apart by an array of bools (flag)
// in each TimeLineItem. If TimeLineItem[i].flag[j] is true, it
// means that timepoint number i exists in timeline number j.

struct TimeLineItem {
  miTime time;
  bool flag[MAXTIMELINES];

  TimeLineItem()
  {
  } // leaves the TimeLineItem in an undefined state

  TimeLineItem(const miTime& t, int tlIndex = 0)
  {
    time = t;
    for (int i = 0; i < MAXTIMELINES; i++)
      flag[i] = false;
    if (tlIndex >= 0 && tlIndex < MAXTIMELINES)
      flag[tlIndex] = true;
  }
  TimeLineItem(const TimeLineItem& rhs)
  {
    time = rhs.time;
    for (int i = 0; i < MAXTIMELINES; i++)
      flag[i] = rhs.flag[i];
  }
  TimeLineItem& operator=(const TimeLineItem& rhs)
  {
    if (&rhs != this) {
      time = rhs.time;
      for (int i = 0; i < MAXTIMELINES; i++)
        flag[i] = rhs.flag[i];
    }
    return *this;
  }
  friend bool operator==(const TimeLineItem& l, const TimeLineItem& r)
  {
    return l.time == r.time;
  }
  friend bool operator==(const TimeLineItem& l, const miTime& r)
  {
    return l.time == r;
  }
  friend bool operator<(const TimeLineItem& l, const TimeLineItem& r)
  {
    return l.time < r.time;
  }
  friend bool operator<(const TimeLineItem& l, const miTime& r)
  {
    return l.time < r;
  }
  friend bool operator<=(const TimeLineItem& l, const miTime& r)
  {
    return l.time <= r;
  }
  friend ostream& operator<<(ostream& out, /*const*/TimeLineItem& t)
  {
    ostringstream s;
    for (int i = 0; i < MAXTIMELINES; i++)
      s << t.flag[i] << ' ';
    return out << t.time << ' ' << s.str();
  }

};

class TimeLine {
private:
  vector<TimeLineItem> data;

public:
  TimeLine()
  {
  }
  TimeLine& operator=(const TimeLine& rhs)
  {
    if (&rhs != this) {
      data = rhs.data;
    }
    return *this;
  }
  bool flag(int i, int j)
  {
    if (i < 0 || i >= data.size())
      return false;
    if (j < 0 || j >= MAXTIMELINES)
      return false;
    return data[i].flag[j];
  }
  int size()
  {
    return data.size();
  }

  miTime operator[](int i)
  {
    miTime tmp;
    if (i < 0 || i >= data.size())
      return tmp;
    return data[i].time;
  }

  friend ostream& operator<<(ostream& out, /*const*/TimeLine& tl)
  {
    for (int i = 0; i < tl.data.size(); ++i)
      out << i << ": " << tl.data[i] << '\n';
    return out;
  }

  void setToFalse(int i, int j)
  {
    if (i < 0 || i >= data.size())
      return;
    if (j < 0 || j >= MAXTIMELINES)
      return;

    data[i].flag[j] = false;
  }

  // returns index of element whose time is tp
  // returns -1 if not found
  int find_equal(const miTime& tp)
  {
    vector<TimeLineItem>::iterator where = find(data.begin(), data.end(),
        TimeLineItem(tp));
    return where != data.end() ? where - data.begin() : -1;
  }

  // returns index of first element whose time is >= tp
  // returns -1 if the data vector is empty or if biggest element < tp
  int find_greater_or_equal(const miTime& tp)
  {
    for (int i = 0; i < data.size(); ++i) {
      if (data[i].time >= tp)
        return i;
    }
    return -1;
  }

  bool insert(const miTime& tp, int index = 0)
  {
    if (index < 0 || index >= MAXTIMELINES)
      return false;
    for (vector<TimeLineItem>::iterator i = data.begin(); i < data.end(); ++i) {
      if (tp <= i->time) { // found position
        if (tp == i->time) // does this timepoint exist?
          i->flag[index] = true;
        else {
          TimeLineItem tli(tp, index);
          data.insert(i, tli);
        }
        return true;
      }
    }
    TimeLineItem tli(tp, index);
    data.push_back(tli); // the timepoint belongs at the end
    return true;
  }

  bool insert(const vector<miTime>& tl, int index = 0)
  {
    bool ok = true;
    for (int i = 0; i < tl.size(); i++) {
      ok &= insert(tl[i], index);
    }
    return ok;
  }

  void clear()
  {
    data.clear();
  }

  bool Timeline(const int& index, vector<miTime>& tline)
  {
    tline.erase(tline.begin(), tline.end());
    if (index >= 0 && index < MAXTIMELINES) {
      for (int i = 0; i < data.size(); i++)
        if (data[i].flag[index])
          tline.push_back(data[i].time);
      return true;
    }
    return false;
  }

  // check if the timeline corresponding to tline already exist
  int Exist(const vector<miTime>& tline)
  {
    if (tline.size() == 0)
      return -1;
    int i, j, k;
    vector<int> indices;
    // check first if all timepoints exists
    for (i = 0; i < tline.size(); i++) {
      if ((k = find_equal(tline[i])) == -1)
        return -1;
      indices.push_back(k);
    }
    // check each separate timeline
    for (j = 0; j < MAXTIMELINES; j++) {
      k = 0;
      for (i = 0; i < data.size(); i++) {
        if (i == indices[k]) {
          if (!data[i].flag[j])
            break;
          if (k < (indices.size() - 1))
            k++;
        } else {
          if (data[i].flag[j])
            break;
        }
      }
      if (i == data.size())
        return j;
    }
    return -1;
  }

  // remove unused timepoints from the timeline
  // if indices>0, only keep these timelines
  void cleanup(vector<int>& indices)
  {
    int i, j, k;
    if (indices.size() > 0) {
      // sort the indices
      sort(indices.begin(), indices.end());
      // check each separate timeline
      k = 0;
      for (j = 0; j < MAXTIMELINES; j++) {
        if (j == indices[k]) {
          if (k < (indices.size() - 1))
            k++;
        } else
          for (i = 0; i < data.size(); i++)
            data[i].flag[j] = false;
      }
    }
    // check for unused timepoints
    bool used;
    vector<miTime> toberemoved;
    for (i = 0; i < data.size(); i++) {
      used = false;
      for (j = 0; j < MAXTIMELINES; j++)
        used = used || data[i].flag[j];
      if (!used)
        toberemoved.push_back(data[i].time);
    }
    // remove them
    if (toberemoved.size())
      for (i = 0; i < toberemoved.size(); i++) {
        for (vector<TimeLineItem>::iterator p = data.begin(); p < data.end(); ++p) {
          if (toberemoved[i] == p->time) {// found position
            data.erase(p);
            break;
          }
        }
      }
  }

  // find first free timeline and return index.
  // returns -1 if no free timelines
  int freeIndex()
  {
    int i, j;
    for (j = 0; j < MAXTIMELINES; j++) {
      bool used = false;
      for (i = 0; i < data.size(); i++) {
        if (used = data[i].flag[j])
          break;
      }
      if (!used)
        return j;
    }
    return -1;
  }

  // adds a new timeline and return index.
  // if this timeline already exist only return index to it.
  // if new timeline, and timeline class is full: return -1
  int addTimeline(const vector<miTime>& tl)
  {
    int k;
    k = Exist(tl);
    if (k != -1)
      return k;

    k = freeIndex();
    if (k != -1)
      insert(tl, k);

    return k;
  }

  // delete timeline by index
  void deleteTimeline(const int index)
  {
    if (index < 0 || index >= MAXTIMELINES)
      return;
    for (int i = 0; i < data.size(); i++)
      data[i].flag[index] = false;

    // remove any unused times
    vector<int> dummy;
    cleanup(dummy);
  }

};
#endif
