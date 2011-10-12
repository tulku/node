
#ifndef ARES_ARES_NAMESER_H
#define ARES_ARES_NAMESER_H

#ifdef HAVE_ARPA_NAMESER_H
#  include <arpa/nameser.h>
#else
#  include "nameser.h"
#endif
#ifdef HAVE_ARPA_NAMESER_COMPAT_H
#  include <arpa/nameser_compat.h>
#else
#  include "nameser_compat.h"
#endif

#endif //ARES_ARES_NAMESER_H
