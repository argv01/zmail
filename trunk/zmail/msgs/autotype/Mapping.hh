#pragma once
#include <HashTab.hh>
#include <stdio.h>
#include <strcase.h>


struct Mapping
{
  char * const source;
  char * const destination;
  const char cached;
  
  inline Mapping(char * const, char * const = NULL, const char = 0);

  inline unsigned hash() const;
  inline static int compare(const Mapping &, const Mapping &);
};



Mapping::Mapping(char * const source, char * const destination, const char cached)
  : source(source),
    destination(destination),
    cached(cached)
{
}


unsigned Mapping::hash() const
{
  return ci_stringHash(source);
}


int Mapping::compare(const Mapping &first, const Mapping &second)
{
  return ci_strcmp(first.source, second.source);
}
