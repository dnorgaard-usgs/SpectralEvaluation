#include "Utils.h"
#include <algorithm>
#include <vector>

bool EqualsIgnoringCase(const std::string& str1, const std::string& str2)
{
#ifdef _MSC_VER
    return (0 == _strnicmp(str1.c_str(), str2.c_str(), std::max(strlen(str1.c_str()), strlen(str2.c_str()))));
#else
    return (0 == strncasecmp(str1.c_str(), str2.c_str(), std::max(strlen(str1.c_str()), strlen(str2.c_str()))));
#endif
}

bool EqualsIgnoringCase(const std::string& str1, const std::string& str2, unsigned int nCharacters)
{
    const size_t compLength = std::min(nCharacters, std::max(strlen(str1.c_str()), strlen(str2.c_str())));
#ifdef _MSC_VER
    return (0 == _strnicmp(str1.c_str(), str2.c_str(), compLength));
#else
    return (0 == strncasecmp(str1.c_str(), str2.c_str(), compLength));
#endif
}

void Trim(std::string& str, const char* characters)
{
    // trim from right
    size_t endpos = str.find_last_not_of(characters);
    size_t startpos = str.find_first_not_of(characters);
    if (std::string::npos != endpos)
    {
        str = str.substr(0, endpos + 1);
        str = str.substr(startpos);
    }

    // trim from left
    startpos = str.find_first_not_of(characters);
    if (std::string::npos != startpos)
    {
        str = str.substr(startpos);
    }
}

void Remove(std::string& str, char character)
{
    str.erase(std::remove(str.begin(), str.end(), character), str.end());
}

void MakeUpper(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](char c) { return char(::toupper(c)); });
}

bool Contains(const std::string& str, const std::string& substring)
{
    const char* pt = strstr(str.c_str(), substring.c_str());
    return (nullptr != pt);
}

void CleanString(const char *in, std::string &out)
{
    std::vector<char> clean;
    clean.reserve(strlen(in));

    for (unsigned int it = 0; it < strlen(in); ++it)
    {
        if ((unsigned char)in[it] >= 32)
        {
            clean.push_back(in[it]);
        }
    }

    out = std::string(begin(clean), end(clean));
}

std::string CleanString(const std::string &in)
{
    std::vector<char> okCharacters;
    okCharacters.reserve(in.size());

    for (size_t it = 0; it < in.size(); ++it)
    {
        if ((unsigned char)in[it] >= 32)
        {
            okCharacters.push_back(in[it]);
        }
    }
    return std::string(begin(okCharacters), end(okCharacters));
}

std::string SimplifyString(const std::string& str)
{
    std::string tempString;

    // Clean the string for non-printable characters
    CleanString(str.c_str(), tempString);

    // Make a local copy of the string
    size_t L = tempString.size();
    std::vector<char> buffer(L + 2);
    for (size_t it = 0; it < L; ++it) {
        buffer[it] = tempString.at((int)it);
    }
    buffer[L] = 0;

    // Check all characters in the string
    for (size_t i = 0; i < L; ++i) {
        // 1. Replace spaces, commas and dots with underscores
        if (buffer[i] == ' ' || buffer[i] == ',' || buffer[i] == '.') {
            buffer[i] = '_';
            continue;
        }

        // 2. Convert the character to lower-case
        buffer[i] = char(tolower(buffer[i]));

        // 3. Remove paranthesis...
        if (buffer[i] == '(' || buffer[i] == '[' || buffer[i] == '{' || buffer[i] == ')' || buffer[i] == ']' || buffer[i] == '}') {
            for (size_t j = i; j < L - 1; ++j) {
                buffer[j] = buffer[j + 1];
            }
            buffer[L - 1] = '\0';
            --L;
            i = i - 1;
            continue;
        }

        // 4. Check if there's any accent on the character
        if ((unsigned char)buffer[i] <= 127) {
            continue;
        }

        char c = buffer[i];

        if (c == '�' || c == '�' || c == '�' || c == '�' || c == '�')
            buffer[i] = 'a';
        else if (c == '�' || c == 'c')
            buffer[i] = 'c';
        else if (c == '�' || c == '�' || c == '�' || c == '�')
            buffer[i] = 'e';
        else if (c == '�' || c == '�' || c == '�' || c == '�')
            buffer[i] = 'i';
        else if (c == '�' || c == '�' || c == '�' || c == '�')
            buffer[i] = 'o';
        else if (c == '�' || c == '�' || c == '�' || c == '�')
            buffer[i] = 'u';
        else if (c == '�')
            buffer[i] = 'n';
    }

    return std::string(begin(buffer), end(buffer));
}