/*
 * $RCSfile: dynstr.h,v $
 * $Revision: 2.20 $
 * $Date: 1995/10/05 04:59:57 $
 * $Author: liblit $
 */

#ifndef DYNSTR_H
#define DYNSTR_H
#ifdef __cplusplus
extern "C" {
#endif /* C++ */

#include <general.h>

struct dynstr {
    int used, allocated;
    char *strng;
};

#define dynstr_Str(d) ((d)->strng)
#define dynstr_EmptyP(d) (dynstr_Length(d)==0)

#define dynstr_Insert(d,start,str) \
    (dynstr_Replace((d),(start),0,(str)))
#define dynstr_InsertN(d,start,str,n) \
    (dynstr_ReplaceN((d),(start),0,(str),(n)))
#define dynstr_InsertChar(d,start,c) \
    (dynstr_ReplaceChar((d),(start),0,(c)))

#define dynstr_Delete(d,start,len) (dynstr_Replace((d),(start),(len),""))

extern int dynstr_Length P((const struct dynstr *));
extern int dynstr_Chop P((struct dynstr *));
extern void dynstr_ChopN P((struct dynstr *, unsigned));
extern void dynstr_KeepN P((struct dynstr *, unsigned));
extern void dynstr_Append P((struct dynstr *, const char *));
extern void dynstr_AppendN P((struct dynstr *, const char *, int));
extern int dynstr_AppendChar P((struct dynstr *, int));
extern void dynstr_Destroy P((struct dynstr *));
extern char *dynstr_GiveUpStr P((struct dynstr *));
extern void dynstr_Init P((struct dynstr *));
extern void dynstr_InitFrom P((struct dynstr *, char *));
extern void dynstr_Set P((struct dynstr *, const char *));
extern void dynstr_Replace P((struct dynstr *, int, int, const char *));
extern void dynstr_ReplaceChar P((struct dynstr *, int, int, int));
extern void dynstr_ReplaceN P((struct dynstr *, int, int, const char *, int));

#ifdef __cplusplus
}

extern "C++" struct ZDynstr : public dynstr {
 public:
    ZDynstr() { dynstr_Init(this); }
    ZDynstr(char *s) { dynstr_InitFrom(this, s); }
    ~ZDynstr() { dynstr_Destroy(this); }
    
    const ZDynstr& operator =(const char *s)
	{ dynstr_Set(this, s); return *this; }
    const ZDynstr& operator +=(const char *s)
	{ Append(s); return *this; }
    const ZDynstr& operator +=(char c)
	{ Append(c); return *this; }

    operator const char *() const { return dynstr_Str(this); }
    
    void Insert(int start, const char *str)
	{ dynstr_Insert(this, start, str); }
    void Insert(int start, const char *str, int n)
	{ dynstr_InsertN(this, start, str, n); }
    void Insert(int start, char c)
	{ dynstr_InsertChar(this, start, c); }
    void Replace(int start, int end, const char *str)
	{ dynstr_Replace(this, start, end, str); }
    void Replace(int start, int end, const char *str, int n)
	{ dynstr_ReplaceN(this, start, end, str, n); }
    void Replace(int start, int end, char c)
	{ dynstr_ReplaceChar(this, start, end, c); }
    void Delete(int start, int len)
	{ dynstr_Delete(this, start, len); }
    int Length() const { return dynstr_Length(this); }
    char Chop() { return dynstr_Chop(this); }
    void Append(const char *str) { dynstr_Append(this, str); }
    void Append(const char *str, int n) { dynstr_AppendN(this, str, n); }
    void Append(char c) { dynstr_AppendChar(this, c); }
# ifdef NOT_NOW
    /* We aren't supposed to call the destructor after calling GiveUpStr(),
     * so we can't provide this function.  dynstr_GiveUpStr() is really
     * an "alternate destructor" for dynstr's, and C++ doesn't have that
     * concept.  We could call dynstr_Init() after dynstr_GiveUpStr(),
     * or we could fix dynstr_GiveUpStr() so that calling
     * dynstr_Destroy() is legal after calling dynstr_GiveUpStr().
     * Not worth screwing around with right now, IMHO.
     */
    char *GiveUpStr() { return dynstr_GiveUpStr(this); }
# endif /* NOT_NOW */
    void InitFrom(char *str)
	{ dynstr_Destroy(this); dynstr_InitFrom(this, str); }
    int EmptyP() const { return dynstr_EmptyP(this); }
};
#endif /* __cplusplus */

#endif /* DYNSTR_H */
