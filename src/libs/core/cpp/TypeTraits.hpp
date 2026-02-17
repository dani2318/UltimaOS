#pragma once

template <typename T>
constexpr bool IsSigned();

template <> constexpr bool IsSigned<char>()               { return true; };
template <> constexpr bool IsSigned<short>()              { return true; };
template <> constexpr bool IsSigned<int>()                { return true; };
template <> constexpr bool IsSigned<long>()               { return true; };
template <> constexpr bool IsSigned<long long>()          { return true; };
template <> constexpr bool IsSigned<unsigned char>()      { return false; };
template <> constexpr bool IsSigned<unsigned short>()     { return false; };
template <> constexpr bool IsSigned<unsigned int>()       { return false; };
template <> constexpr bool IsSigned<unsigned long>()      { return false; };
template <> constexpr bool IsSigned<unsigned long long>() { return false; };


template<typename T>
class MakeUnsigned{
    public:
        typedef T type;
};

template<>
class MakeUnsigned<char>{
    public:
        typedef unsigned char type;
};

template<>
class MakeUnsigned<short>{
    public:
        typedef unsigned short type;
};

template<>
class MakeUnsigned<int>{
    public:
        typedef unsigned int type;
};

template<>
class MakeUnsigned<long>{
    public:
        typedef unsigned long type;
};

template<>
class MakeUnsigned<long long>{
    public:
        typedef unsigned long long type;
};