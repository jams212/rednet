#ifndef _ALLOC_H
#define _ALLOC_H

class aoi
{
  public:
    typedef void *(*Alloc)(void *ud, void *def, size_t sz);
    typedef void (*CallBack)(void *ud, uint32_t watcher, uint32_t marker);

  private:
    aoi() {}
    ~aoi() {}
};

#endif