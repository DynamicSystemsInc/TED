/*
 * Copyright (c) 2004, 2015, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/*
 * auditwrite() - Construct and write user level records to the audit trail.
 */

/* Include common system files first */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <bsm/audit.h>
#include <bsm/adt.h>
#include <bsm/audit_record.h>
#include <bsm/libbsm.h>
#include "auditwrite.h"
#include <bsm/audit_uevents.h>
#include <priv.h>
#include <tsol/label.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <malloc.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <ucred.h>
#include <zone.h>

/*
 * invocation flags
 */

#define	AW_NO_FLAGS		((uint_t)0x00000000) /* No flags. */

#define	AW_ABORT_FLAG		((uint_t)0x0000010) /* Abort gracefully */
#define	AW_APPEND_FLAG		((uint_t)0x0000020) /* Append to save rec */
#define	AW_ATTRIB_FLAG		((uint_t)0x0000040) /* Got some attribs */
#define	AW_DEFAULTRD_FLAG	((uint_t)0x0000080) /* Use default rd */
#define	AW_DISCARDRD_FLAG	((uint_t)0x0000100) /* Get rid of one rec */
#define	AW_DISCARD_FLAG		((uint_t)0x0000200) /* Get rid of all recs */
#define	AW_EVENT_FLAG		((uint_t)0x0000400) /* Event type */
#define	AW_FLUSH_FLAG		((uint_t)0x0000800) /* Flush queued recs */
#define	AW_GETRD_FLAG		((uint_t)0x0001000) /* Get a descriptor */
#define	AW_INVOKED_BEFORE_FLAG	((uint_t)0x0002000) /* Been here before */
#define	AW_NOPRESELECT_FLAG	((uint_t)0x0004000) /* Don't preselect recs */
#define	AW_NOQUEUE_FLAG		((uint_t)0x0008000) /* Stop queueing. Flush */
#define	AW_NOSAVE_FLAG		((uint_t)0x0010000) /* Don't attach save buf */
#define	AW_NOSERVER_FLAG	((uint_t)0x0020000) /* Not a trusted srvr */
#define	AW_PRESELECT_FLAG	((uint_t)0x0040000) /* Preselect recs here */
#define	AW_QUEUE_FLAG		((uint_t)0x0080000) /* Buffer all records */
#define	AW_SAVERD_FLAG		((uint_t)0x0100000) /* Attach save buffer */
#define	AW_SERVER_FLAG		((uint_t)0x0200000) /* A trusted server */
#define	AW_USERD_FLAG		((uint_t)0x0400000) /* Use rec. descriptor */
#define	AW_WRITE_FLAG		((uint_t)0x0800000) /* Write to trail */
#define	AW_SAVEDONE		((uint_t)0x1000000) /* Context is saved, */
						    /* needs to be restored */

/*
 * required attribute flags
 * if these attributes are not included in the record, they are added
 */
#define	AW_REC_RETURN_FLAG	((char)0x0000001) /* return attribute */
#define	AW_REC_SUBJECT_FLAG	((char)0x0000002) /* subject attribute */

#define	AW_CTRLCMD_FLAGS \
	(AW_ABORT_FLAG		| AW_APPEND_FLAG	| AW_DISCARD_FLAG | \
	AW_DISCARDRD_FLAG	| AW_FLUSH_FLAG		| AW_GETRD_FLAG    | \
	AW_NOQUEUE_FLAG		| AW_NOSAVE_FLAG	| AW_NOSERVER_FLAG | \
	AW_PRESELECT_FLAG	| AW_NOPRESELECT_FLAG	| AW_QUEUE_FLAG | \
	AW_SAVERD_FLAG		| AW_SERVER_FLAG	| AW_USERD_FLAG | \
	AW_WRITE_FLAG		| AW_DEFAULTRD_FLAG)

#define	AW_NORMALCMD_FLAGS (AW_CTRLCMD_FLAGS & ~AW_USERD_FLAG)

/*
 * AW_NOUSERDCMD - these cmds may not be used with AW_USERD on auditwrite call.
 */
#define	AW_NOUSERDCMD_FLAGS \
	(AW_DISCARD_FLAG	| AW_DISCARDRD_FLAG	| AW_FLUSH_FLAG	| \
	AW_GETRD_FLAG		| AW_DEFAULTRD_FLAG	| AW_USERD_FLAG)

#define	AW_SUCCESS_RTN	(0)
#define	AW_ERR_RTN	(-1)

#define	AW_PARSE_ERR(flags, error)   \
{                                    \
	if (aw_iflags & flags) {     \
		aw_set_err(error);   \
		aw_restore();	     \
		return (AW_ERR_RTN); \
	}                            \
}

#define	AW_GEN_ERR(error)            \
{                                    \
	aw_set_err(error);           \
	aw_restore();		     \
	return (AW_ERR_RTN);         \
}

#define	AW_CMD_MIN	AW_END
#define	AW_CMD_MAX	AW_SUBJECT_EX

/*
 * Where control commands end and attribute commands begin.
 */
#define	AW_CMD_CUTOFF   AW_ACL

#define	AW_IS_CONTROL_CMD(cmd_num)     \
	((cmd_num) < AW_CMD_CUTOFF)

#define	AW_IS_DATA_CMD(cmd_num)        \
	((cmd_num) >= AW_CMD_CUTOFF)

/*
 * Currently, AW_MAX_REC_SIZE applies to both audit(2). This
 * will need to be changed as the limits associated w/audit(2) and
 * auditctl(2) change. It's defined here because the kernel imposed maximum
 * record size is not program visible.
 */
#define	AW_MAX_REC_SIZE		(0x8000)

/*
 * Number of record pointers to initially allocate
 */
#define	AW_NUM_RECP		(20)

/*
 * A record descriptor that is not allocated.
 */
#define	AW_NO_RD		(-1)

/*
 * This is a fakeout for auditwrite.  We don't have a
 * auditctl(A_AUDIT), so we'll emulate this with existing
 * functionality.
 */
#define	A_AUDIT			(10)

struct aw_context {
	uint_t static_flags;	/* static flags */
	au_mask_t pmask;	/* preselection mask */
	int save_rd;		/* associated save rd */
	int aw_errno;		/* errno for this rec */
};
typedef struct aw_context aw_context_t;

aw_context_t old_context;  	/* For saving previous state */
int old_cur_rd;

/* Data structures used to account for and queue audit record buffers */

struct aw_rec {
	aw_context_t context;	/* saved state for this rec */
	char aflags;		/* for required attribs */
	int len;		/* number of bytes in buffer */
	au_event_t event_id;	/* audit event identifier */
	au_emod_t event_mod;	/* audit event modifier */
	uint_t class;		/* audit event classes associated with event */
	caddr_t buf;		/* audit record buffer */
};
typedef struct aw_rec aw_rec_t;

static aw_rec_t **aw_recs;	/* dynam arr of aud rec ptrs */
static int aw_num_recs;   	/* # token pointers */
static caddr_t aw_queue;	/* a rec queue in user addr space */
static int aw_queue_hiwater;   	/* max number of bytes on queue */
static int aw_queue_bytes;	/* current # of bytes on queue */

static mutex_t mutex_auditwrite = DEFAULTMUTEX; /* Global audiwrite mutex */

/*
 * Command table. Contains the command and the number of args that follow
 * the command.
 *
 * If you add to the list, be aware that this is an array, and
 * in indexed accordingly.  I don't know why the cmd_number field
 * was ever used.
 */
static struct {
	int cmd_number;
	int cmd_numargs;
} aw_cmd_table[] = {
		{AW_END,		0},
/*
 * Control commands. These control the behavior of auditwrite.
 */
		{AW_ABORT,		0},
		{AW_APPEND,		0},
		{AW_DEFAULTRD,		0},
		{AW_DISCARD,		0},
		{AW_DISCARDRD,		1},
		{AW_FLUSH,		0},
		{AW_GETRD,		1},
		{AW_NOPRESELECT,	0},
		{AW_NOQUEUE,		0},
		{AW_NOSAVE,		0},
		{AW_NOSERVER,		0},
		{AW_PRESELECT,		1},
		{AW_QUEUE,		1},
		{AW_SAVERD,		1},
		{AW_SERVER,		0},
		{AW_USERD,		1},
		{AW_WRITE,		0},
		{AW_END,		0},  /* Reserved for future use */
		{AW_END,		0},
		{AW_END,		0},
		{AW_END,		0},
		{AW_END,		0},
		{AW_END,		0},
		{AW_END,		0},
/*
 * Attribute commands. These tell audiwrite how many attributes
 * of data to expect.
 */
		{AW_ACL,		0},  /* Don't support ACL's yet */
		{AW_ARG,		3},
		{AW_ATTR,		6},
		{AW_DATA,		4},
		{AW_EVENT,		1},
		{AW_EVENTNUM,		1},
		{AW_EXEC_ARGS,		1},
		{AW_EXEC_ENV,		1},
		{AW_EXIT,		2},
		{AW_GROUPS,		2},
		{AW_IN_ADDR,		1},
		{AW_IPC,		2},
		{AW_END,		0},	/* obsolete AW_IPC_PERM */
		{AW_IPORT,		1},
		{AW_OPAQUE,		2},
		{AW_PATH,		1},
		{AW_PROCESS,		8},
		{AW_RETURN,		2},
		{AW_SOCKET,		1},
		{AW_SUBJECT,		8},
		{AW_TEXT,		1},
		{AW_UAUTH,		1},
		{AW_CMD,		3},
		{AW_END,		0},  /* Reserved for future use */
		{AW_END,		0},

		{AW_END,		0},
		{AW_END,		0},
		{AW_LEVEL,		1},
		{AW_LIAISON,		1},
		{AW_PRIVILEGE,		2},
		{AW_SLABEL,		1},
		{AW_USEOFPRIV,		2},
		{AW_END,		0},  /* Reserved for future use */
		{AW_END,		0},
		{AW_END,		0},

		{AW_XATOM,		1},
		{AW_XCOLORMAP,		2},
		{AW_XCURSOR,		2},
		{AW_XFONT,		2},
		{AW_XGC,		2},
		{AW_END, /* OBS */	0},
		{AW_XPIXMAP,		2},
		{AW_XPROPERTY,		3},
		{AW_END, /* OBS */	0},
		{AW_XSELECT,		3},
		{AW_XWINDOW,		2},
		{AW_XCLIENT,		1},

		{AW_PROCESS_EX,		8},
		{AW_SUBJECT_EX,		8}
};

/* externally accessible data */
int aw_errno = AW_ERR_NO_ERROR;			/* error number */

static char *aw_errlist[] = {
	"No error",
	"Address invalid",
	"Memory allocation failure",
	"auditon(2) failed",
	"audit(2) failed",
	"Command incomplete",
	"Command invalid",
	"Command in effect",
	"Command not in effect",
	"More than one control command specified",
	"Event ID invalid",
	"Event ID not set",
	"getaudit(2) or getaudit_addr(2) failed",
	"Queue size invalid",
	"Record descriptor invalid",
	"Record too big",
	"No process label",
};

int aw_nerr = sizeof (aw_errlist) / sizeof (char *);

/* Data used by parsing routines and auditwrite */

static int *get_rd_p;		/* descriptor addr */
static int *save_rd_p;		/* descriptor addr */

static uint_t aw_iflags = AW_NO_FLAGS;	/* invocation line flags */

static uint_t aw_static_flags;   /* flags saved between invocations */

static int dflt_rd = AW_NO_RD;	/* default audit record descriptor */
				/* used if invoker doesn't specify one */

static int save_rd = AW_NO_RD;	/* audit record buffer attribs to prepend to */
				/* every audit record */

static int user_rd;		/* descriptor passed by user */

static int cur_rd = AW_NO_RD;	/* the current rd */

static au_mask_t pmask;		/* for preselection */

static int audit_policies;	/* cache the audit policies for the sake */
				/* of efficiency */

static void aw_abort(void);
static int aw_buf_append(caddr_t *b1, int *l1, caddr_t b2, int l2);
static int aw_buf_prepend(caddr_t *b1, int *l1, caddr_t b2, int l2);
static int aw_chk_addr(caddr_t p);
static int aw_chk_event_id(int rd);
static int aw_chk_print(char arg);
static int aw_chk_type(char arg);
static int aw_chk_rd(int rd);
static void aw_cleanup(void);
static char aw_cvrt_path(char *path, char **pathp);
static char aw_cvrt_print(char arg);
static char aw_cvrt_type(char arg);
static int aw_do_subject(int rd);
static int aw_do_write(void);
static int aw_write_cleanup(void);
static void aw_free(caddr_t p);
static void aw_free_tok(token_t *tokp);
static int aw_gen_rec(int param, va_list arglist);
static int aw_head(int rd);
static int aw_parse(int param, va_list arglist);
static int aw_preselect(int rd, au_mask_t *pmaskp);
static void aw_queue_dealloc(void);
static int aw_queue_flush(void);
static int aw_queue_write(int rd);
static int aw_rec_alloc(int *rdp);
#ifdef NOTYET
static int aw_rec_append(int to_rd, int from_rd);
#endif /* NOTYET */
static void aw_rec_dealloc(int rd);
static int aw_rec_prepend(int to_rd, int from_rd);
static int aw_return_attrib(int rd);
static void aw_set_err(int error);
static void aw_set_event(int rd, au_event_t event_id, uint_t class);
static int aw_init(void);
static int aw_set_context(int param, va_list arglist);
static void aw_restore(void);
static int aw_audit_write(int rd);
static int aw_auditctl_write(int rd);
static int auditctl(uint32_t command, uint32_t value, caddr_t data);
#ifdef DEBUG
static void aw_debuglog(char *string, int rc, int param, va_list arglist);
#endif

/* adt private */
extern	void adt_get_asid(const adt_session_data_t *, au_asid_t *);
extern	void adt_get_auid(const adt_session_data_t *, au_id_t *);
extern	void adt_get_mask(const adt_session_data_t *, au_mask_t *);
extern	void adt_get_termid(const adt_session_data_t *, au_tid_addr_t *);

/*
 * a w _ g e t _ a r g s ( )
 *
 * Gets arguments off the stack;
 *
 */
#define	aw_get_args(arglist, ad, numargs) \
{ \
	int i; \
	\
	for (i = 0; i < (numargs); i++) \
		(ad)[i] = (void *)va_arg((arglist), void *); \
}

/*
 * a w _ s k i p _ a r g ( )
 *
 * Skip args on the invocation line.
 *
 */
#define	aw_skip_args(arglist, numskips) \
{ \
	int i; \
	\
	for (i = 0; i < (numskips); i++) \
		(void) va_arg((arglist), void *); \
}

#define	NGROUPS		16	/* XXX - temporary */

int
audit(char *record, int length)
{
	return (syscall(SYS_auditsys, BSM_AUDIT, record, length));
}

/*
 * adr_uid
 */

void
adr_uid(adr_t *adr, uid_t *up, int count)
{
	int i;		/* index for counting */
	uid_t l;	/* value for shifting */

	for (; count-- > 0; up++) {
		for (i = 0, l = *(uint32_t *)up; i < 4; i++) {
			*adr->adr_now++ =
			    (char)((uint32_t)(l & 0xff000000) >> 24);
			l <<= 8;
		}
	}
}

/*
 * adr_ushort - pull out ushorts
 */
void
adr_ushort(adr_t *adr, ushort_t *sp, int count)
{

	for (; count-- > 0; sp++) {
		*adr->adr_now++ = (char)((*sp >> 8) & 0x00ff);
		*adr->adr_now++ = (char)(*sp & 0x00ff);
	}
}

token_t *au_to_arg(char n, char *text, uint32_t v);
#pragma weak au_to_arg = au_to_arg32
token_t *au_to_return(char number, uint32_t value);
#pragma weak au_to_return = au_to_return32

static token_t *au_to_exec(char **, char);

static token_t *
get_token(int s)
{
	token_t *token;	/* Resultant token */

	if ((token = (token_t *)malloc(sizeof (token_t))) == NULL)
		return (NULL);
	if ((token->tt_data = malloc(s)) == NULL) {
		free(token);
		return (NULL);
	}
	token->tt_size = s;
	token->tt_next = NULL;
	return (token);
}

/*
 * au_to_header
 * return s:
 *	pointer to header token.
 */
token_t *
au_to_header(au_event_t e_type, au_emod_t e_mod)
{
	adr_t adr;			/* adr memory stream header */
	token_t *token;			/* token pointer */
	char version = TOKEN_VERSION;	/* version of token family */
	int32_t byte_count;
	struct timeval tv;
#ifdef _LP64
	char data_header = AUT_HEADER64;	/* header for this token */

	token = get_token(2 * sizeof (char) + sizeof (int32_t) +
	    2 * sizeof (int64_t) + 2 * sizeof (short));
#else
	char data_header = AUT_HEADER32;

	token = get_token(2 * sizeof (char) + 3 * sizeof (int32_t) +
	    2 * sizeof (short));
#endif

	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);	/* token ID */
	adr_int32(&adr, &byte_count, 1);	/* length of audit record */
	adr_char(&adr, &version, 1);		/* version of audit tokens */
	adr_ushort(&adr, &e_type, 1);		/* event ID */
	adr_ushort(&adr, &e_mod, 1);		/* event ID modifier */
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tv, 2);	/* time & date */
#else
	adr_int32(&adr, (int32_t *)&tv, 2);	/* time & date */
#endif
	return (token);
}

/*
 * au_to_header_ex
 * return s:
 *	pointer to header token.
 */
token_t *
au_to_header_ex(au_event_t e_type, au_emod_t e_mod)
{
	adr_t adr;			/* adr memory stream header */
	token_t *token;			/* token pointer */
	char version = TOKEN_VERSION;	/* version of token family */
	int32_t byte_count;
	struct timeval tv;
	auditinfo_addr_t audit_info;
	au_tid_addr_t	*host_info = &audit_info.ai_termid;
#ifdef _LP64
	char data_header = AUT_HEADER64_EX;	/* header for this token */
#else
	char data_header = AUT_HEADER32_EX;
#endif

	/* If our host address can't be determined, revert to un-extended hdr */

	if (auditon(A_GETKAUDIT, (caddr_t)&audit_info,
	    sizeof (audit_info)) < 0)
		return (au_to_header(e_type, e_mod));

	if (host_info->at_type == AU_IPv6)
		if (IN6_IS_ADDR_UNSPECIFIED((in6_addr_t *)host_info->at_addr))
			return (au_to_header(e_type, e_mod));
	else
		if (host_info->at_addr[0] == htonl(INADDR_ANY))
			return (au_to_header(e_type, e_mod));

#ifdef _LP64
	token = get_token(2 * sizeof (char) + sizeof (int32_t) +
	    2 * sizeof (int64_t) + 2 * sizeof (short) +
	    sizeof (int32_t) + host_info->at_type);
#else
	token = get_token(2 * sizeof (char) + 3 * sizeof (int32_t) +
	    2 * sizeof (short) + sizeof (int32_t) + host_info->at_type);
#endif

	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);	/* token ID */
	adr_int32(&adr, &byte_count, 1);	/* length of audit record */
	adr_char(&adr, &version, 1);		/* version of audit tokens */
	adr_ushort(&adr, &e_type, 1);		/* event ID */
	adr_ushort(&adr, &e_mod, 1);		/* event ID modifier */
	adr_int32(&adr, (int32_t *)&host_info->at_type, 1);
	adr_char(&adr, (char *)host_info->at_addr,
	    (int)host_info->at_type);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tv, 2);	/* time & date */
#else
	adr_int32(&adr, (int32_t *)&tv, 2);	/* time & date */
#endif
	return (token);
}

/*
 * au_to_trailer
 * return s:
 *	pointer to a trailer token.
 */
token_t *
au_to_trailer(void)
{
	adr_t adr;				/* adr memory stream header */
	token_t *token;				/* token pointer */
	char data_header = AUT_TRAILER;		/* header for this token */
	short magic = (short)AUT_TRAILER_MAGIC;	/* trailer magic number */
	int32_t byte_count;

	token = get_token(sizeof (char) + sizeof (int32_t) + sizeof (short));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);	/* token ID */
	adr_short(&adr, &magic, 1);		/* magic number */
	adr_int32(&adr, &byte_count, 1);	/* length of audit record */

	return (token);
}

/*
 * au_to_arg32
 * return s:
 *	pointer to an argument token.
 */
token_t *
au_to_arg32(char n, char *text, uint32_t v)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_ARG32;	/* header for this token */
	short bytes;			/* length of string */

	bytes = strlen(text) + 1;

	token = get_token((int)(2 * sizeof (char) + sizeof (int32_t) +
	    sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);	/* token type */
	adr_char(&adr, &n, 1);			/* argument id */
	adr_int32(&adr, (int32_t *)&v, 1);	/* argument value */
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, text, bytes);

	return (token);
}

/*
 * au_to_arg64
 * return s:
 *	pointer to an argument token.
 */
token_t *
au_to_arg64(char n, char *text, uint64_t v)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_ARG64;	/* header for this token */
	short bytes;			/* length of string */

	bytes = strlen(text) + 1;

	token = get_token((int)(2 * sizeof (char) + sizeof (int64_t) +
	    sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);	/* token type */
	adr_char(&adr, &n, 1);			/* argument id */
	adr_int64(&adr, (int64_t *)&v, 1);	/* argument value */
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, text, bytes);

	return (token);
}


/*
 * au_to_attr
 * return s:
 *	pointer to an attribute token.
 */
token_t *
au_to_attr(struct stat64 *attr)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	int32_t value;
#ifdef _LP64
	char data_header = AUT_ATTR64;	/* header for this token */

	token = get_token(sizeof (char) +
	    sizeof (int32_t) * 4 +
	    sizeof (int64_t) * 2);
#else
	char data_header = AUT_ATTR32;

	token = get_token(sizeof (char) + sizeof (int32_t) * 5 +
	    sizeof (int64_t));
#endif
#if 0
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	value = (int32_t)attr->va_mode;
	adr_int32(&adr, &value, 1);
	value = (int32_t)attr->va_uid;
	adr_int32(&adr, &value, 1);
	value = (int32_t)attr->va_gid;
	adr_int32(&adr, &value, 1);
	adr_int32(&adr, (int32_t *)&(attr->va_fsid), 1);
	adr_int64(&adr, (int64_t *)&(attr->va_nodeid), 1);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&(attr->va_rdev), 1);
#else
	adr_int32(&adr, (int32_t *)&(attr->va_rdev), 1);
#endif
#endif
	return (token);
}

/*
 * au_to_data
 * return s:
 *	pointer to a data token.
 */
token_t *
au_to_data(char unit_print, char unit_type, char unit_count, char *p)
{
	adr_t adr;			/* adr memory stream header */
	token_t *token;			/* token pointer */
	char data_header = AUT_DATA;	/* header for this token */
	int byte_count;			/* number of bytes */

	if (p == NULL || unit_count < 1)
		return (NULL);

	/*
	 * Check validity of print type
	 */
	if (unit_print < AUP_BINARY || unit_print > AUP_STRING)
		return (NULL);

	switch (unit_type) {
	case AUR_SHORT:
		byte_count = unit_count * sizeof (short);
		break;
	case AUR_INT32:
		byte_count = unit_count * sizeof (int32_t);
		break;
	case AUR_INT64:
		byte_count = unit_count * sizeof (int64_t);
		break;
	/* case AUR_CHAR: */
	case AUR_BYTE:
		byte_count = unit_count * sizeof (char);
		break;
	default:
		return (NULL);
	}

	token = get_token((int)(4 * sizeof (char) + byte_count));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, &unit_print, 1);
	adr_char(&adr, &unit_type, 1);
	adr_char(&adr, &unit_count, 1);

	switch (unit_type) {
	case AUR_SHORT:
		/* LINTED */
		adr_short(&adr, (short *)p, unit_count);
		break;
	case AUR_INT32:
		/* LINTED */
		adr_int32(&adr, (int32_t *)p, unit_count);
		break;
	case AUR_INT64:
		/* LINTED */
		adr_int64(&adr, (int64_t *)p, unit_count);
		break;
	/* case AUR_CHAR: */
	case AUR_BYTE:
		adr_char(&adr, p, unit_count);
		break;
	}

	return (token);
}

/*
 * au_to_privset
 *
 * priv_type (LIMIT, INHERIT...) is the first string and privilege
 * in translated into the second string.  The format is as follows:
 *
 *	token id	adr_char
 *	priv type	adr_string (short, string)
 *	priv set	adr_string (short, string)
 *
 * return s:
 *	pointer to a AUT_PRIV token.
 */
token_t *
au_to_privset(const char *priv_type, const priv_set_t *privilege)
{
	token_t	*token;			/* local token */
	adr_t	adr;			/* adr memory stream header */
	char	data_header = AUT_PRIV;	/* header for this token */
	short	t_bytes;		/* length of type string */
	short	p_bytes;		/* length of privilege string */
	char	*priv_string;		/* privilege string */

	t_bytes = strlen(priv_type) + 1;

	if ((privilege == NULL) || (priv_string =
	    priv_set_to_str(privilege, ',',
	    PRIV_STR_LIT)) == NULL)
		return (NULL);

	p_bytes = strlen(priv_string) + 1;

	token = get_token((int)(sizeof (char) + (2 * sizeof (short)) + t_bytes
	    + p_bytes));
	if (token == NULL)
		return (NULL);

	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &t_bytes, 1);
	adr_char(&adr, (char *)priv_type, t_bytes);
	adr_short(&adr, &p_bytes, 1);
	adr_char(&adr, priv_string, p_bytes);

	free(priv_string);

	return (token);
}

/*
 * au_to_process
 * return s:
 *	pointer to a process token.
 */

token_t *
au_to_process(au_id_t auid, uid_t euid, gid_t egid, uid_t ruid, gid_t rgid,
    pid_t pid, au_asid_t sid, au_tid_t *tid)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
#ifdef _LP64
	char data_header = AUT_PROCESS64;	/* header for this token */

	token = get_token(sizeof (char) + 8 * sizeof (int32_t) +
	    sizeof (int64_t));
#else
	char data_header = AUT_PROCESS32;

	token = get_token(sizeof (char) + 9 * sizeof (int32_t));
#endif

	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&auid, 1);
	adr_int32(&adr, (int32_t *)&euid, 1);
	adr_int32(&adr, (int32_t *)&egid, 1);
	adr_int32(&adr, (int32_t *)&ruid, 1);
	adr_int32(&adr, (int32_t *)&rgid, 1);
	adr_int32(&adr, (int32_t *)&pid, 1);
	adr_int32(&adr, (int32_t *)&sid, 1);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tid->port, 1);
#else
	adr_int32(&adr, (int32_t *)&tid->port, 1);
#endif
	adr_int32(&adr, (int32_t *)&tid->machine, 1);

	return (token);
}

/*
 * au_to_process_ex
 * return s:
 *	pointer to a process_ex token.
 */
token_t *
au_to_process_ex(au_id_t auid, uid_t euid, gid_t egid, uid_t ruid, gid_t rgid,
    pid_t pid, au_asid_t sid, au_tid_addr_t *tid)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header;		/* header for this token */

#ifdef _LP64
	if (tid->at_type == AU_IPv6) {
		data_header = AUT_PROCESS64_EX;
		token = get_token(sizeof (char) + sizeof (int64_t) +
		    12 * sizeof (int32_t));
	} else {
		data_header = AUT_PROCESS64;
		token = get_token(sizeof (char) + sizeof (int64_t) +
		    8 * sizeof (int32_t));
	}
#else
	if (tid->at_type == AU_IPv6) {
		data_header = AUT_PROCESS32_EX;
		token = get_token(sizeof (char) + 13 * sizeof (int32_t));
	} else {
		data_header = AUT_PROCESS32;
		token = get_token(sizeof (char) + 9 * sizeof (int32_t));
	}
#endif
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&auid, 1);
	adr_int32(&adr, (int32_t *)&euid, 1);
	adr_int32(&adr, (int32_t *)&egid, 1);
	adr_int32(&adr, (int32_t *)&ruid, 1);
	adr_int32(&adr, (int32_t *)&rgid, 1);
	adr_int32(&adr, (int32_t *)&pid, 1);
	adr_int32(&adr, (int32_t *)&sid, 1);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tid->at_port, 1);
#else
	adr_int32(&adr, (int32_t *)&tid->at_port, 1);
#endif
	if (tid->at_type == AU_IPv6) {
		adr_int32(&adr, (int32_t *)&tid->at_type, 1);
		adr_char(&adr, (char *)tid->at_addr, 16);
	} else {
		adr_char(&adr, (char *)tid->at_addr, 4);
	}

	return (token);
}

/*
 * au_to_seq
 * return s:
 *	pointer to token chain containing a sequence token
 */
token_t *
au_to_seq(int audit_count)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_SEQ;	/* header for this token */

	token = get_token(sizeof (char) + sizeof (int32_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&audit_count, 1);

	return (token);
}

/*
 * au_to_socket
 * return s:
 *	pointer to mbuf chain containing a socket token.
 */
token_t *
au_to_socket(struct oldsocket *so)
{
	adr_t adr;
	token_t *token;
	char data_header = AUT_SOCKET;
	struct inpcb *inp = so->so_pcb;

	token = get_token(sizeof (char) + sizeof (short) * 3 +
	    sizeof (int32_t) * 2);
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, (short *)&so->so_type, 1);
	adr_short(&adr, (short *)&inp->inp_lport, 1);
	adr_int32(&adr, (int32_t *)&inp->inp_laddr, 1);
	adr_short(&adr, (short *)&inp->inp_fport, 1);
	adr_int32(&adr, (int32_t *)&inp->inp_faddr, 1);

	return (token);
}

/*
 * au_to_subject
 * return s:
 *	pointer to a process token.
 */

token_t *
au_to_subject(au_id_t auid, uid_t euid, gid_t egid, uid_t ruid, gid_t rgid,
    pid_t pid, au_asid_t sid, au_tid_t *tid)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
#ifdef _LP64
	char data_header = AUT_SUBJECT64;	/* header for this token */

	token = get_token(sizeof (char) + sizeof (int64_t) +
	    8 * sizeof (int32_t));
#else
	char data_header = AUT_SUBJECT32;

	token = get_token(sizeof (char) + 9 * sizeof (int32_t));
#endif

	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&auid, 1);
	adr_int32(&adr, (int32_t *)&euid, 1);
	adr_int32(&adr, (int32_t *)&egid, 1);
	adr_int32(&adr, (int32_t *)&ruid, 1);
	adr_int32(&adr, (int32_t *)&rgid, 1);
	adr_int32(&adr, (int32_t *)&pid, 1);
	adr_int32(&adr, (int32_t *)&sid, 1);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tid->port, 1);
#else
	adr_int32(&adr, (int32_t *)&tid->port, 1);
#endif
	adr_int32(&adr, (int32_t *)&tid->machine, 1);

	return (token);
}

/*
 * au_to_subject_ex
 * return s:
 *	pointer to a process token.
 */

token_t *
au_to_subject_ex(au_id_t auid, uid_t euid, gid_t egid, uid_t ruid, gid_t rgid,
    pid_t pid, au_asid_t sid, au_tid_addr_t *tid)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
#ifdef _LP64
	char data_header;		/* header for this token */

	if (tid->at_type == AU_IPv6) {
		data_header = AUT_SUBJECT64_EX;
		token = get_token(sizeof (char) + sizeof (int64_t) +
		    12 * sizeof (int32_t));
	} else {
		data_header = AUT_SUBJECT64;
		token = get_token(sizeof (char) + sizeof (int64_t) +
		    8 * sizeof (int32_t));
	}
#else
	char data_header;		/* header for this token */

	if (tid->at_type == AU_IPv6) {
		data_header = AUT_SUBJECT32_EX;
		token = get_token(sizeof (char) + 13 * sizeof (int32_t));
	} else {
		data_header = AUT_SUBJECT32;
		token = get_token(sizeof (char) + 9 * sizeof (int32_t));
	}
#endif

	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&auid, 1);
	adr_int32(&adr, (int32_t *)&euid, 1);
	adr_int32(&adr, (int32_t *)&egid, 1);
	adr_int32(&adr, (int32_t *)&ruid, 1);
	adr_int32(&adr, (int32_t *)&rgid, 1);
	adr_int32(&adr, (int32_t *)&pid, 1);
	adr_int32(&adr, (int32_t *)&sid, 1);
#ifdef _LP64
	adr_int64(&adr, (int64_t *)&tid->at_port, 1);
#else
	adr_int32(&adr, (int32_t *)&tid->at_port, 1);
#endif
	if (tid->at_type == AU_IPv6) {
		adr_int32(&adr, (int32_t *)&tid->at_type, 1);
		adr_char(&adr, (char *)tid->at_addr, 16);
	} else {
		adr_char(&adr, (char *)tid->at_addr, 4);
	}

	return (token);
}

/*
 * au_to_me
 * return s:
 *	pointer to a process token.
 */

token_t *
au_to_me(void)
{
#if 0
	auditinfo_addr_t info;

	if (getaudit_addr(&info, sizeof (info)))
		return (NULL);
	return (au_to_subject_ex(info.ai_auid, geteuid(), getegid(), getuid(),
	    getgid(), getpid(), info.ai_asid, &info.ai_termid));
#endif
	return NULL;
}
/*
 * au_to_text
 * return s:
 *	pointer to a text token.
 */
token_t *
au_to_text(char *text)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_TEXT;	/* header for this token */
	short bytes;			/* length of string */

	bytes = strlen(text) + 1;
	token = get_token((int)(sizeof (char) + sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, text, bytes);

	return (token);
}

/*
 * au_to_path
 * return s:
 *	pointer to a path token.
 */
token_t *
au_to_path(char *path)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_PATH;	/* header for this token */
	short bytes;			/* length of string */

	bytes = (short)strlen(path) + 1;

	token = get_token((int)(sizeof (char) +  sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, path, bytes);

	return (token);
}

/*
 * au_to_cmd
 * return s:
 *	pointer to an command line argument token
 */
token_t *
au_to_cmd(int argc, char **argv, char **envp)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_CMD;	/* header for this token */
	short len = 0;
	short cnt = 0;
	short envc = 0;
	short largc = (short)argc;

	/*
	 * one char for the header, one short for argc,
	 * one short for # envp strings.
	 */
	len = sizeof (char) + sizeof (short) + sizeof (short);

	/* get sizes of strings */

	for (cnt = 0; cnt < argc; cnt++) {
		len += (short)sizeof (short) + (short)(strlen(argv[cnt]) + 1);
	}

	if (envp != NULL) {
		for (envc = 0; envp[envc] != NULL; envc++) {
			len += (short)sizeof (short) +
			    (short)(strlen(envp[envc]) + 1);
		}
	}

	token = get_token(len);
	if (token == NULL)
		return (NULL);

	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);

	adr_short(&adr, &largc, 1);

	for (cnt = 0; cnt < argc; cnt++) {
		len = (short)(strlen(argv[cnt]) + 1);
		adr_short(&adr, &len, 1);
		adr_char(&adr, argv[cnt], len);
	}

	adr_short(&adr, &envc, 1);

	for (cnt = 0; cnt < envc; cnt++) {
		len = (short)(strlen(envp[cnt]) + 1);
		adr_short(&adr, &len, 1);
		adr_char(&adr, envp[cnt], len);
	}

	return (token);
}

/*
 * au_to_exit
 * return s:
 *	pointer to a exit value token.
 */
token_t *
au_to_exit(int retval, int err)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_EXIT;	/* header for this token */

	token = get_token(sizeof (char) + (2 * sizeof (int32_t)));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&retval, 1);
	adr_int32(&adr, (int32_t *)&err, 1);

	return (token);
}

/*
 * au_to_return
 * return s:
 *	pointer to a return  value token.
 */
token_t *
au_to_return32(char number, uint32_t value)
{
	token_t *token;				/* local token */
	adr_t adr;				/* adr memory stream header */
	char data_header = AUT_RETURN32;	/* header for this token */

	token = get_token(2 * sizeof (char) + sizeof (int32_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, &number, 1);
	adr_int32(&adr, (int32_t *)&value, 1);

	return (token);
}

/*
 * au_to_return
 * return s:
 *	pointer to a return  value token.
 */
token_t *
au_to_return64(char number, uint64_t value)
{
	token_t *token;				/* local token */
	adr_t adr;				/* adr memory stream header */
	char data_header = AUT_RETURN64;	/* header for this token */

	token = get_token(2 * sizeof (char) + sizeof (int64_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, &number, 1);
	adr_int64(&adr, (int64_t *)&value, 1);

	return (token);
}


/*
 * au_to_opaque
 * return s:
 *	pointer to a opaque token.
 */
token_t *
au_to_opaque(char *opaque, short bytes)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_OPAQUE;	/* header for this token */

	if (bytes < 1)
		return (NULL);

	token = get_token((int)(sizeof (char) + sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, opaque, bytes);

	return (token);
}

/*
 * au_to_in_addr
 * return s:
 *	pointer to an internet address token
 */
token_t *
au_to_in_addr(struct in_addr *internet_addr)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_IN_ADDR;	/* header for this token */

	token = get_token(sizeof (char) + sizeof (struct in_addr));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, (char *)internet_addr, sizeof (struct in_addr));

	return (token);
}

/*
 * au_to_in_addr_ex
 * return s:
 *	pointer to an internet extended token
 */
token_t *
au_to_in_addr_ex(struct in6_addr *addr)
{
	token_t *token;
	adr_t adr;

	if (IN6_IS_ADDR_V4MAPPED(addr)) {
		ipaddr_t in4;

		/*
		 * An IPv4-mapped IPv6 address is really an IPv4 address
		 * in IPv6 format.
		 */

		IN6_V4MAPPED_TO_IPADDR(addr, in4);
		return (au_to_in_addr((struct in_addr *)&in4));

	} else {
		char data_header = AUT_IN_ADDR_EX;
		int32_t	type = AU_IPv6;

		if ((token = get_token(sizeof (char) + sizeof (int32_t) +
		    sizeof (struct in6_addr))) == NULL) {
			return (NULL);
		}

		adr_start(&adr, token->tt_data);
		adr_char(&adr, &data_header, 1);
		adr_int32(&adr, &type, 1);
		adr_char(&adr, (char *)addr, sizeof (struct in6_addr));
	}

	return (token);
}

/*
 * au_to_iport
 * return s:
 *	pointer to token chain containing a ip port address token
 */
token_t *
au_to_iport(ushort_t iport)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_IPORT;	/* header for this token */

	token = get_token(sizeof (char) + sizeof (short));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, (short *)&iport, 1);

	return (token);
}

token_t *
au_to_ipc(char type, int id)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_IPC;	/* header for this token */

	token = get_token((2 * sizeof (char)) + sizeof (int32_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, &type, 1);
	adr_int32(&adr, (int32_t *)&id, 1);

	return (token);
}

/*
 * au_to_tid
 *
 * output format depends on type; at present only IP v4 and v6 addresses
 * are defined.
 *
 * IPv4 -- tid type, 16 bit remote port, 16 bit local port, ip type,
 *		32 bit IP address.
 * IPv6 -- tid type, 16 bit remote port, 16 bit local port, ip type,
 *		4 x 32 bit IP address.
 *
 */
token_t *
au_to_tid(au_generic_tid_t *tid)
{
	char		data_header = AUT_TID;	/* header for this token */
	adr_t		adr;			/* adr memory stream header */
	token_t		*token;			/* local token */
	au_ip_t		*ip;

	switch (tid->gt_type) {
	case AU_IPADR:
		ip = &(tid->gt_adr.at_ip);
		token = get_token((int)(2 * sizeof (char) + 2 * sizeof (short) +
		    sizeof (uint32_t) + ip->at_type));
		if (token == NULL)
			return (NULL);

		adr_start(&adr, token->tt_data);
		adr_char(&adr, &data_header, 1);
		adr_char(&adr, (char *)&(tid->gt_type), 1);
		adr_short(&adr, (short *)&(ip->at_r_port), 1);
		adr_short(&adr, (short *)&(ip->at_l_port), 1);
		adr_int32(&adr, (int32_t *)&(ip->at_type), 1);

		adr_char(&adr, (char *)ip->at_addr, ip->at_type);

		break;
	default:
		return (NULL);
	}
	return (token);
}

/*
 * The Modifier tokens
 */

/*
 * au_to_groups
 * return s:
 *	pointer to a group list token.
 *
 * This function is obsolete.  Please use au_to_newgroups.
 */
token_t *
au_to_groups(int *groups)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_GROUPS;	/* header for this token */

	token = get_token(sizeof (char) + NGROUPS * sizeof (int32_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)groups, NGROUPS);

	return (token);
}

/*
 * au_to_newgroups
 * return s:
 *	pointer to a group list token.
 */
token_t *
au_to_newgroups(int n, gid_t *groups)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_NEWGROUPS;	/* header for this token */
	short n_groups;

	if (n < 0 || n > SHRT_MAX || groups == NULL)
		return (NULL);
	token = get_token(sizeof (char) + sizeof (short) + n * sizeof (gid_t));
	if (token == NULL)
		return (NULL);
	n_groups = (short)n;
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &n_groups, 1);
	adr_int32(&adr, (int32_t *)groups, n_groups);

	return (token);
}

/*
 * au_to_exec_args
 * returns:
 *	pointer to an exec args token.
 */
token_t *
au_to_exec_args(char **argv)
{
	return (au_to_exec(argv, AUT_EXEC_ARGS));
}

/*
 * au_to_exec_env
 * returns:
 *	pointer to an exec args token.
 */
token_t *
au_to_exec_env(char **envp)
{
	return (au_to_exec(envp, AUT_EXEC_ENV));
}

/*
 * au_to_exec
 * returns:
 *	pointer to an exec args token.
 */
static token_t *
au_to_exec(char **v, char data_header)
{
	token_t *token;
	adr_t adr;
	char **p;
	int32_t n = 0;
	int len = 0;

	for (p = v; *p != NULL; p++) {
		len += strlen(*p) + 1;
		n++;
	}
	token = get_token(sizeof (char) + sizeof (int32_t) + len);
	if (token == (token_t *)NULL)
		return ((token_t *)NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, &n, 1);
	for (p = v; *p != NULL; p++) {
		adr_char(&adr, *p, strlen(*p) + 1);
	}
	return (token);
}

/*
 * au_to_uauth
 * return s:
 *	pointer to a uauth token.
 */
token_t *
au_to_uauth(char *text)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_UAUTH;	/* header for this token */
	short bytes;			/* length of string */

	bytes = strlen(text) + 1;

	token = get_token((int)(sizeof (char) + sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, text, bytes);

	return (token);
}

/*
 * au_to_upriv
 * return s:
 *	pointer to a use of privilege token.
 */
token_t *
au_to_upriv(char sorf, char *priv)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_UPRIV;	/* header for this token */
	short bytes;			/* length of string */

	bytes = strlen(priv) + 1;

	token = get_token(sizeof (char) + sizeof (char) + sizeof (short) +
	    bytes);
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, &sorf, 1);	/* success/failure */
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, priv, bytes);

	return (token);
}

/*
 * au_to_xatom
 * return s:
 *	pointer to a xatom token.
 */
token_t *
au_to_xatom(char *atom)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_XATOM;	/* header for this token */
	short len;

	len = strlen(atom) + 1;

	token = get_token(sizeof (char) + sizeof (short) + len);
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, (short *)&len, 1);
	adr_char(&adr, atom, len);

	return (token);
}

/*
 * au_to_xselect
 * return s:
 *	pointer to a X select token.
 */
token_t *
au_to_xselect(char *propname, char *proptype, char *windata)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_XSELECT;	/* header for this token */
	short proplen;
	short typelen;
	short datalen;

	proplen = strlen(propname) + 1;
	typelen = strlen(proptype) + 1;
	datalen = strlen(windata) + 1;

	token = get_token(sizeof (char) + (sizeof (short) * 3) +
	    proplen + typelen + datalen);
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &proplen, 1);
	adr_char(&adr, propname, proplen);
	adr_short(&adr, &typelen, 1);
	adr_char(&adr, proptype, typelen);
	adr_short(&adr, &datalen, 1);
	adr_char(&adr, windata, datalen);

	return (token);
}

/*
 * x_common
 * return s:
 *	pointer to a common X token.
 */

static token_t *
x_common(char data_header, int32_t xid, uid_t cuid)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */

	token = get_token(sizeof (char) + sizeof (int32_t) + sizeof (uid_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, &xid, 1);
	adr_uid(&adr, &cuid, 1);

	return (token);
}

/*
 * au_to_xcolormap
 * return s:
 *	pointer to a X Colormap token.
 */

token_t *
au_to_xcolormap(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XCOLORMAP, xid, cuid));
}

/*
 * au_to_xcursor
 * return s:
 *	pointer to a X Cursor token.
 */

token_t *
au_to_xcursor(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XCURSOR, xid, cuid));
}

/*
 * au_to_xfont
 * return s:
 *	pointer to a X Font token.
 */

token_t *
au_to_xfont(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XFONT, xid, cuid));
}

/*
 * au_to_xgc
 * return s:
 *	pointer to a X Graphic Context token.
 */

token_t *
au_to_xgc(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XGC, xid, cuid));
}

/*
 * au_to_xpixmap
 * return s:
 *	pointer to a X Pixal Map token.
 */

token_t *
au_to_xpixmap(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XPIXMAP, xid, cuid));
}

/*
 * au_to_xwindow
 * return s:
 *	pointer to a X Window token.
 */

token_t *
au_to_xwindow(int32_t xid, uid_t cuid)
{
	return (x_common(AUT_XWINDOW, xid, cuid));
}

/*
 * au_to_xproperty
 * return s:
 *	pointer to a X Property token.
 */

token_t *
au_to_xproperty(int32_t xid, uid_t cuid, char *propname)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_XPROPERTY;	/* header for this token */
	short proplen;

	proplen = strlen(propname) + 1;

	token = get_token(sizeof (char) + sizeof (int32_t) + sizeof (uid_t) +
	    sizeof (short) + proplen);
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, &xid, 1);
	adr_uid(&adr, &cuid, 1);
	adr_short(&adr, &proplen, 1);
	adr_char(&adr, propname, proplen);

	return (token);
}

/*
 * au_to_xclient
 * return s:
 *	pointer to a X Client token
 */

token_t *
au_to_xclient(uint32_t client)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_XCLIENT;	/* header for this token */

	token = get_token(sizeof (char) + sizeof (uint32_t));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_int32(&adr, (int32_t *)&client, 1);

	return (token);
}

/*
 * au_to_label
 * return s:
 *	pointer to a label token.
 */
token_t *
au_to_label(m_label_t *label)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_LABEL;	/* header for this token */
	size32_t llen = blabel_size();

	token = get_token(sizeof (char) + llen);
	if (token == NULL) {
		return (NULL);
	} else if (label == NULL) {
		free(token);
		return (NULL);
	}
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_char(&adr, (char *)label, llen);

	return (token);
}

/*
 * au_to_mylabel
 * return s:
 *	pointer to a label token.
 */
token_t *
au_to_mylabel(void)
{
	ucred_t		*uc;
	token_t		*token;

	if ((uc = ucred_get(P_MYID)) == NULL) {
		return (NULL);
	}

	token = au_to_label(ucred_getlabel(uc));
	ucred_free(uc);
	return (token);
}

/*
 * au_to_zonename
 * return s:
 *	pointer to a zonename token.
 */
token_t *
au_to_zonename(char *name)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_ZONENAME;	/* header for this token */
	short bytes;			/* length of string */

	if (name == NULL)
		return (NULL);

	bytes = strlen(name) + 1;
	token = get_token((int)(sizeof (char) + sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, name, bytes);

	return (token);
}

/*
 * au_to_fmri
 * return s:
 *	pointer to a fmri token.
 */
token_t *
au_to_fmri(char *fmri)
{
	token_t *token;			/* local token */
	adr_t adr;			/* adr memory stream header */
	char data_header = AUT_FMRI;	/* header for this token */
	short bytes;			/* length of string */

	if (fmri == NULL)
		return (NULL);

	bytes = strlen(fmri) + 1;
	token = get_token((int)(sizeof (char) + sizeof (short) + bytes));
	if (token == NULL)
		return (NULL);
	adr_start(&adr, token->tt_data);
	adr_char(&adr, &data_header, 1);
	adr_short(&adr, &bytes, 1);
	adr_char(&adr, fmri, bytes);

	return (token);
}

void
audit_use_of_x_privilege(uid_t uid, uid_t euid, gid_t gid, gid_t egid, au_id_t auid,
	m_label_t *label, pid_t pid, int xevent_num, priv_set_t *priv) {
	
	adt_event_data_t *event;
	au_asid_t asid;
	au_tid_addr_t tid;
	token_t *token1;
	token_t *token2;
	token_t *token3;
	static int rd = -1;
	char *privilege;

	static adt_session_data_t *audit_handle = NULL;

        if (!audit_handle) {
                if (adt_start_session (&audit_handle, NULL,
                                       ADT_USE_PROC_DATA) != 0) {
                        audit_handle = NULL;
                        return;
                }
        }
        adt_set_user (audit_handle, uid, gid, euid,
                      egid, NULL, ADT_UPDATE);

        adt_get_asid(audit_handle, &asid);
        adt_get_termid(audit_handle, &tid);
        adt_get_auid(audit_handle, &auid);

        event = adt_alloc_event (audit_handle, xevent_num);
	adt_put_event (event, 0, 0);
	adt_free_event (event);

	rd = au_open ();
	au_write (rd, au_to_subject_ex(
		/*
		 * The audit uid isn't valid in the caller, so
		 * we're just using the real uid instead.
		 * auid,
		 */
		uid,
		uid,
		gid,
		euid,
		egid,
		pid,
		asid,
		&tid));

	token1 = au_to_label (label);
	au_write(rd, token1);
	/*
	token2 = au_to_privset("Inheritable", priv);
	*/
	privilege = priv_set_to_str(priv, ',', PRIV_STR_LIT);
	token2 = au_to_upriv(1, privilege);
	au_write(rd, token2);
	token3 = au_to_return64(ADT_SUCCESS, (uint64_t)0);
	au_write(rd, token3);
	au_close (rd, AU_TO_WRITE, xevent_num, 2);
	aw_free_tok(token1);
	aw_free_tok(token2);
	aw_free_tok(token3);
	free(privilege);
}


/*
 * a u d i t w r i t e ( )
 *
 * Construct and write user level audit records to the audit trail.
 *
 */
/*VARARGS*/
int
auditwrite(int param, ...)
{
	va_list arglist;	/* var args arglist pointer */
	int get_rd;		/* rd to pass back */
	register int i;		/* counter */
	int retval;		/* return value */

	/*
	 * Using audit_use_of_x_privilege() instead because
	 * of missing functionality in Solaris 11.4
	 */
	return (AW_SUCCESS_RTN);

	/* Grab the lock */
	(void) mutex_lock(&mutex_auditwrite);

#ifdef DEBUG
	va_start(arglist, param);
	aw_debuglog("in    ", 0, param, arglist);
	va_end(arglist);
#endif /* DEBUG */

	/*
	 * first time initialization stuff.
	 * get the preselection mask and the audit policy.
	 * allocate a default record buffer.
	 */
	if (aw_init() == AW_ERR_RTN) {
		(void) mutex_unlock(&mutex_auditwrite);
		return (AW_ERR_RTN);
	}

	/*
	 * set context, parse the invocation line and get the command.
	 */
	va_start(arglist, param);
	retval = aw_set_context(param, arglist);
	if (retval != AW_ERR_RTN) {
		/* need to rewind the command line first... */
		va_start(arglist, param);
		retval = aw_parse(param, arglist);
	}
	if (retval == AW_ERR_RTN) {
		aw_abort();
		aw_restore();
		(void) mutex_unlock(&mutex_auditwrite);
		return (AW_ERR_RTN);
	}
	va_end(arglist);

	retval = AW_SUCCESS_RTN;

#ifdef DEBUG
	aw_debuglog("my in ", 0, param, arglist);
#endif /* DEBUG */

	switch (aw_iflags & AW_NORMALCMD_FLAGS) {

	case AW_WRITE_FLAG:
		/*
		 * Preselect the record here.
		 * If we're not going to write it, no sense in constructing it.
		 */
		if (!aw_preselect(cur_rd, &pmask)) {
			retval = aw_write_cleanup();
			break;
		}

		if (aw_iflags & AW_ATTRIB_FLAG) {
			va_start(arglist, param);
			if ((retval = aw_gen_rec(param, arglist)) == AW_ERR_RTN)
				break;
			va_end(arglist);
		}

		if ((retval = aw_do_write()) == AW_SUCCESS_RTN)
			retval = aw_write_cleanup();
		break;

	case AW_APPEND_FLAG:
		va_start(arglist, param);
		if ((retval = aw_gen_rec(param, arglist)) == AW_ERR_RTN)
			break;
		va_end(arglist);
		break;

	case AW_ABORT_FLAG:
		aw_abort();
		break;

	case AW_DEFAULTRD_FLAG:
		cur_rd = dflt_rd;
		break;

	case AW_DISCARD_FLAG:
		/*
		 * deallocate all records
		 */
		for (i = 0; i < aw_num_recs; i++)
			aw_rec_dealloc(i);
		/*
		 * deallocate the queue
		 */
		aw_queue_dealloc();
		/*
		 * save buffer is gone
		 */
		aw_static_flags &= ~AW_SAVERD_FLAG;
		cur_rd = dflt_rd = save_rd = AW_NO_RD;
		break;

	case AW_DISCARDRD_FLAG:
		if ((retval = aw_chk_rd(user_rd)) == AW_ERR_RTN)
			break;
		else {
			aw_rec_dealloc(user_rd);
			if (user_rd == save_rd) {
				aw_static_flags &= ~AW_SAVERD_FLAG;
				save_rd = AW_NO_RD;
			}
			/*
			 * special case - reallocate the default rd
			 * if user blows it away.
			 */
			if (user_rd == dflt_rd) {
				if ((retval = aw_rec_alloc(&dflt_rd))
					== AW_ERR_RTN)
				break;
			}
			/*
			 * if user blows away the current rd, set it back
			 * to the default
			 */
			if (user_rd == cur_rd)
				cur_rd = dflt_rd;
		}
		break;

	case AW_FLUSH_FLAG:
		retval = aw_queue_flush();
		break;

	case AW_GETRD_FLAG:
		if ((retval = aw_rec_alloc(&get_rd)) == AW_ERR_RTN)
			break;
		*get_rd_p = get_rd;
		break;

	case AW_NOQUEUE_FLAG:
		if ((retval = aw_queue_flush()) == AW_ERR_RTN) {
			aw_queue_dealloc();
			break;
		}
		aw_queue_dealloc();
		aw_static_flags &= ~AW_QUEUE_FLAG;
		break;

	case AW_NOPRESELECT_FLAG: {
		adt_session_data_t	*ah;

		/* Get the info from the proc */
		if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0) {
			aw_set_err(AW_ERR_GETAUDIT_FAIL);
			retval = AW_ERR_RTN;
		}

		/* Stuff the real values in */
		adt_get_mask(ah, &pmask);

		(void) adt_end_session(ah);

		aw_static_flags &= ~AW_PRESELECT_FLAG;
		break;
	}
	case AW_NOSAVE_FLAG:
		aw_rec_dealloc(save_rd);
		aw_static_flags &= ~AW_SAVERD_FLAG;
		save_rd = AW_NO_RD;
		break;

	case AW_NOSERVER_FLAG:
		aw_static_flags &= ~AW_SERVER_FLAG;
		break;

	case AW_PRESELECT_FLAG:
		aw_static_flags |= AW_PRESELECT_FLAG;
		break;

	case AW_QUEUE_FLAG:
		aw_static_flags |= AW_QUEUE_FLAG;
		break;

	case AW_SAVERD_FLAG:
		if (aw_rec_alloc(&save_rd) == AW_ERR_RTN)
			break;
		*save_rd_p = save_rd;
		/*
		 * SAVERD can be used by long-running processes
		 * (servers) to set-up context for a new "subject".
		 * Take the opportunity here to force a check of
		 * the event file on disk for any changes.
		 */
		/* XXXX refreshauevcache(); */
		aw_static_flags |= AW_SAVERD_FLAG;
		break;

	case AW_SERVER_FLAG:
		aw_static_flags |= AW_SERVER_FLAG;
		break;

	default:
		break;
	}

	if (retval == AW_ERR_RTN)
		aw_abort();

	/* Free up the lock */
	(void) mutex_unlock(&mutex_auditwrite);

#ifdef DEBUG
	aw_debuglog("my out", retval, param, arglist);
#endif /* DEBUG */
	aw_restore();
#ifdef DEBUG
	aw_debuglog("out   ", retval, param, arglist);
#endif /* DEBUG */
	return (retval);
}

/*
 * a w _ a b o r t ( )
 *
 * Called when an an error occurs. Write any incomplete or buffered records.
 */
static void
aw_abort(void)
{
	uint_t i;

	char *err_mesg;		/* Mesg returned by strerror */
	char *aw_err_mesg;	/* Mesg returned by aw_strerror */

	/* Write queued or partial records */

	if (aw_static_flags & AW_QUEUE_FLAG)
		(void) aw_queue_flush();
	else
		for (i = 0; i < aw_num_recs; i++) {
			if (aw_recs[i] != (aw_rec_t *)0 &&
			    aw_recs[i]->buf != (caddr_t)0 &&
			    aw_recs[i]->len != 0) {
				(void) aw_audit_write(i);
			}
		}

	aw_cleanup();

	(void) openlog("auditwrite(3)", LOG_PID|LOG_CONS, LOG_USER);
	(void) syslog(LOG_ALERT,
	"aborted: aw_errno = %d = %s, errno = %d = %s",
	aw_errno,
	(aw_err_mesg = aw_strerror(aw_errno)) ? aw_err_mesg : "unknown error",
	errno,
	(err_mesg = strerror(errno)) ? err_mesg : "unknown error");
	(void) closelog();

}

/*
 * Several programs, most noticably init(1M), have their own local
 * copies of bcopy(3C), which take precedence over lib C's
 * bcopy(3C) that we previously used here.  memmove(3C) does the same
 * thing, and doesn't have any name collisions...so far.
 */

/*
 * a w _ b u f _ a p p e n d ( )
 *
 * Append some data from one buffer to another
 *
 */
static int
aw_buf_append(caddr_t *b1, int *l1, caddr_t b2, int l2)
{
	if (l2 == 0)
		return (AW_SUCCESS_RTN);

	if (*b1 == (caddr_t)0) {
		if ((*b1 = (caddr_t)calloc(1, (size_t)l2)) == (caddr_t)0)
			AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

		(void) memmove(*b1, b2, l2);

		*l1 = l2;

		return (AW_SUCCESS_RTN);
	}

	if ((*b1 = (caddr_t)realloc((void *)*b1, (size_t)(*l1 + l2)))
	    == (caddr_t)0)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

	(void) memmove(*b1+*l1, b2, l2);

	*l1 += l2;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ b u f _ p r e p e n d ( )
 *
 * Prepend the contents of one buffer to another
 *
 */
static int
aw_buf_prepend(caddr_t *b1, int *l1, caddr_t b2, int l2)
{
	if (l2 == 0)
		return (AW_SUCCESS_RTN);

	if (*b1 == (caddr_t)0) {
		if ((*b1 = (caddr_t)calloc(1, (size_t)l2)) == (caddr_t)0)
			AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

		(void) memmove(*b1, b2, l2);

		*l1 = l2;

		return (AW_SUCCESS_RTN);
	}

	if ((*b1 = (caddr_t)realloc((void *)*b1, (size_t)(*l1 + l2)))
	    == (caddr_t)0)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

	(void) memmove(*b1+l2, *b1, *l1);

	(void) memmove(*b1, b2, l2);

	*l1 += l2;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ c h k _ a d d r ( )
 *
 * Make sure address is within allowable boundaries. This is done to minimize
 * core dumps (mostly SIGSEGV and SIGBUS) that can occur when a bad
 * invocation line is passed.
 *
 */
static int
aw_chk_addr(caddr_t p)
{
	/*
	 * if pointer is null, it's bogus
	 */

	if (p == (caddr_t)0)
		return (AW_ERR_RTN);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ c h k _ e v e n t _ i d ( )
 *
 * Make sure event id is set.
 */
static int
aw_chk_event_id(int rd)
{
	if (aw_recs[rd]->event_id == (au_event_t)NULL)
		return (AW_ERR_RTN);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ c h k _ p r i n t ( )
 *
 * Indicate validity of arbitrary data print arg.
 */
static int
aw_chk_print(char arg)
{
	switch (arg) {
	case AWD_BINARY:
	case AWD_OCTAL:
	case AWD_DECIMAL:
	case AWD_HEX:
	case AWD_STRING:
		return (AW_SUCCESS_RTN);
	default:
		return (AW_ERR_RTN);
	}
	/*NOTREACHED*/
}

/*
 * a w _ c h k _ t y p e ( )
 *
 * Indicate validity of arbitrary data type arg.
 */
static int
aw_chk_type(char arg)
{
	switch (arg) {
	case AWD_BYTE:
	case AWD_CHAR:
	case AWD_SHORT:
	case AWD_INT:
	case AWD_LONG:
	case AWD_INT32:
	case AWD_INT64:
		return (AW_SUCCESS_RTN);
	default:
		return (AW_ERR_RTN);
	}
	/*NOTREACHED*/
}

/*
 * a w _ c h k _ r d ( )
 *
 * Make sure record descriptor is valid
 */
static int
aw_chk_rd(int rd)
{
	if ((rd > aw_num_recs) || (aw_recs[rd] == (aw_rec_t *)0))
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ c l e a n u p ( )
 *
 * Free all buffer space. Reset all static flags.
 *
 */
static void
aw_cleanup(void)
{
	int i;

	/* Deallocate all audit records and record pointer array */

	for (i = 0; i < aw_num_recs; i++) {
		aw_rec_dealloc(i);
	}

	cur_rd = dflt_rd = save_rd = AW_NO_RD;

	aw_free((caddr_t)aw_recs);
	aw_recs = (aw_rec_t **)0;

	/* Deallocate audit record queue */

	aw_queue_dealloc();

	/* Reset flags */

	aw_iflags = aw_static_flags = AW_NO_FLAGS;
}

/*
 * a w _ c v r t _ p a t h ( )
 *
 * Get path ready for the audit trail by prepending the absolute root.
 */
static char
aw_cvrt_path(char *path,	/* orig path */
		char **pathp)	/* converted path */
{
#define	AW_PATH_LEN	(MAXPATHLEN)

	int 		cmd;
	char		absroot[AW_PATH_LEN+1];
	static char 	cvrt_path[AW_PATH_LEN+AW_PATH_LEN+2];

	if (path[0] == '/')
		cmd = A_GETCAR;
	else
		cmd = A_GETCWD;

	if (auditon(cmd, absroot, AW_PATH_LEN+1) != 0)
		AW_GEN_ERR(AW_ERR_AUDITON_FAIL);

	(void) strncpy(cvrt_path, absroot, AW_PATH_LEN);
	(void) strcat(cvrt_path, "/");
	(void) strcat(cvrt_path, path);

	*pathp = cvrt_path;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ c v r t _ t y p e ( )
 *
 * Convert arbitrary data print suggestions to token print suggestions.
 */
static char
aw_cvrt_print(char arg)
{
	switch (arg) {
	case AWD_BINARY:
		return (AUP_BINARY);
	case AWD_OCTAL:
		return (AUP_OCTAL);
	case AWD_DECIMAL:
		return (AUP_DECIMAL);
	case AWD_HEX:
		return (AUP_HEX);
	case AWD_STRING:
		return (AUP_STRING);
	}
	return ((char)~0);
}

/*
 * a w _ c v r t _ t y p e ( )
 *
 * Convert arbitrary data type to token data type.
 */
static char
aw_cvrt_type(char arg)
{
	switch (arg) {
	case AWD_BYTE:
		return (AUR_BYTE);
	case AWD_CHAR:
		return (AUR_CHAR);
	case AWD_SHORT:
		return (AUR_SHORT);
	case AWD_INT:
		return (AUR_INT);
	case AWD_LONG:
		return (AUR_INT32);
	case AWD_INT32:
		return (AUR_INT32);
	case AWD_INT64:
		return (AUR_INT64);
	}
	return ((char)~0);
}

/*
 * a w _ d o _ s u b j e c t ( )
 *
 * Add subject/groups/SL/IL attribs if they haven't already been added.
 * Check the audit policy and add any necessary attribs.
 * The remaining policies such as seq and trailers are done by audit(2).
 */
static int
aw_do_subject(int rd)
{
	token_t *tokp;
	gid_t gidset[NGROUPS_MAX];
	adt_session_data_t      *ah;
	au_asid_t		asid;
	au_id_t			auid;
	au_tid_addr_t		tid;
	bslabel_t label_p;

	/*
	 * if the record does not contain a subject attribute
	 * prepend one now
	 */
	if (AW_REC_SUBJECT_FLAG & aw_recs[rd]->aflags)
		return (AW_SUCCESS_RTN);

	if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0) {
		AW_GEN_ERR(AW_ERR_GETAUDIT_FAIL);
	} 
	adt_get_asid(ah, &asid);
	adt_get_termid(ah, &tid);
	adt_get_auid(ah, &auid);

	/*
	 * Add the subject token using the values we have.
	 * Append them to the record under construction
	 */

	if ((tokp = au_to_subject_ex(auid, geteuid(),
		    getegid(), getuid(), getgid(), getpid(),
		    asid, &tid))
		    == (token_t *)0) {
		(void) adt_end_session(ah);
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
	}
	if (aw_buf_append(&(aw_recs[rd]->buf), &(aw_recs[rd]->len),
		tokp->tt_data, (int)tokp->tt_size) ==
		AW_ERR_RTN) {
		(void) adt_end_session(ah);
		aw_free_tok(tokp);
		return (AW_ERR_RTN);
	}
	(void) adt_end_session(ah);
	aw_free_tok(tokp);

	/* Go grab the sensitivity label for this process */
	if (getplabel(&label_p) != 0)
		AW_GEN_ERR(AW_ERR_NO_PLABEL);

	/* Now output the SL */
	if ((tokp = au_to_label(&label_p)) == (token_t *)0)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
	if (aw_buf_append(&(aw_recs[rd]->buf), &(aw_recs[rd]->len),
	    tokp->tt_data, (int)tokp->tt_size) == AW_ERR_RTN) {
		aw_free_tok(tokp);
		return (AW_ERR_RTN);
	}
	aw_free_tok(tokp);

	/* Now add the groups */
	if (audit_policies & AUDIT_GROUP) {
		(void) getgroups(NGROUPS_MAX, gidset);
		if ((tokp = au_to_groups((int *)gidset)) == (token_t *)0)
			AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

		if (aw_buf_append(&(aw_recs[rd]->buf), &(aw_recs[rd]->len),
		    tokp->tt_data, (int)tokp->tt_size) == AW_ERR_RTN) {
			aw_free_tok(tokp);
			return (AW_ERR_RTN);
		}
		aw_free_tok(tokp);
	}

	/*
	 * The sequence token is no longer required to be added by
	 * auditwrite(), it's added by BSM.
	 */

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ d o _ w r i t e ( )
 *
 * Write an audit record.
 */
static int
aw_do_write(void)
{
	/*
	 * an attempt to write an audit record without an event id
	 * set is a serious error
	 */
	if (aw_static_flags & AW_SAVERD_FLAG) {
		if (aw_chk_event_id(cur_rd) == AW_ERR_RTN &&
		    aw_chk_event_id(save_rd) == AW_ERR_RTN)
			AW_GEN_ERR(AW_ERR_EVENT_ID_NOT_SET);
	} else {
		if (aw_chk_event_id(cur_rd) == AW_ERR_RTN)
			AW_GEN_ERR(AW_ERR_EVENT_ID_NOT_SET);
	}

	if (aw_static_flags & AW_SAVERD_FLAG)
		if (aw_rec_prepend(cur_rd, save_rd) == AW_ERR_RTN)
			return (AW_ERR_RTN);

	/*
	 * if we are a server, we don't need to add subject and return
	 * attributes.
	 */
	if ((aw_static_flags & AW_SERVER_FLAG) == 0) {
		if (aw_do_subject(cur_rd) == AW_ERR_RTN)
			return (AW_ERR_RTN);
	}

	/*
	 * Must add a return attribute in all cases if one
	 * has not already been added. this attribute denotes
	 * the success/failure of the event.
	 */
	if (!(aw_recs[cur_rd]->aflags & AW_REC_RETURN_FLAG))
		if (aw_return_attrib(cur_rd) == AW_ERR_RTN)
			return (AW_ERR_RTN);

	/* Now finish up by writing the header attribute */
	if (aw_head(cur_rd) == AW_ERR_RTN)
		return (AW_ERR_RTN);

	/*
	 * if queueing is on write to the queue
	 */
	if (aw_static_flags & AW_QUEUE_FLAG)
		return (aw_queue_write(cur_rd));

	/*
	 * if we are a server, we need to use auditctl(2)
	 */
	if (aw_static_flags & AW_SERVER_FLAG)
		return (aw_auditctl_write(cur_rd));

	/*
	 * default case. we are not a server and we are not queueing.
	 */
	return (aw_audit_write(cur_rd));
}


static int
aw_write_cleanup(void)
{
	aw_rec_dealloc(cur_rd);

	if (cur_rd == dflt_rd)
		dflt_rd = AW_NO_RD;

	if (cur_rd == save_rd) {
		save_rd = AW_NO_RD;
		aw_static_flags &= ~AW_SAVERD_FLAG;
	}

	if ((aw_iflags & AW_SAVEDONE) && (save_rd != AW_NO_RD)) {
		/*
		 * If we are in context for a descriptor, now that the
		 * descriptor is gone we must clean up its save rd if any.
		 */
		aw_rec_dealloc(save_rd);
		save_rd = AW_NO_RD;
		aw_static_flags &= ~AW_SAVERD_FLAG;
	}

	cur_rd = AW_NO_RD;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ f r e e ( )
 *
 * Only free good addrs.
 */
static void
aw_free(caddr_t p)
{
	if (p != (caddr_t)0)
		free((void *)p);
}

/*
 * a w _ f r e e _ t o k ( )
 *
 * Free tokens.
 */
static void
aw_free_tok(token_t *tokp)
{
	aw_free((caddr_t)tokp->tt_data);
	aw_free((caddr_t)tokp);
}

/*
 * a w _ g e n _ r e c ( )
 *
 * Traverse the invocation line again. This time grab all the record attributes,
 * convert them to ADR format and append them to the current record buffer.
 */
static int
aw_gen_rec(int param, va_list arglist)
{
	void *ad[8] = { NULL };		/* attribute data */
	token_t *tokp;			/* token for converted data */
	int a;				/* invocation line argument */
	au_event_ent_t *auevent;	/* event for this call */
	char *apath;			/* anchored path */

	a = param;

	while (a != AW_END) {

		if (AW_IS_CONTROL_CMD(a)) {
			aw_skip_args(arglist, aw_cmd_table[a].cmd_numargs);
			a = va_arg(arglist, int);
			continue;
		}

		aw_get_args(arglist, ad, aw_cmd_table[a].cmd_numargs);

		switch (a) {

		case AW_ARG:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_arg32((char)(uintptr_t)ad[0],
			    (char *)ad[1],
			    (uint32_t)(uintptr_t)ad[2])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
				&(aw_recs[cur_rd]->len), tokp->tt_data,
				(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_ATTR: {
			/*
			 * This is a bit of a hack.  Rather than
			 * write a new au_to_attr() routine, we
			 * simply allocate a new vattr and stuff
			 * values in.
			 */
#if 0
			vattr_t attr;

			if (aw_chk_addr((caddr_t)ad[6]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);

			attr.va_mode = (int)(uintptr_t)ad[0];
			attr.va_uid = (int)(uintptr_t)ad[1];
			attr.va_gid = (int)(uintptr_t)ad[2];
			attr.va_fsid = (int)(uintptr_t)ad[3];
			attr.va_nodeid = (int)(uintptr_t)ad[4];
			attr.va_rdev = (int)(uintptr_t)ad[5];

			if ((tokp = au_to_attr(&attr)) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
				&(aw_recs[cur_rd]->len), tokp->tt_data,
				(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
#endif
			break;

		}

		case AW_DATA:
			if (aw_chk_print((char)(uintptr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_CMD_INVALID);
			ad[0] = (void *)(uintptr_t)aw_cvrt_print(
			    (char)(uintptr_t)ad[0]);
			if (aw_chk_type((char)(uintptr_t)ad[1]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_CMD_INVALID);
			ad[1] = (void *)(uintptr_t)aw_cvrt_type(
			    (char)(uintptr_t)ad[1]);
			if (aw_chk_addr((caddr_t)ad[3]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_data((char)(uintptr_t)ad[0],
			    (char)(uintptr_t)ad[1],
			    (char)(uintptr_t)ad[2], (char *)ad[3])) ==
			    (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_EVENT:
			aw_iflags |= AW_EVENT_FLAG;
			if ((auevent = getauevnam((char *)ad[0]))
					== (au_event_ent_t *)NULL)
				AW_GEN_ERR(AW_ERR_EVENT_ID_INVALID)
			else
				aw_set_event(cur_rd, auevent->ae_number,
					auevent->ae_class);
			break;

		case AW_EVENTNUM:
			aw_iflags |= AW_EVENT_FLAG;
			if ((cacheauevent(&auevent,
			    (au_event_t)(uintptr_t)ad[0])) != 1)
				AW_GEN_ERR(AW_ERR_EVENT_ID_INVALID)
			else
				aw_set_event(cur_rd, auevent->ae_number,
					auevent->ae_class);
			break;

		case AW_EXEC_ARGS:
			if (!(audit_policies & AUDIT_ARGV))
				break;
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_exec_args((char **)ad[0]))
			    == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_EXEC_ENV:
			if (!(audit_policies & AUDIT_ARGE))
				break;
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_exec_env((char **)ad[0]))
			    == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_EXIT:
			if ((tokp = au_to_exit((int)(uintptr_t)ad[0],
			    (int)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_GROUPS:
			if (!(audit_policies & AUDIT_GROUP))
				break;
			if (aw_chk_addr((caddr_t)ad[1]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_newgroups((int)(uintptr_t)ad[0],
			    (gid_t *)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_IN_ADDR:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_in_addr((struct in_addr *)
					ad[0])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_IPC:
			if ((tokp = au_to_ipc((char)(uintptr_t)ad[0],
			    (int)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_IPORT:
			if ((tokp = au_to_iport((ushort_t)(uintptr_t)ad[0])) ==
				(token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_OPAQUE:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_opaque((char *)ad[0],
			    (short)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_PATH:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				return (AW_ERR_RTN);
			if (aw_cvrt_path((char *)ad[0], &apath) == AW_ERR_RTN)
				return (AW_ERR_RTN);
			if ((tokp = au_to_path(apath)) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_PRIVILEGE:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_privset("",
			    (priv_set_t *)ad[0])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_LEVEL:
		case AW_SLABEL:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_label((bslabel_t *)ad[0])) ==
			    (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_PROCESS:
			if (aw_chk_addr((caddr_t)ad[7]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_process((uint32_t)(uintptr_t)ad[0],
					(uint32_t)(uintptr_t)ad[1],
					(uint32_t)(uintptr_t)ad[2],
					(uint32_t)(uintptr_t)ad[3],
					(uint32_t)(uintptr_t)ad[4],
					(uint32_t)(uintptr_t)ad[5],
					(uint32_t)(uintptr_t)ad[6],
					(au_tid_t *)ad[7])) ==
				(token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_PROCESS_EX:
			if (aw_chk_addr((caddr_t)ad[7]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_process_ex((uint32_t)(uintptr_t)ad[0],
					(uint32_t)(uintptr_t)ad[1],
					(uint32_t)(uintptr_t)ad[2],
					(uint32_t)(uintptr_t)ad[3],
					(uint32_t)(uintptr_t)ad[4],
					(uint32_t)(uintptr_t)ad[5],
					(uint32_t)(uintptr_t)ad[6],
					(au_tid_addr_t *)ad[7])) ==
				(token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_RETURN:
			aw_recs[cur_rd]->aflags |= AW_REC_RETURN_FLAG;
			if ((tokp = au_to_return32((char)(uintptr_t)ad[0],
			    (uint_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			/*
			 * Set up event for success/failure preselection
			 */
			if ((char)(uintptr_t)ad[0] != 0)
				aw_recs[cur_rd]->event_mod |= PAD_FAILURE;
			break;

#if 0
		case AW_SOCKET:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_socket((struct socket *)ad[0])) ==
			    (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;
#endif

		case AW_SUBJECT:
			aw_recs[cur_rd]->aflags |= AW_REC_SUBJECT_FLAG;
			if (aw_chk_addr((caddr_t)ad[7]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_subject((uint32_t)(uintptr_t)ad[0],
						(uint32_t)(uintptr_t)ad[1],
						(uint32_t)(uintptr_t)ad[2],
						(uint32_t)(uintptr_t)ad[3],
						(uint32_t)(uintptr_t)ad[4],
						(uint32_t)(uintptr_t)ad[5],
						(uint32_t)(uintptr_t)ad[6],
						(au_tid_t *)ad[7])) ==
				(token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_SUBJECT_EX:
			aw_recs[cur_rd]->aflags |= AW_REC_SUBJECT_FLAG;
			if (aw_chk_addr((caddr_t)ad[7]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_subject_ex((uint32_t)(uintptr_t)ad[0],
						(uint32_t)(uintptr_t)ad[1],
						(uint32_t)(uintptr_t)ad[2],
						(uint32_t)(uintptr_t)ad[3],
						(uint32_t)(uintptr_t)ad[4],
						(uint32_t)(uintptr_t)ad[5],
						(uint32_t)(uintptr_t)ad[6],
						(au_tid_addr_t *)ad[7])) ==
				(token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_USEOFPRIV:
				if ((tokp = au_to_upriv((char)(uintptr_t)ad[0],
				    (char *)ad[1])) == (token_t *)0) {
				aw_free_tok(tokp);
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL)
			}
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_TEXT:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_text((char *)
						ad[0])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_UAUTH:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_uauth((char *)
						ad[0])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_CMD: {
			char **env = NULL;

			if (aw_chk_addr((caddr_t)ad[1]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if (aw_chk_addr((caddr_t)ad[2]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);

			if (audit_policies & AUDIT_ARGE)
				env = (char **)ad[2];

			if ((tokp = au_to_cmd((int)(uintptr_t)ad[0],
			    (char **)ad[1], env)) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
			    &(aw_recs[cur_rd]->len), tokp->tt_data,
			    (int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;
		}

		case AW_XATOM:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_xatom((char *)ad[0]))
			    == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XCLIENT:
			if ((tokp = au_to_xclient(
			    (uint32_t)(uintptr_t)ad[0])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XCURSOR:
			if ((tokp = au_to_xcursor(
			    (int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XCOLORMAP:
			if ((tokp = au_to_xcolormap(
			    (int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XFONT:
			if ((tokp = au_to_xfont((int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XGC:
			if ((tokp = au_to_xgc((int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XPIXMAP:
			if ((tokp = au_to_xpixmap(
			    (int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XPROPERTY:
			if (aw_chk_addr((caddr_t)ad[2]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_xproperty(
			    (int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1], (char *)ad[2])) ==
			    (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XSELECT:
			if (aw_chk_addr((caddr_t)ad[0]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if (aw_chk_addr((caddr_t)ad[1]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if (aw_chk_addr((caddr_t)ad[2]) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			if ((tokp = au_to_xselect((char *)ad[0],
			    (char *)ad[1], (char *)ad[2])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		case AW_XWINDOW:
			if ((tokp = au_to_xwindow(
			    (int32_t)(uintptr_t)ad[0],
			    (uid_t)(uintptr_t)ad[1])) == (token_t *)0)
				AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
			if (aw_buf_append(&(aw_recs[cur_rd]->buf),
					&(aw_recs[cur_rd]->len),
					tokp->tt_data,
					(int)tokp->tt_size) == AW_ERR_RTN) {
				aw_free_tok(tokp);
				return (AW_ERR_RTN);
			}
			aw_free_tok(tokp);
			break;

		default:
			AW_GEN_ERR(AW_ERR_CMD_INVALID)

		}		/* switch */

		a = va_arg(arglist, int);

	}			/* while */

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ h e a d ( )
 *
 * Add a header to an audit record.
 *
 * Note that we no longer need to add a trailer in Solaris 2.x, as this
 * is handled by the audit(2) system call.
 */
static int
aw_head(int rd)
{

	token_t *tokp;
	adr_t adr;	/* tmp pointer */
	char id;	/* tmp variable to hold value */
	int32_t len;	/* Value to fix */

	if ((tokp = au_to_header_ex(aw_recs[rd]->event_id,
	    aw_recs[rd]->event_mod)) == (token_t *)NULL)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

	/*
	 * Need to fix up the size correctly.
	 */
	len = aw_recs[rd]->len + tokp->tt_size;
	adrm_start(&adr, tokp->tt_data);	/* beginning of pointer */
	adrm_char(&adr, &id, 1);		/* move past attr id */
	adr_int32(&adr, &len, 1);		/* fix the length */

	if (aw_buf_prepend(&(aw_recs[rd]->buf), &(aw_recs[rd]->len),
	    tokp->tt_data, (int)tokp->tt_size) == AW_ERR_RTN) {
		aw_free_tok(tokp);
		return (AW_ERR_RTN);
	}

	aw_free_tok(tokp);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ p a r s e ( )
 *
 * Parse the invocation line looking for invalid commands and invalid data
 * (bad pointers).
 *
 * Returns AW_ERR_RTN when:
 *     more than one control command has been specified.
 *     AW_APPEND is specified w/out any attribute commands.
 *
 * Returns AW_SUCCESS_RTN upon success.
 */
static int
aw_parse(int param, va_list arglist)
{
	int a;
	au_event_ent_t *auevent;
	void *ad[8] = { NULL }; /* argument data */

	/*
	 * During the port from TS 2, we had to slightly reorg code, thus
	 * I'm simply going to continue to use the "a" variable.
	 */
	a = param;

	while (a != AW_END) {

		if (a < AW_CMD_MIN || a > AW_CMD_MAX)
			AW_GEN_ERR(AW_ERR_CMD_INVALID);

		/*
		 * EVENT attribute and RETURN attribute have preselection
		 * info. Need to gobble up that info here so that we can
		 * preselect without the overhead of record construction.
		 */
		if (AW_IS_DATA_CMD(a) &&
		    ((a != AW_EVENT) && (a != AW_EVENTNUM)) &&
		    (a != AW_RETURN)) {
			aw_iflags |= AW_ATTRIB_FLAG;
			aw_skip_args(arglist, aw_cmd_table[a].cmd_numargs);
			/* Reload with next value of "a" */
			a = va_arg(arglist, int);
			continue;
		}

		aw_get_args(arglist, ad, aw_cmd_table[a].cmd_numargs);

		switch (a) {

		/* AW_ABORT is not documented. It is used for debugging */

		case AW_ABORT:
			aw_iflags |= AW_ABORT_FLAG;
			break;

		case AW_APPEND:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_APPEND_FLAG;
			break;

		case AW_EVENT:
			aw_iflags |= AW_EVENT_FLAG;
			aw_iflags |= AW_ATTRIB_FLAG;
			if ((auevent = getauevnam((char *)ad[0]))
			    == (au_event_ent_t *)0)
				AW_GEN_ERR(AW_ERR_EVENT_ID_INVALID)
			else
				aw_set_event(cur_rd, auevent->ae_number,
					auevent->ae_class);
			break;

		case AW_EVENTNUM:
			aw_iflags |= AW_EVENT_FLAG;
			aw_iflags |= AW_ATTRIB_FLAG;
			if ((cacheauevent(&auevent,
			    (au_event_t)(uintptr_t)ad[0])) != 1)
				AW_GEN_ERR(AW_ERR_EVENT_ID_INVALID)
			else
				aw_set_event(cur_rd, auevent->ae_number,
					auevent->ae_class);
			break;

		case AW_QUEUE:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (aw_static_flags & AW_QUEUE_FLAG)
				AW_GEN_ERR(AW_ERR_CMD_IN_EFFECT);
			aw_queue_hiwater = (int)(uintptr_t)ad[0];
			if (aw_queue_hiwater > AW_MAX_REC_SIZE ||
			    aw_queue_hiwater == 0)
				AW_GEN_ERR(AW_ERR_QUEUE_SIZE_INVALID);
			aw_iflags |= AW_QUEUE_FLAG;
			break;

		case AW_DEFAULTRD:
			AW_PARSE_ERR(AW_CTRLCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_DEFAULTRD_FLAG;
			break;

		case AW_DISCARD:
			AW_PARSE_ERR(AW_CTRLCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_DISCARD_FLAG;
			break;

		case AW_DISCARDRD:
			AW_PARSE_ERR(AW_CTRLCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			user_rd = (int)(uintptr_t)ad[0];
			/*
			 * A specified rd of -1 here means that the
			 * default rd should be discarded.
			 */
			if (user_rd == -1)
				user_rd = dflt_rd;
			aw_iflags |= AW_DISCARDRD_FLAG;
			break;

		case AW_FLUSH:
			AW_PARSE_ERR(AW_CTRLCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (!(aw_static_flags & AW_QUEUE_FLAG))
				AW_GEN_ERR(AW_ERR_CMD_NOT_IN_EFFECT);
			aw_iflags |= AW_FLUSH_FLAG;
			break;

		case AW_GETRD:
			AW_PARSE_ERR(AW_CTRLCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			get_rd_p = (int *)ad[0];
			if (aw_chk_addr((caddr_t)get_rd_p) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			aw_iflags |= AW_GETRD_FLAG;
			break;

		case AW_NOPRESELECT:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (!(aw_static_flags & AW_PRESELECT_FLAG))
				AW_GEN_ERR(AW_ERR_CMD_NOT_IN_EFFECT);
			aw_iflags |= AW_NOPRESELECT_FLAG;
			break;

		case AW_NOQUEUE:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (!(aw_static_flags & AW_QUEUE_FLAG))
				AW_GEN_ERR(AW_ERR_CMD_NOT_IN_EFFECT);
			aw_queue_hiwater = 0;
			aw_iflags |= AW_NOQUEUE_FLAG;
			break;

		case AW_NOSAVE:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (!(aw_static_flags & AW_SAVERD_FLAG))
				AW_GEN_ERR(AW_ERR_CMD_NOT_IN_EFFECT);
			aw_iflags |= AW_NOSAVE_FLAG;
			break;

		case AW_NOSERVER:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if (!(aw_static_flags & AW_SERVER_FLAG))
				AW_GEN_ERR(AW_ERR_CMD_NOT_IN_EFFECT);
			aw_iflags |= AW_NOSERVER_FLAG;
			break;

		case AW_PRESELECT:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			if ((au_mask_t *)ad[0] != (au_mask_t *)0) {
				(void) memcpy((char *)&pmask,
					(char *)((au_mask_t *)ad[0]),
					sizeof (au_mask_t));
			}
			aw_iflags |= AW_PRESELECT_FLAG;
			break;

		case AW_RETURN:
			/*
			 * Set up event for success/failure preselection
			 */
			aw_iflags |= AW_ATTRIB_FLAG;
			if ((char)(uintptr_t)ad[0] != 0)
				aw_recs[cur_rd]->event_mod |= PAD_FAILURE;
			break;

		case AW_SAVERD:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			save_rd_p = (int *)ad[0];
			if (aw_chk_addr((caddr_t)save_rd_p) == AW_ERR_RTN)
				AW_GEN_ERR(AW_ERR_ADDR_INVALID);
			aw_iflags |= AW_SAVERD_FLAG;
			break;

		case AW_SERVER:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_SERVER_FLAG;
			break;

		case AW_USERD:
			/*
			 * Already handled a valid one in aw_set_context().
			 */
			AW_PARSE_ERR(AW_NOUSERDCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_USERD_FLAG;
			break;

		case AW_WRITE:
			AW_PARSE_ERR(AW_NORMALCMD_FLAGS, AW_ERR_CMD_TOO_MANY);
			aw_iflags |= AW_WRITE_FLAG;
			break;

		default:
			AW_GEN_ERR(AW_ERR_CMD_INVALID)

		}		/* switch */

		/* Reload with the next value of "a" */
		a = va_arg(arglist, int);

	}			/* while */

	/* Must have a control command */
	if (!(aw_iflags & AW_CTRLCMD_FLAGS))
		AW_GEN_ERR(AW_ERR_CMD_INCOMPLETE);

	/* Must be an attribute command with AW_APPEND control command */
	if ((aw_iflags & AW_APPEND_FLAG) &&
	    !(aw_iflags & AW_ATTRIB_FLAG))
		AW_GEN_ERR(AW_ERR_CMD_INCOMPLETE);

	/*
	 * If there was an attribute command, need a control command to that
	 * tells what to do with the attributes.
	 */
	if ((aw_iflags & AW_ATTRIB_FLAG) &&
	    !((aw_iflags & AW_APPEND_FLAG) ||
	    (aw_iflags & AW_WRITE_FLAG)))
	    AW_GEN_ERR(AW_ERR_CMD_INCOMPLETE);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ p r e s e l e c t ( )
 *
 * Do user level audit preselection
 *
 * Returns:
 *     1 - audit event is preselected
 *     0 - audit event is not preselected
 */
static int
aw_preselect(int rd, au_mask_t *pmaskp)
{
	if (aw_recs[rd]->event_mod & PAD_FAILURE)
		return (aw_recs[rd]->class & pmaskp->am_failure);

	return (aw_recs[rd]->class & pmaskp->am_success);
}

/*
 * a w _ q u e u e _ d e a l l o c ( )
 *
 * Deallocate audit record queue.
 */
static void
aw_queue_dealloc(void)
{
	aw_queue_bytes = 0;
	aw_free(aw_queue);
	aw_queue = (caddr_t)0;
}

/*
 * a w _ q u e u e _ f l u s h ( )
 *
 * Flush audit record queue.
 */
static int
aw_queue_flush(void)
{
	/* write all records on queue to trail */

	if (aw_queue_bytes) {
		if (auditctl(A_AUDIT, (uint32_t)aw_queue_bytes,
		    aw_queue) == -1)
			AW_GEN_ERR(AW_ERR_AUDIT_FAIL);

		aw_queue_bytes = 0;
	}

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ q u e u e _ w r i t e ( )
 *
 * "Queue" an audit record. Actually, each record passed is appended to a
 * buffer. This buffer eventually gets written with auditctl(2). Auditctl(2)
 * will process the records in the order in which it receives them. This
 * creates a "queueing" effect.
 */
static int
aw_queue_write(int rd)
{
	if (aw_queue_bytes + aw_recs[rd]->len > AW_MAX_REC_SIZE) {

		(void) aw_queue_flush();

		if ((auditctl(A_AUDIT, (uint32_t)aw_recs[rd]->len,
		    aw_recs[rd]->buf)) == -1)
			AW_GEN_ERR(AW_ERR_AUDIT_FAIL);

		return (AW_SUCCESS_RTN);
	}

	/* no queue? allocate it now. */

	if (aw_buf_append(&aw_queue, &aw_queue_bytes,
	    aw_recs[rd]->buf, aw_recs[rd]->len) == AW_ERR_RTN)
		return (AW_ERR_RTN);

	/* did we reach the hi-water mark? */

	if (aw_queue_bytes >= aw_queue_hiwater)
		if (aw_queue_flush() == AW_ERR_RTN)
			return (AW_ERR_RTN);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ r e c _ i n i t ( )
 *
 * Init a rec area.
 *
 */
static void
aw_rec_init(aw_rec_t *rec)
{
	rec->event_id = 0;
	rec->event_mod = 0;

	rec->context.static_flags = AW_NO_FLAGS;
	rec->context.save_rd = AW_NO_RD;
	rec->context.aw_errno = AW_ERR_NO_ERROR;
	rec->context.pmask.am_success = AU_MASK_NONE;
	rec->context.pmask.am_failure = AU_MASK_NONE;
}
/*
 * a w _ r e c _ a l l o c ( )
 *
 * Allocate a buffer to store an audit record.
 *
 */
static int
aw_rec_alloc(int *rdp)
{
	int slot;

	/* allocate the audit record buffer pointers */

	if (aw_recs == (aw_rec_t **)0) {
		if ((aw_recs = (aw_rec_t **)calloc(AW_NUM_RECP,
		    (size_t)sizeof (aw_rec_t *))) == (aw_rec_t **)0)
			AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

		aw_num_recs = AW_NUM_RECP;

		/* allocate the record */

		if ((aw_recs[0] = (aw_rec_t *)calloc(1,
		    (size_t)sizeof (aw_rec_t))) == (aw_rec_t *)0)
			return (AW_ERR_RTN);

		aw_rec_init(aw_recs[0]);
		*rdp = 0;

		return (AW_SUCCESS_RTN);
	}
	/* linear search to find the next open slot */

	for (slot = 0; slot < aw_num_recs; slot++)
		if (aw_recs[slot] == (aw_rec_t *)0) {

			/* allocate the record */

			if ((aw_recs[slot] = (aw_rec_t *)calloc(1,
			    (size_t)sizeof (aw_rec_t))) == (aw_rec_t *)0)
				return (AW_ERR_RTN);

			aw_rec_init(aw_recs[slot]);
			*rdp = slot;

			return (AW_SUCCESS_RTN);
		}
	/* no open slots, allocate for another */
	if ((aw_recs = (aw_rec_t **)realloc((void *)aw_recs,
	    ((size_t)(aw_num_recs + 1) * sizeof (aw_rec_t *)))) ==
	    (aw_rec_t **)0)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);

	/* allocate the record */

	slot = aw_num_recs;

	if ((aw_recs[slot] = (aw_rec_t *)calloc(1, (size_t)sizeof (aw_rec_t)))
	    == (aw_rec_t *)0)
		return (AW_ERR_RTN);

	aw_rec_init(aw_recs[aw_num_recs]);
	*rdp = aw_num_recs++;

	return (AW_SUCCESS_RTN);
}

#ifdef NOTYET
/*
 * a w _ r e c _ a p p e n d ( )
 *
 * Concatentate two previously allocated records.
 *
 */
static int
aw_rec_append(int to_rd, int from_rd)
{
	if (aw_chk_rd(to_rd) == AW_ERR_RTN)
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	if (aw_chk_rd(from_rd) == AW_ERR_RTN)
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	if (aw_recs[from_rd]->event_id) {
		aw_recs[to_rd]->event_id = aw_recs[from_rd]->event_id;
		aw_recs[to_rd]->class = aw_recs[from_rd]->class;
	}

	aw_recs[to_rd]->aflags |= aw_recs[from_rd]->aflags;

	if ((aw_recs[to_rd]->len +
	    aw_recs[from_rd]->len) > AW_MAX_REC_SIZE)
		AW_GEN_ERR(AW_ERR_REC_TOO_BIG);

	return (aw_buf_append(&(aw_recs[to_rd]->buf),
		&(aw_recs[to_rd]->len),
		aw_recs[from_rd]->buf,
		aw_recs[from_rd]->len));
}
#endif /* NOTYET */

/*
 * a w _ r e c _ d e a l l o c ( )
 *
 * Deallocate a previously allocated audit record buffer.
 *
 */
static void
aw_rec_dealloc(int rd)
{
	/* free the audit record buffer */

	if (aw_recs[rd] == (aw_rec_t *)0)
		return;

	aw_free(aw_recs[rd]->buf);
	aw_recs[rd]->buf = (caddr_t)0;

	/* free the record */

	aw_free((caddr_t)aw_recs[rd]);
	aw_recs[rd] = (aw_rec_t *)0;
}

/*
 * a w _ r e c _ p r e p e n d ( )
 *
 * Prepend a record buffer with the contents of another record buffer.
 *
 */
static int
aw_rec_prepend(int to_rd, int from_rd)
{
	if (aw_chk_rd(to_rd) == AW_ERR_RTN)
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	if (aw_chk_rd(from_rd) == AW_ERR_RTN)
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	if (aw_recs[from_rd]->event_id)
		aw_recs[to_rd]->event_id = aw_recs[from_rd]->event_id;

	if ((aw_recs[to_rd]->len +
	    aw_recs[from_rd]->len) > AW_MAX_REC_SIZE)
		AW_GEN_ERR(AW_ERR_REC_TOO_BIG);

	return (aw_buf_prepend(&(aw_recs[to_rd]->buf),
		&(aw_recs[to_rd]->len),
		aw_recs[from_rd]->buf,
		aw_recs[from_rd]->len));
}

static int
aw_return_attrib(int rd)
{
	token_t *tokp;

	/*
	 * append a return token indicating a success event
	 */
	if ((tokp = au_to_return32(0, 0)) == (token_t *)0)
		AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
	if (aw_buf_append(&(aw_recs[rd]->buf), &(aw_recs[rd]->len),
	    tokp->tt_data, (int)tokp->tt_size) == AW_ERR_RTN) {
		aw_free_tok(tokp);
		return (AW_ERR_RTN);
	}

	aw_free_tok(tokp);
	return (AW_SUCCESS_RTN);
}

/*
 * a w _ s e t _ e r r ( )
 *
 * This routine sets aw_errno. It insures aw_errno is set
 * once per invocation.
 */
static void
aw_set_err(int error)
{
	if (error == AW_ERR_NO_ERROR || aw_errno == AW_ERR_NO_ERROR)
		aw_errno = error;
}

/*
 * a w _ s e t _ e v e n t ( )
 *
 * Set event id and class.
 */
static void
aw_set_event(int rd, au_event_t event_id, uint_t class)
{
	aw_recs[rd]->event_id = event_id;
	aw_recs[rd]->class = class;
}

/*
 * a w _ i n i t ( )
 *
 * if this is the first invocation of auditwrite(3), do some setup
 */
static int
aw_init(void)
{
	adt_session_data_t	*ah;

	aw_errno = AW_ERR_NO_ERROR;	/* No error so far */

	/*
	 * allocate a default audit record buf if we don't have one.
	 */
	if (dflt_rd == AW_NO_RD) {
		if (aw_rec_alloc(&dflt_rd) == AW_ERR_RTN)
			AW_GEN_ERR(AW_ERR_ALLOC_FAIL);
	}

	/*
	 * current record buf was recently deallocated
	 * set it back to the default
	 */
	if (cur_rd == AW_NO_RD)
		cur_rd = dflt_rd;

	if (aw_static_flags & AW_INVOKED_BEFORE_FLAG) {
		aw_iflags = AW_NO_FLAGS;
		return (AW_SUCCESS_RTN);
	}

	/*
	 * First call setup
	 *
	 * Cache the preselection mask and the audit policies in order
	 * to reduce system call overhead. If they change, we will be
	 * auditing with stale values.
	 */
	if (adt_start_session(&ah, NULL, ADT_USE_PROC_DATA) != 0) {
		AW_GEN_ERR(AW_ERR_GETAUDIT_FAIL);
	}

	/* Stuff the real values in */
	adt_get_mask(ah, &pmask);

	(void) adt_end_session(ah);

	if (auditon(A_GETPOLICY, (caddr_t)&audit_policies, 0) == -1)
		AW_GEN_ERR(AW_ERR_AUDIT_FAIL);

	aw_static_flags |= AW_INVOKED_BEFORE_FLAG;

	cur_rd = dflt_rd;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ s e t _ c o n t e x t ( )
 *
 * set context as needed.
 */
static int
aw_set_context(int param, va_list arglist)
{
	int a;
	void *ad[8] = { NULL }; /* argument data */

	a = param;

	/*
	 * If the input params start with USERD and after this there's
	 * anything besides AW_END, then we need to switch to the context
	 * for that record descriptor.
	 */
	if (a != AW_USERD)
		return (AW_SUCCESS_RTN);

	aw_get_args(arglist, ad, aw_cmd_table[a].cmd_numargs);
	user_rd = (int)(uintptr_t)ad[0];
	if (aw_chk_rd(user_rd) == AW_ERR_RTN)
		AW_GEN_ERR(AW_ERR_RD_INVALID);

	a = va_arg(arglist, int);
	if (a != AW_END) {
		/*
		 * USERD is used with another command.
		 * Save context first, then load context for the
		 * given rd.
		 */
		old_context.static_flags = aw_static_flags;
		old_context.save_rd = save_rd;
		old_context.aw_errno = aw_errno;
		old_context.pmask = pmask;

		old_cur_rd = cur_rd;

		/*
		 * Now load up our values.
		 */
		aw_static_flags =
		    aw_recs[user_rd]->context.static_flags;
		save_rd = aw_recs[user_rd]->context.save_rd;
		aw_errno = aw_recs[user_rd]->context.aw_errno;
		pmask = aw_recs[user_rd]->context.pmask;

		/* initialize as needed... */
		if (aw_init() == AW_ERR_RTN) {
			return (AW_ERR_RTN);
		}

		aw_iflags |= AW_SAVEDONE;
	}

	/*
	 * Finish basic USERD processing here.
	 */
	cur_rd = user_rd;

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ r e s t o r e ( )
 *
 * restore context if needed.
 */
static void
aw_restore(void)
{
	if (aw_iflags & AW_SAVEDONE) {
		/*
		 * Save context for our rd first... If our rd is gone
		 * by now, we can't and won't need to do this part.
		 */
		if ((user_rd <= aw_num_recs) &&
		    (aw_recs[user_rd] != (aw_rec_t *)0)) {
			aw_recs[user_rd]->context.static_flags =
			    aw_static_flags;
			aw_recs[user_rd]->context.save_rd = save_rd;
			aw_recs[user_rd]->context.aw_errno = aw_errno;
			aw_recs[user_rd]->context.pmask = pmask;
		}

		/*
		 * Now restore the old values.
		 */
		aw_static_flags = old_context.static_flags;
		save_rd = old_context.save_rd;
		pmask = old_context.pmask;

		cur_rd = old_cur_rd;

		aw_iflags &= ~AW_SAVEDONE;
	}
}

/*
 * a w _ a u d i t _ w r i t e ( )
 *
 * Write an audit record to the audit trail using the audit(2) system call.
 */
static int
aw_audit_write(int rd)
{
	if (audit(aw_recs[rd]->buf, aw_recs[rd]->len) == -1)
		AW_GEN_ERR(AW_ERR_AUDIT_FAIL);

	return (AW_SUCCESS_RTN);
}

/*
 * a w _ a u d i t c t l  _ w r i t e ( )
 *
 * Write an audit record to the audit trail using the auditctl(2) system call.
 */
static int
aw_auditctl_write(int rd)
{
	if (auditctl(A_AUDIT, (uint32_t)aw_recs[rd]->len,
	    aw_recs[rd]->buf) == -1)
		AW_GEN_ERR(AW_ERR_AUDIT_FAIL);

	return (AW_SUCCESS_RTN);
}

#ifdef DEBUG
/*
 * aw_debuglog: dump auditwrite parameters and current state
 * to the debug file.
 *
 * General format of such an entry:
 *
 * xxx e pid= nnn stat-flags= xxxxxx aw_errno= n cur_rd= n
 *	dflt_rd= n save_rd= n arg1 arg2 ... arg11
 *
 * ...where:
 * xxx   = text, generally either "in" (auditwrite begin) or "out" (exit)
 * e     = auditwrite return value (significant only with "out")
 * arg1, = auditwrite arguments (1st 11 only, whether used or not)
 * arg2
 * ...
 *
 */
static int cntr = 0;

static void
aw_debuglog(char *s, int rc, int param, va_list arglist)
{
	void *a;
	int i;
	FILE	*f;

	f = fopen("/var/audit/awlog", "a");
	if (f == NULL) {
		return;
	}

	if (strcmp(s, "in    ") == 0) {
		cntr++;
		(void) fprintf(f, "\n%4d \n", cntr);
	}

	(void) fprintf(f, "%9s ", s);
	(void) fprintf(f, "%8d ", rc);
	(void) fprintf(f, "pid= %7ld ", (long)getpid());

	(void) fprintf(f, "stat-flags= %8x ", aw_static_flags);
	(void) fprintf(f, "aw_errno= %8d ", aw_errno);

	(void) fprintf(f, "cur_rd= %8x ", cur_rd);
	(void) fprintf(f, "dflt_rd= %8x ", dflt_rd);
	(void) fprintf(f, "save_rd= %8x\n\t\t", save_rd);

	if ((arglist == 0) && (param == 0))
		goto done;

	/*
	 * Now dump in the arguments auditwrite was called with... Since
	 * the number of args is variable, just dump the first 10 or so
	 * (some of which may not actually have been passed).
	 */
	a = (void *)param;
	for (i = 1; i <= 10; i++) {
		(void) fprintf(f, "%d ", a);
		a = va_arg(arglist, void *);
	}

done:
	(void) fprintf(f, "\n");
	(void) fclose(f);
}
#endif

/*
 * aw_strerror: return the error string
 */
char *
aw_strerror(const int aw_errnum)
{
	if ((aw_errnum < aw_nerr) && (aw_errnum >= 0))
		return (aw_errlist[aw_errnum]);
	else
		return (NULL);
}

/*
 * aw_geterrno: return the aw_errno for the given descriptor.
 */
int
aw_geterrno(const int rd)
{
	int err;

	(void) mutex_lock(&mutex_auditwrite);

	if (aw_chk_rd(rd) == AW_ERR_RTN) {
		(void) mutex_unlock(&mutex_auditwrite);
		return (AW_ERR_RD_INVALID);
	}

	err = aw_recs[rd]->context.aw_errno;

	(void) mutex_unlock(&mutex_auditwrite);
	return (err);
}

/*
 * aw_perror_c: common internal routine to return the error string
 */
static void
aw_perror_c(const int err, const char *s)
{
	char *c;

	if ((err < aw_nerr) && (err >= 0))
		c = aw_errlist[err];
	else
		c = "Unknown error";

	if (s && *s) {
		(void) write(2, s, strlen(s));
		(void) write(2, ": ", 2);
	}

	(void) write(2, c, strlen(c));
	(void) write(2, "\n", 1);
}

/*
 * aw_perror: return the error string
 */
void
aw_perror(const char *s)
{
	aw_perror_c(aw_errno, s);
}

/*
 * aw_perror_r: return the error string
 */
void
aw_perror_r(const int rd, const char *s)
{
	int err;

	err = aw_geterrno(rd);
	aw_perror_c(err, s);
}

/*
 * Currently we are using Solaris 2.x BSM's audit mechanism,
 * which doesn't have the large queue buffer mechanism.  Rather
 * than rewrite the system call, we'll simply emulate the large
 * buffer write.  If this gets to be problem in performance,
 * we can readd the system call.
 */
static int
auditctl(uint32_t command, uint32_t value, caddr_t data)
{
	uint32_t bytes_left = value;	/* number of bytes to write */
	caddr_t mover = data;		/* moving pointer */
	adr_t adr;			/* byte independent addressing */
	char id;			/* check for proper audit record */
	int32_t bytes;			/* number of bytes for this record */

	/* The only remaining option to auditctl is be A_AUDIT */
	if (command != A_AUDIT)
		return (-1);

	while (bytes_left > (uint32_t)0) {

		/* Where to start parsing */
		adrm_start(&adr, mover);
		adrm_char(&adr, &id, 1);
		adrm_int32(&adr, &bytes, 1);

		/* Make sure we have a header and output the record */

		if (!((id == AUT_HEADER32) || (id == AUT_HEADER64) ||
		    (id == AUT_HEADER32_EX) || (id == AUT_HEADER64_EX)) ||
		    (bytes > bytes_left)) {
			errno = EINVAL;
			return (-1);
		}

		if (audit((caddr_t)mover, bytes) != 0) {
			/* Use the audit(2) errno */
			return (-1);
		}

		mover += bytes;
		bytes_left -= bytes;
	}

	/* Last minute check to make sure we wrote out the exact number */
	if (bytes_left != 0) {
		errno = E2BIG;
		return (-1);
	}

	return (0);
}
