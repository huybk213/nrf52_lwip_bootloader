/*
 * lwip_fsm.c - {Link, IP} Control Protocol Finite State Machine.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

/*
 * @todo:
 * Randomize lwip_fsm id on link/init.
 * Deal with variable outgoing MTU.
 */

#if 0 /* UNUSED */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/lwip_fsm.h"

static void lwip_fsm_timeout (void *);
static void lwip_fsm_rconfreq(lwip_fsm *f, u_char id, u_char *inp, int len);
static void lwip_fsm_rconfack(lwip_fsm *f, int id, u_char *inp, int len);
static void lwip_fsm_rconfnakrej(lwip_fsm *f, int code, int id, u_char *inp, int len);
static void lwip_fsm_rtermreq(lwip_fsm *f, int id, u_char *p, int len);
static void lwip_fsm_rtermack(lwip_fsm *f);
static void lwip_fsm_rcoderej(lwip_fsm *f, u_char *inp, int len);
static void lwip_fsm_sconfreq(lwip_fsm *f, int retransmit);

#define PROTO_NAME(f)	((f)->callbacks->proto_name)

/*
 * lwip_fsm_init - Initialize lwip_fsm.
 *
 * Initialize lwip_fsm state.
 */
void lwip_fsm_init(lwip_fsm *f) {
    ppp_pcb *pcb = f->pcb;
    f->state = PPP_FSM_INITIAL;
    f->flags = 0;
    f->id = 0;				/* XXX Start with random id? */
    f->maxnakloops = pcb->settings.lwip_fsm_max_nak_loops;
    f->term_reason_len = 0;
}


/*
 * lwip_fsm_lowerup - The lower layer is up.
 */
void lwip_fsm_lowerup(lwip_fsm *f) {
    switch( f->state ){
    case PPP_FSM_INITIAL:
	f->state = PPP_FSM_CLOSED;
	break;

    case PPP_FSM_STARTING:
	if( f->flags & OPT_SILENT )
	    f->state = PPP_FSM_STOPPED;
	else {
	    /* Send an initial configure-request */
	    lwip_fsm_sconfreq(f, 0);
	    f->state = PPP_FSM_REQSENT;
	}
	break;

    default:
	FSMDEBUG(("%s: Up event in state %d!", PROTO_NAME(f), f->state));
	/* no break */
    }
}


/*
 * lwip_fsm_lowerdown - The lower layer is down.
 *
 * Cancel all timeouts and inform upper layers.
 */
void lwip_fsm_lowerdown(lwip_fsm *f) {
    switch( f->state ){
    case PPP_FSM_CLOSED:
	f->state = PPP_FSM_INITIAL;
	break;

    case PPP_FSM_STOPPED:
	f->state = PPP_FSM_STARTING;
	if( f->callbacks->starting )
	    (*f->callbacks->starting)(f);
	break;

    case PPP_FSM_CLOSING:
	f->state = PPP_FSM_INITIAL;
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	break;

    case PPP_FSM_STOPPING:
    case PPP_FSM_REQSENT:
    case PPP_FSM_ACKRCVD:
    case PPP_FSM_ACKSENT:
	f->state = PPP_FSM_STARTING;
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	break;

    case PPP_FSM_OPENED:
	if( f->callbacks->down )
	    (*f->callbacks->down)(f);
	f->state = PPP_FSM_STARTING;
	break;

    default:
	FSMDEBUG(("%s: Down event in state %d!", PROTO_NAME(f), f->state));
	/* no break */
    }
}


/*
 * lwip_fsm_open - Link is allowed to come up.
 */
void lwip_fsm_open(lwip_fsm *f) {
    switch( f->state ){
    case PPP_FSM_INITIAL:
	f->state = PPP_FSM_STARTING;
	if( f->callbacks->starting )
	    (*f->callbacks->starting)(f);
	break;

    case PPP_FSM_CLOSED:
	if( f->flags & OPT_SILENT )
	    f->state = PPP_FSM_STOPPED;
	else {
	    /* Send an initial configure-request */
	    lwip_fsm_sconfreq(f, 0);
	    f->state = PPP_FSM_REQSENT;
	}
	break;

    case PPP_FSM_CLOSING:
	f->state = PPP_FSM_STOPPING;
	/* fall through */
	/* no break */
    case PPP_FSM_STOPPED:
    case PPP_FSM_OPENED:
	if( f->flags & OPT_RESTART ){
	    lwip_fsm_lowerdown(f);
	    lwip_fsm_lowerup(f);
	}
	break;
    default:
	break;
    }
}

/*
 * terminate_layer - Start process of shutting down the FSM
 *
 * Cancel any timeout running, notify upper layers we're done, and
 * send a terminate-request message as configured.
 */
static void terminate_layer(lwip_fsm *f, int nextstate) {
    ppp_pcb *pcb = f->pcb;

    if( f->state != PPP_FSM_OPENED )
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
    else if( f->callbacks->down )
	(*f->callbacks->down)(f);	/* Inform upper layers we're down */

    /* Init restart counter and send Terminate-Request */
    f->retransmits = pcb->settings.lwip_fsm_max_term_transmits;
    lwip_fsm_sdata(f, TERMREQ, f->reqid = ++f->id,
	      (const u_char *) f->term_reason, f->term_reason_len);

    if (f->retransmits == 0) {
	/*
	 * User asked for no terminate requests at all; just close it.
	 * We've already fired off one Terminate-Request just to be nice
	 * to the peer, but we're not going to wait for a reply.
	 */
	f->state = nextstate == PPP_FSM_CLOSING ? PPP_FSM_CLOSED : PPP_FSM_STOPPED;
	if( f->callbacks->finished )
	    (*f->callbacks->finished)(f);
	return;
    }

    TIMEOUT(lwip_fsm_timeout, f, pcb->settings.lwip_fsm_timeout_time);
    --f->retransmits;

    f->state = nextstate;
}

/*
 * lwip_fsm_close - Start closing connection.
 *
 * Cancel timeouts and either initiate close or possibly go directly to
 * the PPP_FSM_CLOSED state.
 */
void lwip_fsm_close(lwip_fsm *f, const char *reason) {
    f->term_reason = reason;
    f->term_reason_len = (reason == NULL? 0: (u8_t)LWIP_MIN(strlen(reason), 0xFF) );
    switch( f->state ){
    case PPP_FSM_STARTING:
	f->state = PPP_FSM_INITIAL;
	break;
    case PPP_FSM_STOPPED:
	f->state = PPP_FSM_CLOSED;
	break;
    case PPP_FSM_STOPPING:
	f->state = PPP_FSM_CLOSING;
	break;

    case PPP_FSM_REQSENT:
    case PPP_FSM_ACKRCVD:
    case PPP_FSM_ACKSENT:
    case PPP_FSM_OPENED:
	terminate_layer(f, PPP_FSM_CLOSING);
	break;
    default:
	break;
    }
}


/*
 * lwip_fsm_timeout - Timeout expired.
 */
static void lwip_fsm_timeout(void *arg) {
    lwip_fsm *f = (lwip_fsm *) arg;
    ppp_pcb *pcb = f->pcb;

    switch (f->state) {
    case PPP_FSM_CLOSING:
    case PPP_FSM_STOPPING:
	if( f->retransmits <= 0 ){
	    /*
	     * We've waited for an ack long enough.  Peer probably heard us.
	     */
	    f->state = (f->state == PPP_FSM_CLOSING)? PPP_FSM_CLOSED: PPP_FSM_STOPPED;
	    if( f->callbacks->finished )
		(*f->callbacks->finished)(f);
	} else {
	    /* Send Terminate-Request */
	    lwip_fsm_sdata(f, TERMREQ, f->reqid = ++f->id,
		      (const u_char *) f->term_reason, f->term_reason_len);
	    TIMEOUT(lwip_fsm_timeout, f, pcb->settings.lwip_fsm_timeout_time);
	    --f->retransmits;
	}
	break;

    case PPP_FSM_REQSENT:
    case PPP_FSM_ACKRCVD:
    case PPP_FSM_ACKSENT:
	if (f->retransmits <= 0) {
	    ppp_warn("%s: timeout sending Config-Requests", PROTO_NAME(f));
	    f->state = PPP_FSM_STOPPED;
	    if( (f->flags & OPT_PASSIVE) == 0 && f->callbacks->finished )
		(*f->callbacks->finished)(f);

	} else {
	    /* Retransmit the configure-request */
	    if (f->callbacks->retransmit)
		(*f->callbacks->retransmit)(f);
	    lwip_fsm_sconfreq(f, 1);		/* Re-send Configure-Request */
	    if( f->state == PPP_FSM_ACKRCVD )
		f->state = PPP_FSM_REQSENT;
	}
	break;

    default:
	FSMDEBUG(("%s: Timeout event in state %d!", PROTO_NAME(f), f->state));
	/* no break */
    }
}


/*
 * lwip_fsm_input - Input packet.
 */
void lwip_fsm_input(lwip_fsm *f, u_char *inpacket, int l) {
    u_char *inp;
    u_char code, id;
    int len;

    /*
     * Parse header (code, id and length).
     * If packet too short, drop it.
     */
    inp = inpacket;
    if (l < HEADERLEN) {
	FSMDEBUG(("lwip_fsm_input(%x): Rcvd short header.", f->protocol));
	return;
    }
    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);
    if (len < HEADERLEN) {
	FSMDEBUG(("lwip_fsm_input(%x): Rcvd illegal length.", f->protocol));
	return;
    }
    if (len > l) {
	FSMDEBUG(("lwip_fsm_input(%x): Rcvd short packet.", f->protocol));
	return;
    }
    len -= HEADERLEN;		/* subtract header length */

    if( f->state == PPP_FSM_INITIAL || f->state == PPP_FSM_STARTING ){
	FSMDEBUG(("lwip_fsm_input(%x): Rcvd packet in state %d.",
		  f->protocol, f->state));
	return;
    }

    /*
     * Action depends on code.
     */
    switch (code) {
    case CONFREQ:
	lwip_fsm_rconfreq(f, id, inp, len);
	break;
    
    case CONFACK:
	lwip_fsm_rconfack(f, id, inp, len);
	break;
    
    case CONFNAK:
    case CONFREJ:
	lwip_fsm_rconfnakrej(f, code, id, inp, len);
	break;
    
    case TERMREQ:
	lwip_fsm_rtermreq(f, id, inp, len);
	break;
    
    case TERMACK:
	lwip_fsm_rtermack(f);
	break;
    
    case CODEREJ:
	lwip_fsm_rcoderej(f, inp, len);
	break;
    
    default:
	if( !f->callbacks->extcode
	   || !(*f->callbacks->extcode)(f, code, id, inp, len) )
	    lwip_fsm_sdata(f, CODEREJ, ++f->id, inpacket, len + HEADERLEN);
	break;
    }
}


/*
 * lwip_fsm_rconfreq - Receive Configure-Request.
 */
static void lwip_fsm_rconfreq(lwip_fsm *f, u_char id, u_char *inp, int len) {
    int code, reject_if_disagree;

    switch( f->state ){
    case PPP_FSM_CLOSED:
	/* Go away, we're closed */
	lwip_fsm_sdata(f, TERMACK, id, NULL, 0);
	return;
    case PPP_FSM_CLOSING:
    case PPP_FSM_STOPPING:
	return;

    case PPP_FSM_OPENED:
	/* Go down and restart negotiation */
	if( f->callbacks->down )
	    (*f->callbacks->down)(f);	/* Inform upper layers */
	lwip_fsm_sconfreq(f, 0);		/* Send initial Configure-Request */
	f->state = PPP_FSM_REQSENT;
	break;

    case PPP_FSM_STOPPED:
	/* Negotiation started by our peer */
	lwip_fsm_sconfreq(f, 0);		/* Send initial Configure-Request */
	f->state = PPP_FSM_REQSENT;
	break;
    default:
	break;
    }

    /*
     * Pass the requested configuration options
     * to protocol-specific code for checking.
     */
    if (f->callbacks->reqci){		/* Check CI */
	reject_if_disagree = (f->nakloops >= f->maxnakloops);
	code = (*f->callbacks->reqci)(f, inp, &len, reject_if_disagree);
    } else if (len)
	code = CONFREJ;			/* Reject all CI */
    else
	code = CONFACK;

    /* send the Ack, Nak or Rej to the peer */
    lwip_fsm_sdata(f, code, id, inp, len);

    if (code == CONFACK) {
	if (f->state == PPP_FSM_ACKRCVD) {
	    UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	    f->state = PPP_FSM_OPENED;
	    if (f->callbacks->up)
		(*f->callbacks->up)(f);	/* Inform upper layers */
	} else
	    f->state = PPP_FSM_ACKSENT;
	f->nakloops = 0;

    } else {
	/* we sent CONFACK or CONFREJ */
	if (f->state != PPP_FSM_ACKRCVD)
	    f->state = PPP_FSM_REQSENT;
	if( code == CONFNAK )
	    ++f->nakloops;
    }
}


/*
 * lwip_fsm_rconfack - Receive Configure-Ack.
 */
static void lwip_fsm_rconfack(lwip_fsm *f, int id, u_char *inp, int len) {
    ppp_pcb *pcb = f->pcb;

    if (id != f->reqid || f->seen_ack)		/* Expected id? */
	return;					/* Nope, toss... */
    if( !(f->callbacks->ackci? (*f->callbacks->ackci)(f, inp, len):
	  (len == 0)) ){
	/* Ack is bad - ignore it */
	ppp_error("Received bad configure-ack: %P", inp, len);
	return;
    }
    f->seen_ack = 1;
    f->rnakloops = 0;

    switch (f->state) {
    case PPP_FSM_CLOSED:
    case PPP_FSM_STOPPED:
	lwip_fsm_sdata(f, TERMACK, id, NULL, 0);
	break;

    case PPP_FSM_REQSENT:
	f->state = PPP_FSM_ACKRCVD;
	f->retransmits = pcb->settings.lwip_fsm_max_conf_req_transmits;
	break;

    case PPP_FSM_ACKRCVD:
	/* Huh? an extra valid Ack? oh well... */
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	lwip_fsm_sconfreq(f, 0);
	f->state = PPP_FSM_REQSENT;
	break;

    case PPP_FSM_ACKSENT:
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	f->state = PPP_FSM_OPENED;
	f->retransmits = pcb->settings.lwip_fsm_max_conf_req_transmits;
	if (f->callbacks->up)
	    (*f->callbacks->up)(f);	/* Inform upper layers */
	break;

    case PPP_FSM_OPENED:
	/* Go down and restart negotiation */
	if (f->callbacks->down)
	    (*f->callbacks->down)(f);	/* Inform upper layers */
	lwip_fsm_sconfreq(f, 0);		/* Send initial Configure-Request */
	f->state = PPP_FSM_REQSENT;
	break;
    default:
	break;
    }
}


/*
 * lwip_fsm_rconfnakrej - Receive Configure-Nak or Configure-Reject.
 */
static void lwip_fsm_rconfnakrej(lwip_fsm *f, int code, int id, u_char *inp, int len) {
    int ret;
    int treat_as_reject;

    if (id != f->reqid || f->seen_ack)	/* Expected id? */
	return;				/* Nope, toss... */

    if (code == CONFNAK) {
	++f->rnakloops;
	treat_as_reject = (f->rnakloops >= f->maxnakloops);
	if (f->callbacks->nakci == NULL
	    || !(ret = f->callbacks->nakci(f, inp, len, treat_as_reject))) {
	    ppp_error("Received bad configure-nak: %P", inp, len);
	    return;
	}
    } else {
	f->rnakloops = 0;
	if (f->callbacks->rejci == NULL
	    || !(ret = f->callbacks->rejci(f, inp, len))) {
	    ppp_error("Received bad configure-rej: %P", inp, len);
	    return;
	}
    }

    f->seen_ack = 1;

    switch (f->state) {
    case PPP_FSM_CLOSED:
    case PPP_FSM_STOPPED:
	lwip_fsm_sdata(f, TERMACK, id, NULL, 0);
	break;

    case PPP_FSM_REQSENT:
    case PPP_FSM_ACKSENT:
	/* They didn't agree to what we wanted - try another request */
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	if (ret < 0)
	    f->state = PPP_FSM_STOPPED;		/* kludge for stopping CCP */
	else
	    lwip_fsm_sconfreq(f, 0);		/* Send Configure-Request */
	break;

    case PPP_FSM_ACKRCVD:
	/* Got a Nak/reject when we had already had an Ack?? oh well... */
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	lwip_fsm_sconfreq(f, 0);
	f->state = PPP_FSM_REQSENT;
	break;

    case PPP_FSM_OPENED:
	/* Go down and restart negotiation */
	if (f->callbacks->down)
	    (*f->callbacks->down)(f);	/* Inform upper layers */
	lwip_fsm_sconfreq(f, 0);		/* Send initial Configure-Request */
	f->state = PPP_FSM_REQSENT;
	break;
    default:
	break;
    }
}


/*
 * lwip_fsm_rtermreq - Receive Terminate-Req.
 */
static void lwip_fsm_rtermreq(lwip_fsm *f, int id, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;

    switch (f->state) {
    case PPP_FSM_ACKRCVD:
    case PPP_FSM_ACKSENT:
	f->state = PPP_FSM_REQSENT;		/* Start over but keep trying */
	break;

    case PPP_FSM_OPENED:
	if (len > 0) {
	    ppp_info("%s terminated by peer (%0.*v)", PROTO_NAME(f), len, p);
	} else
	    ppp_info("%s terminated by peer", PROTO_NAME(f));
	f->retransmits = 0;
	f->state = PPP_FSM_STOPPING;
	if (f->callbacks->down)
	    (*f->callbacks->down)(f);	/* Inform upper layers */
	TIMEOUT(lwip_fsm_timeout, f, pcb->settings.lwip_fsm_timeout_time);
	break;
    default:
	break;
    }

    lwip_fsm_sdata(f, TERMACK, id, NULL, 0);
}


/*
 * lwip_fsm_rtermack - Receive Terminate-Ack.
 */
static void lwip_fsm_rtermack(lwip_fsm *f) {
    switch (f->state) {
    case PPP_FSM_CLOSING:
	UNTIMEOUT(lwip_fsm_timeout, f);
	f->state = PPP_FSM_CLOSED;
	if( f->callbacks->finished )
	    (*f->callbacks->finished)(f);
	break;
    case PPP_FSM_STOPPING:
	UNTIMEOUT(lwip_fsm_timeout, f);
	f->state = PPP_FSM_STOPPED;
	if( f->callbacks->finished )
	    (*f->callbacks->finished)(f);
	break;

    case PPP_FSM_ACKRCVD:
	f->state = PPP_FSM_REQSENT;
	break;

    case PPP_FSM_OPENED:
	if (f->callbacks->down)
	    (*f->callbacks->down)(f);	/* Inform upper layers */
	lwip_fsm_sconfreq(f, 0);
	f->state = PPP_FSM_REQSENT;
	break;
    default:
	break;
    }
}


/*
 * lwip_fsm_rcoderej - Receive an Code-Reject.
 */
static void lwip_fsm_rcoderej(lwip_fsm *f, u_char *inp, int len) {
    u_char code, id;

    if (len < HEADERLEN) {
	FSMDEBUG(("lwip_fsm_rcoderej: Rcvd short Code-Reject packet!"));
	return;
    }
    GETCHAR(code, inp);
    GETCHAR(id, inp);
    ppp_warn("%s: Rcvd Code-Reject for code %d, id %d", PROTO_NAME(f), code, id);

    if( f->state == PPP_FSM_ACKRCVD )
	f->state = PPP_FSM_REQSENT;
}


/*
 * lwip_fsm_protreject - Peer doesn't speak this protocol.
 *
 * Treat this as a catastrophic error (RXJ-).
 */
void lwip_fsm_protreject(lwip_fsm *f) {
    switch( f->state ){
    case PPP_FSM_CLOSING:
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	/* fall through */
	/* no break */
    case PPP_FSM_CLOSED:
	f->state = PPP_FSM_CLOSED;
	if( f->callbacks->finished )
	    (*f->callbacks->finished)(f);
	break;

    case PPP_FSM_STOPPING:
    case PPP_FSM_REQSENT:
    case PPP_FSM_ACKRCVD:
    case PPP_FSM_ACKSENT:
	UNTIMEOUT(lwip_fsm_timeout, f);	/* Cancel timeout */
	/* fall through */
	/* no break */
    case PPP_FSM_STOPPED:
	f->state = PPP_FSM_STOPPED;
	if( f->callbacks->finished )
	    (*f->callbacks->finished)(f);
	break;

    case PPP_FSM_OPENED:
	terminate_layer(f, PPP_FSM_STOPPING);
	break;

    default:
	FSMDEBUG(("%s: Protocol-reject event in state %d!",
		  PROTO_NAME(f), f->state));
	/* no break */
    }
}


/*
 * lwip_fsm_sconfreq - Send a Configure-Request.
 */
static void lwip_fsm_sconfreq(lwip_fsm *f, int retransmit) {
    ppp_pcb *pcb = f->pcb;
    struct pbuf *p;
    u_char *outp;
    int cilen;

    if( f->state != PPP_FSM_REQSENT && f->state != PPP_FSM_ACKRCVD && f->state != PPP_FSM_ACKSENT ){
	/* Not currently negotiating - reset options */
	if( f->callbacks->resetci )
	    (*f->callbacks->resetci)(f);
	f->nakloops = 0;
	f->rnakloops = 0;
    }

    if( !retransmit ){
	/* New request - reset retransmission counter, use new ID */
	f->retransmits = pcb->settings.lwip_fsm_max_conf_req_transmits;
	f->reqid = ++f->id;
    }

    f->seen_ack = 0;

    /*
     * Make up the request packet
     */
    if( f->callbacks->cilen && f->callbacks->addci ){
	cilen = (*f->callbacks->cilen)(f);
	if( cilen > pcb->peer_mru - HEADERLEN )
	    cilen = pcb->peer_mru - HEADERLEN;
    } else
	cilen = 0;

    p = pbuf_alloc(PBUF_RAW, (u16_t)(cilen + HEADERLEN + PPP_HDRLEN), PPP_CTRL_PBUF_TYPE);
    if(NULL == p)
        return;
    if(p->tot_len != p->len) {
        pbuf_free(p);
        return;
    }

    /* send the request to our peer */
    outp = (u_char*)p->payload;
    MAKEHEADER(outp, f->protocol);
    PUTCHAR(CONFREQ, outp);
    PUTCHAR(f->reqid, outp);
    PUTSHORT(cilen + HEADERLEN, outp);
    if (cilen != 0) {
	(*f->callbacks->addci)(f, outp, &cilen);
	LWIP_ASSERT("cilen == p->len - HEADERLEN - PPP_HDRLEN", cilen == p->len - HEADERLEN - PPP_HDRLEN);
    }

    ppp_write(pcb, p);

    /* start the retransmit timer */
    --f->retransmits;
    TIMEOUT(lwip_fsm_timeout, f, pcb->settings.lwip_fsm_timeout_time);
}


/*
 * lwip_fsm_sdata - Send some data.
 *
 * Used for all packets sent to our peer by this module.
 */
void lwip_fsm_sdata(lwip_fsm *f, u_char code, u_char id, const u_char *data, int datalen) {
    ppp_pcb *pcb = f->pcb;
    struct pbuf *p;
    u_char *outp;
    int outlen;

    /* Adjust length to be smaller than MTU */
    if (datalen > pcb->peer_mru - HEADERLEN)
	datalen = pcb->peer_mru - HEADERLEN;
    outlen = datalen + HEADERLEN;

    p = pbuf_alloc(PBUF_RAW, (u16_t)(outlen + PPP_HDRLEN), PPP_CTRL_PBUF_TYPE);
    if(NULL == p)
        return;
    if(p->tot_len != p->len) {
        pbuf_free(p);
        return;
    }

    outp = (u_char*)p->payload;
    if (datalen) /* && data != outp + PPP_HDRLEN + HEADERLEN)  -- was only for lwip_fsm_sconfreq() */
	MEMCPY(outp + PPP_HDRLEN + HEADERLEN, data, datalen);
    MAKEHEADER(outp, f->protocol);
    PUTCHAR(code, outp);
    PUTCHAR(id, outp);
    PUTSHORT(outlen, outp);
    ppp_write(pcb, p);
}

#endif /* PPP_SUPPORT */
