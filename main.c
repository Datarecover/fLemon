/*+@@file@@----------------------------------------------------------------*//*!
 \file		main.c
 \par Description 
            Main program file for the LEMON parser generator.
 \par  Status: 
            
 \par Project: 
            Lemon parser
 \date		Created  on Sat Sep  1 17:47:25 2018
 \date		Modified on Sat Sep  1 17:47:25 2018
 \author	
\*//*-@@file@@----------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include "i_o_fmt.h"
#include "struct.h"
#include "option.h"
#include "table.h"
#include "parse.h"
#include "report.h"
#include "set.h"
#include "build.h"

int showPrecedenceConflict = 0;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		memory_error
 \details	Report an out-of-memory condition and abort.\n
            This function is used mostly by the "MemoryCheck" macro in 
            struct.h
 \date		Created  on Sat Sep  1 21:49:59 2018
 \date		Modified on Sat Sep  1 21:49:59 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
void memory_error(void)
{
	fprintf(stderr, "Out of memory.  Aborting...\n");
	exit(1);
}

int     nDefine = 0;	/* Number of -D options on the command line */
char **azDefine = 0;	/* Name of the -D macros */

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		handle_D_option
 \details	This routine is called with the argument to each -D 
            command-line option.
            Add the macro defined to the azDefine array.
 \date		Created  on Sat Sep  1 16:08:14 2018
 \date		Modified on Sat Sep  1 16:08:14 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static void handle_D_option(char *z)
{
	char **paz;
	nDefine++;
	azDefine = (char **)realloc(azDefine, sizeof(azDefine[0]) * nDefine);
	if (azDefine == 0)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	paz  = &azDefine[nDefine - 1];
	*paz = (char *)malloc(lemonStrlen(z) + 1);
	if (*paz == 0)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	lemon_strcpy(*paz, z);
	for (z = *paz; *z && *z != '='; z++)
	{
	}
	*z = 0;
}

/*
 * Rember the name of the output directory 
 */
char *outputDir = NULL;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		handle_d_option
 \date		Created  on Sat Sep  1 16:08:43 2018
 \date		Modified on Sat Sep  1 16:08:43 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static void handle_d_option(char *z)
{
	outputDir = (char *)malloc(lemonStrlen(z) + 1);
	if (outputDir == 0)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	lemon_strcpy(outputDir, z);
}

char *user_templatename = NULL;

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		handle_T_option
 \date		Created  on Sat Sep  1 16:08:52 2018
 \date		Modified on Sat Sep  1 16:08:52 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static void handle_T_option(char *z)
{
	user_templatename = (char *)malloc(lemonStrlen(z) + 1);
	if (user_templatename == 0)
	{
		memory_error();
	}
	lemon_strcpy(user_templatename, z);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		Rule_merge
 \details	Merge together to lists of rules ordered by rule.iRule
 \date		Created  on Sat Sep  1 16:09:17 2018
 \date		Modified on Sat Sep  1 16:09:17 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static struct rule *Rule_merge(struct rule *pA, struct rule *pB)
{
	struct rule *pFirst = 0;
	struct rule **ppPrev = &pFirst;
	while (pA && pB)
	{
		if (pA->iRule < pB->iRule)
		{
			*ppPrev = pA;
			ppPrev = &pA->next;
			pA = pA->next;
		}
		else
		{
			*ppPrev = pB;
			ppPrev = &pB->next;
			pB = pB->next;
		}
	}
	if (pA)
	{
		*ppPrev = pA;
	}
	else
	{
		*ppPrev = pB;
	}
	return pFirst;
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		Rule_sort
 \details	Sort a list of rules in order of increasing iRule value
 \date		Created  on Sat Sep  1 16:09:32 2018
 \date		Modified on Sat Sep  1 16:09:32 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static struct rule *Rule_sort(struct rule *rp)
{
	int i;
	struct rule *pNext;
	struct rule *x[32];
	memset(x, 0, sizeof(x));
	while (rp)
	{
		pNext = rp->next;
		rp->next = 0;
		for (i = 0; i < sizeof(x) / sizeof(x[0]) && x[i]; i++)
		{
			rp = Rule_merge(x[i], rp);
			x[i] = 0;
		}
		x[i] = rp;
		rp = pNext;
	}
	rp = 0;
	for (i = 0; i < sizeof(x) / sizeof(x[0]); i++)
	{
		rp = Rule_merge(x[i], rp);
	}
	return rp;
}

/* forward reference */
static const char *minimum_size_type(int lwr, int upr, int *pnByte);

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		stats_line
 \details	Print a single line of the "Parser Stats" output
 \date		Created  on Sat Sep  1 16:09:57 2018
 \date		Modified on Sat Sep  1 16:09:57 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
static void stats_line(const char *zLabel, int iValue)
{
	int nLabel = lemonStrlen(zLabel);
	printf("  %s%.*s %5d\n", zLabel,
				35 - nLabel, "................................", iValue);
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		main
 \details	The main program. 
            Parse the command line and do it...
 \date		Created  on Sat Sep  1 16:10:23 2018
 \date		Modified on Sat Sep  1 16:10:23 2018
\*//*-@@fnc@@----------------------------------------------------------------*/
int main(int argc, char **argv)
{
	static int version       = 0;
	static int rpflag        = 0;
	static int basisflag     = 0;
	static int emitenumflag  = 0;
	static int compress      = 0;
	static int quiet         = 0;
	static int statistics    = 0;
	static int mhflag        = 0;
	static int nolinenosflag = 0;
	static int noResort      = 0;

	static struct s_options options[] =
	{
		{OPT_FLAG, "b", (char *)&basisflag,				"Print only the basis in report."},
		{OPT_FLAG, "c", (char *)&compress,				"Don't compress the action table."},
		{OPT_FSTR, "d", (char *)&handle_d_option,		"Output directory.  Default '.'"},
		{OPT_FSTR, "D", (char *)handle_D_option,		"Define an %ifdef macro."},
		{OPT_FLAG, "e", (char *)&emitenumflag,			"Emit terminals in an enum instead of defines."},
		{OPT_FSTR, "f", 0,								"Ignored.  (Placeholder for -f compiler options.)"},
		{OPT_FLAG, "g", (char *)&rpflag,				"Print grammar without actions."},
		{OPT_FSTR, "I", 0,								"Ignored.  (Placeholder for '-I' compiler options.)"},
		{OPT_FLAG, "m", (char *)&mhflag,				"Output a makeheaders compatible file."},
		{OPT_FLAG, "l", (char *)&nolinenosflag,			"Do not print #line statements."},
		{OPT_FSTR, "O", 0,								"Ignored.  (Placeholder for '-O' compiler options.)"},
		{OPT_FLAG, "p", (char *)&showPrecedenceConflict,"Show conflicts resolved by precedence rules"},
		{OPT_FLAG, "q", (char *)&quiet,					"(Quiet) Don't print the report file."},
		{OPT_FLAG, "r", (char *)&noResort,				"Do not sort or renumber states"},
		{OPT_FLAG, "s", (char *)&statistics,			"Print parser stats to standard output."},
		{OPT_FLAG, "x", (char *)&version,				"Print the version number."},
		{OPT_FSTR, "T", (char *)handle_T_option,		"Specify a template file."},
		{OPT_FSTR, "W", 0,								"Ignored.  (Placeholder for '-W' compiler options.)"},
		{OPT_FLAG, 0, 0, 0}
	};
	int i;
	int exitcode;
	struct lemon lem;
	struct rule *rp;

	OptInit(argv, options, stderr);
	if (version)
	{
		printf("Lemon version 1.0 (frankie hack 1.00)\n");
		exit(0);
	}
	if (OptNArgs() != 1)
	{
		fprintf(stderr, "fLemon: Exactly one filename argument is required.\n");
		exit(1);
	}
	memset(&lem, 0, sizeof(lem));
	lem.errorcnt = 0;

	/* Initialize the machine */
	Strsafe_init();
	Symbol_init();
	State_init();
	lem.argv0         = argv[0];
	lem.filename      = OptArg(0);
	lem.basisflag     = basisflag;
	lem.emitenumflag  = emitenumflag;
	lem.nolinenosflag = nolinenosflag;
	Symbol_new("$");

	/* Parse the input file */
	Parse(&lem);
	if (lem.errorcnt)
		exit(lem.errorcnt);
	if (lem.nrule == 0)
	{
		fprintf(stderr, "Empty grammar.\n");
		exit(1);
	}
	lem.errsym = Symbol_find("error");

	/*
	 * Count and index the symbols of the grammar
	 */
	Symbol_new("{default}");
	lem.nsymbol = Symbol_count();
	lem.symbols = Symbol_arrayof();
	for (i = 0; i < lem.nsymbol; i++)
		lem.symbols[i]->index = i;
	qsort(lem.symbols, lem.nsymbol, sizeof(struct symbol *), Symbolcmpp);
	for (i = 0; i < lem.nsymbol; i++)
		lem.symbols[i]->index = i;
	while (lem.symbols[i - 1]->type == MULTITERMINAL)
	{
		i--;
	}
	assert(strcmp(lem.symbols[i - 1]->name, "{default}") == 0);
	lem.nsymbol = i - 1;
	for (i = 1; ISUPPER(lem.symbols[i]->name[0]); i++) ;
	lem.nterminal = i;

	/*
	 * Assign sequential rule numbers. 
	 * Start with 0.
	 * Put rules that have no reduce action C-code associated with them last,
	 * so that the switch() statement that selects reduction actions will
	 * have a smaller jump table.
	 */
	for (i = 0, rp = lem.rule; rp; rp = rp->next)
	{
		rp->iRule = rp->code ? i++ : -1;
	}
	for (rp = lem.rule; rp; rp = rp->next)
	{
		if (rp->iRule < 0)
			rp->iRule = i++;
	}
	lem.startRule = lem.rule;
	lem.rule = Rule_sort(lem.rule);

	/*
	 * Generate a reprint of the grammar,
	 * if requested on the command line
	 */
	if (rpflag)
	{
		Reprint(&lem);
	}
	else
	{
		/*
		 * Initialize the size for all follow and first sets
		 */
		SetSize(lem.nterminal + 1);

		/*
		 * Find the precedence for every production rule (that has one)
		 */
		FindRulePrecedences(&lem);

		/*
		 * Compute the lambda-nonterminals and the first-sets for every
		 * nonterminal
		 */
		FindFirstSets(&lem);

		/*
		 * Compute all LR(0) states.
		 * Also record follow-set propagation links so that the follow-set
		 * can be computed later
		 */
		lem.nstate = 0;
		FindStates(&lem);
		lem.sorted = State_arrayof();

		/*
		 * Tie up loose ends on the propagation links
		 */
		FindLinks(&lem);

		/*
		 * Compute the follow set of every reducible configuration
		 */
		FindFollowSets(&lem);

		/*
		 * Compute the action tables
		 */
		FindActions(&lem);

		/*
		 * Compress the action tables
		 */
		if (compress == 0)
			CompressTables(&lem);

		/*
		 * Reorder and renumber the states so that states with fewer choices
		 * occur at the end.  This is an optimization that helps make the
		 * generated parser tables smaller.
		 */
		if (noResort == 0)
			ResortStates(&lem);

		/*
		 * Generate a report of the parser generated.
		 * (the "y.output" file)
		 */
		if (!quiet)
			ReportOutput(&lem);

		/*
		 * Generate the source code for the parser
		 */
		ReportTable(&lem, mhflag);

		/*
		 * Produce a header file for use by the scanner.
		 * (This step is omitted if the "-m" option is used because
		 * makeheaders will generate the file for us.)
		 */
		if (!mhflag)
			ReportHeader(&lem);
	}
	if (statistics)
	{
		printf("Parser statistics:\n");
		stats_line("terminal symbols", lem.nterminal);
		stats_line("non-terminal symbols", lem.nsymbol - lem.nterminal);
		stats_line("total symbols", lem.nsymbol);
		stats_line("rules", lem.nrule);
		stats_line("states", lem.nxstate);
		stats_line("conflicts", lem.nconflict);
		stats_line("action table entries", lem.nactiontab);
		stats_line("lookahead table entries", lem.nlookaheadtab);
		stats_line("total table size (bytes)", lem.tablesize);
	}
	if (lem.nconflict > 0)
	{
		fprintf(stderr, "%d parsing conflicts.\n", lem.nconflict);
	}

	/*
	 * return 0 on success, 1 on failure.
	 */
	exitcode = ((lem.errorcnt > 0) || (lem.nconflict > 0)) ? 1 : 0;
	return (exitcode);
}
