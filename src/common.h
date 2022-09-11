//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_COMMON_H
#define WEB_SERVER_COMMON_H

#ifdef _ENABLE_LIKELY_
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely
#define unlikely
#endif

#endif //WEB_SERVER_COMMON_H
