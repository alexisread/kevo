/* Kevo -- a prototype-based object-oriented language *//* (c) Antero Taivalsaari 1991-1993 				  *//* Some parts (c) Antero Taivalsaari 1986-1988 		  *//* debug.c: Debugger internals and primitives 		  */#include "global.h"#include "portGlobal.h"/* --------------------------------------------------------------- *//* This is an auxiliary operation which is not visible to the user *//* Print a bar which shows the current depth of the return stack */void showRSBar(){  int i;  int n = returnSp - returnStackBottom();  for (i = 1 ; i <= n; i++) {	if (i % 5) fprintf(confile, ">"); 	else fprintf(confile, "|");  }}/* This isn't user-accessible either *//* Print the contents of data and return stacks to console */void showStacks(string, inpair)char* string;PAIR* inpair;{    fprintf(confile, "%s '%s'\n", string, inpair->nfa);    fprintf(confile, "\tData stack: ");    { 		int* ptr = dataStackBottom();		if (dataSp > dataStackBottom()) { 			while (dataSp >= ++ptr) {				PAIR* pair = findNameForward(*ptr);				if (pair) fprintf(confile, "%s ", pair->nfa);				else {					pair = findTypeForward(*ptr);					if (pair) {						fprintf(confile, "%s:", pair->nfa);						fprintf(confile, "%d ", *ptr);					}					else fprintf(confile, "%d ", *ptr);				}			}		}	}    fprintf(confile, "\n\tReturn stack: ");     {    	int** ptr = returnStackBottom();  		if (returnSp > returnStackBottom()) {  			while (returnSp >= ++ptr) {				PAIR* pair = findNameForward(maskedFetch(*ptr));				if (pair) { fprintf(confile, "%s ", pair->nfa); }				else fprintf(confile, "%d ", (int)*ptr);  			}  		}  	}	fprintf(confile, "\n\tContext stack: "); 	{		int* ptr = (int*)contextStackBottom();		if (contextSp > contextStackBottom()) { 			while ((int*)contextSp >= ++ptr) { 			  	PAIR* pair = findTypeForward(*ptr);  				if (pair) {  					fprintf(confile, "%s:", pair->nfa);					fprintf(confile, "%d ", *ptr);				}    			else {    				pair = findNameForward(*ptr);    				if (pair) fprintf(confile, "%s ", pair->nfa);    				else fprintf(confile, "%d ", *ptr);				}			}		}	}    fprintf(confile, "\n");}/* This is also inaccessible *//* Print the current process number to console */void showTaskID(){	fprintf(confile, " Task: %d ==\n", up);}/* --------------------------------------------------------------------------- *//* Here are the actual debugger primitives *//* debugExit(): same as 'exit' but breaks and goes to shell. *//* Allows interactive debugging */   /*      Note that there is only one longjump buffer and one set of debugger      variables (defined in global.c). Thus only one task can be debugged     at a time, while other tasks will continue running normally. The     debug task is determined by the currently selected task (theTask)     in the system. After encountering a breakpoint, no other tasks can      cause a breakpoint to happen, nor can they invoke 'resume'.   */void debugExit(){  PAIR* pair = findNameForward((OBJECT*)*(topReturn-1));  /*   	Only one task can be debugged at a time. That task must   	currently have its input and output directed to a window  	(rather than a file).  */  /* yyy warning: reference to 'theTask' is non-portable */	if (!debugTask && up == theTask && infile == 0 && outfile == 0) {    	if (pair) {	    	showRSBar();     		showStacks("Exiting", pair);		}    	rpStore = returnSp;		debugTask = up;		execute(oShell);		(void)setjmp(debug);		/* 'resume' command will resume execution here */		if (!debugTask) {			returnSp = rpStore;			ip = (int**)popReturn();		}	}	else ip = (int**)popReturn();}/* Resume execution after 'debugExit' *//* Terminates the temporary shell and resets previous execution environment */void resume(){  	/*   		Resume command will be accepted only from a task that has executed the 		previous 'debugExit()' command  	*/	if (up == debugTask) {  		debugTask = NIL;  		longjmp(debug, TRUE);	}}/*---------------------------------------------------------------------------*//* Inner interpreters/multitaskers for execution tracing */ /* An alternative inner interpreter: each time an operation is executed *//* its name will be printed to console */void traceInterpreter(){  OBJECT* object;  PAIR*   pair;  int 	  count = 0;  TASK**  upStore = up;  fprintf(confile, "== Trace mode activated ==\n");  traceMode = TRACE;  supervisor = FALSE;  slice = (*up)->priority;  while(TRUE) {  	if (traceMode == QUITTRACE) ownLongJmp();	if ((object = (OBJECT*)*ip++)->sfa) {	/* High-level definition */		pair = findNameForward(object);		if (pair) {			showRSBar(); 			fprintf(confile, "Entering '%s' (%d)\n", pair->nfa, count); 		}		pushReturn((int*)ip);		ip = (int**)(op = object)->mfa;	}	else {									/* Primitive definition */		pair = findPrimName((int*)object->mfa);		if (pair) {			showRSBar(); 			fprintf(confile, "Primitive '%s' (%d)\n", pair->nfa, count);		}				((void (*)())object->mfa)();		if (--slice <= 0) yield();		if (up != upStore) fprintf(confile, "\n##### Task switch #####\n");		upStore = up;		EventLoop();	/* Call the event loop to enable better mouse control */	}	/* Increment count every time inner interpreter proceeds */	count++;  }}/* An alternative inner interpreter: each time an operation is executed *//* its name and current stack contents will be printed to console */void fullTraceInterpreter(){  OBJECT* object;  PAIR*   pair;  TASK**  upStore = up;  fprintf(confile, "== Full trace mode activated ==\n");  traceMode = FULLTRACE;  supervisor = FALSE;  slice = (*up)->priority;  while(TRUE) {  	if (traceMode == QUITTRACE) ownLongJmp();  	if ((object = (OBJECT*)*ip++)->sfa) {	/* High-level definition */		pair = findNameForward(object);		if (pair) showStacks("Entering", pair);		pushReturn((int*)ip);		ip = (int**)(op = object)->mfa;	}	else {									/* Primitive definition */		pair = findPrimName((int*)object->mfa);		if (pair) showStacks("Primitive", pair);		((void (*)())object->mfa)();		if (--slice <= 0) yield();		if (up != upStore) fprintf(confile, "\n##### Task switch #####\n\n");		upStore = up;		EventLoop();	/* Call the event loop to enable better mouse control */	}  }}