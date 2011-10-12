
#ifndef ARES_NAMESER_COMPAT_H
#define ARES_NAMESER_COMPAT_H

/* header file provided by liren@vivisimo.com */

#ifndef HAVE_ARPA_NAMESER_COMPAT_H

#define PACKETSZ         NS_PACKETSZ
#define MAXDNAME         NS_MAXDNAME
#define MAXCDNAME        NS_MAXCDNAME
#define MAXLABEL         NS_MAXLABEL
#define HFIXEDSZ         NS_HFIXEDSZ
#define QFIXEDSZ         NS_QFIXEDSZ
#define RRFIXEDSZ        NS_RRFIXEDSZ
#define INDIR_MASK       NS_CMPRSFLGS
#define NAMESERVER_PORT  NS_DEFAULTPORT

#define QUERY           ns_o_query

#define SERVFAIL        ns_r_servfail
#define NOTIMP          ns_r_notimpl
#define REFUSED         ns_r_refused
#undef NOERROR /* it seems this is already defined in winerror.h */
#define NOERROR         ns_r_noerror
#define FORMERR         ns_r_formerr
#define NXDOMAIN        ns_r_nxdomain

#define C_IN            ns_c_in
#define C_CHAOS         ns_c_chaos
#define C_HS            ns_c_hs
#define C_NONE          ns_c_none
#define C_ANY           ns_c_any

#define T_A             ns_t_a
#define T_NS            ns_t_ns
#define T_MD            ns_t_md
#define T_MF            ns_t_mf
#define T_CNAME         ns_t_cname
#define T_SOA           ns_t_soa
#define T_MB            ns_t_mb
#define T_MG            ns_t_mg
#define T_MR            ns_t_mr
#define T_NULL          ns_t_null
#define T_WKS           ns_t_wks
#define T_PTR           ns_t_ptr
#define T_HINFO         ns_t_hinfo
#define T_MINFO         ns_t_minfo
#define T_MX            ns_t_mx
#define T_TXT           ns_t_txt
#define T_RP            ns_t_rp
#define T_AFSDB         ns_t_afsdb
#define T_X25           ns_t_x25
#define T_ISDN          ns_t_isdn
#define T_RT            ns_t_rt
#define T_NSAP          ns_t_nsap
#define T_NSAP_PTR      ns_t_nsap_ptr
#define T_SIG           ns_t_sig
#define T_KEY           ns_t_key
#define T_PX            ns_t_px
#define T_GPOS          ns_t_gpos
#define T_AAAA          ns_t_aaaa
#define T_LOC           ns_t_loc
#define T_NXT           ns_t_nxt
#define T_EID           ns_t_eid
#define T_NIMLOC        ns_t_nimloc
#define T_SRV           ns_t_srv
#define T_ATMA          ns_t_atma
#define T_NAPTR         ns_t_naptr
#define T_TSIG          ns_t_tsig
#define T_IXFR          ns_t_ixfr
#define T_AXFR          ns_t_axfr
#define T_MAILB         ns_t_mailb
#define T_MAILA         ns_t_maila
#define T_ANY           ns_t_any

#endif /* HAVE_ARPA_NAMESER_COMPAT_H */

#endif /* ARES_NAMESER_COMPAT_H */
