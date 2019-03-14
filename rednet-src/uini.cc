#include "uini.h"
#include "umemory.h"
#include <algorithm>
#include <assert.h>
#include <string.h>

using namespace rednet;

uini::uini()
{
    flags__.push_back("#");
    flags__.push_back(";");
}

int uini::load(const string &filename)
{
    fname__ = filename;
    FILE *fp = fopen(filename.c_str(), "r");
    if (!fp)
    {
        return -1;
    }

    string line;
    while (local_read_line(line, fp) > 0)
    {
        __trimright__(line, '\n');
        __trimright__(line, '\r');
        __trim__(line);

        if (local_iscomment(line))
        {
            continue;
        }

        __trim__(line);
        if (line.length() <= 0)
        {
            continue;
        }

        string key;
        string value;

        if (!local_parse(line, key, value))
            continue;
        sections__[key] = value;
    }

    return 0;
}

string uini::get_string_value(const string &key, const string default_v)
{
    string value = local_get_value(key);
    if (value.empty())
        return default_v;
    return value;
}

int uini::get_int_value(const string &key, const int default_v)
{
    string value = local_get_value(key);
    if (value.empty())
        return default_v;
    return atoi(value.c_str());
}

float uini::get_float_value(const string &key, const float default_v)
{
    string value = local_get_value(key);
    if (value.empty())
        return default_v;
    return (float)atof(value.c_str());
}

double uini::get_double_value(const string &key, double default_v)
{
    string value = local_get_value(key);
    if (value.empty())
        return default_v;
    return atof(value.c_str());
}

bool uini::get_boolean_value(const string &key, bool default_v)
{
    string value = local_get_value(key);
    if (value.empty())
        return default_v;
    transform(value.begin(), value.end(), value.begin(), ::tolower);

    return value.compare("true") == 0 ? true : false;
}

int uini::local_read_line(string &str, FILE *fp)
{
    int plen = 0;
    int buf_size = UINI_BUF_SIZE * sizeof(char);
    char *buf = (char *)umemory::malloc(buf_size);
    char *pbuf = NULL;
    char *p = buf;

    assert(buf);
    memset(buf, 0, buf_size);
    int total_size = buf_size;

    while (fgets(p, buf_size, fp) != NULL)
    {
        plen = strlen(p);
        if (plen > 0 && p[plen - 1] != '\n' && !feof(fp))
        {

            total_size = strlen(buf) + buf_size;
            pbuf = (char *)umemory::realloc(buf, total_size);
            assert(pbuf);
            buf = pbuf;
            p = buf + strlen(buf);
            continue;
        }
        else
        {
            break;
        }
    }

    str = buf;

    umemory::free(buf);
    buf = NULL;
    return str.length();
}

string uini::local_get_value(const string &key)
{
    string str_null;
    str_null.clear();
    if (sections__.empty())
        return str_null;
    iterator it = sections__.find(key);
    if (it != sections__.end())
    {
        return it->second;
    }
    return str_null;
}

bool uini::local_iscomment(const string &str)
{
    bool bret = false;
    for (size_t i = 0; i < flags__.size(); ++i)
    {
        size_t k = 0;
        if (str.length() < flags__[i].length())
        {
            continue;
        }

        for (k = 0; k < flags__[i].length(); ++k)
        {
            if (str[k] != flags__[i][k])
            {
                break;
            }
        }

        if (k == flags__[i].length())
        {
            bret = true;
            break;
        }
    }

    return bret;
}

bool uini::local_parse(const string &content, string &key, string &value, char c /* '=' */)
{
    int i = 0;
    int len = content.length();

    while (i < len && content[i] != c)
    {
        ++i;
    }

    if (i >= 0 && i < len)
    {
        key = string(content.c_str(), i);
        value = string(content.c_str() + i + 1, len - i - 1);
        __trim__(key);
        __trim__(value);
        return true;
    }

    return false;
}

void uini::local_release()
{
    sections__.clear();
    flags__.clear();
}

void uini::__trimleft__(string &str, char c /*' '*/)
{
    int len = str.length();

    int i = 0;

    while (str[i] == c && str[i] != '\0')
    {
        i++;
    }

    if (i != 0)
    {
        str = string(str, i, len - i);
    }
}

void uini::__trimright__(string &str, char c /*' '*/)
{
    int i = 0;
    int len = str.length();

    for (i = len - 1; i >= 0; --i)
    {
        if (str[i] != c)
        {
            break;
        }
    }

    str = string(str, 0, i + 1);
}

void uini::__trim__(string &str)
{
    int len = str.length();

    int i = 0;

    while (isspace(str[i]) && str[i] != '\0')
    {
        i++;
    }

    if (i != 0)
    {
        str = string(str, i, len - i);
    }

    //trim tail
    len = str.length();

    for (i = len - 1; i >= 0; --i)
    {
        if (!isspace(str[i]))
        {
            break;
        }
    }

    str = string(str, 0, i + 1);
}