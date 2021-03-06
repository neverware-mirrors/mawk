/********************************************
init.c
copyright 2008-2016,2017, Thomas E. Dickey
copyright 1991-1994,1995, Michael D. Brennan

This is a source file for mawk, an implementation of
the AWK programming language.

Mawk is distributed without warranty under the terms of
the GNU General Public License, version 2, 1991.
********************************************/

/*
 * $MawkId: init.c,v 1.45 2017/10/17 01:19:15 tom Exp $
 */

/* init.c */
#include <mawk.h>
#include <code.h>
#include <memory.h>
#include <symtype.h>
#include <init.h>
#include <bi_funct.h>
#include <bi_vars.h>
#include <files.h>
#include <field.h>

#include <ctype.h>

typedef enum {
    W_UNKNOWN = 0,
#if USE_BINMODE
    W_BINMODE,
#endif
    W_VERSION,
    W_DUMP,
    W_HELP,
    W_INTERACTIVE,
    W_EXEC,
    W_RANDOM,
    W_SPRINTF,
    W_POSIX_SPACE,
    W_USAGE
} W_OPTIONS;

static void process_cmdline(int, char **);
static void set_ARGV(int, char **, int);
static void bad_option(char *);
static void no_program(void);

#ifdef  MSDOS
#if  HAVE_REARGV
void reargv(int *, char ***);
#endif
#endif

const char *progname;
short interactive_flag = 0;

#ifndef	 SET_PROGNAME
#define	 SET_PROGNAME() \
   {char *p = strrchr(argv[0],'/') ;\
    progname = p ? p+1 : argv[0] ; }
#endif

void
initialize(int argc, char **argv)
{

    SET_PROGNAME();

    bi_vars_init();		/* load the builtin variables */
    bi_funct_init();		/* load the builtin functions */
    kw_init();			/* load the keywords */
    field_init();

#if USE_BINMODE
    {
	char *p = getenv("MAWKBINMODE");

	if (p)
	    set_binmode(atoi(p));
    }
#endif

    process_cmdline(argc, argv);

    code_init();
    fpe_init();
    set_stdio();

#if USE_BINMODE
    stdout_init();
#endif
}

int dump_code_flag;		/* if on dump internal code */
short posix_space_flag;

#ifdef	 DEBUG
int dump_RE = 1;		/* if on dump compiled REs  */
#endif

static void
bad_option(char *s)
{
    errmsg(0, "not an option: %s", s);
    if (strcmp(s, "--lint") &&
	strcmp(s, "--lint-old") &&
	strcmp(s, "--posix") &&
	strcmp(s, "--re-interval") &&
	strcmp(s, "--traditional")) {
	mawk_exit(2);
    }
}

static void
no_program(void)
{
    mawk_exit(0);
}

static void
usage(void)
{
    static const char *msg[] =
    {
	"Usage: mawk [Options] [Program] [file ...]",
	"",
	"Program:",
	"    The -f option value is the name of a file containing program text.",
	"    If no -f option is given, a \"--\" ends option processing; the following",
	"    parameters are the program text.",
	"",
	"Options:",
	"    -f program-file  Program  text is read from file instead of from the",
	"                     command-line.  Multiple -f options are accepted.",
	"    -F value         sets the field separator, FS, to value.",
	"    -v var=value     assigns value to program variable var.",
	"    --               unambiguous end of options.",
	"",
	"    Implementation-specific options are prefixed with \"-W\".  They can be",
	"    abbreviated:",
	"",
	"    -W version       show version information and exit.",
#if USE_BINMODE
	"    -W binmode",
#endif
	"    -W dump          show assembler-like listing of program and exit.",
	"    -W help          show this message and exit.",
	"    -W interactive   set unbuffered output, line-buffered input.",
	"    -W exec file     use file as program as well as last option.",
	"    -W random=number set initial random seed.",
	"    -W sprintf=number adjust size of sprintf buffer.",
	"    -W posix_space   do not consider \"\\n\" a space.",
	"    -W usage         show this message and exit.",
    };
    size_t n;
    for (n = 0; n < sizeof(msg) / sizeof(msg[0]); ++n) {
	fprintf(stderr, "%s\n", msg[n]);
    }
    mawk_exit(0);
}

/*
 * Compare ignoring case, but warn about mismatches.
 */
static int
ok_abbrev(const char *fullName, const char *partName, int partLen)
{
    int result = 1;
    int n;

    for (n = 0; n < partLen; ++n) {
	UChar ch = (UChar) partName[n];
	if (isalpha(ch))
	    ch = (UChar) toupper(ch);
	if (ch != (UChar) fullName[n]) {
	    result = 0;
	    break;
	}
    }
    return result;
}

static char *
skipValue(char *value)
{
    while (*value != '\0' && *value != ',') {
	++value;
    }
    return value;
}

static int
haveValue(char *value)
{
    int result = 0;

    if (*value++ == '=') {
	if (*value != '\0' && strchr("=,", *value) == 0)
	    result = 1;
    }
    return result;
}

static int
allow_long_options(char *arg)
{
    static int result = -1;

    if (result < 0) {

	char *env = getenv("MAWK_LONG_OPTIONS");
	result = 0;
	if (env != 0) {
	    switch (*env) {
	    default:
	    case 'e':		/* error */
		bad_option(arg);
		break;
	    case 'w':		/* warn */
		errmsg(0, "ignored option: %s", arg);
		break;
	    case 'i':		/* ignore */
		break;
	    case 'a':		/* allow */
		result = 1;
		break;
	    }
	} else {
	    bad_option(arg);
	}
    }
    return result;
}

static W_OPTIONS
parse_w_opt(char *source, char **next)
{
#define DATA(name) { W_##name, #name }
#define DATA2(name) { W_##name, name }
    static const struct {
	W_OPTIONS code;
	const char *name;
    } w_options[] = {
	DATA(VERSION),
#if USE_BINMODE
	    DATA(BINMODE),
#endif
	    DATA(DUMP),
	    DATA(HELP),
	    DATA(INTERACTIVE),
	    DATA(EXEC),
	    DATA(RANDOM),
	    DATA(SPRINTF),
	    DATA(POSIX_SPACE),
	    DATA(USAGE)
    };
#undef DATA
    W_OPTIONS result = W_UNKNOWN;
    int n;
    int match = -1;
    const char *first;

    /* forgive and ignore empty options */
    while (*source == ',') {
	++source;
    }

    first = source;
    if (*source != '\0') {
	while (*source != '\0' && *source != ',' && *source != '=') {
	    ++source;
	}
	for (n = 0; n < (int) (sizeof(w_options) / sizeof(w_options[0])); ++n) {
	    if (ok_abbrev(w_options[n].name, first, (int) (source - first))) {
		if (match >= 0) {
		    errmsg(0, "? ambiguous -W value: %s vs %s\n",
			   w_options[match].name,
			   w_options[n].name);
		} else {
		    match = n;
		}
	    }
	}
    }
    *next = source;

    if (match >= 0)
	result = w_options[match].code;

    return result;
}

static void
process_cmdline(int argc, char **argv)
{
    int i, j, nextarg;
    char *optArg;
    char *optNext;
    PFILE dummy;		/* starts linked list of filenames */
    PFILE *tail = &dummy;
    size_t length;

    if (argc <= 1)
	usage();

    for (i = 1; i < argc && argv[i][0] == '-'; i = nextarg) {
	if (argv[i][1] == 0)	/* -  alone */
	{
	    if (!pfile_name)
		no_program();
	    break;		/* the for loop */
	}
	/* safe to look at argv[i][2] */

	/*
	 * Check for "long" options and decide how to handle them.
	 */
	if (strlen(argv[i]) > 2 && !strncmp(argv[i], "--", (size_t) 2)) {
	    if (!allow_long_options(argv[i])) {
		nextarg = i + 1;
		continue;
	    }
	}

	if (argv[i][2] == 0) {
	    if (i == argc - 1 && argv[i][1] != '-') {
		if (strchr("WFvf", argv[i][1])) {
		    errmsg(0, "option %s lacks argument", argv[i]);
		    mawk_exit(2);
		}
		bad_option(argv[i]);
	    }

	    optArg = argv[i + 1];
	    nextarg = i + 2;
	} else {		/* argument glued to option */
	    optArg = &argv[i][2];
	    nextarg = i + 1;
	}

	switch (argv[i][1]) {

	case 'W':
	    for (j = 0; j < (int) strlen(optArg); j = (int) (optNext - optArg)) {
		switch (parse_w_opt(optArg + j, &optNext)) {
		case W_VERSION:
		    print_version();
		    break;
#if USE_BINMODE
		case W_BINMODE:
		    if (haveValue(optNext)) {
			set_binmode(atoi(optNext + 1));
			optNext = skipValue(optNext);
		    } else {
			errmsg(0, "missing value for -W binmode");
			mawk_exit(2);
		    }
		    break;
#endif
		case W_DUMP:
		    dump_code_flag = 1;
		    break;

		case W_EXEC:
		    if (pfile_name) {
			errmsg(0, "-W exec is incompatible with -f");
			mawk_exit(2);
		    } else if (nextarg == argc) {
			no_program();
		    }
		    if (haveValue(optNext)) {
			pfile_name = optNext + 1;
			i = nextarg;
		    } else {
			pfile_name = argv[nextarg];
			i = nextarg + 1;
		    }
		    goto no_more_opts;

		case W_INTERACTIVE:
		    interactive_flag = 1;
		    setbuf(stdout, (char *) 0);
		    break;

		case W_POSIX_SPACE:
		    posix_space_flag = 1;
		    break;

		case W_RANDOM:
		    if (haveValue(optNext)) {
			int x = atoi(optNext + 1);
			CELL c[2];

			memset(c, 0, sizeof(c));
			c[1].type = C_DOUBLE;
			c[1].dval = (double) x;
			/* c[1] is input, c[0] is output */
			bi_srand(c + 1);
			optNext = skipValue(optNext);
		    } else {
			errmsg(0, "missing value for -W random");
			mawk_exit(2);
		    }
		    break;

		case W_SPRINTF:
		    if (haveValue(optNext)) {
			int x = atoi(optNext + 1);

			if (x > (int) sizeof(string_buff)) {
			    if (sprintf_buff != string_buff &&
				sprintf_buff != 0) {
				zfree(sprintf_buff,
				      (size_t) (sprintf_limit - sprintf_buff));
			    }
			    sprintf_buff = (char *) zmalloc((size_t) x);
			    sprintf_limit = sprintf_buff + x;
			}
			optNext = skipValue(optNext);
		    } else {
			errmsg(0, "missing value for -W sprintf");
			mawk_exit(2);
		    }
		    break;

		case W_HELP:
		    /* FALLTHRU */
		case W_USAGE:
		    usage();
		    /* NOTREACHED */
		    break;
		case W_UNKNOWN:
		    errmsg(0, "vacuous option: -W %s", optArg + j);
		    break;
		}
		while (*optNext == '=') {
		    errmsg(0, "unexpected option value %s", optArg + j);
		    optNext = skipValue(optNext);
		}
	    }
	    break;

	case 'v':
	    if (!is_cmdline_assign(optArg)) {
		errmsg(0, "improper assignment: -v %s", optArg);
		mawk_exit(2);
	    }
	    break;

	case 'F':

	    rm_escape(optArg, &length);		/* recognize escape sequences */
	    cell_destroy(FS);
	    FS->type = C_STRING;
	    FS->ptr = (PTR) new_STRING1(optArg, length);
	    cast_for_split(cellcpy(&fs_shadow, FS));
	    break;

	case '-':
	    if (argv[i][2] != 0) {
		bad_option(argv[i]);
	    }
	    i++;
	    goto no_more_opts;

	case 'f':
	    /* first file goes in pfile_name ; any more go
	       on a list */
	    if (!pfile_name)
		pfile_name = optArg;
	    else {
		tail = tail->link = ZMALLOC(PFILE);
		tail->fname = optArg;
	    }
	    break;

	default:
	    bad_option(argv[i]);
	}
    }

  no_more_opts:

    tail->link = (PFILE *) 0;
    pfile_list = dummy.link;

    if (pfile_name) {
	set_ARGV(argc, argv, i);
	scan_init((char *) 0);
    } else {			/* program on command line */
	if (i == argc)
	    no_program();
	set_ARGV(argc, argv, i + 1);

#if  defined(MSDOS) && ! HAVE_REARGV	/* reversed quotes */
	{
	    char *p;

	    for (p = argv[i]; *p; p++)
		if (*p == '\'')
		    *p = '\"';
	}
#endif
	scan_init(argv[i]);
/* #endif  */
    }
}

   /* argv[i] = ARGV[i] */
static void
set_ARGV(int argc, char **argv, int i)
{
    SYMTAB *st_p;
    CELL argi;
    register CELL *cp;

    st_p = insert("ARGV");
    st_p->type = ST_ARRAY;
    Argv = st_p->stval.array = new_ARRAY();
    no_leaks_array(Argv);
    argi.type = C_DOUBLE;
    argi.dval = 0.0;
    cp = array_find(st_p->stval.array, &argi, CREATE);
    cp->type = C_STRING;
    cp->ptr = (PTR) new_STRING(progname);

    /* ARGV[0] is set, do the rest
       The type of ARGV[1] ... should be C_MBSTRN
       because the user might enter numbers from the command line */

    for (argi.dval = 1.0; i < argc; i++, argi.dval += 1.0) {
	cp = array_find(st_p->stval.array, &argi, CREATE);
	cp->type = C_MBSTRN;
	cp->ptr = (PTR) new_STRING(argv[i]);
    }
    ARGC->type = C_DOUBLE;
    ARGC->dval = argi.dval;
}

/*----- ENVIRON ----------*/

#ifdef DECL_ENVIRON
#ifndef	 MSDOS_MSC		/* MSC declares it near */
#ifndef environ
extern char **environ;
#endif
#endif
#endif

void
load_environ(ARRAY ENV)
{
    CELL c;
    register char **p = environ;	/* walks environ */
    char *s;			/* looks for the '=' */
    CELL *cp;			/* pts at ENV[&c] */

    c.type = C_STRING;

    while (*p) {
	if ((s = strchr(*p, '='))) {	/* shouldn't fail */
	    size_t len = (size_t) (s - *p);
	    c.ptr = (PTR) new_STRING0(len);
	    memcpy(string(&c)->str, *p, len);
	    s++;

	    cp = array_find(ENV, &c, CREATE);
	    cp->type = C_MBSTRN;
	    cp->ptr = (PTR) new_STRING(s);

	    free_STRING(string(&c));
	}
	p++;
    }
}

#ifdef NO_LEAKS
typedef struct _all_arrays {
    struct _all_arrays *next;
    ARRAY a;
} ALL_ARRAYS;

static ALL_ARRAYS *all_arrays;

void
array_leaks(void)
{
    while (all_arrays != 0) {
	ALL_ARRAYS *next = all_arrays->next;
	array_clear(all_arrays->a);
	ZFREE(all_arrays->a);
	free(all_arrays);
	all_arrays = next;
    }
}

/* use this to identify array leaks */
void
no_leaks_array(ARRAY a)
{
    ALL_ARRAYS *p = calloc(1, sizeof(ALL_ARRAYS));
    p->next = all_arrays;
    p->a = a;
    all_arrays = p;
}
#endif
