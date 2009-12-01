#pragma once

extern "C" {
#include "zmstring.h"
}
#include <HashTab.hh>
#include "Mapping.hh"
#undef erase


class MapTable : public HashTab<Mapping>
{
public:
  inline MapTable(const unsigned buckets);
  inline ~MapTable();

  inline void add(const char * const, const char * const, const char = 0);
  inline const char * find(const char * const);

  void erase();

private:
  void wipe();
};



MapTable::MapTable(const unsigned buckets)
  : HashTab<Mapping>(buckets)
{
}


MapTable::~MapTable()
{
  wipe();
}


void MapTable::add(const char * const source, const char * const destination, const char cached)
{
  HashTab<Mapping>::add(Mapping(savestr(source), destination ? savestr(destination) : NULL, cached));
}


const char * MapTable::find(const char * const source)
{
  // XXX casting away const
  Mapping *match = HashTab<Mapping>::find(Mapping((char * const)source));
  return match ? match->destination : NULL;
}
