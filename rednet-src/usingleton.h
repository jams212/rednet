#ifndef REDNET_USINGLETON_H
#define REDNET_USINGLETON_H

#include <stdlib.h>

namespace rednet
{
template <class T>
class usingleton
{
  public:
    virtual ~usingleton()
    {
        if (_inst)
        {
            delete _inst;
            _inst = NULL;
        }
    }

    static T *instance()
    {
        if (_inst)
            return _inst;
        _inst = new T();
        return _inst;
    }

  protected:
    usingleton()
    {
    }

  private:
    static T *_inst;
};

template <class T>
T *usingleton<T>::_inst = NULL;

} // namespace rednet

#endif