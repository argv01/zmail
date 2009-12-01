// -*- c++ -*-

#include <dlist.h>

template <class Element> struct DList : public dlist
{
  inline DList(unsigned);
  inline ~DList();

  inline int head() const;
  inline int tail() const;

  inline emptyP() const;
  inline unsigned length() const;
  inline Element & nth(unsigned);
  
  inline int next(unsigned) const;
  inline int prev(unsigned) const;

  inline unsigned insertAfter(unsigned, const Element &);
  inline unsigned insertBefore(unsigned, const Element &);

  inline unsigned prepend(const Element &);
  inline unsigned append(const Element &);

  inline void remove(unsigned);
  inline void replace(unsigned, const Element &);

  class Iterator
  {
  public:
    inline Iterator(DList<Element> &list)
      : position(list.head()),
	list(list)
    {
    }

    inline operator Element * () const
    {
      return position < 0 ? NULL : list[position];
    }

    inline void next()
    {
      position = position < 0 ? position : list.next(position);
    }

  private:
    int position;
    DList<Element> &list;
  };
};



template <class Element> DList<Element>::DList(unsigned growSize)
{
  dlist_Init(this, sizeof(Element), growSize);
}


template <class Element> DList<Element>::~DList()
{
  dlist_Destroy(this);
}



template <class Element> int DList<Element>::head() const
{
  // the cast is because of confusion with dlist::head
  return dlist_Head((const dlist * const) this);
}


template <class Element> int DList<Element>::tail() const
{
  // the cast is because of confusion with dlist::head
  return dlist_Tail((const dlist * const) this);
}



template <class Element> DList<Element>::emptyP() const
{
  // the cast is because of confusion with dlist::head
  return dlist_EmptyP((const dlist * const) this);
}


template <class Element> unsigned DList<Element>::length() const
{
  // the cast is because of confusion with dlist::head
  return dlist_Length((const dlist * const) this);
}



template <class Element> Element & DList<Element>::nth(unsigned position)
{
  return *((Element *) dlist_Nth(this, position));
}



template <class Element> int DList<Element>::next(unsigned position) const
{
  return dlist_Next(this, position);
}


template <class Element> int DList<Element>::prev(unsigned position) const
{
  return dlist_Prev(this, position);
}



template <class Element> unsigned DList<Element>::insertAfter(unsigned position, const Element &arrival)
{
  return dlist_InsertAfter(this, position, &arrival);
}


template <class Element> unsigned DList<Element>::insertBefore(unsigned position, const Element &arrival)
{ 
  return dlist_InsertBefore(this, position, &arrival);
}



template <class Element> unsigned DList<Element>::prepend(const Element &arrival)
{
  return dlist_Prepend(this, &arrival);
}


template <class Element> unsigned DList<Element>::append(const Element &arrival)
{
  return dlist_Append(this, &arrival);
}



template <class Element> void DList<Element>::remove(unsigned chaff)
{
  dlist_Remove(this, chaff);
}


template <class Element> void DList<Element>::replace(unsigned chaff, const Element &replacement)
{
  dlist_Replace(this, chaff, &replacement);
}
