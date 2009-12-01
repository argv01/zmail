#pragma once

extern "C" {
#include <hashtab.h>
}


template <class Element> class HashTab : public hashtab
{
public:
  inline HashTab(unsigned);
  inline void reinitialize();
  inline ~HashTab();

  inline void add(const Element &);
  inline Element *find(const Element &);
  inline void remove(const Element &);
  inline void remove();

  inline int emptyP() const;
  inline unsigned length() const;
  inline unsigned numBuckets() const;

  inline double mean() const;
  inline double variance() const;
  inline void stats(double &, double &) const;
  inline void rehash(unsigned);

  inline static unsigned stringHash(const char * const);

  class Iterator : private hashtab_iterator
  {
  public:
    inline Iterator(HashTab<Element> &table)
      : table(table)
    {
      hashtab_InitIterator(this);
      current = (Element *) hashtab_Iterate(&table, this);
    }

    inline operator Element * () const
    {
      return current;
    }
    
    inline Element * operator -> () const
    {
      return current;
    }
    
    inline void next()
    {
        if (current) current = (Element *) hashtab_Iterate(&table, this);
    }

  private:
    HashTab<Element> &table;
    Element *current;
  };

private:
  inline void destroy();
  inline void initialize(unsigned);

  static inline unsigned hash(const Element * const);
  static inline int compare(const Element * const, const Element * const);
};



template <class Element> HashTab<Element>::HashTab(unsigned buckets)
{
  initialize(buckets);
}


template <class Element> void HashTab<Element>::initialize(unsigned buckets)
{
  hashtab_Init(this, (unsigned (*)(const void *)) hash,
	       (int (*)(const void *, const void *)) compare,
	       sizeof(Element), buckets);
}


template <class Element> void HashTab<Element>::reinitialize()
{
  const unsigned buckets = numBuckets();
  destroy();
  initialize(buckets);
}


template <class Element> HashTab<Element>::~HashTab()
{
  destroy();
}


template <class Element> void HashTab<Element>::destroy()
{
  hashtab_Destroy(this);
}


template <class Element> void HashTab<Element>::add(const Element &arrival)
{
  hashtab_Add(this, &arrival);
}


template <class Element> Element * HashTab<Element>::find(const Element &probe)
{
  return (Element *) hashtab_Find(this, &probe);
}


template <class Element> void HashTab<Element>::remove(const Element &probe)
{
  hashtab_Remove(this, &probe);
}


template <class Element> void HashTab<Element>::remove()
{
  hashtab_Remove(this, NULL);
}



template <class Element> int HashTab<Element>::emptyP() const
{
  return hashtab_EmptyP((const hashtab * const) this);
}


template <class Element> unsigned HashTab<Element>::length() const
{
  return hashtab_Length((const hashtab * const) this);
}


template <class Element> unsigned HashTab<Element>::numBuckets() const
{
  return hashtab_NumBuckets(this);
}



template <class Element> double HashTab<Element>::mean() const
{
  double mean;
  hashtab_Stats(this, &mean, NULL);
  return mean;
}


template <class Element> double HashTab<Element>::variance() const
{
  double variance;
  hashtab_Stats(this, NULL, &variance);
  return variance;
}


template <class Element> void HashTab<Element>::stats(double &mean, double &variance) const
{
  hashtab_Stats(this, &mean, &variance);
}


template <class Element> unsigned HashTab<Element>::stringHash(const char * const string)
{
  return hashtab_StringHash(string);
}


template <class Element> unsigned HashTab<Element>::hash(const Element * const element)
{
  return element->hash();
}


template <class Element> int HashTab<Element>::compare(const Element * const first, const Element * const second)
{
  return Element::compare(*first, *second);
}
