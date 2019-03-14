#ifndef _MATH_H
#define _MATH_H

#define DISP2(p1, p2) ((p1[0] - p2[0]) * (p1[0] - p2[0]) + (p1[1] - p2[1]) * (p1[1] - p2[1]) + (p1[2] - p2[2]) * (p1[2] - p2[2]))

class math
{
  public:
    static inline float disp__(float p1[3], float p2[3])
    {
        return DISP2(p1, p2);
    }

    static inline float near__(float p1[3], float p2[3], float radis = 10.0f)
    {
        return DISP2(p1, p2) < pow__(radis) * 0.25f;
    }

    static inline float pow__(float value)
    {
        return value * value;
    }

  private:
    math()
    {
    }
    ~math() {}
};

#endif