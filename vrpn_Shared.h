#ifndef VPRN_SHARED_H

#ifdef _WIN32

  /* from HP-UX */
struct timezone {
    int     tz_minuteswest; /* minutes west of Greenwich */
    int     tz_dsttime;     /* type of dst correction */
};

extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
#define close closesocket

#endif

#define SDI_SHARED_H
#endif /* SDI_SHARED_H */
