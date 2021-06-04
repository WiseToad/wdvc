#ifndef PTR_H
#define PTR_H

template <typename P>
struct Ptr {
    using Type = P *;
};

template <typename P>
struct Ptr<P *> {
    using Type = P *;
};

template <typename P>
using PtrType = typename Ptr<P>::Type;

template <typename P>
P * ptr(P & p) {
    return &p;
}

template <typename P>
P * ptr(P * p) {
    return p;
}

#endif // PTR_H
